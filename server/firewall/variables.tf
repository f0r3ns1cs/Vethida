variable "cloudflare_api_token" {
  description = "Token with Zone WAF: Edit scope on the vethida.com zone. Pull from a secret manager - never commit."
  type        = string
  sensitive   = true
}

variable "cloudflare_zone_id" {
  description = "Zone id for vethida.com."
  type        = string
}

variable "device_auth_header" {
  description = <<-EOT
    Request header whose presence marks a genuine Vethida desktop client. The
    desktop client authenticates with device-auth headers (X-Device-Signature,
    X-Device-Pubkey-Id, X-Request-Timestamp) and sends no custom User-Agent, so
    the WAF keys the desktop bypass and exceptions off this header's presence.
    The origin still cryptographically verifies the signature; the edge only
    checks that the header is present. Must be lowercase -
    http.request.headers keys are lowercased.
  EOT
  type        = string
  default     = "x-device-signature"
}

variable "allowlisted_hosts" {
  description = "Hosts the WAF will accept. Anything else is blocked."
  type        = list(string)
  default = [
    "vethida.com",
    "www.vethida.com",
    "my.vethida.com",
    "api.vethida.com",
  ]
}

variable "browser_hosts" {
  description = <<-EOT
    Hosts that serve the browser-facing site. The CHALLENGER (managed challenge)
    applies only to these. api.vethida.com is machine-facing - desktop client,
    provider webhooks, health checks, license lookup - and must never be
    challenged, since those clients cannot solve a challenge.
  EOT
  type        = list(string)
  default = [
    "vethida.com",
    "www.vethida.com",
    "my.vethida.com",
  ]
}

variable "post_allowlist_paths" {
  description = <<-EOT
    Exact paths that may receive a mutating (non-GET/HEAD) method. Default-deny:
    every other write is blocked at the edge before it reaches the origin. The
    Better Auth catch-all is allowed by prefix (var.auth_path_prefix) rather than
    listed here. Keep this in sync with the application's mutating route table -
    a missing entry means the edge 403s that route.
  EOT
  type        = list(string)
  default = [
    # my.vethida.com (auth catch-all allowed via auth_path_prefix; Server
    # Actions allowed by the next-action header, see blocker-access.tpl)
    "/api/checkout",
    "/api/account",
    "/api/admin/release-announcement",
    # api.vethida.com - desktop paths additionally require the device-auth header
    "/v1/desktop/activate",
    "/v1/desktop/deactivate",
    "/v1/desktop/heartbeat",
    "/v1/desktop/auth/start",
    "/v1/desktop/auth/poll",
    "/v1/cleanup",
    "/webhooks/polar",
  ]
  # "/api/free-license" and "/v1/free-license" were listed here after both
  # routes had already been deleted from the applications. Vethida Free needs no
  # key and no server call, so neither route has anything to serve. An allowlist
  # entry for a route that does not exist is not harmful, but it describes a
  # product that is not the one being shipped.
  #
  # Added: /api/account (DELETE, account deletion), /api/admin/release-
  # announcement (POST), and the two desktop browser-authorization endpoints,
  # which are POST and were missing - so desktop sign-in through the browser
  # would have been 403'd at the edge before it reached the origin.
}

variable "desktop_path_prefix" {
  description = <<-EOT
    Path prefix served exclusively for the desktop client
    (POST /v1/desktop/{activate,deactivate,heartbeat} on api.vethida.com).
    Requests under it must be POST to api.vethida.com and carry the device-auth
    header; anything else is blocked. Browser fetch() to these paths is
    rejected.
  EOT
  type        = string
  default     = "/v1/desktop/"
}

variable "auth_path_prefix" {
  description = <<-EOT
    Path prefix for the browser auth endpoints (Better Auth catch-all, served
    on my.vethida.com). Exempt from the write-auth cookie gate because sign-in
    and sign-up legitimately carry no session cookie yet.
  EOT
  type        = string
  default     = "/api/auth/"
}

variable "geo_challenge_countries" {
  description = "Country codes that get challenged by default. Adjust per actual user base."
  type        = list(string)
  default     = ["CN", "RU", "KP", "IR"]
}

variable "scanner_user_agent_substrings" {
  description = <<-EOT
    User-agent substrings to outright block: pen-test and scraping tool
    signatures. Webhook receivers (var.webhook_path_prefix) are exempt from this
    block. Note the generic HTTP-client entries (curl/, wget, python-requests,
    Go-http-client, Java/, libwww-perl) also match legitimate automation; keep
    them only if you do not expect first-party programmatic API access.
  EOT
  type        = list(string)
  default = [
    "sqlmap",
    "nikto",
    "nmap",
    "Nessus",
    "masscan",
    "ZmEu",
    "DirBuster",
    "Gobuster",
    "wpscan",
    "Acunetix",
    "BurpSuite",
    "Havij",
    "OpenVAS",
    "w3af",
    "HTTrack",
    "wget",
    "python-requests",
    "Go-http-client",
    "Java/",
    "libwww-perl",
    "curl/",
  ]
}

