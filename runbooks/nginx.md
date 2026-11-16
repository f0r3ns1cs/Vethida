# nginx

## Layout

```
/etc/nginx/conf.d/vethida-shared.conf     Cloudflare ranges, $vethida_connection_upgrade
/etc/nginx/snippets/vethida-proxy.conf    the proxy headers every location uses
/etc/nginx/sites-available/*.conf         one file per host
/etc/nginx/sites-enabled/*.conf           symlinks to the above
```

The repository mirrors that under `server/nginx/`. The shared file exists so the
Cloudflare range list lives in one place. It is published, it changes, and three
copies of it means two are wrong after the first update, with a symptom that is
easy to miss: `$remote_addr` silently stays the Cloudflare edge IP, so every rate
limit and every abuse log entry attributes the whole internet to about twenty
addresses.

## Changing a config

Never edit on the server. Edit in the repository, prove it, then copy.

```bash
pnpm check:server
```

That runs `nginx -t` over all three site files inside `nginx:1.29-alpine` with
the real includes wired up, runs `systemd-analyze verify` over the units, and
cross-checks each unit's `PORT=` against the `proxy_pass` in its site file. It
fails on warnings as well as errors, because a deprecated directive today is a
broken config after the next `apt upgrade`.

Then on the server:

```bash
sudo cp <files> /etc/nginx/...
sudo nginx -t                 # again, against the real certificates
sudo systemctl reload nginx   # reload, not restart: no dropped connections
```

If `nginx -t` fails on the server after passing locally, the difference is
almost always a certificate path; the local run stubs those.

## Refreshing the Cloudflare ranges

Check <https://www.cloudflare.com/ips/> quarterly, and after any Cloudflare
announcement about address space. Update `server/nginx/conf.d/vethida-shared.conf`,
run `pnpm check:server`, deploy as above.

Stale ranges do not break anything visibly. That is what makes them worth a
calendar entry rather than a reaction.

## Rotating the origin certificate

One Cloudflare Origin Certificate covers `vethida.com` and `*.vethida.com`, and
all three site files reference the same pair. Origin certificates are issued for
up to 15 years, so this is rare, which is exactly why it needs writing down.

1. Issue the replacement in the Cloudflare dashboard (SSL/TLS > Origin Server).
   Keep the old one installed until step 4 passes.
2. Write the new pair to `/etc/ssl/cloudflare/vethida.com.pem` and `.key`.
   The key is `0600`, owned by root.
3. `sudo nginx -t && sudo systemctl reload nginx`
4. Verify all three hosts answer over TLS:
   ```bash
   for h in vethida.com my.vethida.com api.vethida.com; do
     curl -fsSI "https://$h" >/dev/null && echo "$h ok" || echo "$h FAILED"
   done
   ```

`authenticated_origin_pull_ca.pem` is a **different** file: Cloudflare's client
CA, used by `ssl_verify_client on`. It is not rotated with the origin
certificate, and replacing it with the origin cert makes every request fail
client verification with an error that does not say so.

## Authenticated Origin Pulls

`ssl_verify_client on` is what makes the edge firewall in `server/firewall/`
meaningful. Without it the origin IP is reachable directly and every WAF rule,
rate limit and bot check is optional for anyone who finds the address.

If you ever need to test the origin directly, do it over the loopback interface
from on the box, not by turning this off.

## Common failures

| Symptom | Cause |
|---|---|
| 502 from nginx, `/health` fine on loopback | Unit and site file disagree on the port. `pnpm check:server`. |
| 400 "No required SSL certificate was sent" | Request reached the origin without going through Cloudflare, or `authenticated_origin_pull_ca.pem` is the wrong file. |
| Rate limits firing for everyone at once | `set_real_ip_from` list is stale; every request is being attributed to a handful of edge IPs. |
| Duplicate `Content-Security-Policy` | Something added a header in nginx. The apps own their response headers; nginx owns transport only. |
