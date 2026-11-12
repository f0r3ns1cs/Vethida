import { and, desc, eq, isNull, sql } from "drizzle-orm";
import {
  db,
  device,
  generateLicenseKey,
  hashLicenseKey,
  isLicensed,
  isUuid,
  license,
  PAID_TIERS,
  parseLicenseKey,
  resolveEntitlement,
  TRIAL_TIER,
  trialExpiryFrom,
  type Device,
  type License,
  type PaidTier,
  type Tier,
} from "@vethida/db";

export interface LicenseWithDevices {
  license: License;
  devices: Device[];
}

// live Pro license to upgrade from (not canceling, not revoked/expired), newest first
export async function findUpgradeableProLicense(
  userId: string,
): Promise<License | null> {
  const [row] = await db
    .select()
    .from(license)
    .where(
      and(
        eq(license.userId, userId),
        eq(license.tier, "pro"),
        eq(license.cancelAtPeriodEnd, false),
        isNull(license.revokedAt),
        sql`(expires_at IS NULL OR expires_at > now())`,
      ),
    )
    .orderBy(desc(license.createdAt))
    .limit(1);
  return row ?? null;
}

export async function getUserLicenses(
  userId: string,
): Promise<LicenseWithDevices[]> {
  const licenses = await db
    .select()
    .from(license)
    .where(eq(license.userId, userId))
    .orderBy(desc(license.createdAt));
  if (licenses.length === 0) return [];

  const activeDevices = await db
    .select()
    .from(device)
    .where(and(eq(device.userId, userId), isNull(device.deactivatedAt)))
    .orderBy(desc(device.lastSeenAt));

  const byLicense = new Map<string, Device[]>();
  for (const d of activeDevices) {
    const list = byLicense.get(d.licenseId);
    if (list) list.push(d);
    else byLicense.set(d.licenseId, [d]);
  }

  return licenses.map((lic) => ({
    license: lic,
    devices: byLicense.get(lic.id) ?? [],
  }));
}

export type ClaimResult = "claimed" | "already_yours" | "not_claimable";

// Claims an unowned key. not_claimable doesn't say whether the key exists or
// belongs to someone else.
export async function claimLicense(
  userId: string,
  rawKey: string,
): Promise<ClaimResult> {
  const canonicalKey = parseLicenseKey(rawKey);
  if (!canonicalKey) return "not_claimable";
  const keyHash = hashLicenseKey(canonicalKey);

  const claimed = await db
    .update(license)
    .set({ userId })
    .where(and(eq(license.keyHash, keyHash), sql`user_id IS NULL`))
    .returning({ id: license.id });
  if (claimed.length > 0) return "claimed";

  const [existing] = await db
    .select({ userId: license.userId })
    .from(license)
    .where(eq(license.keyHash, keyHash))
    .limit(1);
  if (existing?.userId === userId) return "already_yours";
  return "not_claimable";
}

// stops someone subscribing to the same tier twice. canceling ones don't count
export async function findActiveSubscriptionForTier(
  userId: string,
  tier: PaidTier,
): Promise<License | null> {
  const [row] = await db
    .select()
    .from(license)
    .where(
      and(
        eq(license.userId, userId),
        eq(license.tier, tier),
        eq(license.cancelAtPeriodEnd, false),
        isNull(license.revokedAt),
        sql`(expires_at IS NULL OR expires_at > now())`,
      ),
    )
    .orderBy(desc(license.createdAt))
    .limit(1);
  return row ?? null;
}

// lifetime = expires_at IS NULL. Stops buying lifetime twice for the same tier.
export async function findPerpetualLicenseForTier(
  userId: string,
  tier: PaidTier,
): Promise<License | null> {
  const [row] = await db
    .select()
    .from(license)
    .where(
      and(
        eq(license.userId, userId),
        eq(license.tier, tier),
        isNull(license.revokedAt),
        isNull(license.expiresAt),
      ),
    )
    .orderBy(desc(license.createdAt))
    .limit(1);
  return row ?? null;
}

