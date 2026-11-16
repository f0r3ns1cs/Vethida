# Deploy a web change

Covers `vethida.com`, `my.vethida.com`, `api.vethida.com` and the Discord bot.
The desktop application ships through [the release
process](../release/code-signing.md), not this.

**Time:** 10 minutes if nothing is wrong.
**Blast radius:** the surface you are deploying. The apps are independent
processes behind independent nginx server blocks; restarting one does not touch
the others.

---

## Before you start

Run the gate locally. All of it, not the part you think is relevant.

```bash
pnpm check
```

That is preflight, environment documentation, version agreement, the server
config parse and the test suite. If any of it fails, stop. A red gate that you
deploy past is how a schema change reaches production without its migration.

Then check what you are about to ship:

```bash
git log --oneline origin/main..HEAD
```

If that list contains a migration, read [database.md](database.md) first. The
order is migrate, then deploy, and it is not reversible in the other direction.

---

## Deploy

Each app lives in `/opt/<host>` on the VPS and runs under a systemd unit named
after it.

1. **Pull.**

   ```bash
   cd /opt/api.vethida.com && git fetch --all && git reset --hard origin/main
   ```

   `reset --hard`, not `pull`: the working tree on the server is a deployment
   artifact, not a place anyone edits. A merge conflict here at 2am is not a
   thing that should be possible.

2. **Install exactly what the lockfile says.**

   ```bash
   pnpm install --frozen-lockfile
   ```

   `--frozen-lockfile` is the point. Without it, a lockfile that disagrees with
   `package.json` is silently resolved on the server, which means the thing you
   tested is not the thing you deployed.

3. **Build.**

   ```bash
   pnpm build
   ```

   Boot-time environment validation is skipped during the build
   (`NEXT_PHASE=phase-production-build`), so a successful build does **not** mean
   the environment is complete. Step 5 is what proves that.

4. **Restart.**

   ```bash
   sudo systemctl restart api.vethida.com
   ```

5. **Confirm it actually came up.**

   ```bash
   curl -fsS http://127.0.0.1:3001/health && echo
   curl -fsS http://127.0.0.1:3001/ready  && echo
   ```

   `/health` proves the process is answering. `/ready` proves it can reach
   Postgres. A missing required environment variable throws at boot with every
   missing name listed, so if the unit is restarting in a loop, read the reason:

   ```bash
   journalctl -u api.vethida.com -n 50 --no-pager
   ```

6. **Confirm it is reachable from outside.**

   ```bash
   curl -fsS https://api.vethida.com/health && echo
   ```

   A 502 here with a healthy loopback check means nginx and the unit disagree
   about the port. `pnpm check:server` catches that before it ships; if you are
   seeing it in production, the config on the box is not the config in the repo.

Ports, once, so they are in one place: portal **3000**, api **3001**, site
**3002**. They match each app's `dev` script, the `PORT=` line in its systemd
unit, and the `proxy_pass` in its nginx site file. `pnpm check:server` fails if
any of those three drift apart.

---

## Rolling back

There is no blue/green here and there does not need to be.

```bash
cd /opt/api.vethida.com
git reset --hard <previous-sha>
pnpm install --frozen-lockfile && pnpm build
sudo systemctl restart api.vethida.com
```

**A code rollback does not roll back a migration.** Migrations are forward-only
by policy. If the deploy included one, rolling the code back leaves the schema
ahead of it, which is safe for an additive migration and broken for anything
else. This is the reason additive-only is the rule and not a preference.

---

## Deploying the Discord bot

Same shape, no build step: it runs TypeScript directly through `tsx`.

```bash
cd /opt/vethida-discord-bot
git fetch --all && git reset --hard origin/main
pnpm install --frozen-lockfile
sudo systemctl restart vethida-discord-bot
curl -fsS http://127.0.0.1:8080/health && echo
```

Slash commands are re-registered on every boot (`main()` in `bot.ts` PUTs the
whole command list before logging in), so a restart is also how a command change
reaches Discord. There is no separate registration step to forget.

The health endpoint listens on loopback only. If `curl` from another machine
answers, `HEALTH_HOST` has been overridden and `/metrics` is public.
