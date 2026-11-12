import { sql } from "drizzle-orm";
import {
  check,
  index,
  jsonb,
  pgTable,
  text,
  timestamp,
  uuid,
  varchar,
} from "drizzle-orm/pg-core";

import { user } from "./auth-schema";
import { license } from "./licensing";

// One row per cancellation. Reasons use Polar's CustomerCancellationReason values
// so they can be passed straight through.
export const CANCELLATION_REASONS = [
  "too_expensive",
  "missing_features",
  "switched_service",
  "unused",
  "customer_service",
  "low_quality",
  "too_complex",
  "other",
] as const;

export type CancellationReason = (typeof CANCELLATION_REASONS)[number];

export const SAVE_OUTCOMES = [
  "pending",
  "saved_discount",
  "saved_downgrade",
  "saved_support",
  "declined_offer",
  "canceled",
  "abandoned",
] as const;

export type SaveOutcome = (typeof SAVE_OUTCOMES)[number];

export const cancellationRequest = pgTable(
  "cancellation_request",
  {
    id: uuid("id").primaryKey().defaultRandom(),
    // set null so the row survives account deletion (anonymised by the deletion job)
    userId: text("user_id").references(() => user.id, {
      onDelete: "set null",
    }),
    licenseId: uuid("license_id").references(() => license.id, {
      onDelete: "set null",
    }),
    polarSubscriptionId: text("polar_subscription_id"),
    tier: varchar("tier", { length: 20 }).notNull(),

    reason: varchar("reason", { length: 32 }).notNull(), // 'other' until they answer the optional question
    comment: text("comment"), // customer's own words, sent to Polar

    feedbackAt: timestamp("feedback_at", { withTimezone: true }), // null = never answered
    offerShown: varchar("offer_shown", { length: 32 }),
    outcome: varchar("outcome", { length: 32 }).notNull().default("pending"),

    offerDetail: jsonb("offer_detail").$type<Record<string, unknown>>(),

    createdAt: timestamp("created_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
    resolvedAt: timestamp("resolved_at", { withTimezone: true }),
  },
  (t) => [
    index("cancellation_request_user_idx").on(t.userId),
    index("cancellation_request_created_idx").on(t.createdAt),
    index("cancellation_request_outcome_idx").on(t.outcome),
    check(
      "cancellation_request_reason_check",
      sql`reason IN ('too_expensive', 'missing_features', 'switched_service', 'unused', 'customer_service', 'low_quality', 'too_complex', 'other')`,
    ),
    check(
      "cancellation_request_outcome_check",
      sql`outcome IN ('pending', 'saved_discount', 'saved_downgrade', 'saved_support', 'declined_offer', 'canceled', 'abandoned')`,
    ),
  ],
);

export type CancellationRequest = typeof cancellationRequest.$inferSelect;
export type NewCancellationRequest = typeof cancellationRequest.$inferInsert;
