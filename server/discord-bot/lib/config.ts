import { config } from "dotenv";
import { resolve } from "path";
import { z } from "zod";

import { PAID_TIERS, tierLabel, tierRank, type PaidTier } from "@vethida/db";

config({ path: resolve(process.cwd(), ".env.local") });
config({ path: resolve(process.cwd(), ".env") });

const boolFromString = (fallback: boolean) =>
  z
    .string()
    .optional()
    .transform((value) => {
      if (value === undefined || value.trim() === "") {
        return fallback;
      }
      const normalized = value.trim().toLowerCase();
      return normalized === "1" || normalized === "true" || normalized === "yes";
    });

const intFromString = (fallback: number) =>
  z
    .string()
    .optional()
    .transform((value, ctx) => {
      if (value === undefined || value.trim() === "") {
        return fallback;
      }
      const parsed = Number.parseInt(value.trim(), 10);
      if (!Number.isFinite(parsed) || parsed < 0) {
        ctx.addIssue({ code: z.ZodIssueCode.custom, message: "expected non-negative integer" });
        return z.NEVER;
      }
      return parsed;
    });

const urlSchema = z
  .string()
  .min(1)
  .refine(
    (value) => {
      try {
        const url = new URL(value);
        const isLocalHttp =
          url.protocol === "http:" &&
          (url.hostname === "localhost" || url.hostname === "127.0.0.1");
        return url.protocol === "https:" || isLocalHttp;
      } catch {
        return false;
      }
    },
    { message: "must be https:// (or http://localhost)" }
  );

const EnvSchema = z.object({
  NODE_ENV: z.enum(["development", "production", "test"]).default("production"),
  LOG_LEVEL: z.enum(["trace", "debug", "info", "warn", "error", "fatal"]).default("info"),
  HEALTH_PORT: intFromString(8080),
  HEALTH_HOST: z.string().default("127.0.0.1"),

  DISCORD_BOT_TOKEN: z.string().min(10, "DISCORD_BOT_TOKEN is required"),
  DISCORD_CLIENT_ID: z.string().regex(/^\d{17,20}$/, "invalid DISCORD_CLIENT_ID"),
  DISCORD_GUILD_ID: z.string().regex(/^\d{17,20}$/, "invalid DISCORD_GUILD_ID"),

  WEBSITE_URL: urlSchema.default("https://vethida.com"),
  PORTAL_URL: urlSchema.default("https://my.vethida.com"),
  API_BASE_URL: urlSchema.default("https://api.vethida.com"),
  STATUS_URL: urlSchema.optional(), // no status page yet

  VETHIDA_INTERNAL_API_TOKEN: z.string().optional().default(""),
  ADMIN_TOKEN: z.string().optional().default(""),

  DISCORD_ALLOW_LEGACY_KEY_DM: boolFromString(false),

  // staff roles (manual)
  DISCORD_ROLE_FOUNDER: z.string().default(""),
  DISCORD_ROLE_TEAM: z.string().default(""),
  DISCORD_ROLE_PARTNER: z.string().default(""),

  // synced from licenses
  DISCORD_ROLE_CUSTOMER: z.string().default(""),

  DISCORD_ROLE_BOOSTER: z.string().default(""),
  DISCORD_ROLE_ANNOUNCEMENTS: z.string().default(""),
  DISCORD_ROLE_CHANGELOG: z.string().default(""),
  DISCORD_ROLE_MUTED: z.string().default(""),
  DISCORD_ROLE_VERIFIED: z.string().default(""),
  DISCORD_ROLE_QUARANTINE: z.string().default(""),

  AUTOMOD_ENABLED: boolFromString(true),
  MIN_ACCOUNT_AGE_HOURS: intFromString(72),
  RAID_JOIN_THRESHOLD: intFromString(10),
  RAID_JOIN_WINDOW_SECONDS: intFromString(30),
  SPAM_MESSAGE_THRESHOLD: intFromString(5),
  SPAM_WINDOW_SECONDS: intFromString(6),
  DUPLICATE_THRESHOLD: intFromString(3),

  // AI (optional, without a key the AI commands just say it's disabled)
  ANTHROPIC_API_KEY: z.string().optional().default(""),
  AI_ENABLED: boolFromString(true),
  AI_MODEL: z.string().default("claude-haiku-4-5"),
  AI_SUPPORT_CHANNEL_ID: z.string().default(""),
  AI_MAX_HISTORY: intFromString(10),
  AI_USER_DAILY_TOKEN_LIMIT: intFromString(50_000),
  AI_USER_RATE_LIMIT: intFromString(5),
  AI_USER_RATE_WINDOW_SECONDS: intFromString(60),
  AI_RESPONSE_MAX_TOKENS: intFromString(800),

  DATABASE_URL: z.string().min(1),
});

type Env = z.infer<typeof EnvSchema>;

function loadEnv(): Env {
  const parsed = EnvSchema.safeParse(process.env);
  if (!parsed.success) {
    const messages = parsed.error.issues
      .map((issue) => `  ${issue.path.join(".")}: ${issue.message}`)
      .join("\n");
    // logger isn't available yet
    console.error(`[config] invalid environment:\n${messages}`);
    process.exit(1);
  }
  return parsed.data;
}

const env = loadEnv();

