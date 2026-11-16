# Skips the remaining custom rules for the desktop client's own API calls.
#
# Kept narrow, because `skip` here bypasses every block, rate limit, injection
# and path rule after it, and a request header is whatever the caller sends. A
# bare "has an X-Device-Signature header" match would let anyone opt out of the
# whole WAF.
#
# So it matches only POST, to the API host, under the desktop path prefix,
# carrying the header. Those endpoints verify an Ed25519
# signature at the origin before doing anything, so a forged header buys nothing
# there - the origin is the real gate, and this rule exists only so Cloudflare
# does not challenge a client that cannot solve a challenge.
#
# Everything else - the portal, the marketing site, the webhook receiver, every
# other path on the API - stays behind the ruleset no matter what headers the
# caller invents.
(
  http.request.method eq "POST"
  and http.host eq "api.vethida.com"
  and starts_with(http.request.uri.path, "${desktop_prefix}")
  and any(http.request.headers["${device_header}"][*] ne "")
)
