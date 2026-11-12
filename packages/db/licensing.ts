import { relations, sql } from "drizzle-orm";
import {
  boolean,
  check,
  customType,
  index,
  integer,
  jsonb,
  pgTable,
  text,
  timestamp,
  uniqueIndex,
  uuid,
  varchar,
} from "drizzle-orm/pg-core";
import { user } from "./auth-schema";

// no built-in bytea in pg-core
const bytea = customType<{ data: Buffer; driverData: Buffer }>({
  dataType() {
    return "bytea";
  },
});

export const license = pgTable(
  "license",
  {
    id: uuid("id").primaryKey().defaultRandom(),
    key: text("key").notNull(),
    keyHash: text("key_hash").notNull(), // sha256 hex, used for lookups
    tier: varchar("tier", { length: 20 }).notNull(),
    trial: boolean("trial").notNull().default(false), // trials are ultimate + expires_at, not a tier
    // null until claimed. set null so bought keys survive account deletion
    userId: text("user_id").references(() => user.id, { onDelete: "set null" }),
    machineResetsRemaining: integer("machine_resets_remaining")
      .notNull()
      .default(3),
    // bumped on revoke/refund. Only a counter, the heartbeat is what actually cuts a client off
    tokenGeneration: integer("token_generation").notNull().default(0),
    polarOrderId: text("polar_order_id"), // unique, so webhook redeliveries don't double provision
    polarSubscriptionId: text("polar_subscription_id"), // null for one-time/lifetime
    cancelAtPeriodEnd: boolean("cancel_at_period_end").notNull().default(false),
    // set when an upgrade replaced this license. If Polar later says it's still renewing
    // we raise a flag instead of turning renewal back on
    supersededAt: timestamp("superseded_at", { withTimezone: true }),
    // 'month' | 'year', null for one-time. Polar can't switch a sub across intervals
    billingInterval: varchar("billing_interval", { length: 10 }),
    expiresAt: timestamp("expires_at", { withTimezone: true }),
    revokedAt: timestamp("revoked_at", { withTimezone: true }),
    revokedBy: text("revoked_by").references(() => user.id, {
      onDelete: "set null",
    }),
    // the code can be shown to the user, revoked_reason is an internal note
    revokedReasonCode: varchar("revoked_reason_code", { length: 32 }),
    revokedReason: text("revoked_reason"),
    createdAt: timestamp("created_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
    renewalReminderSentAt: timestamp("renewal_reminder_sent_at", {
      withTimezone: true,
    }),
    // Payment failed notice sent. Also how we tell "retries ran out" apart from a normal
    // cancel when subscription.revoked comes in.
    pastDueNoticeSentAt: timestamp("past_due_notice_sent_at", {
      withTimezone: true,
    }),
  },
  (t) => [
    uniqueIndex("license_key_unique").on(t.key),
    uniqueIndex("license_key_hash_unique").on(t.keyHash),
    uniqueIndex("license_polar_order_unique")
      .on(t.polarOrderId)
      .where(sql`polar_order_id IS NOT NULL`),
    uniqueIndex("license_polar_subscription_unique")
      .on(t.polarSubscriptionId)
      .where(sql`polar_subscription_id IS NOT NULL`),
    index("license_user_id_idx").on(t.userId),
    index("license_created_at_idx").on(t.createdAt),
    // no 'free' rows, free just means no license
    check("license_tier_check", sql`tier IN ('pro', 'ultimate')`),
    check(
      "license_trial_expiry_check",
      sql`trial = false OR expires_at IS NOT NULL`,
    ),
    check(
      "license_trial_unpaid_check",
      sql`trial = false OR (polar_order_id IS NULL AND polar_subscription_id IS NULL)`,
    ),
    // one trial per account
    uniqueIndex("license_one_trial_per_user")
      .on(t.userId)
      .where(sql`trial = true AND user_id IS NOT NULL`),
    check(
      "license_machine_resets_check",
      sql`machine_resets_remaining >= 0`,
    ),
    check("license_token_generation_check", sql`token_generation >= 0`),
    check(
      "license_revoked_reason_code_check",
      sql`revoked_reason_code IS NULL OR revoked_reason_code IN (
        'chargeback', 'tos_violation', 'fraud', 'user_request', 'admin_error', 'other'
      )`,
    ),
    check(
      "license_revoked_consistency_check",
      sql`(revoked_at IS NULL AND revoked_reason_code IS NULL)
          OR (revoked_at IS NOT NULL AND revoked_reason_code IS NOT NULL)`,
    ),
  ],
);

// One row per install. The device's P-256 public key (raw X||Y) is its identity.
export const device = pgTable(
  "device",
  {
    id: uuid("id").primaryKey().defaultRandom(),
    licenseId: uuid("license_id")
      .notNull()
      .references(() => license.id, { onDelete: "cascade" }),
    // cascade: the fingerprint hashes still count as personal data
    userId: text("user_id").references(() => user.id, { onDelete: "cascade" }),
    devicePublicKey: bytea("device_public_key").notNull(),
    devicePubkeyId: varchar("device_pubkey_id", { length: 32 }).notNull(), // X-Device-Pubkey-Id
    hardwareFingerprint: jsonb("hardware_fingerprint").notNull(),
    lastIp: varchar("last_ip", { length: 45 }),
    deactivatedAt: timestamp("deactivated_at", { withTimezone: true }),
    lastSeenAt: timestamp("last_seen_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
    activatedAt: timestamp("activated_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
    fingerprintNotifiedAt: timestamp("fingerprint_notified_at", {
      withTimezone: true,
    }),
  },
  (t) => [
    uniqueIndex("device_pubkey_id_unique").on(t.devicePubkeyId),
    index("device_user_id_idx").on(t.userId),
    index("device_license_active_idx")
      .on(t.licenseId)
      .where(sql`deactivated_at IS NULL`),
    index("device_license_id_idx").on(t.licenseId),
    check(
      "device_public_key_length_check",
      sql`octet_length(device_public_key) = 64`,
    ),
  ],
);

// Desktop browser sign-in (RFC 8628 style device flow). Polls are signed with the
// device key that started the session, so the device code alone isn't enough.
// user_code is for the person to compare, device_code (stored hashed) is the secret.
export const desktopAuthSession = pgTable(
  "desktop_auth_session",
  {
    id: uuid("id").primaryKey().defaultRandom(),
    userCode: varchar("user_code", { length: 9 }).notNull(),
    deviceCodeHash: text("device_code_hash").notNull(),
    devicePublicKey: bytea("device_public_key").notNull(),
    devicePubkeyId: varchar("device_pubkey_id", { length: 32 }).notNull(),
    hardwareFingerprint: jsonb("hardware_fingerprint").notNull(),
    status: varchar("status", { length: 16 }).notNull().default("pending"),
    // set on approval
    userId: text("user_id").references(() => user.id, { onDelete: "cascade" }),
    licenseId: uuid("license_id").references(() => license.id, {
      onDelete: "cascade",
    }),
    startIp: varchar("start_ip", { length: 45 }),
    createdAt: timestamp("created_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
    expiresAt: timestamp("expires_at", { withTimezone: true }).notNull(),
    approvedAt: timestamp("approved_at", { withTimezone: true }),
    consumedAt: timestamp("consumed_at", { withTimezone: true }),
  },
  (t) => [
    uniqueIndex("desktop_auth_device_code_unique").on(t.deviceCodeHash),
    // only unique while pending, codes are short enough to repeat
    uniqueIndex("desktop_auth_user_code_live_unique")
      .on(t.userCode)
      .where(sql`status = 'pending'`),
    index("desktop_auth_expires_idx").on(t.expiresAt),
    index("desktop_auth_user_id_idx").on(t.userId),
    check(
      "desktop_auth_status_check",
      sql`status IN ('pending', 'approved', 'denied', 'consumed')`,
    ),
    check(
      "desktop_auth_approved_consistency_check",
      sql`status <> 'approved' OR (user_id IS NOT NULL AND license_id IS NOT NULL)`,
    ),
    check(
      "desktop_auth_public_key_length_check",
      sql`octet_length(device_public_key) = 64`,
    ),
  ],
);

// suspended = has a row with lifted_at null. Rows are kept after lifting.
export const userSuspension = pgTable(
  "user_suspension",
  {
    id: uuid("id").primaryKey().defaultRandom(),
    userId: text("user_id")
      .notNull()
      .references(() => user.id, { onDelete: "cascade" }),
    reasonCode: varchar("reason_code", { length: 32 }).notNull(),
    reasonDetail: text("reason_detail"),
    source: varchar("source", { length: 32 }).notNull(),
    createdAt: timestamp("created_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
    liftedAt: timestamp("lifted_at", { withTimezone: true }),
    liftedBy: text("lifted_by").references(() => user.id, {
      onDelete: "set null",
    }),
  },
  (t) => [
    // suspend() relies on this for ON CONFLICT
    uniqueIndex("user_suspension_active_unique")
      .on(t.userId, t.reasonCode)
      .where(sql`lifted_at IS NULL`),
    index("user_suspension_user_history_idx").on(t.userId, t.createdAt),
    check(
      "user_suspension_reason_check",
      sql`reason_code IN ('device_sharing', 'chargeback', 'admin_action')`,
    ),
    check(
      "user_suspension_source_check",
      sql`source IN ('auto_antishare', 'chargeback_webhook', 'admin')`,
    ),
  ],
);

export const licenseRelations = relations(license, ({ one, many }) => ({
  owner: one(user, { fields: [license.userId], references: [user.id] }),
  devices: many(device),
}));

export const deviceRelations = relations(device, ({ one }) => ({
  license: one(license, {
    fields: [device.licenseId],
    references: [license.id],
  }),
  user: one(user, { fields: [device.userId], references: [user.id] }),
}));

export type License = typeof license.$inferSelect;
export type NewLicense = typeof license.$inferInsert;
export type Device = typeof device.$inferSelect;
export type NewDevice = typeof device.$inferInsert;
export type DesktopAuthSession = typeof desktopAuthSession.$inferSelect;
export type NewDesktopAuthSession = typeof desktopAuthSession.$inferInsert;
export type UserSuspension = typeof userSuspension.$inferSelect;
export type NewUserSuspension = typeof userSuspension.$inferInsert;
