// Tier names and descriptions shared by the portal, the api emails and the bot.
// Free is never stored on a license row, it's just what you get without one.

export type PaidTier = "pro" | "ultimate"; // matches license_tier_check
export type Tier = "free" | PaidTier;

// lowest first, also used as the rank order
export const PAID_TIERS: readonly PaidTier[] = ["pro", "ultimate"] as const;
export const TIERS: readonly Tier[] = ["free", "pro", "ultimate"] as const;

// 0 = free (or unknown), only meaningful when comparing two tiers
export function tierRank(tier: string): number {
  return PAID_TIERS.indexOf(tier as PaidTier) + 1;
}

const LABELS: Record<Tier, string> = {
  free: "Free",
  pro: "Pro",
  ultimate: "Ultimate",
};

// vethida.com has its own copy in lib/pricing.ts (check-integrity compares them)
const BLURBS: Record<Tier, string> = {
  free: "The basics, with no account and no expiry.",
  pro: "Everything in Free, plus the processor, memory, storage, graphics and network settings, and the debloating.",
  ultimate:
    "Adds Genesis, firmware, deep power management and the driver-level graphics settings.",
};

export function tierBlurb(tier: string): string {
  return BLURBS[tier as Tier] ?? "";
}

// longer version used by the discord bot. Not checked against the site, update both
const INCLUDES: Record<Tier, string> = {
  free: "The low-risk settings, the system readouts and Health score, disk cleanup, the quick fixes, System Restore, the curated installs and the speed test. No account and no key.",
  pro: "Access to paid Vethida features for one machine.",
  ultimate:
    "Everything in Pro plus Genesis, BIOS and firmware settings, the NVIDIA driver profile and power settings, advanced network and storage power settings, the low-latency power plan, and early access to experimental tweaks.",
};

export function tierIncludes(tier: string): string {
  return INCLUDES[tier as Tier] ?? "";
}

// Must match the EULA and the pricing page word for word (check-integrity checks it).
export const LIFETIME_SCOPE =
  "A lifetime license covers every update to the tier you bought, including new major versions, at no further charge.";

// unknown values come back as-is so they're easy to spot
export function tierLabel(tier: string): string {
  return LABELS[tier as Tier] ?? tier;
}

export function isPaidTier(tier: unknown): tier is PaidTier {
  return tier === "pro" || tier === "ultimate";
}
