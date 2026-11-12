import { isPaidTier, type Tier, tierRank } from "./tiers";

// Works out what an account gets from all of its licenses (dead ones included).
// `now` is passed in so tests can control it.

export interface EntitlementInput {
  tier: string; // raw column value
  trial: boolean;
  expiresAt: Date | null; // null = lifetime
  revokedAt: Date | null;
}

export interface ResolvedEntitlement {
  tier: Tier; // always set, falls back to "free"
  trial: boolean;
  expiresAt: Date | null;
}

// Use this to check for a license. resolveEntitlement always returns a tier
// (at least "free") so its result is always truthy.
export function isLicensed(entitlement: ResolvedEntitlement): boolean {
  return isPaidTier(entitlement.tier);
}

// dead if revoked, expired, or the tier isn't one we sell
export function licenseIsLive(row: EntitlementInput, now: Date): boolean {
  if (row.revokedAt !== null) return false;
  if (row.expiresAt !== null && row.expiresAt.getTime() <= now.getTime()) {
    return false;
  }
  return isPaidTier(row.tier);
}

// Picks the best live license, or Free if there isn't one.
// Order: higher tier > paid over trial > lifetime over dated > later expiry
export function resolveEntitlement(
  rows: readonly EntitlementInput[],
  now: Date,
): ResolvedEntitlement {
  let best: EntitlementInput | null = null;

  for (const row of rows) {
    if (!licenseIsLive(row, now)) continue;
    if (best === null || beats(row, best)) best = row;
  }

  // new object each time, callers spread/modify it
  if (best === null || !isPaidTier(best.tier)) {
    return { tier: "free", trial: false, expiresAt: null };
  }
  return { tier: best.tier, trial: best.trial, expiresAt: best.expiresAt };
}

function beats(a: EntitlementInput, b: EntitlementInput): boolean {
  const byTier = tierRank(a.tier) - tierRank(b.tier);
  if (byTier !== 0) return byTier > 0;

  if (a.trial !== b.trial) return !a.trial;

  // lifetime wins over dated
  if ((a.expiresAt === null) !== (b.expiresAt === null)) {
    return a.expiresAt === null;
  }

  if (a.expiresAt === null || b.expiresAt === null) return false;
  return a.expiresAt.getTime() > b.expiresAt.getTime();
}
