import type { CancellationReason } from "@vethida/db";

// Labels for cancellation_request reasons. The cancel flow doesn't ask (click-to-cancel
// rules), so those rows are all 'other'.

export type ReasonLabel = {
  id: CancellationReason;
  label: string;
};

export const CANCELLATION_REASON_LABELS: ReasonLabel[] = [
  { id: "too_expensive", label: "It costs more than it is worth to me" },
  { id: "unused", label: "I am not using it enough" },
  { id: "missing_features", label: "It is missing something I need" },
  { id: "low_quality", label: "Something is broken or did not work" },
  { id: "too_complex", label: "It is harder to use than I expected" },
  { id: "switched_service", label: "I am using something else now" },
  { id: "customer_service", label: "Support did not sort my problem out" },
  { id: "other", label: "Something else" },
];

// use this in admin pages, 'other' with no feedback_at means nobody answered
export function reasonLabelFor(id: string, answered: boolean): string {
  if (!answered) return "Not given";
  return reasonById(id)?.label ?? id;
}

export function reasonById(id: string): ReasonLabel | null {
  return CANCELLATION_REASON_LABELS.find((r) => r.id === id) ?? null;
}
