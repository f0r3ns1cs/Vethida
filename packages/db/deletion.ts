import {
  boolean,
  index,
  pgTable,
  text,
  timestamp,
  uuid,
  varchar,
} from "drizzle-orm/pg-core";
import { user } from "./auth-schema";

// Soft-delete state per user (kept out of the generated user table).
export const accountDeletion = pgTable(
  "account_deletion",
  {
    userId: text("user_id")
      .primaryKey()
      .references(() => user.id, { onDelete: "cascade" }),
    requestedAt: timestamp("requested_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
    hardDeleteAt: timestamp("hard_delete_at", { withTimezone: true }).notNull(),
    processingRestricted: boolean("processing_restricted")
      .notNull()
      .default(true),
    // open_chargeback | legal_hold | fraud_review | security_investigation
    blockedReason: varchar("blocked_reason", { length: 32 }),
    // subs this request set to cancel, so undoing the request only restores those
    cancelledSubscriptionIds: text("cancelled_subscription_ids").array(),
  },
  (t) => [index("account_deletion_hard_delete_idx").on(t.hardDeleteAt)],
);

// No FK so it outlives the user. user_ref is a hash, no PII.
export const deletionAudit = pgTable("deletion_audit", {
  id: uuid("id").primaryKey().defaultRandom(),
  // requested | canceled | hard_deleted | blocked
  event: varchar("event", { length: 24 }).notNull(),
  userRef: text("user_ref").notNull(),
  at: timestamp("at", { withTimezone: true }).notNull().defaultNow(),
  detail: text("detail"),
});

export type AccountDeletion = typeof accountDeletion.$inferSelect;
export type DeletionAudit = typeof deletionAudit.$inferSelect;
