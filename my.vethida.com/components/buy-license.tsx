"use client";

import { useState } from "react";

import { PAID_TIERS, tierBlurb, tierLabel } from "@vethida/db/tiers";

import { CADENCES, type Cadence, type Tier } from "@/lib/polar";

const CADENCE_LABEL: Record<Cadence, string> = {
  monthly: "Monthly",
  yearly: "Yearly",
  lifetime: "Lifetime",
};

const CADENCE_SUFFIX: Record<Cadence, string> = {
  monthly: "/mo",
  yearly: "/yr",
  lifetime: " one-time",
};

// names and blurbs live in @vethida/db
const TIERS = PAID_TIERS.map((tier) => ({
  tier,
  name: tierLabel(tier),
  blurb: tierBlurb(tier),
}));

export interface PriceInfo {
  tier: Tier;
  cadence: Cadence;
  amount: number; // minor units
  currency: string;
}

export interface BuyLicenseProps {
  ownedTiers?: Tier[]; // live subscriptions
  perpetualTiers?: Tier[]; // lifetime
  availableCadences?: Record<Tier, Cadence[]>;
  prices?: PriceInfo[]; // empty if the Polar lookup failed
}

function formatPrice(p: PriceInfo): string {
  return (p.amount / 100).toLocaleString("en-US", {
    style: "currency",
    currency: p.currency,
    minimumFractionDigits: p.amount % 100 === 0 ? 0 : 2,
  });
}

function RefundPolicyLink() {
  return (
    <a
      href="https://vethida.com/legal/refunds"
      className="underline underline-offset-2 hover:text-foreground"
    >
      Refund Policy
    </a>
  );
}

export function BuyLicense({
  ownedTiers = [],
  perpetualTiers = [],
  availableCadences = { pro: ["yearly"], ultimate: ["yearly"] },
  prices = [],
}: BuyLicenseProps) {
  const [cadence, setCadence] = useState<Cadence>("yearly");
  const [pending, setPending] = useState<Tier | null>(null);
  const [error, setError] = useState<string | null>(null);

  const owned = new Set(ownedTiers);
  const perpetual = new Set(perpetualTiers);
  const hasUltimate = owned.has("ultimate");

  // only cadences that have a product configured
  const offered = CADENCES.filter((c) =>
    PAID_TIERS.some((t) => (availableCadences[t] ?? []).includes(c)),
  );
  const showToggle = offered.length > 1;

  function priceFor(tier: Tier): PriceInfo | null {
    return prices.find((p) => p.tier === tier && p.cadence === cadence) ?? null;
  }

  // keep in sync with the checks in /api/checkout
  function unavailableReason(tier: Tier): string | null {
    if (perpetual.has(tier)) return "You own this for life.";
    if (tier === "pro" && hasUltimate) {
      return "Included in Ultimate. To move down, use Cancel and choose the smaller plan.";
    }
    // subscribers can still buy lifetime
    if (owned.has(tier) && cadence !== "lifetime") {
      return "You're subscribed to this.";
    }
    if (!(availableCadences[tier] ?? []).includes(cadence)) {
      return `Not available ${CADENCE_LABEL[cadence].toLowerCase()} yet.`;
    }
    return null;
  }

  function ctaLabel(tier: Tier): string {
    const name = tierLabel(tier);
    if (cadence !== "lifetime") return `Subscribe to ${name}`;
    return owned.has(tier)
      ? `Upgrade ${name} to lifetime`
      : `Buy ${name} for life`;
  }

  async function buy(tier: Tier) {
    setError(null);
    setPending(tier);
    try {
      const res = await fetch("/api/checkout", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ tier, cadence }),
      });
      const json = (await res.json()) as { url?: string; error?: string };
      if (!res.ok || !json.url) {
        setError(json.error ?? "Couldn't start checkout. Please try again.");
        setPending(null);
        return;
      }
      window.location.assign(json.url);
    } catch {
      setError("Couldn't start checkout. Please try again.");
      setPending(null);
    }
  }

  return (
    <section className="card p-5">
      <div className="mb-4 flex flex-wrap items-start justify-between gap-3">
        <p className="text-sm text-foreground-muted">
          Buy a license. Each key activates one machine.
        </p>
        {showToggle ? (
          <div
            role="radiogroup"
            aria-label="Billing interval"
            className="flex shrink-0 rounded-md border border-border p-0.5"
          >
            {offered.map((c) => (
              <button
                key={c}
                type="button"
                role="radio"
                aria-checked={cadence === c}
                onClick={() => setCadence(c)}
                className={`rounded px-3 py-1 text-xs font-medium transition-colors ${
                  cadence === c
                    ? "bg-surface-elevated text-foreground"
                    : "text-foreground-muted hover:text-foreground"
                }`}
              >
                {CADENCE_LABEL[c]}
              </button>
            ))}
          </div>
        ) : null}
      </div>

      {/* has to match the selected cadence. Don't say "guarantee" here (16 CFR 239.3) */}
      <p className="mb-4 text-xs text-foreground-muted">
        {cadence === "lifetime" ? (
          <>
            A lifetime license is a single charge. Your card is billed once at
            checkout and never again: there is no renewal, and nothing to
            cancel. The exact price is confirmed before you pay. If you
            currently subscribe to the tier you&apos;re buying, that
            subscription stops renewing and runs out the term you have already
            paid for. Refundable within 14 days of purchase, under the{" "}
            <RefundPolicyLink />.
          </>
        ) : (
          <>
            This is a subscription. Your card is charged at checkout and then
            automatically at the start of each term until you cancel. The exact
            price and renewal interval are confirmed before you pay. You can
            cancel any time from your dashboard; canceling keeps your tier
            through the end of the term you&apos;ve paid for and is not refunded
            for the unused portion. Refundable within 14 days of your first
            purchase, under the <RefundPolicyLink />.
          </>
        )}
      </p>

      <div className="grid gap-3 sm:grid-cols-2">
        {TIERS.map(({ tier, name, blurb }) => {
          const reason = unavailableReason(tier);
          const price = priceFor(tier);
          return (
            <div
              key={tier}
              className="flex flex-col justify-between gap-3 rounded-md border border-border bg-background p-4"
            >
              <div>
                <div className="flex items-baseline justify-between gap-2">
                  <p className="text-sm font-semibold">{name}</p>
                  {price ? (
                    <p className="text-sm font-semibold">
                      {formatPrice(price)}
                      <span className="text-xs font-normal text-foreground-muted">
                        {CADENCE_SUFFIX[cadence]}
                      </span>
                    </p>
                  ) : null}
                </div>
                <p className="mt-1 text-xs text-foreground-muted">{blurb}</p>
              </div>
              {reason ? (
                <p className="text-xs text-foreground-faint">{reason}</p>
              ) : (
                <button
                  type="button"
                  disabled={pending !== null}
                  className="btn btn-secondary"
                  onClick={() => buy(tier)}
                >
                  {pending === tier ? "Redirecting..." : ctaLabel(tier)}
                </button>
              )}
            </div>
          );
        })}
      </div>
      {error ? (
        <p role="alert" className="mt-3 text-sm text-danger">
          {error}
        </p>
      ) : null}
    </section>
  );
}
