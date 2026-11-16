# Scoped to the browser-facing hosts. api.vethida.com is machine-facing
# (desktop client, webhooks, health, license lookup) and must never be
# challenged, so it is excluded here.
http.host in {${browser_hosts}}
and (
    (not ssl)
    or (not cf.edge.server_port in {443})
    or (http.request.uri.path contains "//")
    or (
        http.request.version in {"HTTP/1.0" "HTTP/1.1"}
        and not (
            http.user_agent eq ""
            or http.user_agent contains "WindowsPowerShell"
            or cf.client.bot
        )
    )
    or (ip.geoip.country in {${geo_country}})
    or (cf.threat_score gt 14)
    or (
        not any(http.request.headers["accept-language"][*] ne "")
        and not cf.client.bot
    )
)
