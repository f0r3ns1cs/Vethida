# Cloudflare WAF (Terraform)

Edge security for the `vethida.com` zone, managed as code: a default-deny web
application firewall, edge rate limits, and security response headers. Applied
through the Cloudflare Terraform provider (`~> 4.40`).

## Files

| File | Purpose | Cloudflare phase |
|------|---------|------------------|
| `versions.tf` | Provider + Terraform version pins | - |
| `variables.tf` | All tunable inputs (allowlists, thresholds, device/auth/webhook prefixes) | - |
| `ruleset.tf` | Custom firewall: bypass / block / challenge | `http_request_firewall_custom` |
| `rate-limit.tf` | Per-IP edge rate limits | `http_ratelimit` |
| `transform-headers.tf` | Static security response headers | `http_response_headers_transform` |
| `rules/*.tpl` | Rule expression fragments (rendered by `ruleset.tf`) | - |

The `.tpl` fragments carry `#` documentation comments for reviewers. Cloudflare's
expression parser has no comment syntax, so `ruleset.tf` strips every `#` line
before an expression reaches the API (`local.rule_expr`).

## Custom ruleset order (first match wins)

1. **BYPASSER** - skips the remaining rules for the desktop client, identified by
   the presence of its device-signature header (`var.device_auth_header`,
   default `x-device-signature`). The client sends no User-Agent; it signs each
   request and the origin verifies the signature - the edge only checks presence.
2. **BLOCKER 2A-2H** - one `block` rule per attack class (bad protocol/method/UA,
   exploit paths, query-string injection, host/size guards, route access control
   and the write-auth gate). Webhook receivers are exempt from the UA block.
3. **CHALLENGER** - `managed_challenge` for suspicious browser traffic. Scoped to
   `var.browser_hosts`; `api.vethida.com` is machine-facing (desktop, webhooks,
   health, license lookup) and is never challenged.

## Live surface

The route allowlists, rate limits, and challenger scope reflect this surface:

- **`vethida.com`**: `GET /`, `GET /.well-known/security.txt`
- **`my.vethida.com`**: `/api/auth/*` (Better Auth, allowed by prefix),
  `POST /api/checkout`, dashboard/admin pages (GET)
- **`api.vethida.com`**: `POST /v1/desktop/{activate,deactivate,heartbeat}`
  (device-auth), `GET /v1/license/lookup`,
  `POST /v1/cleanup`, `POST /webhooks/polar`, `GET /health`

Writes are allowed only on `write_allowed_hosts` (all three hosts). The
`/v1/desktop/*` prefix additionally requires the device-auth header. Auth
(`auth_path_prefix`) and webhooks (`webhook_path_prefix`) are exempt from the
write-auth cookie gate; webhooks are also exempt from the User-Agent scanner
block.

## Zone

Create or confirm the Cloudflare zone for `vethida.com`; `www.`, `my.`, and
`api.` are subdomains under it. Put the zone id in `cloudflare_zone_id`.

## Verify before the first apply

No code changes are needed to deploy, but confirm these deployment-specific
values - a wrong one blocks real traffic:

- **`session_cookie_marker`** - must equal your real session cookie's name prefix
  (with the trailing `=`). Sign in on my.vethida.com and read the cookie name in
  DevTools -> Application -> Cookies. The default is the Better Auth
  secure-prefixed name; a wrong value blocks authenticated writes (checkout).
- **Better Auth route names** - the auth prefix and the auth rate limit assume
  Better Auth defaults (`/api/auth/sign-in/*`, `/api/auth/sign-up/*`,
  `/api/auth/forget-password`, `/api/auth/reset-password`). Confirm against your
  config.
- **`device_auth_header`** - the header the desktop client signs under (default
  `x-device-signature`).
- **Generic HTTP-client UAs** - `scanner_user_agent_substrings` still blocks
  `curl/`, `wget`, `python-requests`, etc. Webhooks are exempt, but if you expect
  first-party programmatic access to the public API, trim those entries.

## Deploy

Required inputs have no defaults - supply them via `TF_VAR_` environment
variables, a `*.tfvars` file kept out of version control, or your CI secret
store. Never commit the API token.

- `cloudflare_api_token` - token scoped to *Zone -> WAF -> Edit* on the zone only
- `cloudflare_zone_id`

```sh
terraform init
terraform plan      # run on every PR that touches this directory
terraform apply     # on merge, after one review
```

## Security notes

- **The desktop bypass is header-presence, not signature verification.** Anyone
  can send an `X-Device-Signature` header and skip the custom ruleset; the origin
  is what cryptographically verifies the signature, so a forged header buys
  nothing past the edge. The bypass exists so the machine client is never shown a
  challenge it cannot solve.
- **Edge rate limits run in a separate phase**, so they apply to the desktop
  client too (the custom-ruleset bypass does not cover them). Thresholds leave
  headroom for normal desktop traffic (~1 heartbeat/min), but several desktop
  instances behind one NAT / corporate IP share the per-IP budget.
- **Content-Security-Policy is owned by the application, not the edge** (it needs
  per-route nonces). `Cross-Origin-Embedder-Policy` is intentionally unset too
  (see `transform-headers.tf`).
