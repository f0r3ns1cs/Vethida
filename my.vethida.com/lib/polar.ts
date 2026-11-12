import { Polar } from "@polar-sh/sdk";
import { PAID_TIERS, type PaidTier } from "@vethida/db/tiers";

let client: Polar | null = null;

export function getPolar(): Polar {
  if (client) return client;

  const accessToken = process.env.POLAR_ACCESS_TOKEN;
  if (!accessToken) {
    throw new Error("POLAR_ACCESS_TOKEN is not configured");
  }

  // sandbox outside production unless POLAR_SERVER says otherwise
  const override = process.env.POLAR_SERVER;
  const server: "production" | "sandbox" =
    override === "production" || override === "sandbox"
      ? override
      : process.env.NODE_ENV === "production"
        ? "production"
        : "sandbox";

  client = new Polar({ accessToken, server });
  return client;
}

export type Tier = PaidTier;

// Each cadence is its own Polar product. Lifetime is a one-time purchase.
export type Cadence = "monthly" | "yearly" | "lifetime";

export const CADENCES: readonly Cadence[] = [
  "monthly",
  "yearly",
  "lifetime",
] as const;

export function isCadence(v: unknown): v is Cadence {
  return v === "monthly" || v === "yearly" || v === "lifetime";
}

// No plan switching on purpose (click-to-cancel rules), you cancel and buy again.

// unsuffixed POLAR_PRODUCT_PRO / _ULTIMATE also mean yearly
const PRODUCT_ENV: Record<Tier, Record<Cadence, string[]>> = {
  pro: {
    yearly: ["POLAR_PRODUCT_PRO_YEARLY", "POLAR_PRODUCT_PRO"],
    monthly: ["POLAR_PRODUCT_PRO_MONTHLY"],
    lifetime: ["POLAR_PRODUCT_PRO_LIFETIME"],
  },
  ultimate: {
    yearly: ["POLAR_PRODUCT_ULTIMATE_YEARLY", "POLAR_PRODUCT_ULTIMATE"],
    monthly: ["POLAR_PRODUCT_ULTIMATE_MONTHLY"],
    lifetime: ["POLAR_PRODUCT_ULTIMATE_LIFETIME"],
  },
};

function firstConfigured(names: string[]): string | null {
  for (const n of names) {
    const v = process.env[n];
    if (v) return v;
  }
  return null;
}

export function getPolarProductId(tier: Tier, cadence: Cadence): string | null {
  return firstConfigured(PRODUCT_ENV[tier][cadence]);
}

export function availableCadences(tier: Tier): Cadence[] {
  return CADENCES.filter((c) => getPolarProductId(tier, c) !== null);
}

export function allAvailableCadences(): Record<Tier, Cadence[]> {
  return Object.fromEntries(
    PAID_TIERS.map((tier) => [tier, availableCadences(tier)]),
  ) as Record<Tier, Cadence[]>;
}

// Upgrades (Pro -> Ultimate, subscription -> lifetime) buy the normal product
// with a one-time discount for the unused time. Not a separate product, that
// would charge the reduced price on every renewal.

const TERM_DAYS: Record<"month" | "year", number> = { month: 30, year: 365 };

export interface UpgradeCreditInput {
  paidMinor: number; // cents
  expiresAt: Date | null; // null = lifetime
  billingInterval: string | null;
  targetMinor: number;
  now: Date;
}

// credit = paid * remaining / term, clamped to [0, target]. Lifetime carries over in full.
export function upgradeCreditMinor(input: UpgradeCreditInput): number {
  const { paidMinor, expiresAt, billingInterval, targetMinor, now } = input;
  if (paidMinor <= 0 || targetMinor <= 0) return 0;

  const clamp = (v: number) => Math.max(0, Math.min(Math.round(v), targetMinor));

  const interval =
    billingInterval === "month" || billingInterval === "year"
      ? billingInterval
      : null;
  if (!interval || !expiresAt) return clamp(paidMinor);

  const msRemaining = expiresAt.getTime() - now.getTime();
  if (!Number.isFinite(msRemaining) || msRemaining <= 0) return 0;
  const termMs = TERM_DAYS[interval] * 24 * 60 * 60 * 1000;
  return clamp(paidMinor * Math.min(msRemaining / termMs, 1));
}

export interface ProductPrice {
  tier: Tier;
  cadence: Cadence;
  amount: number; // minor units
  currency: string;
}

// Prices come from Polar. Returns [] on failure and the page just leaves the price off.
export async function fetchProductPrices(): Promise<ProductPrice[]> {
  const wanted: { tier: Tier; cadence: Cadence; id: string }[] = [];
  for (const tier of PAID_TIERS) {
    for (const cadence of CADENCES) {
      const id = getPolarProductId(tier, cadence);
      if (id) wanted.push({ tier, cadence, id });
    }
  }
  if (wanted.length === 0) return [];

  try {
    const byId = new Map<string, { amount: number; currency: string }>();
    const res = await getPolar().products.list({
      limit: 100,
      isArchived: false,
    });
    for await (const page of res) {
      for (const p of page.result.items) {
        const price = (p.prices?.[0] ?? null) as {
          priceAmount?: number;
          priceCurrency?: string;
        } | null;
        if (price && typeof price.priceAmount === "number") {
          byId.set(p.id, {
            amount: price.priceAmount,
            currency: (price.priceCurrency ?? "usd").toUpperCase(),
          });
        }
      }
    }
    return wanted
      .map(({ tier, cadence, id }) => {
        const hit = byId.get(id);
        return hit ? { tier, cadence, ...hit } : null;
      })
      .filter((p): p is ProductPrice => p !== null);
  } catch (err) {
    console.error("[polar] price lookup failed", err);
    return [];
  }
}

// throws instead of returning null, the upgrade path can't guess a price
export async function fetchProductAmount(
  tier: Tier,
  cadence: Cadence,
): Promise<{ minor: number; currency: string }> {
  const id = getPolarProductId(tier, cadence);
  if (!id) throw new Error(`no product configured for ${tier}/${cadence}`);
  const product = await getPolar().products.get({ id });
  const price = (product.prices?.[0] ?? null) as {
    priceAmount?: number;
    priceCurrency?: string;
  } | null;
  if (!price || typeof price.priceAmount !== "number") {
    throw new Error(`product ${id} has no readable fixed price`);
  }
  return {
    minor: price.priceAmount,
    currency: (price.priceCurrency ?? "usd").toLowerCase(),
  };
}

// One-off discount for a single checkout: no code, 1 redemption, one product, expires in 24h.
export async function createUpgradeCredit(args: {
  amountMinor: number;
  currency: string;
  productId: string;
  licenseId: string;
  userId: string;
  now: Date;
}): Promise<string> {
  const discount = await getPolar().discounts.create({
    name: `Upgrade credit for license ${args.licenseId}`,
    type: "fixed",
    duration: "once",
    amounts: { [args.currency]: args.amountMinor },
    products: [args.productId],
    maxRedemptions: 1,
    endsAt: new Date(args.now.getTime() + UPGRADE_CREDIT_TTL_MS),
    metadata: { userId: args.userId, supersedesLicenseId: args.licenseId },
  });
  return discount.id;
}

const UPGRADE_CREDIT_TTL_MS = 24 * 60 * 60 * 1000;
