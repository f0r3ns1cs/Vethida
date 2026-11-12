import type { TierKey } from "@/lib/features";

// Prices live here and nowhere else. checkout = Stripe Payment Link.
type Price = { price: string; cadence: string; checkout?: string };

export type Tier = {
  key: TierKey;
  name: string;
  billing: {
    year: Price;
    month: Price;
    lifetime: Price;
  };
  blurb: string;
  badge?: string;
  highlight?: boolean;
  cta?: { href: string; label: string }; // free only, paid plans use the checkout link
  includes?: string;
  features: string[];
};

export const tiers: Tier[] = [
  {
    key: "free",
    name: "Free",
    billing: {
      year: { price: "$0", cadence: "" },
      month: { price: "$0", cadence: "" },
      lifetime: { price: "$0", cadence: "" },
    },
    blurb: "The low-risk settings and the system tools.",
    cta: { href: "/download", label: "Download" },
    features: [
      "Low-impact settings that carry no warning",
      "Dashboard and Health score",
      "Storage breakdown and cleaner",
      "One-click fixes and restore points",
      "Installed and startup apps, listed",
      "Get apps and the connection test",
    ],
  },
  {
    key: "pro",
    name: "Pro",
    billing: {
      year: { price: "$49.99", cadence: "per year", checkout: "https://buy.stripe.com/REPLACE-PRO-YEARLY" },
      month: { price: "$5.99", cadence: "per month", checkout: "https://buy.stripe.com/REPLACE-PRO-MONTHLY" },
      lifetime: { price: "$99.99", cadence: "one-time", checkout: "https://buy.stripe.com/REPLACE-PRO-LIFETIME" },
    },
    blurb:
      "The settings that change how your PC performs, connects and behaves, plus the debloating.",
    highlight: true,
    includes: "Everything in Free",
    features: [
      "Processor, memory and power settings",
      "GPU settings and the NVIDIA driver profile",
      "Network adapter, TCP/IP and DNS settings",
      "Traffic priority and bandwidth caps per app",
      "Telemetry, privacy and security settings",
      "App removal, Windows extras, startup and services",
    ],
  },
  {
    key: "ultimate",
    name: "Ultimate",
    billing: {
      year: { price: "$79.99", cadence: "per year", checkout: "https://buy.stripe.com/REPLACE-ULTIMATE-YEARLY" },
      month: { price: "$9.99", cadence: "per month", checkout: "https://buy.stripe.com/REPLACE-ULTIMATE-MONTHLY" },
      lifetime: { price: "$159.99", cadence: "one-time", checkout: "https://buy.stripe.com/REPLACE-ULTIMATE-LIFETIME" },
    },
    blurb:
      "Everything in Pro, plus the settings that trade power draw and heat for response time.",
    includes: "Everything in Pro",
    features: [
      "Genesis, one switch that clears background work for a play session",
      "NVIDIA clock and power settings",
      "Deep power management and NVMe driver settings",
      "Low-latency power plan",
    ],
  },
];

// "$49.99" -> 49.99
function numericPrice(display: string): number {
  const parsed = Number.parseFloat(display.replace(/[^0-9.]/g, ""));
  return Number.isFinite(parsed) ? parsed : 0;
}

export type BillingInterval = "year" | "month" | "lifetime";

export const intervals: ReadonlyArray<{
  value: BillingInterval;
  label: string;
}> = [
  { value: "year", label: "Yearly" },
  { value: "month", label: "Monthly" },
  { value: "lifetime", label: "Lifetime" },
];

// has to match legal/eula.md and legal/terms.md (check-integrity checks it)
export const lifetimeScope =
  "A lifetime license covers every update to the tier you bought, including new major versions, at no further charge.";

// yearly vs 12x monthly, rounded down. null if yearly isn't cheaper
export function yearlySavingPercent(tier: Tier): number | null {
  const monthly = numericPrice(tier.billing.month.price) * 12;
  const yearly = numericPrice(tier.billing.year.price);
  if (monthly <= 0 || yearly >= monthly) return null;
  return Math.floor(((monthly - yearly) / monthly) * 100);
}

// `published` is passed in because plans.tsx (client) imports this file and
// lib/download.ts uses node:fs
export function pricingAggregateOffer(published: boolean) {
  const prices = tiers.flatMap((t) => [
    ...new Set(Object.values(t.billing).map((b) => numericPrice(b.price))),
  ]);
  return {
    "@type": "AggregateOffer" as const,
    priceCurrency: "USD",
    lowPrice: Math.min(...prices).toFixed(2),
    highPrice: Math.max(...prices).toFixed(2),
    offerCount: prices.length,
    availability: published
      ? "https://schema.org/InStock"
      : "https://schema.org/PreOrder",
  };
}

// for the Yearly tab label
export function bestYearlySavingPercent(): number | null {
  const all = tiers
    .map(yearlySavingPercent)
    .filter((n): n is number => n !== null);
  return all.length ? Math.max(...all) : null;
}
