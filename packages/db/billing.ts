import { sql } from "drizzle-orm";
import {
  check,
  index,
  pgTable,
  primaryKey,
  text,
  timestamp,
  varchar,
} from "drizzle-orm/pg-core";

// Webhook dedup. event_id is the delivery id (webhook-id header), not the order id.
export const webhookEvent = pgTable(
  "webhook_event",
  {
    provider: varchar("provider", { length: 32 }).notNull(),
    eventId: text("event_id").notNull(),
    eventType: text("event_type").notNull(),
    status: varchar("status", { length: 16 }).notNull().default("received"),
    receivedAt: timestamp("received_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
    processedAt: timestamp("processed_at", { withTimezone: true }),
    expiresAt: timestamp("expires_at", { withTimezone: true }).notNull(),
  },
  (t) => [
    primaryKey({ columns: [t.provider, t.eventId] }),
    index("webhook_event_expires_idx").on(t.expiresAt),
    check(
      "webhook_event_status_check",
      sql`status IN ('received', 'processed')`,
    ),
  ],
);

export type WebhookEvent = typeof webhookEvent.$inferSelect;
export type NewWebhookEvent = typeof webhookEvent.$inferInsert;
