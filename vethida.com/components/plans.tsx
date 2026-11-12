"use client";

import { useState } from "react";
import Link from "next/link";

import { PlanCard } from "@/components/plan-card";
import { Segmented } from "@/components/segmented";
import {
  intervals,
  lifetimeScope,
  tiers,
  type BillingInterval,
} from "@/lib/pricing";

// plan grid for the homepage and /pricing

const NAME = "billing";

type Props = {
  variant?: "summary" | "full"; // summary = homepage (own heading), full = /pricing
};

// required billing disclosures
function PlanTerms({ interval }: { interval: BillingInterval }) {
  return (
    <div className="plan-terms">
      <p>
        {interval === "lifetime"
          ? `One payment, and it never expires. ${lifetimeScope}`
          : `Pro and Ultimate renew ${interval === "year" ? "every year" : "every month"} until you cancel. Cancel from the billing portal and your plan runs to the end of the term you have paid for.`}
      </p>
      <p>
        One license for your PC, with three activations for reinstalls and new
        hardware.
      </p>
      <p>
        Prices in USD, taxes at checkout.{" "}
        {interval === "lifetime" ? null : (
          <>
            <Link href="/legal/subscription-terms">Subscription Terms</Link>,{" "}
          </>
        )}
        <Link href="/legal/refunds">Refund Policy</Link>.
      </p>
    </div>
  );
}

export function Plans({ variant = "summary" }: Props) {
  const [interval, setInterval] = useState<BillingInterval>("year");
  const full = variant === "full";

  return (
    <section className={full ? "band band--tight" : "band band--ruled"}>
      <div className="band__inner">
        {full ? null : (
          <div className="section-head section-head--center">
            <h2 className="section-head__title">Pick a plan</h2>
          </div>
        )}

        <div className="flex justify-center">
          <Segmented
            name={NAME}
            label="Billing interval"
            options={intervals}
            value={interval}
            onChange={setInterval}
          />
        </div>

        {intervals.map(({ value }) => (
          <div
            key={value}
            id={`${NAME}-panel-${value}`}
            role="tabpanel"
            aria-labelledby={`${NAME}-tab-${value}`}
            hidden={value !== interval}
            tabIndex={0}
          >
            <div className="plan-grid mt-12">
              {tiers.map((tier) => (
                <PlanCard
                  key={tier.key}
                  tier={tier}
                  as={full ? "h2" : "h3"}
                  interval={value}
                />
              ))}
            </div>

            <PlanTerms interval={value} />
          </div>
        ))}
      </div>
    </section>
  );
}
