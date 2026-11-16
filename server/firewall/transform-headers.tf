# Response-header injection at the edge. The application already sets these
# via Next.js middleware; the edge layer guarantees coverage for any cached
# response that bypasses the origin. Only static, single-value headers belong
# here.
#
# Deliberately NOT set at the edge:
#   - Content-Security-Policy: it is route-specific and nonce-based, so the
#     application owns it. A `set` here would overwrite the app's real policy.
#   - Cross-Origin-Embedder-Policy: require-corp is only needed for cross-origin
#     isolation (SharedArrayBuffer) and breaks third-party embeds (e.g. Stripe)
#     unless every subresource sends CORP/CORS. This app does not need it.
#
# Configure as a transform-rules ruleset (not a custom WAF rule).

resource "cloudflare_ruleset" "vethida_response_headers" {
  zone_id     = var.cloudflare_zone_id
  name        = "vethida-response-headers"
  description = "Edge security-header injection (Terraform-managed)."
  kind        = "zone"
  phase       = "http_response_headers_transform"

  rules {
    description = "Inject security headers on every response"
    expression  = "true"
    action      = "rewrite"
    action_parameters {
      headers {
        name      = "X-Content-Type-Options"
        operation = "set"
        value     = "nosniff"
      }
      headers {
        name      = "X-Frame-Options"
        operation = "set"
        value     = "DENY"
      }
      headers {
        name      = "X-XSS-Protection"
        operation = "set"
        value     = "0"
      }
      headers {
        name      = "X-Download-Options"
        operation = "set"
        value     = "noopen"
      }
      headers {
        name      = "Referrer-Policy"
        operation = "set"
        value     = "strict-origin-when-cross-origin"
      }
      headers {
        name      = "Permissions-Policy"
        operation = "set"
        value     = "camera=(), microphone=(), geolocation=(), browsing-topics=()"
      }
      headers {
        name      = "Strict-Transport-Security"
        operation = "set"
        value     = "max-age=63072000; includeSubDomains; preload"
      }
      headers {
        name      = "Cross-Origin-Opener-Policy"
        operation = "set"
        value     = "same-origin"
      }
      headers {
        name      = "Cross-Origin-Resource-Policy"
        operation = "set"
        value     = "same-origin"
      }
      headers {
        name      = "X-Permitted-Cross-Domain-Policies"
        operation = "set"
        value     = "none"
      }
    }
    enabled = true
  }
}
