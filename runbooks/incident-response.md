# Incident response

## Severity

| | What | First |
|---|---|---|
| **S1** | Money or credentials: checkout broken, licenses wrongly revoked, leaked key, unauthorized access | Contain first, investigate after. |
| **S2** | A public site or activation is down, nothing being lost | Restore service, then find the cause. |
| **S3** | Degraded or cosmetic | Normal work. |

## First checks

```bash
for h in vethida.com my.vethida.com api.vethida.com; do
  curl -fsS -o /dev/null -w "$h %{http_code}\n" "https://$h/" || echo "$h UNREACHABLE"
done
curl -fsS http://127.0.0.1:3001/health
curl -fsS http://127.0.0.1:3001/ready
systemctl status api.vethida.com my.vethida.com vethida.com --no-pager
journalctl -u api.vethida.com -n 100 --no-pager
```

- `/health` up, `/ready` down: Postgres.
- Both up, public URL down: nginx or Cloudflare.
- Both down: the unit. Read the journal.

## S1: leaked key

Revoke first, investigate after.

1. Find which key it is in [key-rotation.md](key-rotation.md).
2. Sign everyone out: `DELETE FROM auth_session;`
3. Invalidate desktop tokens: bump `token_generation` on the affected licenses.
4. Database: rotate the app role's password, update `DATABASE_URL`, restart.
5. Save the journal (`journalctl --since ... > file`) before restarting anything else.

## S1: wrong license outcome

- Don't edit the database first. Check the order with the payment provider, then fix the license to match it.
- Check `/admin/flags`. Supersede and double-billing problems are listed there with the fix.
- Webhooks are deduplicated by delivery id in `webhook_event`. To reprocess one, delete its row, then replay it from the provider's dashboard.

## S2: service down

```bash
sudo systemctl restart <unit>
journalctl -u <unit> -n 100 --no-pager
```

If it keeps restarting, read the error before restarting again. Rolling back is in [deploy.md](deploy.md).

## S2: database unreachable

`/ready` returns 503 with the reason. Check the provider's status page and connection count ([database.md](database.md)).

## Communication

- Post on Discord within an hour for S1 or S2, even if all you can say is that you're on it. Email users for anything involving accounts or money.
- Don't describe a security incident's scope until you know it.

## Personal data

If personal data may have been exposed, the GDPR deadline is 72 hours from becoming aware to notify the supervisory authority, and affected people without undue delay if the risk to them is high.

Vethida holds email addresses, license entitlements and device fingerprints. Payment details stay with the payment provider.

## Afterwards

Write up what happened the same day: timeline, cause, what users saw, and what would have caught it sooner. Then add that check.
