# Database

Managed Postgres. The app connects as a least-privilege role (`packages/db/roles.sql`) over TLS. In production `packages/db/client.ts` requires `DATABASE_CA_CERT`.

## Backups

The provider's automatic backups are the main ones. `server/backup/pg_dump.cron` takes a nightly dump as a second copy:

```bash
DATABASE_URL=<production> node scripts/db/backup.mjs --out /var/backups/vethida --keep 14
```

`--keep 14` keeps 14 dumps, not 14 days. Testing and restoring are in [restore.md](restore.md).

## Migrations

Forward-only and generated, never written by hand:

```bash
pnpm -C packages/db generate   # after a schema change
pnpm -C packages/db migrate
pnpm -C packages/db generate   # must report no changes
```

In production, take a dump, migrate, then deploy the code that needs it. Keep migrations additive so a code rollback still works.

## Connection limits

`DATABASE_POOL_MAX` defaults to 10 per process, and every process shares the plan's limit. If sign-ins fail while everything reports healthy, check:

```sql
SELECT count(*), state FROM pg_stat_activity GROUP BY state;
```

Connections stuck in `idle in transaction` are a code bug. Raising the limit only hides it.
