# 2C path-based attack probes and directory traversal.
(${blocked_paths})
or (${traversal})
or (http.request.uri.path contains ".zip" and not http.request.uri.path contains "/api/")
or (http.request.uri.path contains ".gz" and not http.request.uri.path contains "/api/")
