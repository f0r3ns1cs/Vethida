# nginx

## Layout

```
/etc/nginx/conf.d/vethida-shared.conf     Cloudflare IP ranges, shared variables
/etc/nginx/snippets/vethida-proxy.conf    proxy headers
/etc/nginx/sites-available/*.conf         one file per host
```

The repo mirrors this in `server/nginx/`.

## Change the config

Edit it in the repo, not on the server. Then:

```bash
sudo cp <files> /etc/nginx/...
sudo nginx -t
sudo systemctl reload nginx
```

## Cloudflare IP ranges

Check <https://www.cloudflare.com/ips/> every few months and update `vethida-shared.conf`. If the list is out of date, real client IPs are lost and rate limits hit everyone at once.

## Origin certificate

One Cloudflare Origin Certificate covers `vethida.com` and `*.vethida.com`.

1. Issue a new one in Cloudflare (SSL/TLS > Origin Server). Leave the old one in place until step 4 passes.
2. Write it to `/etc/ssl/cloudflare/vethida.com.pem` and `.key` (key `0600`, owned by root).
3. `sudo nginx -t && sudo systemctl reload nginx`
4. Check every host:
   ```bash
   for h in vethida.com my.vethida.com api.vethida.com; do curl -fsSI "https://$h" >/dev/null && echo "$h ok" || echo "$h FAILED"; done
   ```

`authenticated_origin_pull_ca.pem` is Cloudflare's client CA, a different file. Don't replace it with the origin certificate.

## Authenticated Origin Pulls

`ssl_verify_client on` makes the origin accept only Cloudflare, so the WAF rules can't be bypassed by hitting the server IP. Test the origin over loopback rather than turning it off.

## Common failures

| Symptom | Cause |
|---|---|
| 502, loopback `/health` is fine | Unit and site file use different ports. |
| 400 "No required SSL certificate was sent" | Request bypassed Cloudflare, or the client CA file is wrong. |
| Rate limits hitting everyone | Cloudflare IP list is out of date. |
| Duplicate `Content-Security-Policy` | nginx added a header. The apps set their own headers. |
