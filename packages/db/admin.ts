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
import { device, license } from "./licensing";

// Append only. Admin actions go through recordAdminAction, webhook changes through recordSystemAction.
export const adminAuditLog = pgTable(
  "admin_audit_log",
  {
    id: uuid("id").primaryKey().defaultRandom(),
    // null for system rows or a deleted admin, check `actor` to tell them apart
    adminId: text("admin_id").references(() => user.id, {
      onDelete: "set null",
    }),
    actor: varchar("actor", { length: 16 }).notNull().default("admin"),
    action: varchar("action", { length: 64 }).notNull(),
    targetType: varchar("target_type", { length: 32 }).notNull(),
    targetId: text("target_id").notNull(),
    ipAddress: varchar("ip_address", { length: 45 }),
    details: jsonb("details"),
    createdAt: timestamp("created_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
  },
  (t) => [
    index("admin_audit_log_admin_idx").on(t.adminId, t.createdAt),
    index("admin_audit_log_target_idx").on(
      t.targetType,
      t.targetId,
      t.createdAt,
    ),
    index("admin_audit_log_created_idx").on(t.createdAt),
    check("admin_audit_log_actor_check", sql`actor IN ('admin', 'system')`),
    // admin rows need an ip, system rows can't have an admin id
    check(
      "admin_audit_log_admin_ip_check",
      sql`actor <> 'admin' OR ip_address IS NOT NULL`,
    ),
    check(
      "admin_audit_log_system_actor_check",
      sql`actor <> 'system' OR admin_id IS NULL`,
    ),
  ],
);

// Flags for admin review only, actual blocking is in user_suspension.
export const abuseFlag = pgTable(
  "abuse_flag",
  {
    id: uuid("id").primaryKey().defaultRandom(),
    userId: text("user_id").references(() => user.id, { onDelete: "set null" }),
    licenseId: uuid("license_id").references(() => license.id, {
      onDelete: "set null",
    }),
    deviceId: uuid("device_id").references(() => device.id, {
      onDelete: "set null",
    }),
    type: varchar("type", { length: 40 }).notNull(),
    details: jsonb("details"),
    resolvedAt: timestamp("resolved_at", { withTimezone: true }),
    resolvedBy: text("resolved_by").references(() => user.id, {
      onDelete: "set null",
    }),
    createdAt: timestamp("created_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
  },
  (t) => [
    index("abuse_flag_unresolved_idx")
      .on(t.createdAt)
      .where(sql`resolved_at IS NULL`),
    index("abuse_flag_user_idx").on(t.userId),
    index("abuse_flag_license_idx").on(t.licenseId),
    // keep in sync with ABUSE_FLAG_TYPES (abuse.ts)
    check(
      "abuse_flag_type_check",
      sql`type IN (
        'fingerprint_flag',
        'supersede_failed',
        'supersede_not_honoured',
        'superseded_renewal_charged'
      )`,
    ),
  ],
);

export type AdminAuditLogEntry = typeof adminAuditLog.$inferSelect;
export type NewAdminAuditLogEntry = typeof adminAuditLog.$inferInsert;
export type AbuseFlag = typeof abuseFlag.$inferSelect;
export type NewAbuseFlag = typeof abuseFlag.$inferInsert;
