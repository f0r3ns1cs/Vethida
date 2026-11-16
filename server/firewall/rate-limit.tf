# Edge-layer rate limits - a first line of defense before traffic reaches the
# origin's own limiter.
#
# Each rule is its own ruleset entry so hit-counter observability is per-rule.
# Tuning lives here, not in code, so thresholds can be ratcheted during an
# incident without a release. Paths track the live surface. The desktop client
# IS subject to these limits: rate limiting runs in a separate phase that the
# custom-ruleset bypass does not cover.
#
# mitigation_timeout accepts only 0, 10, 60, 120, 300, 600, 3600, 86400
# (non-Enterprise). Auth paths follow Better Auth's route conventions - verify
# them against your deployment.

resource "cloudflare_ruleset" "vethida_waf_rate_limit" {
  zone_id     = var.cloudflare_zone_id
  name        = "vethida-waf-rate-limit"
  description = "Edge rate limits (Terraform-managed)."
  kind        = "zone"
  phase       = "http_ratelimit"

  rules {
    description = "Auth abuse - 10/min/IP, block 10min"
    expression  = "starts_with(http.request.uri.path, \"/api/auth/sign-in\") or starts_with(http.request.uri.path, \"/api/auth/sign-up\") or starts_with(http.request.uri.path, \"/api/auth/forget-password\") or starts_with(http.request.uri.path, \"/api/auth/reset-password\")"
    action      = "block"
    ratelimit {
      characteristics     = ["ip.src", "cf.colo.id"]
      period              = 60
      requests_per_period = 10
      mitigation_timeout  = 600
    }
    enabled = true
  }

  rules {
    description = "License activation / lookup abuse - 20/min/IP, block 5min"
    expression  = "http.request.uri.path in {\"/v1/desktop/activate\" \"/v1/desktop/deactivate\" \"/v1/license/lookup\"}"
    action      = "block"
    ratelimit {
      characteristics     = ["ip.src", "cf.colo.id"]
      period              = 60
      requests_per_period = 20
      mitigation_timeout  = 300
    }
    enabled = true
  }

  rules {
    description = "Heartbeat flood - 6/min/IP, block 5min"
    expression  = "http.request.uri.path eq \"/v1/desktop/heartbeat\""
    action      = "block"
    ratelimit {
      characteristics     = ["ip.src", "cf.colo.id"]
      period              = 60
      requests_per_period = 6
      mitigation_timeout  = 300
    }
    enabled = true
  }

  rules {
    description = "License creation / checkout abuse - 15/min/IP, block 10min"
    expression  = "http.request.uri.path eq \"/api/checkout\""
    action      = "block"
    ratelimit {
      characteristics     = ["ip.src", "cf.colo.id"]
      period              = 60
      requests_per_period = 15
      mitigation_timeout  = 600
    }
    enabled = true
  }

  rules {
    description = "General API flood - 120/min/IP, block 1min"
    expression  = "http.host eq \"api.vethida.com\""
    action      = "block"
    ratelimit {
      characteristics     = ["ip.src", "cf.colo.id"]
      period              = 60
      requests_per_period = 120
      mitigation_timeout  = 60
    }
    enabled = true
  }
}
