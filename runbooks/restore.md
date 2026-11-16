# Restore

Dumps are custom format (`pg_dump -Fc`).

## Test the backups

After any schema, backup or Postgres version change:

```bash
node scripts/db/verify-restore.mjs
```

It restores into a throwaway database and compares tables, row counts, constraints and indexes. A non-zero exit means the backups don't work.

## Take a backup now

```bash
DATABASE_URL=... node scripts/db/backup.mjs --out ./backups --keep 14
```

## Restore production

1. Stop writers: `sudo systemctl stop api.vethida.com my.vethida.com vethida-discord-bot`
2. Dump the current state first.
3. Restore into a new database, never over the live one:
   ```bash
   createdb -T template0 vethida_restored
   pg_restore -d vethida_restored --no-owner --no-privileges <dump>
   psql -d vethida_restored -f packages/db/roles.sql
   ```
4. Check row counts and the newest `created_at` in `auth_user`, `license` and `auth_session`.
5. Point `DATABASE_URL` at `vethida_restored` and run `pnpm -C packages/db migrate`.
6. Start the units one at a time and check `systemctl status` after each.
7. Keep the old database until the next backup is verified.

There's no point-in-time recovery. Anything written after the last dump is lost.
