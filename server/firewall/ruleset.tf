# Cloudflare WAF - custom ruleset for vethida.com.
#
# Sections:
#
#   1. CHALLENGER - managed_challenge for suspicious-but-not-definitively-bad
#   2. BLOCKER    - drop outright, split into one rule per attack class so each
#                  has its own hit counter and no single expression approaches
#                  Cloudflare's per-rule expression length limit
#   3. BYPASSER   - skip remaining rules for the desktop client
#
# Each section is rendered from a templatefile() so the .tpl fragments stay the
# canonical thing reviewers read. The Terraform here is glue.
#
# Rule order in `rules { ... }` matters: rules evaluate top-to-bottom, first
# match wins. The bypass is listed first so the desktop client never trips a
# downstream block; the blockers are next; the challenger is last.

locals {
  # Quoted, space-separated path/host lists for the .tpl `in {...}` sets.
  post_allowlist_expr = join(" ", [for p in var.post_allowlist_paths : "\"${p}\""])
  host_allowlist_expr = join(" ", [for h in var.allowlisted_hosts : "\"${h}\""])
  geo_country_expr    = join(" ", [for c in var.geo_challenge_countries : "\"${c}\""])
  post_only_expr      = join(" ", [for p in var.post_only_paths : "\"${p}\""])
  write_hosts_expr    = join(" ", [for h in var.write_allowed_hosts : "\"${h}\""])
  browser_hosts_expr  = join(" ", [for h in var.browser_hosts : "\"${h}\""])

  # Cloudflare has no "contains any of a set" operator, so each substring
  # becomes its own `contains` clause OR'd together. Built here from the
  # variable lists so variables.tf stays the single source of truth.
  scanner_ua_clause   = join(" or ", [for s in var.scanner_user_agent_substrings : "http.user_agent contains \"${s}\""])
  blocked_path_clause = join(" or ", [for p in var.blocked_path_substrings : "http.request.uri.path contains \"${p}\""])
  traversal_clause    = join(" or ", [for t in var.path_traversal_substrings : "http.request.uri.path contains \"${t}\""])
  sqli_clause         = join(" or ", [for s in var.sqli_query_substrings : "lower(http.request.uri.query) contains \"${s}\""])
  xss_clause          = join(" or ", [for s in var.xss_query_substrings : "lower(http.request.uri.query) contains \"${s}\""])
  cmdi_clause         = join(" or ", [for s in var.cmdi_query_substrings : "lower(http.request.uri.query) contains \"${s}\""])

  # Raw templatefile renders. The .tpl fragments carry `#` documentation
  # comments for the humans who review them; they are stripped in
  # local.rule_expr below before any expression reaches Cloudflare.
  rule_src = {
    bypasser = templatefile("${path.module}/rules/bypasser.tpl", {
      device_header  = var.device_auth_header
      desktop_prefix = var.desktop_path_prefix
    })
    challenger = templatefile("${path.module}/rules/challenger.tpl", {
      geo_country   = local.geo_country_expr
      browser_hosts = local.browser_hosts_expr
    })
    blocker_protocol = templatefile("${path.module}/rules/blocker-protocol.tpl", {
      scanner_ua_clause = local.scanner_ua_clause
      webhook_prefix    = var.webhook_path_prefix
    })
    blocker_paths = templatefile("${path.module}/rules/blocker-paths.tpl", {
      blocked_paths = local.blocked_path_clause
      traversal     = local.traversal_clause
    })
    blocker_injection = templatefile("${path.module}/rules/blocker-injection.tpl", {
      sqli = local.sqli_clause
      xss  = local.xss_clause
      cmdi = local.cmdi_clause
    })
    blocker_limits = templatefile("${path.module}/rules/blocker-limits.tpl", {
      host_allowlist = local.host_allowlist_expr
    })
    blocker_access = templatefile("${path.module}/rules/blocker-access.tpl", {
      post_allowlist = local.post_allowlist_expr
      post_only      = local.post_only_expr
      write_hosts    = local.write_hosts_expr
      browser_hosts  = local.browser_hosts_expr
      cookie_marker  = var.session_cookie_marker
      device_header  = var.device_auth_header
      desktop_prefix = var.desktop_path_prefix
      auth_prefix    = var.auth_path_prefix
      webhook_prefix = var.webhook_path_prefix
    })
  }

  # Cloudflare's expression parser (wirefilter) has no comment syntax, so every
  # `#` line is dropped and the result trimmed. What remains is a pure boolean
  # expression, kept well under the 4,096-character per-rule limit.
  rule_expr = {
    for k, v in local.rule_src :
    k => trimspace(join("\n", [
      for line in split("\n", replace(v, "\r", "")) : line
      if !startswith(trimspace(line), "#")
    ]))
  }
}

resource "cloudflare_ruleset" "vethida_waf_custom" {
  zone_id     = var.cloudflare_zone_id
  name        = "vethida-waf-custom"
  description = "Default-deny WAF for vethida.com (Terraform-managed)."
  kind        = "zone"
  phase       = "http_request_firewall_custom"

  rules {
    description = "BYPASSER - skip remaining rules for Vethida desktop client"
    expression  = local.rule_expr.bypasser
    action      = "skip"
    action_parameters {
      ruleset = "current"
    }
    enabled = true
  }

  rules {
    description = "BLOCKER 2A/2B - bad protocol, method, or user agent"
    expression  = local.rule_expr.blocker_protocol
    action      = "block"
    enabled     = true
  }

  rules {
    description = "BLOCKER 2C - exploit-path probes and directory traversal"
    expression  = local.rule_expr.blocker_paths
    action      = "block"
    enabled     = true
  }

  rules {
    description = "BLOCKER 2D - SQLi / XSS / command injection in query string"
    expression  = local.rule_expr.blocker_injection
    action      = "block"
    enabled     = true
  }

  rules {
    description = "BLOCKER 2E/2F - host validation and request-size guards"
    expression  = local.rule_expr.blocker_limits
    action      = "block"
    enabled     = true
  }

  rules {
    description = "BLOCKER 2G/2H - route access control and write-auth gate"
    expression  = local.rule_expr.blocker_access
    action      = "block"
    enabled     = true
  }

  rules {
    description = "CHALLENGER - Turnstile for suspicious traffic"
    expression  = local.rule_expr.challenger
    action      = "managed_challenge"
    enabled     = true
  }
}
