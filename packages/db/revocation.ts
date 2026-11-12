import { and, eq, isNotNull, isNull, sql } from "drizzle-orm";
import { db } from "./client";
import { license } from "./licensing";

// keep in sync with license_revoked_reason_code_check
export type RevokeReasonCode =
  | "chargeback"
  | "tos_violation"
  | "fraud"
  | "user_request"
  | "admin_error"
  | "other";

export async function revokeLicense(args: {
  licenseId: string;
  reasonCode: RevokeReasonCode;
  reason?: string; // internal only
  revokedBy?: string;
}): Promise<boolean> {
  const updated = await db
    .update(license)
    .set({
      revokedAt: new Date(),
      revokedBy: args.revokedBy ?? null,
      revokedReasonCode: args.reasonCode,
      revokedReason: args.reason ?? null,
      tokenGeneration: sql`${license.tokenGeneration} + 1`,
    })
    .where(and(eq(license.id, args.licenseId), isNull(license.revokedAt)))
    .returning({ id: license.id });
  return updated.length > 0;
}

export async function unrevokeLicense(licenseId: string): Promise<boolean> {
  const updated = await db
    .update(license)
    .set({
      revokedAt: null,
      revokedBy: null,
      revokedReasonCode: null,
      revokedReason: null,
    })
    .where(and(eq(license.id, licenseId), isNotNull(license.revokedAt)))
    .returning({ id: license.id });
  return updated.length > 0;
}
