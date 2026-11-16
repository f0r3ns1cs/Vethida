# Deploy

For `vethida.com`, `my.vethida.com`, `api.vethida.com` and the Discord bot. The desktop app is in [release.md](release.md).

## Before

```bash
pnpm check
git log --oneline origin/main..HEAD
```

If the log includes a migration, run it first ([database.md](database.md)).

## Deploy an app

Each app lives in `/opt/<host>` and runs as a systemd unit of the same name. Ports: portal 3000, api 3001, site 3002.

```bash
cd /opt/api.vethida.com
git fetch --all && git reset --hard origin/main
pnpm install --frozen-lockfile
pnpm build
sudo systemctl restart api.vethida.com
curl -fsS http://127.0.0.1:3001/health && curl -fsS http://127.0.0.1:3001/ready
curl -fsS https://api.vethida.com/health
```

- `/health` checks the process, `/ready` checks Postgres.
- A build doesn't validate the environment. A missing variable shows up at startup: `journalctl -u api.vethida.com -n 50 --no-pager`.
- 502 from the public URL with a healthy loopback check means nginx and the unit disagree on the port.

## Discord bot

No build step.

```bash
cd /opt/vethida-discord-bot
git fetch --all && git reset --hard origin/main
pnpm install --frozen-lockfile
sudo systemctl restart vethida-discord-bot
curl -fsS http://127.0.0.1:8080/health
```

Slash commands re-register on every start. The health endpoint should only answer on loopback.

## Roll back

```bash
git reset --hard <previous-sha>
pnpm install --frozen-lockfile && pnpm build
sudo systemctl restart <unit>
```

Migrations don't roll back. Keep them additive so older code still runs.
