import Link from "next/link";

import { CheckIcon } from "@/components/icons";
import {
  yearlySavingPercent,
  type BillingInterval,
  type Tier,
} from "@/lib/pricing";
import { buyPlanLabel } from "@/lib/site";

type PlanCardProps = {
  tier: Tier;
  interval: BillingInterval;
  as?: "h2" | "h3"; // h2 on /pricing, h3 on the homepage
};

export function PlanCard({
  tier,
  interval,
  as: Heading = "h3",
}: PlanCardProps) {
  const { price, cadence, checkout } = tier.billing[interval];
  const href = checkout ?? tier.cta?.href ?? "/pricing";
  const label = tier.cta?.label ?? buyPlanLabel(tier.name);
  const variant = tier.highlight ? "btn-primary" : "btn-secondary";
  const saving = interval === "year" ? yearlySavingPercent(tier) : null;

  return (
    <div className={`plan-card${tier.highlight ? " plan-card--featured" : ""}`}>
      <div className="plan-card__head">
        <Heading className="plan-card__name">{tier.name}</Heading>
        {tier.badge ? (
          <span
            className={`tier-pill${tier.highlight ? " tier-pill--strong" : ""}`}
          >
            {tier.badge}
          </span>
        ) : null}
      </div>

      <p className="plan-card__price">
        <span className="plan-card__amount">{price}</span>
        <span className="plan-card__cadence">{cadence}</span>
        {saving === null ? null : (
          <span className="plan-card__save">Save {saving}%</span>
        )}
      </p>

      <p className="plan-card__blurb">{tier.blurb}</p>

      <div className="plan-card__cta">
        {href.startsWith("/") ? (
          <Link href={href} className={`btn btn-lg ${variant} w-full`}>
            {label}
          </Link>
        ) : (
          <a
            href={href}
            target="_blank"
            rel="noopener noreferrer"
            className={`btn btn-lg ${variant} w-full`}
          >
            {label}
          </a>
        )}
      </div>

      <div className="plan-card__body">
        <hr className="plan-card__rule" />
        <ul className="plan-card__features">
          {tier.includes ? (
            <li className="plan-card__feature plan-card__feature--lead">
              <CheckIcon className="plan-card__check" aria-hidden />
              <span>{tier.includes}</span>
            </li>
          ) : null}

          {tier.features.map((feature) => (
            <li key={feature} className="plan-card__feature">
              <CheckIcon className="plan-card__check" aria-hidden />
              <span>{feature}</span>
            </li>
          ))}
        </ul>
      </div>
    </div>
  );
}
