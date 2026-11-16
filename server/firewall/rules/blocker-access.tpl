# 2G route-level access control + 2H write-auth gate.
#
# The desktop client authenticates with a device-signature header
# (var.device_auth_header) and sends no User-Agent; the BYPASSER skips it ahead
# of these blocks, so traffic reaching this rule is browser/other. The origin
# still verifies the signature - the edge only checks that the header is
# present.
#
# Next.js Server Actions do NOT post to an API route. They post to the URL of
# the page the form is on - /dashboard/settings, /dashboard/billing/cancel,
# /admin - carrying a `next-action` header that names the action. There is no
# fixed set of paths to allowlist, because the set is "every page that renders a
# form", and it changes whenever a page does.
#
# The exact-path allowlist below therefore denied all of them, which is most of
# the portal: canceling a subscription, deleting an account, deactivating a
# device, claiming a license, changing email preferences, linking Discord, and
# every admin action. Each would have returned 403 from the edge, before
# reaching the origin, on a working application.
#
# Recognized here by that header, and only on a browser host with a session
# cookie already present, so this is not a general write exemption: the request
# has to look like what the portal's own pages send.
(http.request.uri.path contains "/internal_")
or (
    not http.request.method in {"GET" "HEAD"}
    and not (
        http.host in {${browser_hosts}}
        and any(http.request.headers["next-action"][*] ne "")
        and http.cookie contains "${cookie_marker}"
    )
    and (
        (
            not http.request.uri.path in {${post_allowlist}}
            and not starts_with(http.request.uri.path, "${auth_prefix}")
        )
        or not http.host in {${write_hosts}}
    )
)
or (
    starts_with(http.request.uri.path, "${desktop_prefix}")
    and (
        http.request.method ne "POST"
        or http.host ne "api.vethida.com"
        or not any(http.request.headers["${device_header}"][*] ne "")
    )
)
or (
    http.request.uri.path in {${post_only}}
    and http.request.method ne "POST"
)
or (
    not http.request.method in {"GET" "HEAD"}
    and not http.cookie contains "${cookie_marker}"
    and not starts_with(http.request.uri.path, "${webhook_prefix}")
    and not starts_with(http.request.uri.path, "${auth_prefix}")
    and not any(http.request.headers["${device_header}"][*] ne "")
)
