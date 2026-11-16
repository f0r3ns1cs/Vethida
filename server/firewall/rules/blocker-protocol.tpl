# 2A protocol/method + 2B user-agent filtering.
# Webhook receivers are exempt from the UA block: providers send their own
# User-Agent and the origin verifies the payload signature.
(not http.request.version in {"HTTP/1.0" "HTTP/1.1" "HTTP/2" "HTTP/3"})
or (not http.request.method in {"GET" "POST" "PUT" "DELETE" "PATCH" "HEAD" "OPTIONS"})
or (http.request.method in {"TRACE" "TRACK" "CONNECT"})
or (
    not starts_with(http.request.uri.path, "${webhook_prefix}")
    and (
        http.user_agent in {"undefined" "null" "-" "CheckHost (https://check-host.net/)"}
        or ${scanner_ua_clause}
    )
)
