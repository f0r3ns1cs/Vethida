# Database

Managed Postgres. The application connects with a least-privilege role from
`packages/db/roles.sql`, over TLS with an explicit CA. `packages/db/client.ts`
refuses to start in production without `DATABASE_CA_CERT`, deliberately, because
the two spellings of `sslmode=require` mean opposite things and one of them
verifies nothing while reporting success.

## Backups

```bash
DATABASE_URL=<production> node scripts/db/backup.mjs --out /var/backups/vethida --keep 14
```

Custom format (`-Fc`), not plain SQL: it compresses, it restores selectively, and
`pg_restore` refuses a truncated archive outright. A truncated `.sql` restores
most of the way and then stops, which looks like partial success and is not.

`server/backup/pg_dump.cron` runs this nightly. `--keep 14` is fourteen dumps,
not fourteen days; if the cron misses a night the retention window silently
lengthens, which is the safe direction.

**The managed provider's own automated backups are the primary.** These dumps are
the secondary, and they exist for the case the provider cannot help with: a
migration that destroyed data, or losing access to the provider account.

## Proving a backup is a backup

```bash
node scripts/db/verify-restore.mjs
```

Restores into a throwaway database and checks that every table came back, that
row counts match table by table, and that constraints and indexes exist, not
just that `pg_restore` exited zero.

Run it **after any schema change** and on a schedule. A dump nobody has restored
is a file, not a backup, and the first time you find out which is during an
incident.

## Restoring for real

Stop writing first. A restore that races live traffic produces a database that is
neither the backup nor the current state.

```bash
sudo systemctl stop api.vethida.com my.vethida.com vethida-discord-bot
```

Then restore into a **new** database and repoint, rather than over the top of the
existing one:

```bash
createdb -T template0 vethida_restored
pg_restore -d vethida_restored --no-owner --no-privileges <dump>
```

Restoring over the live database destroys the evidence of whatever went wrong,
and if the dump turns out to be the wrong one you have nothing to go back to.
Point `DATABASE_URL` at `vethida_restored`, start one service, verify, then the
rest.

After any restore, re-apply the roles:

```bash
psql -d vethida_restored -f packages/db/roles.sql
```

`pg_restore --no-owner --no-privileges` deliberately drops grants. Skipping this
leaves the app connecting as a role with no rights, or (worse) falling back to
a superuser connection string somebody adds to make it work.

## Migrations

Forward-only. Generated, never hand-written:

```bash
pnpm db:generate      # after editing a schema file
pnpm db:migrate       # apply
pnpm db:generate      # must now say "No schema changes"
```

That third command is the check. CI runs it and fails if drizzle still sees a
difference, which is how a table edited without a migration gets caught.

**Against production, migrate before deploying the code that needs it.** The
running app tolerates a column it does not know about; it does not tolerate a
missing one. This ordering is also why additive-only matters: a code rollback
does not roll back a migration, so a destructive migration makes rollback
impossible.

Take a dump immediately before any production migration. Not the nightly one:
one you took just now and can name.

## Connection exhaustion

Every app instance, the bot and the migration runner draw from one plan-capped
allowance. `DATABASE_POOL_MAX` defaults to 10 per process. Four processes at 10
is 40 connections before any admin session.

Symptom is customers unable to sign in while everything reports healthy. Check:

```sql
SELECT count(*), state FROM pg_stat_activity GROUP BY state;
```

Idle-in-transaction rows accumulating means something acquired a connection and
did not release it; that is a code bug, not a capacity problem, and raising the
cap hides it.