export const NODE_ENV = env.NODE_ENV;
export const LOG_LEVEL = env.LOG_LEVEL;
export const HEALTH_PORT = env.HEALTH_PORT;
export const HEALTH_HOST = env.HEALTH_HOST;

export const BOT_TOKEN = env.DISCORD_BOT_TOKEN;
export const CLIENT_ID = env.DISCORD_CLIENT_ID;
export const GUILD_ID = env.DISCORD_GUILD_ID;

export const WEBSITE_URL = env.WEBSITE_URL;
export const PORTAL_URL = env.PORTAL_URL;
export const API_BASE_URL = env.API_BASE_URL;
export const STATUS_URL = env.STATUS_URL;

export const REFUND_WINDOW_DAYS = 14;

export const INTERNAL_API_TOKEN =
  env.VETHIDA_INTERNAL_API_TOKEN || env.ADMIN_TOKEN || "";

export const ALLOW_LEGACY_DISCORD_KEY_DM = env.DISCORD_ALLOW_LEGACY_KEY_DM;

export const ROLE_IDS = {
  founder: env.DISCORD_ROLE_FOUNDER,
  team: env.DISCORD_ROLE_TEAM,
  partner: env.DISCORD_ROLE_PARTNER,

  customer: env.DISCORD_ROLE_CUSTOMER,

  booster: env.DISCORD_ROLE_BOOSTER,
  announcements: env.DISCORD_ROLE_ANNOUNCEMENTS,
  changelog: env.DISCORD_ROLE_CHANGELOG,
  muted: env.DISCORD_ROLE_MUTED,
  verified: env.DISCORD_ROLE_VERIFIED,
  quarantine: env.DISCORD_ROLE_QUARANTINE,
} as const;

export const AUTOMOD = {
  enabled: env.AUTOMOD_ENABLED,
  minAccountAgeHours: env.MIN_ACCOUNT_AGE_HOURS,
  raidJoinThreshold: env.RAID_JOIN_THRESHOLD,
  raidJoinWindowSeconds: env.RAID_JOIN_WINDOW_SECONDS,
  spamMessageThreshold: env.SPAM_MESSAGE_THRESHOLD,
  spamWindowSeconds: env.SPAM_WINDOW_SECONDS,
  duplicateThreshold: env.DUPLICATE_THRESHOLD,
} as const;

// enabled = AI_ENABLED and a key is set
export const AI = {
  enabled: env.AI_ENABLED && env.ANTHROPIC_API_KEY.length > 0,
  apiKey: env.ANTHROPIC_API_KEY,
  model: env.AI_MODEL,
  supportChannelId: env.AI_SUPPORT_CHANNEL_ID,
  maxHistory: env.AI_MAX_HISTORY,
  dailyTokenLimit: env.AI_USER_DAILY_TOKEN_LIMIT,
  rateLimit: env.AI_USER_RATE_LIMIT,
  rateWindowSeconds: env.AI_USER_RATE_WINDOW_SECONDS,
  maxTokens: env.AI_RESPONSE_MAX_TOKENS,
} as const;

export const DATABASE_URL = env.DATABASE_URL;

export const BOT_NAME = "Vethida Discord Bot";
export const BOT_COLOR = 0xfa4b00;
export const BOT_COLOR_SUCCESS = 0x2ecc71;
export const BOT_COLOR_WARNING = 0xf1c40f;
export const BOT_COLOR_ERROR = 0xe74c3c;
export const BOT_COLOR_INFO = 0x3498db;

export const TICKET_WARN_HOURS = 48;
export const TICKET_CLOSE_HOURS = 72;
export const TICKET_CATEGORIES = ["Billing", "Technical", "Account", "General"] as const;
export type TicketCategory = (typeof TICKET_CATEGORIES)[number];

export const MEMBER_COUNT_INTERVAL = 15 * 60 * 1000;

export const ACTIVATION_PREFIX = "Vethida-Activate-";

// tier names/ranks come from @vethida/db, only the colors are bot specific
export type TierSlug = PaidTier;

export interface Tier {
  slug: TierSlug;
  label: string;
  color: number;
  rank: number; // higher wins when someone has more than one license
}

const TIER_COLORS: Record<TierSlug, number> = {
  pro: 0x3498db,
  ultimate: 0xf1c40f,
};

export const TIERS: Record<TierSlug, Tier> = Object.fromEntries(
  PAID_TIERS.map((slug) => [
    slug,
    {
      slug,
      label: tierLabel(slug),
      color: TIER_COLORS[slug],
      rank: tierRank(slug),
    },
  ]),
) as Record<TierSlug, Tier>;

export const PURCHASABLE_TIERS: readonly TierSlug[] = PAID_TIERS;

export const CHANNEL_NAMES = {
  welcome: "welcome",
  rules: "rules",
  announcements: "announcements",
  changelog: "changelog",
  downloads: "downloads",
  status: "status",
  general: "general",
  offtopic: "off-topic",
  purchase: "purchase",
  tickets: "tickets",
  staffTickets: "staff-tickets",
  staffLogs: "staff-logs",
  auditLogs: "audit-logs",
  autoModLogs: "automod-logs",
  honeypot: "free-keys",
  bugs: "bug-reports",
  features: "feature-requests",
  knownIssues: "known-issues",
} as const;