variable "blocked_path_substrings" {
  description = <<-EOT
    Path substrings that only automated scanners probe: dotfiles, VCS
    metadata, CMS/admin panels, app-server consoles, and dropped
    source/backup artifacts. A product marketing site + JSON API never
    serves these, so a substring match is a safe hard block.
  EOT
  type        = list(string)
  default = [
    "/.env",
    "/.git",
    "/.svn",
    "/.htaccess",
    "/.htpasswd",
    "/.DS_Store",
    "/wp-admin",
    "/wp-login",
    "/wp-content",
    "/wp-includes",
    "/wp-config",
    "/wordpress",
    "/xmlrpc.php",
    "/phpmyadmin",
    "/phpMyAdmin",
    "/pma",
    "/adminer",
    "/cgi-bin",
    "/shell",
    "/eval",
    "/debug",
    "/console",
    "/actuator",
    "/manager",
    "/solr",
    "/struts",
    "/server-status",
    "/server-info",
    "/config.json",
    "/config.yml",
    "/config.yaml",
    "/backup",
    ".sql",
    ".bak",
    ".old",
    ".orig",
    ".tar",
  ]
}

variable "path_traversal_substrings" {
  description = "Directory-traversal markers, including double- and overlong-encoded forms."
  type        = list(string)
  default = [
    "..",
    "%2e%2e",
    "%252e",
    "%c0%ae",
  ]
}

# Query-string injection matching runs case-insensitively via lower(), so
# every pattern below MUST be lowercase. This catches mixed-case evasion that
# a case-sensitive substring list would miss.
variable "sqli_query_substrings" {
  description = "Lowercase SQL-injection markers matched against lower(http.request.uri.query)."
  type        = list(string)
  default = [
    "union select",
    "union%20select",
    "or 1=1",
    "' or '",
    "1' or '1'='1",
    "--",
    "/*",
    "xp_cmdshell",
    "exec(",
    "execute(",
    "sp_executesql",
  ]
}

variable "xss_query_substrings" {
  description = "Lowercase reflected-XSS markers matched against lower(http.request.uri.query)."
  type        = list(string)
  default = [
    "<script",
    "%3cscript",
    "javascript:",
    "onerror=",
    "onload=",
    "onclick=",
    "onfocus=",
    "onmouseover=",
    "eval(",
    "document.cookie",
    "document.location",
    "window.location",
    "string.fromcharcode",
  ]
}

variable "cmdi_query_substrings" {
  description = "Lowercase command-injection markers matched against lower(http.request.uri.query)."
  type        = list(string)
  default = [
    "; ls",
    ";ls",
    "| ls",
    "|ls",
    "; cat ",
    ";cat ",
    "$()",
    "`",
    "/etc/passwd",
    "/etc/shadow",
    "cmd.exe",
    "powershell",
    "/bin/sh",
    "/bin/bash",
  ]
}

variable "post_only_paths" {
  description = <<-EOT
    Mutating endpoints that must reject every non-POST method (a GET to these is
    blocked, not just non-GET/HEAD writes). Auth is excluded because Better Auth
    serves GET routes (session, provider callbacks) under the same prefix.
  EOT
  type        = list(string)
  default = [
    "/api/checkout",
    "/v1/cleanup",
  ]
}

variable "write_allowed_hosts" {
  description = <<-EOT
    Hosts permitted to receive mutating (non-GET/HEAD) requests. Writes to any
    other host are blocked even for allowlisted paths. The live surface takes
    writes on all three: vethida.com (Server Actions), my.vethida.com (auth,
    checkout, Server Actions), and api.vethida.com (/v1/*, webhooks).
  EOT
  type        = list(string)
  default = [
    "vethida.com",
    "my.vethida.com",
    "api.vethida.com",
  ]
}

variable "session_cookie_marker" {
  description = <<-EOT
    Substring that identifies an authenticated browser session cookie. Mutating
    browser requests that lack it - and are not a webhook, an auth endpoint, or
    the desktop client - are blocked at the edge.

    This must be the cookie my.vethida.com actually sets. The name is built in
    my.vethida.com/lib/auth.ts from advanced.cookiePrefix ("__Host-vethida" in
    production) plus the cookie key, and useSecureCookies is false there so
    Better Auth does not add its own "__Secure-" prefix on top - the __Host-
    prefix is applied by hand because it is the stricter of the two.

    Better Auth's stock name, "__Secure-better-auth.session_token=", is not what
    this deployment sets. Using it would block every authenticated mutating call
    from a browser: checkout, account deletion, and every Server Action.

    If the prefix in lib/auth.ts ever changes, change this with it. To confirm
    against a live deployment: sign in on my.vethida.com, open DevTools ->
    Application -> Cookies, and read the name.
  EOT
  type        = string
  default     = "__Host-vethida.session_token="
}

variable "webhook_path_prefix" {
  description = <<-EOT
    Path prefix for inbound provider webhooks (e.g. /webhooks/polar). They arrive
    cookieless and with a provider User-Agent, and are verified by signature at
    the origin, so requests under this prefix are exempt from the write-auth
    cookie gate and from the User-Agent scanner block.
  EOT
  type        = string
  default     = "/webhooks/"
}

