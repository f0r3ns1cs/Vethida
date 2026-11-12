// Env vars the portal reads. required = refuses to start without it.
interface EnvSpec {
  name: string;
  required: boolean;
  why: string;
}

const SPEC: readonly EnvSpec[] = [
  {
    name: "DATABASE_URL",
    required: true,
    why: "Postgres connection string. Accounts, sessions and licenses all live there.",
  },
  {
    name: "BETTER_AUTH_SECRET",
    required: true,
    why: "Signs and encrypts session material. Without it nobody can sign in.",
  },
  {
    name: "BETTER_AUTH_URL",
    required: true,
    why: "Canonical origin of the portal. Cookie scope and the Polar return URL are built from it.",
  },
  {
    name: "TURNSTILE_SECRET_KEY",
    required: true,
    why: "Server-side verification for the sign-in challenge. Absent, the OTP endpoint has no bot protection.",
  },
  {
    name: "NEXT_PUBLIC_TURNSTILE_SITE_KEY",
    required: true,
    why: "Client half of the same challenge. A mismatch between the two silently fails every sign-in.",
  },
  {
    name: "POLAR_ACCESS_TOKEN",
    required: true,
    why: "Creates checkouts and customer-portal sessions. Without it nothing can be bought or canceled.",
  },
  {
    name: "CRON_SECRET",
    required: true,
    why: "Authorizes the account-deletion job. A missing value would leave that route open.",
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
    why: "'file' writes mail to disk for development, and logs the filename rather than the code.",
  },
  {
    name: "LEGAL_ENTITY_ADDRESS",
    required: false,
    why: "Postal address for the email footer. Left out of the footer when unset.",
  },
  {
    name: "POLAR_SERVER",
    required: false,
    why: "'sandbox' or 'production'. Defaults to sandbox off production so a dev box cannot create real charges.",
  },
  {
    name: "POLAR_PRODUCT_PRO_YEARLY",
    required: false,
    why: "Product id. A cadence with no product is not offered rather than failing at checkout.",
  },
  {
    name: "POLAR_PRODUCT_PRO_MONTHLY",
    required: false,
    why: "Product id for monthly Pro.",
  },
  {
    name: "POLAR_PRODUCT_PRO_LIFETIME",
    required: false,
    why: "One-time product id for Pro lifetime. Must be a one-time product, not a subscription.",
  },
  {
    name: "POLAR_PRODUCT_ULTIMATE_YEARLY",
    required: false,
    why: "Product id for yearly Ultimate.",
  },
  {
    name: "POLAR_PRODUCT_ULTIMATE_MONTHLY",
    required: false,
    why: "Product id for monthly Ultimate.",
  },
  {
    name: "POLAR_PRODUCT_ULTIMATE_LIFETIME",
    required: false,
    why: "One-time product id for Ultimate lifetime.",
  },
  {
    name: "DISCORD_CLIENT_ID",
    required: false,
    why: "Account linking. The portal hides the link button until client id, secret and bot token are all set.",
  },
  {
    name: "DISCORD_CLIENT_SECRET",
    required: false,
    why: "OAuth secret for account linking.",
  },
  {
    name: "DISCORD_BOT_TOKEN",
    required: false,
    why: "Grants the role once an account is linked.",
  },
  {
    name: "DISCORD_ROLE_PRO",
    required: false,
    why: "Role id granted to Pro license holders.",
  },
  {
    name: "DISCORD_ROLE_ULTIMATE",
    required: false,
    why: "Role id granted to Ultimate license holders.",
  },
  {
    name: "HARD_DELETE_ENABLED",
    required: false,
    why: "Turns on the permanent purge in the account-deletion job. Off by default.",
  },
  {
    name: "NEXT_PUBLIC_DOWNLOAD_URL",
    required: false,
    why: "Direct installer link. Until it's set the download center says the installer isn't published yet.",
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
    `my.vethida.com cannot start. ${missing.length} required environment ` +
      `variable${missing.length === 1 ? " is" : "s are"} missing:\n\n${detail}\n\n` +
      `Every variable this app reads is documented in my.vethida.com/.env.example.`,
  );
}

export const ENV_NAMES: readonly string[] = SPEC.map((e) => e.name);