// expiry checked in SQL so there's no Date.now() during render
export async function getLiveSubscribedTiers(
  userId: string,
): Promise<PaidTier[]> {
  const rows = await db
    .select({ tier: license.tier })
    .from(license)
    .where(
      and(
        eq(license.userId, userId),
        eq(license.cancelAtPeriodEnd, false),
        isNull(license.revokedAt),
        sql`(expires_at IS NULL OR expires_at > now())`,
      ),
    );
  const tiers = new Set(rows.map((r) => r.tier));
  return PAID_TIERS.filter((t) => tiers.has(t));
}

// subscribers can still buy lifetime, lifetime owners can't buy anything on that tier
export async function getPerpetualTiers(
  userId: string,
): Promise<PaidTier[]> {
  const rows = await db
    .select({ tier: license.tier })
    .from(license)
    .where(
      and(
        eq(license.userId, userId),
        isNull(license.revokedAt),
        isNull(license.expiresAt),
      ),
    );
  const tiers = new Set(rows.map((r) => r.tier));
  return PAID_TIERS.filter((t) => tiers.has(t));
}

// Worked out from the licenses each time. Always has a tier ("free" with no
// license), so use isLicensed() to check for a license, not truthiness.
export type Entitlement = {
  tier: Tier;
  trial: boolean; // only changes wording
  expiresAt: Date | null; // null for free and lifetime
};

export async function getEntitlement(userId: string): Promise<Entitlement> {
  // all rows on purpose, resolveEntitlement does the filtering
  const rows = await db
    .select({
      tier: license.tier,
      trial: license.trial,
      expiresAt: license.expiresAt,
      revokedAt: license.revokedAt,
    })
    .from(license)
    .where(eq(license.userId, userId));

  return resolveEntitlement(rows, new Date());
}

export async function hasUsedTrial(userId: string): Promise<boolean> {
  const rows = await db
    .select({ id: license.id })
    .from(license)
    .where(and(eq(license.userId, userId), eq(license.trial, true)))
    .limit(1);
  return rows.length > 0;
}

export type StartTrialResult =
  | { ok: true; key: string; expiresAt: Date }
  | { ok: false; reason: "already_used" | "already_entitled" };

// One trial per account. The unique index (license_one_trial_per_user) is what
// actually enforces it, the check here just gives a nicer answer.
export async function startTrial(userId: string): Promise<StartTrialResult> {
  const [entitlement, used] = await Promise.all([
    getEntitlement(userId),
    hasUsedTrial(userId),
  ]);
  if (used) return { ok: false, reason: "already_used" };
  if (isLicensed(entitlement)) return { ok: false, reason: "already_entitled" };

  const key = generateLicenseKey();
  const expiresAt = trialExpiryFrom(new Date());

  try {
    await db.insert(license).values({
      key,
      keyHash: hashLicenseKey(key),
      tier: TRIAL_TIER,
      trial: true,
      userId,
      expiresAt,
    });
  } catch (error) {
    // lost a race with another request
    if (isUniqueViolation(error)) return { ok: false, reason: "already_used" };
    throw error;
  }

  return { ok: true, key, expiresAt };
}

function isUniqueViolation(error: unknown): boolean {
  return (
    typeof error === "object" &&
    error !== null &&
    "code" in error &&
    (error as { code?: unknown }).code === "23505"
  );
}

// false if it isn't the caller's device or it's already inactive
export async function deactivateOwnedDevice(
  userId: string,
  deviceId: string,
): Promise<boolean> {
  // postgres throws 22P02 on a non-uuid
  if (!isUuid(deviceId)) return false;

  const [owned] = await db
    .select({ id: device.id })
    .from(device)
    .innerJoin(license, eq(device.licenseId, license.id))
    .where(
      and(
        eq(device.id, deviceId),
        eq(license.userId, userId),
        isNull(device.deactivatedAt),
      ),
    )
    .limit(1);
  if (!owned) return false;

  const updated = await db
    .update(device)
    .set({ deactivatedAt: new Date() })
    .where(and(eq(device.id, deviceId), isNull(device.deactivatedAt)))
    .returning({ id: device.id });
  return updated.length > 0;
}
