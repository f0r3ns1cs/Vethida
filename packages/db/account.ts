import { boolean, pgTable, text, timestamp } from "drizzle-orm/pg-core";
import { user } from "./auth-schema";

// Polar customer id per user, from webhooks. Used for customer portal sessions.
export const polarCustomer = pgTable("polar_customer", {
  userId: text("user_id")
    .primaryKey()
    .references(() => user.id, { onDelete: "cascade" }),
  polarCustomerId: text("polar_customer_id").notNull().unique(),
  updatedAt: timestamp("updated_at", { withTimezone: true })
    .notNull()
    .defaultNow(),
});

// No row = defaults (alerts on, marketing off). Transactional mail ignores this.
export const emailPreference = pgTable("email_preference", {
  userId: text("user_id")
    .primaryKey()
    .references(() => user.id, { onDelete: "cascade" }),
  fingerprintAlerts: boolean("fingerprint_alerts").notNull().default(true),
  releaseAnnouncements: boolean("release_announcements")
    .notNull()
    .default(false),
  updatedAt: timestamp("updated_at", { withTimezone: true })
    .notNull()
    .defaultNow(),
});

export type PolarCustomer = typeof polarCustomer.$inferSelect;
export type EmailPreference = typeof emailPreference.$inferSelect;
