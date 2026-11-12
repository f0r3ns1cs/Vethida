import Link from "next/link";

import { AlertTriangleIcon, CheckIcon } from "@/components/icons";
import { FeatureIcon } from "@/components/feature-icon";
import { capabilityGroups, type TierKey } from "@/lib/features";
import { tiers } from "@/lib/pricing";
import { buyPlanLabel } from "@/lib/site";

// A row is in its own tier and every tier above it: Pro includes Free, and
// Ultimate includes Pro. The tick is derived from that order, never typed.
const ORDER: readonly TierKey[] = ["free", "pro", "ultimate"];

function includes(plan: TierKey, rowTier: TierKey): boolean {
  return ORDER.indexOf(plan) >= ORDER.indexOf(rowTier);
}

export function CompareTable() {
  return (
    <div className="compare-wrap">
      <table className="compare" id="compare">
        <caption>What each Vethida plan includes</caption>
        <thead>
          <tr>
            <th scope="col" className="compare__corner">
              <span className="sr-only">Feature</span>
            </th>
            {tiers.map((tier) => (
              <th
                key={tier.key}
                scope="col"
                className={tier.highlight ? "is-featured" : undefined}
              >
                <span className="compare__plan">{tier.name}</span>
                <span className="compare__price">
                  {tier.key === "free" ? "$0" : tier.billing.month.price}
                  {tier.key === "free" ? null : (
                    <span className="compare__per">/mo</span>
                  )}
                </span>
              </th>
            ))}
          </tr>
        </thead>

        {capabilityGroups.map((group) => (
          <tbody key={group.id} id={group.id}>
            <tr className="compare__group">
              <th scope="colgroup" colSpan={4}>
                <span className="compare__group-title">
                  <span className="compare__group-icon">
                    <FeatureIcon icon={group.icon} className="size-4" />
                  </span>
                  {group.title}
                </span>
              </th>
            </tr>

            {group.capabilities.map((capability) => (
              <tr key={capability.name}>
                <th scope="row">{capability.name}</th>
                {tiers.map((tier) => {
                  const yes = includes(tier.key, capability.tier);
                  return (
                    <td
                      key={tier.key}
                      className={tier.highlight ? "is-featured" : undefined}
                    >
                      {yes ? (
                        <span className="compare__yes">
                          <CheckIcon aria-hidden />
                        </span>
                      ) : (
                        <span className="compare__no" aria-hidden />
                      )}
                      <span className="sr-only">
                        {yes ? "Included" : "Not included"} in {tier.name}
                      </span>
                    </td>
                  );
                })}
              </tr>
            ))}

            {group.warning ? (
              <tr className="compare__warn">
                <td colSpan={4}>
                  <AlertTriangleIcon aria-hidden />
                  {group.warning.body}
                </td>
              </tr>
            ) : null}
          </tbody>
        ))}
        <tfoot className="compare__foot">
          <tr>
            <td />
            {tiers.map((tier) => {
              const href = tier.key === "free" ? "/download" : "/pricing";
              const label = tier.key === "free" ? "Download" : buyPlanLabel(tier.name);
              return (
                <td
                  key={tier.key}
                  className={tier.highlight ? "is-featured" : undefined}
                >
                  <Link
                    href={href}
                    className={`btn ${tier.highlight ? "btn-primary" : "btn-secondary"}`}
                  >
                    {label}
                  </Link>
                </td>
              );
            })}
          </tr>
        </tfoot>
      </table>
    </div>
  );
}
