# Key rotation

| Secret | Where | How |
|---|---|---|
| `ED25519_SIGNING_KEY` + `LICENSE_TOKEN_KID` | api.vethida.com | Rolling, needs a client release first |
| Update manifest key | Offline only | Rolling, needs a client release first |
| `BETTER_AUTH_SECRET` | my.vethida.com | Immediate, signs everyone out |
| `POLAR_WEBHOOK_SECRET` | api.vethida.com | Immediate, coordinated with Polar |
| `INTERNAL_API_TOKEN` | api, portal, bot | Immediate, all three together |
| `CRON_SECRET` | apps and `/etc/vethida/cron.env` | Immediate, all together |

## Rolling keys

Installed builds verify license tokens and update manifests against public keys built into the app, so the server can't sign with a new key until clients trust it. Each build has a primary and a backup slot for each (`vth_core/Services/SecurityConfig.h`).

1. Generate the new pair and keep the private key offline:
   - Update key: `node scripts/release/gen-update-keys.mjs --out ./update-manifest-v2.pem`
   - License key: `node scripts/gen-secrets.mjs` (private and public key from the same run)
2. Put the new public key in the backup slot and remove `-PLACEHOLDER` from its kid.
3. Release that build and wait for most users to update.
4. Switch the server: set `ED25519_SIGNING_KEY` and `LICENSE_TOKEN_KID` together, or pass the new key to the stamp script.
5. Next release: make the new key primary and put the next one in the backup slot.

If the server switches before step 3, clients reject every token and update until they get a newer build.

**If a key leaked:** bump `token_generation` on affected licenses to invalidate their tokens, and ship a client release as fast as possible. For the update key, empty the feed first; release builds refuse an unsigned feed.

## Immediate keys

- **`BETTER_AUTH_SECRET`:** change it and restart the portal. Everyone is signed out. To sign everyone out without rotating, run `DELETE FROM auth_session;`.
- **`POLAR_WEBHOOK_SECRET`:** add the new secret in Polar, deploy it, then remove the old one. Removing first rejects deliveries already signed with the old one.
- **`INTERNAL_API_TOKEN`:** change it for api, portal and bot at the same time. Calls between them fail for a few seconds.
- **`CRON_SECRET`:** change it in the app environments and `/etc/vethida/cron.env`, then test a job:
  ```bash
  sudo systemctl start vethida-cleanup.service
  journalctl -u vethida-cleanup -n 20 --no-pager
  ```
  A 401 means the two copies don't match.

## After rotating

Run `node scripts/release/check-release-ready.mjs`. It fails if a key is still a placeholder.
