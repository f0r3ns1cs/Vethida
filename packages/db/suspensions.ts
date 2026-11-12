import { and, eq, isNull, sql } from "drizzle-orm";
import { db } from "./client";
import { userSuspension, type UserSuspension } from "./licensing";

// keep in sync with the user_suspension check constraints
export type SuspensionReason =
  | "device_sharing"
  | "chargeback"
  | "admin_action";

export type SuspensionSource =
  | "auto_antishare"
  | "chargeback_webhook"
  | "admin";

export async function hasActiveSuspension(userId: string): Promise<boolean> {
  const rows = await db
    .select({ id: userSuspension.id })
    .from(userSuspension)
    .where(
      and(eq(userSuspension.userId, userId), isNull(userSuspension.liftedAt)),
    )
    .limit(1);
  return rows.length > 0;
}

export async function getActiveSuspension(
  userId: string,
): Promise<UserSuspension | null> {
  const rows = await db
    .select()
    .from(userSuspension)
    .where(
      and(eq(userSuspension.userId, userId), isNull(userSuspension.liftedAt)),
    )
    .orderBy(userSuspension.createdAt)
    .limit(1);
  return rows[0] ?? null;
}

export interface SuspendParams {
  userId: string;
  reasonCode: SuspensionReason;
  source: SuspensionSource;
  reasonDetail?: string;
}

// one active row per (user, reason), so calling twice is fine
export async function suspend(params: SuspendParams): Promise<string> {
  const inserted = await db
    .insert(userSuspension)
    .values({
      userId: params.userId,
      reasonCode: params.reasonCode,
      source: params.source,
      reasonDetail: params.reasonDetail ?? null,
    })
    .onConflictDoNothing({
      target: [userSuspension.userId, userSuspension.reasonCode],
      where: sql`lifted_at IS NULL`,
    })
    .returning({ id: userSuspension.id });
  const [insertedRow] = inserted;
  if (insertedRow) return insertedRow.id;

  const rows = await db
    .select({ id: userSuspension.id })
    .from(userSuspension)
    .where(
      and(
        eq(userSuspension.userId, params.userId),
        eq(userSuspension.reasonCode, params.reasonCode),
        isNull(userSuspension.liftedAt),
      ),
    )
    .limit(1);
  const [existing] = rows;
  if (existing) return existing.id;

  // got lifted in between, try again
  return suspend(params);
}

export async function liftSuspension(
  suspensionId: string,
  adminUserId: string,
): Promise<boolean> {
  const lifted = await db
    .update(userSuspension)
    .set({ liftedAt: new Date(), liftedBy: adminUserId })
    .where(
      and(eq(userSuspension.id, suspensionId), isNull(userSuspension.liftedAt)),
    )
    .returning({ id: userSuspension.id });
  return lifted.length > 0;
}

export async function liftAllSuspensions(
  userId: string,
  adminUserId: string,
): Promise<number> {
  const lifted = await db
    .update(userSuspension)
    .set({ liftedAt: new Date(), liftedBy: adminUserId })
    .where(
      and(eq(userSuspension.userId, userId), isNull(userSuspension.liftedAt)),
    )
    .returning({ id: userSuspension.id });
  return lifted.length;
}
