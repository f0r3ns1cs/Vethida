import { createHash } from "node:crypto";
import { and, eq, inArray, isNull, lt, sql } from "drizzle-orm";
import {
  accountDeletion,
  cancellationRequest,
  db,
  deletionAudit,
  license,
  session,
  user,
  verification,
  type AccountDeletion,
} from "@vethida/db";
import { sendEmail } from "./email";
import { formatDate } from "./format";
import { legalEntityAddress } from "./email/identity";
import { renderTemplate } from "./email/render";
import { getPolar } from "./polar";

const GRACE_DAYS = 30;

// best-effort, the local flag + Polar webhook sort it out if this fails
async function cancelPolarSubscriptions(
  licenses: { subscriptionId: string | null }[],
): Promise<void> {
  const ids = [
    ...new Set(
      licenses
        .map((l) => l.subscriptionId)
        .filter((id): id is string => id !== null),
    ),
  ];
  if (ids.length === 0) return;

  let polar: ReturnType<typeof getPolar>;
  try {
    polar = getPolar();
  } catch (err) {
    console.error(
      "[account-deletion] Polar not configured; relying on webhook reconcile",
      err,
    );
    return;
  }

  for (const id of ids) {
    try {
      await polar.subscriptions.update({
        id,
        subscriptionUpdate: { cancelAtPeriodEnd: true },
      });
    } catch (err) {
      console.error(`[account-deletion] Polar cancel failed for ${id}`, err);
    }
  }
}

// undo of the above when a deletion is canceled
async function restorePolarSubscriptions(ids: string[]): Promise<void> {
  if (ids.length === 0) return;

  let polar: ReturnType<typeof getPolar>;
  try {
    polar = getPolar();
  } catch (err) {
    console.error(
      "[account-deletion] Polar not configured; cannot restore subscriptions",
      err,
    );
    return;
  }

  for (const id of ids) {
    try {
      await polar.subscriptions.update({
        id,
        subscriptionUpdate: { cancelAtPeriodEnd: false },
      });
    } catch (err) {
      console.error(`[account-deletion] Polar restore failed for ${id}`, err);
    }
  }
}

// audit rows outlive the user, so only store a hash of the id
function anonRef(userId: string): string {
  return createHash("sha256").update(userId).digest("hex").slice(0, 32);
}

export async function getPendingDeletion(
  userId: string,
): Promise<AccountDeletion | null> {
  const [row] = await db
    .select()
    .from(accountDeletion)
    .where(eq(accountDeletion.userId, userId))
    .limit(1);
  return row ?? null;
}

export interface RequestResult {
  alreadyPending: boolean;
  hardDeleteAt: Date;
}

export async function requestAccountDeletion(
  userId: string,
): Promise<RequestResult> {
  const existing = await getPendingDeletion(userId);
  if (existing) {
    return { alreadyPending: true, hardDeleteAt: existing.hardDeleteAt };
  }

  const [owner] = await db
    .select({ email: user.email })
    .from(user)
    .where(eq(user.id, userId))
    .limit(1);

  const hardDeleteAt = new Date(Date.now() + GRACE_DAYS * 86_400_000);

  // Cancel at period end. Only ones not already canceling, so we know exactly
  // which ones to restore if the deletion is canceled.
  const canceled = await db
    .update(license)
    .set({ cancelAtPeriodEnd: true })
    .where(
      and(
        eq(license.userId, userId),
        isNull(license.revokedAt),
        eq(license.cancelAtPeriodEnd, false),
        sql`polar_subscription_id IS NOT NULL`,
      ),
    )
    .returning({ subscriptionId: license.polarSubscriptionId });

  const cancelledIds = [
    ...new Set(
      canceled
        .map((c) => c.subscriptionId)
        .filter((id): id is string => id !== null),
    ),
  ];

  await db.insert(accountDeletion).values({
    userId,
    hardDeleteAt,
    cancelledSubscriptionIds: cancelledIds,
  });

  await cancelPolarSubscriptions(canceled);

  await db.delete(session).where(eq(session.userId, userId));

  await db.insert(deletionAudit).values({
    event: "requested",
    userRef: anonRef(userId),
    detail: `grace_until=${hardDeleteAt.toISOString()}`,
  });

  if (owner?.email) {
    const rendered = renderTemplate("account-deletion-received", {
      hardDeleteDate: formatDate(hardDeleteAt),
      legalEntityAddress: legalEntityAddress(),
    });
    await sendEmail({ to: owner.email, ...rendered }).catch(() => {});
  }

  return { alreadyPending: false, hardDeleteAt };
}

export async function cancelAccountDeletion(userId: string): Promise<boolean> {
  const removed = await db
    .delete(accountDeletion)
    .where(eq(accountDeletion.userId, userId))
    .returning({
      userId: accountDeletion.userId,
      cancelledSubscriptionIds: accountDeletion.cancelledSubscriptionIds,
    });
  if (removed.length === 0) return false;

  // restore only the subscriptions the deletion request canceled
  const restoreIds = removed[0].cancelledSubscriptionIds ?? [];
  if (restoreIds.length > 0) {
    await db
      .update(license)
      .set({ cancelAtPeriodEnd: false })
      .where(
        and(
          eq(license.userId, userId),
          isNull(license.revokedAt),
          inArray(license.polarSubscriptionId, restoreIds),
        ),
      );
    await restorePolarSubscriptions(restoreIds);
  }

  await db
    .insert(deletionAudit)
    .values({ event: "canceled", userRef: anonRef(userId) });
  return true;
}

// past the grace period and not blocked, soonest first
export async function findHardDeleteEligible(): Promise<string[]> {
  const rows = await db
    .select({ userId: accountDeletion.userId })
    .from(accountDeletion)
    .where(
      and(
        lt(accountDeletion.hardDeleteAt, sql`now()`),
        isNull(accountDeletion.blockedReason),
      ),
    )
    .orderBy(accountDeletion.hardDeleteAt);
  return rows.map((r) => r.userId);
}

// IRREVERSIBLE. Licenses (SET NULL fk) and verification rows (keyed by email)
// have to be deleted by hand, the rest cascades from user.
export async function hardDeleteUser(userId: string): Promise<void> {
  const [u] = await db
    .select({ email: user.email })
    .from(user)
    .where(eq(user.id, userId))
    .limit(1);

  await db.transaction(async (tx) => {
    await tx.delete(license).where(eq(license.userId, userId));
    if (u?.email) {
      await tx.delete(verification).where(eq(verification.identifier, u.email));
    }

    // keep the cancellation reason but drop the free-text comment (can have PII)
    await tx
      .update(cancellationRequest)
      .set({ comment: null })
      .where(eq(cancellationRequest.userId, userId));

    await tx.delete(user).where(eq(user.id, userId));
    await tx
      .insert(deletionAudit)
      .values({ event: "hard_deleted", userRef: anonRef(userId) });
  });

  // after the commit, email was read before the delete
  if (u?.email) {
    const rendered = renderTemplate("account-deleted", {
      deletionDate: formatDate(new Date()),
      formerEmail: u.email,
      legalEntityAddress: legalEntityAddress(),
    });
    await sendEmail({ to: u.email, ...rendered }).catch(() => {});
  }
}
