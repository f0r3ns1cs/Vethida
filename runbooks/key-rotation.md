# Key rotation

Six secrets, four rotation shapes. The shape is what matters: which of these you
can change in one move and which need a rolling cutover is not obvious, and
getting it wrong locks out every customer at once.

| Secret | Lives in | Shape |
|---|---|---|
| `ED25519_SIGNING_KEY` + `LICENSE_TOKEN_KID` | api.vethida.com | **Rolling.** Two-phase, needs a client release first. |
| Update manifest key | Offline, never on a server | **Rolling.** Two-phase, needs a client release first. |
| `BETTER_AUTH_SECRET` | my.vethida.com | Immediate. Signs everyone out. |
| `POLAR_WEBHOOK_SECRET` | api.vethida.com | Immediate, coordinated with Polar. |
| `INTERNAL_API_TOKEN` | api + portal + bot | Immediate, all three at once. |
| `CRON_SECRET` | apps + `/etc/vethida/cron.env` | Immediate, all at once. |

---

## The two rolling ones

The license-token key and the update-manifest key are both verified by desktop
builds already installed on customer machines, against public keys compiled into
the binary. **You cannot start signing with a key no deployed client trusts.**

Each build carries two of each: primary and backup (`SecurityConfig.h`,
`LicenseTokenPrimaryKid` / `LicenseTokenBackupKid` and the update-manifest pair).
That is what makes rotation a cutover instead of a flag day, and it only works in
this order:

1. **Generate the new pair.** Keep the private key offline.
   ```bash
   pnpm release:gen-update-keys --out ./update-manifest-v2.pem
   ```
   For the license key, `pnpm gen:secrets` prints the private key and the raw
   public key together. They must come from the same run.
2. **Bake the new public key into the client as the *backup*.** Leave the primary
   alone. Drop the `-PLACEHOLDER` suffix from the corresponding kid constant.
3. **Ship that release, and wait for adoption.** Weeks, not days. Anyone still on
   an older build trusts only the old key.
4. **Only then** switch the server to sign with the new key: change
   `ED25519_SIGNING_KEY` and `LICENSE_TOKEN_KID` together, or start passing the
   new `--sign-key` to `pnpm release:stamp`.
5. **In the release after that,** promote new to primary and put the next
   generation in the backup slot.

Skipping step 3 is the mistake worth naming: every client that has not updated
sees a token or manifest signed by a key it does not hold, rejects it, and
reports it as an invalid license or a failed update. There is no server-side fix
once it has happened; you have to ship again and wait.

**If the key is compromised rather than merely old,** the rolling shape does not
apply and there is no clean answer: the honest move is to revoke server-side
(bump `token_generation` on affected licenses, which invalidates outstanding
tokens immediately) and ship a client release as fast as possible. For the update
key, pull the feed first: an unsigned feed is refused by release builds, so
emptying it is safer than leaving a manifest signed with a leaked key.

The two keys are deliberately separate systems. A compromised API cannot publish
an installer, because the update key is not on the API host and never has been.

---

## The immediate ones

**`BETTER_AUTH_SECRET`**: change it and restart the portal. Every session cookie
becomes invalid, so every signed-in customer is signed out. That is the intended
behavior during an incident and an unpleasant surprise otherwise. Sessions are
opaque rows in `auth_session`, so if the goal is "sign everybody out" you do not
need to rotate this at all:

```sql
DELETE FROM auth_session;
```

That is the entire reason sessions are database rows rather than self-issued
JWTs. Revoking one is a `DELETE`; revoking all of them is a `DELETE` with no
`WHERE`.

**`POLAR_WEBHOOK_SECRET`**: Polar's dashboard can hold more than one signing
secret during a rotation, and the verifier accepts any valid `v1` entry in a
space-separated signature header. So: add the new secret in Polar, deploy the new
value, remove the old one in Polar. Do not remove first; deliveries in flight are
signed with the old secret and would be rejected and retried into the void.

**`INTERNAL_API_TOKEN`**: read by api.vethida.com and sent by both the portal and
the bot. There is no dual-accept path, so all three change together and there is a
gap of a few seconds where cross-service calls 401. Do it when nobody is buying.

**`CRON_SECRET`**: lives in both the app environments and
`/etc/vethida/cron.env`. Change both, then run one job by hand to prove it:

```bash
sudo systemctl start vethida-cleanup.service
journalctl -u vethida-cleanup -n 20 --no-pager
```

A 401 in that log means the two copies disagree.

---

## After any rotation

`pnpm release:check` refuses a build whose kids still carry `-PLACEHOLDER` or
whose keys are still all-zero. Run it. A release that ships with placeholder keys
has an updater that trusts nothing and a license verifier that returns
`KeyNotInstalled`, which reads to a customer as "my paid license does not work".
