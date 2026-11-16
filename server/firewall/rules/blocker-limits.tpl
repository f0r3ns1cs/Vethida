# 2E host validation + 2F request-size / header-abuse guards.
(not http.host in {${host_allowlist}})
or (http.host eq "")
or (len(http.request.uri.path) gt 2048)
or (len(http.request.uri.query) gt 4096)
or (len(http.user_agent) gt 1024)
or (len(http.referer) gt 2048)
