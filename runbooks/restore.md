# Restore

Dumps are `pg_dump -Fc`. `pg_restore` refuses a truncated custom-format file
outright, which a truncated `.sql` does not.

## Prove the backups work

Run this after any change to the schema, the backup cron or the database
version. A dump nobody has restored is a file.

```
pnpm db:verify-restore
```

It dumps, restores into a throwaway database, compares table lists, compares row
counts table by table, checks constraints and indexes came back, and drops the
throwaway. Non-zero exit means the backups are not backups.

## Take one now

```
DATABASE_URL=... pnpm db:backup
```

Writes to `./backups`, keeps 14, and verifies the file it wrote.

## Restore production

1. Stop what writes: `systemctl stop api.vethida.com my.vethida.com vethida-discord-bot`
2. Take a dump of the current state first, whatever state it is in. You cannot
   get it back afterwards.
3. Restore into a **new** database, never over the live one:
   `createdb vethida_restored && pg_restore -d vethida_restored --no-owner <dump>`
4. Check it: row counts on `auth_user`, `license`, `auth_session`; the newest
   `created_at` in each.
5. Repoint `DATABASE_URL` at the restored database.
6. `pnpm db:migrate`. The dump may predate the current schema.
7. Start the units one at a time, `systemctl status` after each.
8. Keep the old database until the next verified backup has been taken.

## Point in time

There is none. Recovery granularity is the cron interval in
`server/backup/pg_dump.cron`. Anything written after the last dump is gone, and
that is the number to quote to anyone asking how much data was lost.
