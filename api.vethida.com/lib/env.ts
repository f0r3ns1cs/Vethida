export function requiredEnv(name: string): string {
  const value = process.env[name];
  if (!value) {
    throw new Error(`Missing required environment variable: ${name}`);
  }
  return value;
}

// Env vars the service reads. required = refuses to start without it (requiredEnv()
// alone would only fail on first use).
interface EnvSpec {
  name: string;
  required: boolean;
  why: string;
}

const SPEC: readonly EnvSpec[] = [
  {
    name: "DATABASE_URL",
    required: true,
    why: "Postgres connection string. Nothing in this service works without it.",
  },
  {
    name: "LICENSE_SIGNING_KEY",
    required: true,
    why: "base64 DER PKCS8 P-256 private key that signs license tokens. Without it every activation fails closed.",
  },
  {
    name: "LICENSE_TOKEN_KID",
    required: true,
    why: "Key id embedded in issued tokens so the desktop client can select the right public key.",
  },
  {
    name: "POLAR_WEBHOOK_SECRET",
    required: true,
    why: "Verifies the signature on Polar deliveries. Absent, provisioning stops on the first purchase.",
  },
  {
    name: "INTERNAL_API_TOKEN",
    required: true,
    why: "Bearer token for service-to-service calls from the portal and the Discord bot.",
  },
  {
    name: "CRON_SECRET",
    required: true,
    why: "Authorizes the scheduled cleanup and reminder routes. A missing value would leave them open.",
  },
  {
    name: "FINGERPRINT_PEPPER",
    required: true,
    why:
      "HMAC key applied to every hardware-fingerprint hash before it is stored, " +
      "so a database leak can't be brute-forced. Can't be changed later without " +
      "invalidating every stored fingerprint.",
  },
  {
    name: "RESEND_API_KEY",
    required: false,
    why: "Transactional email. Optional only because EMAIL_TRANSPORT=file is a supported development mode.",
  },
  {
    name: "RESEND_FROM",
    required: false,
    why: "From address for transactional email. Needed in practice whenever RESEND_API_KEY is set.",
  },
  {
    name: "EMAIL_TRANSPORT",
    required: false,
    why: "'file' writes mail to disk for development. Unset means real delivery.",
  },
  {
    name: "LEGAL_ENTITY_ADDRESS",
    required: false,
    why: "Postal address for the email footer. Left out of the footer when unset.",
  },
  {
    name: "POLAR_ACCESS_TOKEN",
    required: false,
    why: "Read-only Polar calls made while handling webhooks. Provisioning degrades rather than fails without it.",
  },
  {
    name: "POLAR_SERVER",
    required: false,
    why: "'sandbox' or 'production'. Defaults to sandbox off production so a misconfigured box cannot take real money.",
  },
  {
    name: "PORTAL_ORIGIN",
    required: false,
    why: "Portal origin used when building links in outbound email.",
  },
  {
    name: "UPDATE_FEED_URL",
    required: false,
    why: "Where /v1/version reads the desktop update feed from.",
  },
  {
    name: "NEXT_PUBLIC_SENTRY_DSN",
    required: false,
    why: "Error reporting. Absent means telemetry is a local no-op.",
  },
];

// throws listing every missing variable at once
export function validateEnv(): void {
  // next build runs instrumentation too, without secrets
  if (process.env.NEXT_PHASE === "phase-production-build") return;

  const missing = SPEC.filter((e) => e.required && !process.env[e.name]);
  if (missing.length === 0) return;

  const detail = missing.map((e) => `  ${e.name}\n      ${e.why}`).join("\n");

  throw new Error(
    `api.vethida.com cannot start. ${missing.length} required environment ` +
      `variable${missing.length === 1 ? " is" : "s are"} missing:\n\n${detail}\n\n` +
      `Every variable this service reads is documented in api.vethida.com/.env.example.`,
  );
}

export const ENV_NAMES: readonly string[] = SPEC.map((e) => e.name);
