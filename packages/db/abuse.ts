// Abuse flag types + the text shown in the admin triage list.
// Must match abuse_flag_type_check in admin.ts. When adding one: add it here,
// update the constraint, and ship the migration before anything writes it.

export type AbuseFlagSeverity = "low" | "medium" | "high" | "critical";
export type AbuseFlagCategory = "account" | "hwid" | "network" | "billing";

export const ABUSE_FLAG_TYPES = {
  FINGERPRINT_FLAG: "fingerprint_flag", // heartbeat fingerprint returned "flag"
  SUPERSEDE_FAILED: "supersede_failed", // couldn't cancel the old sub after an upgrade
  SUPERSEDE_NOT_HONOURED: "supersede_not_honoured", // superseded sub still set to renew
  SUPERSEDED_RENEWAL_CHARGED: "superseded_renewal_charged", // ...and it actually got charged
} as const;

export type AbuseFlagType =
  (typeof ABUSE_FLAG_TYPES)[keyof typeof ABUSE_FLAG_TYPES];

export interface AbuseFlagMetadata {
  label: string;
  description: string;
  severity: AbuseFlagSeverity;
  category: AbuseFlagCategory;
  recommendedAction: string;
  deprecated?: boolean; // nothing writes it anymore, kept for old rows
}

export const ABUSE_FLAG_METADATA: Record<AbuseFlagType, AbuseFlagMetadata> = {
  [ABUSE_FLAG_TYPES.FINGERPRINT_FLAG]: {
    label: "Hardware fingerprint drift",
    description:
      "Enough hardware signals still match to service the request, but not enough to accept it outright. That is consistent with a motherboard or drive swap, and also with a cloned installation.",
    severity: "medium",
    category: "hwid",
    recommendedAction:
      "Check matchCount, comparableCount and delta in the details. A reinstall of Windows changes the volume serial and nothing else; more than that on an account with prior resets is worth contacting the licensee about.",
  },
  [ABUSE_FLAG_TYPES.SUPERSEDE_FAILED]: {
    label: "Upgrade did not stop the old subscription",
    description:
      "The customer upgraded and the replaced license was retired here, but Polar was not told to cancel the subscription behind it. They are being billed for both tiers.",
    severity: "critical",
    category: "billing",
    recommendedAction:
      "Cancel subscriptionId in Polar by hand, then refund anything charged on it after the upgrade date.",
  },
  [ABUSE_FLAG_TYPES.SUPERSEDE_NOT_HONOURED]: {
    label: "Superseded subscription still renewing",
    description:
      "Polar reports a subscription we superseded as set to renew. Nothing has been charged twice yet, but it will be at the end of the period.",
    severity: "critical",
    category: "billing",
    recommendedAction:
      "Cancel subscriptionId in Polar before its period end. If it is already canceled there, this was a stale event and can be resolved.",
  },
  [ABUSE_FLAG_TYPES.SUPERSEDED_RENEWAL_CHARGED]: {
    label: "Renewal charged on a superseded subscription",
    description:
      "A customer paid for a tier they had already upgraded away from. Money has changed hands.",
    severity: "critical",
    category: "billing",
    recommendedAction:
      "Refund orderId, cancel subscriptionId in Polar, and check whether the same subscription has renewed more than once.",
  },
};

export function isKnownAbuseFlagType(value: unknown): value is AbuseFlagType {
  return (
    typeof value === "string" &&
    (Object.values(ABUSE_FLAG_TYPES) as string[]).includes(value)
  );
}

// falls back instead of throwing so one odd row doesn't break the list
export function describeAbuseFlag(
  type: string,
): AbuseFlagMetadata & { known: boolean } {
  if (isKnownAbuseFlagType(type)) {
    return { ...ABUSE_FLAG_METADATA[type], known: true };
  }
  return {
    label: type,
    description:
      "Unrecognized flag type - either historical, or a producer is writing a value this build does not know about.",
    severity: "low",
    category: "account",
    recommendedAction:
      "Check whether a newer deploy added this type; investigate the row manually.",
    known: false,
  };
}
