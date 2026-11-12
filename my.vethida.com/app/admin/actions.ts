"use server";

import { revalidatePath } from "next/cache";
import { redirect } from "next/navigation";
import { and, eq, isNull } from "drizzle-orm";
import {
  abuseFlag,
  db,
  user,
  generateLicenseKey,
  hashLicenseKey,
  isUuid,
  license,
  liftAllSuspensions,
  recordAdminAction,
  revokeLicense,
  suspend,
  unrevokeLicense,
  type RevokeReasonCode,
} from "@vethida/db";
import { requestIp, requireAdmin } from "@/lib/admin";

// form uuid or null (postgres throws 22P02 on bad uuids). Just a shape check.
function formUuid(formData: FormData, field: string): string | null {
  const v = formData.get(field);
  return isUuid(v) ? v : null;
}

const REVOKE_REASON_CODES: readonly RevokeReasonCode[] = [
  "chargeback",
  "tos_violation",
  "fraud",
  "user_request",
  "admin_error",
  "other",
];

function isRevokeReasonCode(v: unknown): v is RevokeReasonCode {
  return (
    typeof v === "string" &&
    (REVOKE_REASON_CODES as readonly string[]).includes(v)
  );
}

export async function lookupUserAction(formData: FormData) {
  await requireAdmin(); // server actions can be called directly

  const email = formData.get("email");
  if (typeof email !== "string" || !email.trim()) redirect("/admin");

  const [account] = await db
    .select({ id: user.id })
    .from(user)
    .where(eq(user.email, email.toLowerCase().trim()))
    .limit(1);

  redirect(account ? `/admin?u=${account.id}` : "/admin?notfound=1");
}

export async function suspendUserAction(formData: FormData) {
  const session = await requireAdmin();

  const userId = formUuid(formData, "userId");
  if (!userId) return;
  const detail = formData.get("reasonDetail");
  const reasonDetail =
    typeof detail === "string" && detail.trim() ? detail.trim() : undefined;

  const suspensionId = await suspend({
    userId,
    reasonCode: "admin_action",
    source: "admin",
    reasonDetail,
  });
  await recordAdminAction({
    adminId: session.user.id,
    action: "user_suspended",
    targetType: "user",
    targetId: userId,
    ipAddress: await requestIp(),
    details: { suspensionId, reasonDetail: reasonDetail ?? null },
  });

  revalidatePath("/admin");
}

export async function liftSuspensionsAction(formData: FormData) {
  const session = await requireAdmin();

  const userId = formUuid(formData, "userId");
  if (!userId) return;

  const lifted = await liftAllSuspensions(userId, session.user.id);
  await recordAdminAction({
    adminId: session.user.id,
    action: "suspension_lifted",
    targetType: "user",
    targetId: userId,
    ipAddress: await requestIp(),
    details: { liftedCount: lifted },
  });

  revalidatePath("/admin");
}

export async function revokeLicenseAction(formData: FormData) {
  const session = await requireAdmin();

  const licenseId = formUuid(formData, "licenseId");
  const reasonCode = formData.get("reasonCode");
  if (!licenseId) return;
  if (!isRevokeReasonCode(reasonCode)) return;
  const note = formData.get("reason");
  const reason =
    typeof note === "string" && note.trim() ? note.trim() : undefined;

  const revoked = await revokeLicense({
    licenseId,
    reasonCode,
    reason,
    revokedBy: session.user.id,
  });
  if (revoked) {
    await recordAdminAction({
      adminId: session.user.id,
      action: "license_revoked",
      targetType: "license",
      targetId: licenseId,
      ipAddress: await requestIp(),
      details: { reasonCode, reason: reason ?? null },
    });
  }

  revalidatePath("/admin");
}

export async function unrevokeLicenseAction(formData: FormData) {
  const session = await requireAdmin();

  const licenseId = formUuid(formData, "licenseId");
  if (!licenseId) return;

  const restored = await unrevokeLicense(licenseId);
  if (restored) {
    await recordAdminAction({
      adminId: session.user.id,
      action: "license_unrevoked",
      targetType: "license",
      targetId: licenseId,
      ipAddress: await requestIp(),
    });
  }

  revalidatePath("/admin");
}

export async function resolveFlagAction(formData: FormData) {
  const session = await requireAdmin();

  const flagId = formUuid(formData, "flagId");
  if (!flagId) return;

  const resolved = await db
    .update(abuseFlag)
    .set({ resolvedAt: new Date(), resolvedBy: session.user.id })
    .where(and(eq(abuseFlag.id, flagId), isNull(abuseFlag.resolvedAt)))
    .returning({ id: abuseFlag.id });
  if (resolved.length > 0) {
    await recordAdminAction({
      adminId: session.user.id,
      action: "flag_resolved",
      targetType: "abuse_flag",
      targetId: flagId,
      ipAddress: await requestIp(),
    });
  }

  revalidatePath("/admin/flags");
}

// days, or perpetual
const MINT_TERMS = ["30", "90", "365", "perpetual"] as const;
type MintTerm = (typeof MINT_TERMS)[number];

function isMintTerm(v: unknown): v is MintTerm {
  return typeof v === "string" && (MINT_TERMS as readonly string[]).includes(v);
}

export async function mintLicenseAction(formData: FormData) {
  const session = await requireAdmin();

  const userId = formUuid(formData, "userId");
  const tier = formData.get("tier");
  const term = formData.get("term");
  if (!userId) return;
  if (tier !== "pro" && tier !== "ultimate") return;
  if (!isMintTerm(term)) return;

  const expiresAt =
    term === "perpetual"
      ? null
      : new Date(Date.now() + Number(term) * 24 * 60 * 60 * 1000);

  const key = generateLicenseKey();
  const [minted] = await db
    .insert(license)
    .values({ key, keyHash: hashLicenseKey(key), tier, userId, expiresAt })
    .returning({ id: license.id });

  // no raw key in the audit log
  await recordAdminAction({
    adminId: session.user.id,
    action: "license_minted",
    targetType: "license",
    targetId: minted.id,
    ipAddress: await requestIp(),
    details: {
      tier,
      userId,
      term,
      expiresAt: expiresAt ? expiresAt.toISOString() : null,
    },
  });

  revalidatePath("/admin");
}
