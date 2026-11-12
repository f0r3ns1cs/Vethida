import { db } from "./client";
import { adminAuditLog } from "./admin";

export type AdminAction =
  | "user_suspended"
  | "suspension_lifted"
  | "license_revoked"
  | "license_unrevoked"
  | "license_minted"
  | "flag_resolved"
  | "release_announcement_sent"
  | "data_export";

// Changes made by webhooks, not by an admin.
export type SystemAction =
  | "license_issued"
  | "license_renewed"
  | "license_past_due"
  | "license_lapsed"
  | "license_refunded"
  | "license_superseded";

export type AdminTargetType = "user" | "license" | "abuse_flag" | "broadcast";

export interface RecordAdminActionArgs {
  adminId: string;
  action: AdminAction;
  targetType: AdminTargetType;
  targetId: string;
  ipAddress: string;
  details?: Record<string, unknown>; // never put raw license keys in here
}

// throws if the insert fails, so the admin action fails too
export async function recordAdminAction(
  args: RecordAdminActionArgs,
): Promise<void> {
  await db.insert(adminAuditLog).values({
    adminId: args.adminId,
    actor: "admin",
    action: args.action,
    targetType: args.targetType,
    targetId: args.targetId,
    ipAddress: args.ipAddress,
    details: args.details ?? null,
  });
}

export interface RecordSystemActionArgs {
  action: SystemAction;
  targetType: AdminTargetType;
  targetId: string;
  details?: Record<string, unknown>;
}

// Doesn't throw. The webhook already did its work, failing here would just make Polar retry it.
export async function recordSystemAction(
  args: RecordSystemActionArgs,
): Promise<void> {
  try {
    await db.insert(adminAuditLog).values({
      adminId: null,
      actor: "system",
      action: args.action,
      targetType: args.targetType,
      targetId: args.targetId,
      ipAddress: null,
      details: args.details ?? null,
    });
  } catch (err) {
    console.error(`[audit] failed to record ${args.action}`, err);
  }
}
