# Vethida cron jobs (systemd timers)

Scheduled server jobs run as systemd timers on VPS. Each
timer's `oneshot` service `curl`s the app's cron endpoint over localhost with a
shared `CRON_SECRET`; the endpoints reject anything without
`Authorization: Bearer $CRON_SECRET`.

## Jobs

| Timer | Schedule | Endpoint (app) | Purpose |
|-------|----------|----------------|---------|
| `vethida-delete-accounts` | daily 03:00 | `/api/cron/delete-accounts` (my.vethida.com) | GDPR Art. 17 hard-delete of accounts past their 30-day grace window. **Dry-run until `HARD_DELETE_ENABLED=true`** is set in the portal app's environment. |
| `vethida-license-reminders` | daily 09:00 | `/api/cron/license-reminders` (api.vethida.com) | "Renews soon" emails for subscriptions renewing within 3 days. |
| `vethida-cleanup` | daily 04:00 | `POST /v1/cleanup` (api.vethida.com) | Retention enforcement: reaps expired webhook-dedup, session, and verification rows. |
| `vethida-discord-reconcile` | daily 05:00 | `/api/cron/discord-reconcile` (my.vethida.com) | Re-syncs linked Discord accounts' roles against entitlement. Entitlement lapses on a clock, not an event, so without this a lapsed customer keeps the paid channels and the AI assistant indefinitely. |

## Install

1. Copy `cron.env.example` to `/etc/vethida/cron.env`, set `CRON_SECRET` (the
   same value the Next apps get in their environment) and the two URLs to match
   each app's local port, then `chmod 600 /etc/vethida/cron.env`.
2. Copy the `.service` and `.timer` units to `/etc/systemd/system/`.
3. Enable:
   ```sh
   sudo systemctl daemon-reload
   sudo systemctl enable --now vethida-delete-accounts.timer
   sudo systemctl enable --now vethida-license-reminders.timer
   sudo systemctl enable --now vethida-cleanup.timer
   sudo systemctl enable --now vethida-discord-reconcile.timer
   ```
4. Verify: `systemctl list-timers | grep vethida`; logs via
   `journalctl -u vethida-delete-accounts`.

To run a job by hand: `sudo systemctl start vethida-delete-accounts.service`.
