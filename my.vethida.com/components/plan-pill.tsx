import { isPaidTier, tierLabel } from "@vethida/db";

// tierLabel as a pill. tier is a plain string since license.tier is a varchar.
export function PlanPill({
  tier,
  trial = false,
}: {
  tier: string;
  trial?: boolean;
}) {
  // unknown values get the free style
  const variant = trial ? "trial" : isPaidTier(tier) ? tier : "free";
  const label = trial ? "Trial" : tierLabel(tier);
  return <span className={`plan-pill plan-pill--${variant}`}>{label}</span>;
}
