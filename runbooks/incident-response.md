# Incident response

One person operates this. That is the constraint everything below is written
around: there is no rota to escalate to, so the plan has to be short enough to
follow alone and specific enough not to need thinking about.

## Severity, and what it changes

| | Definition | First action |
|---|---|---|
| **S1** | Money or credentials. Checkout broken, licenses wrongly revoked, a key leaked, unauthorized access. | Stop the bleeding now, diagnose after. |
| **S2** | A customer-facing surface is down but nothing is being lost. Site or portal unreachable, activation failing. | Restore service, then find out why. |
| **S3** | Degraded or cosmetic. One page broken, the bot offline, a slow query. | Normal work. Do not wake up for it. |

The only distinction that matters at 3am is S1 versus everything else. S1 means
act before you understand; anything else means understand first.

---

## First five minutes

```bash
# Is it us or is it the internet?
for h in vethida.com my.vethida.com api.vethida.com; do
  curl -fsS -o /dev/null -w "$h %{http_code} %{time_total}s\n" "https://$h/" || echo "$h UNREACHABLE"
done

# Which layer?
curl -fsS http://127.0.0.1:3001/health   # process alive
curl -fsS http://127.0.0.1:3001/ready    # can reach Postgres
systemctl status api.vethida.com my.vethida.com vethida.com --no-pager
journalctl -u api.vethida.com -n 100 --no-pager
```

`/health` up and `/ready` down is Postgres. Both up and the public host down is
nginx or Cloudflare. Both down is the unit: read the journal; a missing
environment variable throws at boot and names every missing variable at once.

---

## S1: a key or credential leaked

Order matters. Revoke before you investigate.

1. **Which key?** [key-rotation.md](key-rotation.md) has the shape of each. The
   license-signing and update-manifest keys cannot be rotated instantly; the rest
   can.
2. **Sessions:** `DELETE FROM auth_session;` signs everyone out
   immediately. No deploy, no restart.
3. **Desktop tokens:** bump `token_generation` on the affected licenses. Every
   outstanding signed token for that license stops verifying.
4. **Database:** rotate the application role's password at the provider, update
   `DATABASE_URL`, restart. The app role is least-privilege
   (`packages/db/roles.sql`), so this is containment and not a full compromise.
5. **Only then** work out how it leaked. Preserve the journal first
   (`journalctl --since` to a file), because restarts roll it.

## S1: wrong licensing outcome

A customer charged and not licensed, or licensed and not charged, or revoked
without cause.

- **Do not fix it by editing the database first.** Polar is the system of record
  for the payment. Read the order there, then reconcile the license to it. The
  reverse produces a customer whose invoice and whose entitlement disagree, which
  is the one failure that cannot be sorted out without a human on both sides.
- Check `/admin/flags` before anything else. The webhook dispatcher raises
  `supersede_failed`, `supersede_not_honoured` and `superseded_renewal_charged`
  there, all `critical`, and each carries the exact remediation in its details.
  If the incident is double billing, the answer is probably already sitting in
  that list.
- Webhook deliveries are deduplicated on Polar's delivery id in `webhook_event`.
  To force a redelivery to be processed, delete that row and replay from Polar's
  dashboard. Do not replay without deleting; it will be acked and ignored.

## S2: a service is down

```bash
sudo systemctl restart <unit>
journalctl -u <unit> -n 100 --no-pager
```

If it restarts and stays up, the incident is over and the investigation is not.
If it restart-loops, read the reason before restarting again. `Restart=on-failure`
with `RestartSec=5` will happily loop forever and bury the original error.

Rolling back is in [deploy.md](deploy.md). Remember that a code rollback does not
roll back a migration.

## S2: database unreachable

`/ready` returns 503 with the reason. Check the provider's status page and the
connection count:

```sql
SELECT count(*), state FROM pg_stat_activity GROUP BY state;
```

Exhaustion presents as customers unable to sign in while every process reports
healthy. See [database.md](database.md).

---

## Telling people

Vethida has no status page (see [ADR-0006](../adr/0006-no-status-page-at-v1.md)),
so the channels are the Discord server and, for anything touching accounts or
money, email.

**Say something within the hour for S1 or S2**, even with nothing to report.
"We know, we are on it, next update at X" is worth more than an accurate
explanation two hours later.

**Do not describe a security incident before you understand it.** A wrong scope
statement is worse than silence, and correcting it publicly costs more trust than
the incident did.

## Notification obligations

If personal data was or may have been exposed, the GDPR clock is **72 hours from
becoming aware** to notify the supervisory authority, and affected individuals
without undue delay if the risk to them is high. That clock starts at awareness,
not at confirmation, and "we were still investigating" is not an extension.

What Vethida holds: email addresses, license entitlements, device fingerprints,
and a payment relationship held by the payment provider as merchant of record. Card data is never on this side of the boundary.

---

## Afterwards

Write it down the same day, in `docs/incidents/YYYY-MM-DD-<slug>.md`: what
happened, what the customer saw, the timeline, what actually caused it, and what
would have caught it earlier. The last one is the only part that matters. If the
answer is "a test", write the test that week, while it still feels urgent.
