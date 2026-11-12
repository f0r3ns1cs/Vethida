import { and, eq, isNull, sql, type SQL } from "drizzle-orm";
import {
  type AbuseFlagType,
  abuseFlag,
  db,
  generateLicenseKey,
  hashLicenseKey,
  isPaidTier,
  license,
  LIFETIME_SCOPE,
  type PaidTier,
  polarCustomer,
  recordSystemAction,
  tierLabel,
  user,
} from "@vethida/db";
import { sendEmail } from "../email";
import { legalEntityAddress } from "../email/identity";
import { renderTemplate } from "../email/render";
import { getPolar } from "./client";

const BILLING_URL = "https://my.vethida.com/dashboard/billing";

// first name guessed from the email when there's no display name
function deriveFirstName(email: string): string {
  const local = email.split("@")[0] ?? "";
  const first = local.split(/[._-]/)[0] ?? "";
  return first
    ? first.charAt(0).toUpperCase() + first.slice(1).toLowerCase()
    : "there";
}

// Puts a billing problem on the /admin/flags queue (reuses abuse_flag, it already
// has the admin page). Typed so a bad `type` fails at build time, not silently
// in the catch below.
async function raiseBillingAlert(
  type: AbuseFlagType,
  licenseId: string | null,
  userId: string | null,
  details: Record<string, unknown>,
): Promise<void> {
  try {
    await db.insert(abuseFlag).values({ userId, licenseId, type, details });
  } catch (err) {
    console.error(`[polar] failed to record ${type} alert`, err);
  }
}

// Retires the license an order replaces (Pro -> Ultimate, or subscription ->
// lifetime on the same tier). Cancels at period end instead of revoking, a
// revoke shows the user a scary "license revoked" message right after they
// upgraded. A superseded lifetime license just keeps working.
// The local flag drives our side, the Polar call is what stops the billing.
async function supersedeLicense(
  licenseId: string,
  userId: string,
): Promise<void> {
  const [retired] = await db
    .update(license)
    .set({ cancelAtPeriodEnd: true, supersededAt: new Date() })
    .where(
      and(
        eq(license.id, licenseId),
        eq(license.userId, userId), // buyer's own license only
        isNull(license.revokedAt),
      ),
    )
    .returning({ subscriptionId: license.polarSubscriptionId });

  if (!retired) {
    console.warn("[polar] supersede target not found or not eligible", licenseId);
    return;
  }

  await recordSystemAction({
    action: "license_superseded",
    targetType: "license",
    targetId: licenseId,
    details: { userId, subscriptionId: retired.subscriptionId },
  });

  if (!retired.subscriptionId) return;

  try {
    await getPolar().subscriptions.update({
      id: retired.subscriptionId,
      subscriptionUpdate: { cancelAtPeriodEnd: true },
    });
  } catch (err) {
    console.error(
      `[polar] failed to cancel superseded subscription ${retired.subscriptionId}`,
      err,
    );
    // don't throw (they'd lose the tier they just paid for), flag it for a human
    await raiseBillingAlert("supersede_failed", licenseId, userId, {
      subscriptionId: retired.subscriptionId,
      consequence:
        "Polar was not told to stop this subscription; the customer is being billed for both tiers until it is canceled by hand.",
    });
  }
}

// Our own type for the Polar events we use (the SDK's types keep changing).
// data is the order or the subscription depending on the event. metadata comes
// from our checkout and Polar copies it onto renewals and the subscription.
export interface PolarWebhookEvent {
  type: string;
  data: {
    id: string;
    status?: string;
    billing_reason?: string;
    refunded_amount?: number; // order.refunded, minor units
    amount?: number;
    currency?: string;
    subscription_id?: string | null;
    current_period_end?: string | null;
    cancel_at_period_end?: boolean;
    ends_at?: string | null;
    customer?: { id?: string; email?: string };
    product?: { id?: string };
    metadata?: {
      userId?: string;
      tier?: string;
      cadence?: string; // 'monthly' | 'yearly' | 'lifetime'
      supersedesLicenseId?: string; // license this order replaces
    };
    subscription?: {
      id?: string;
      current_period_end?: string | null;
      cancel_at_period_end?: boolean;
    };
  };
}

function toDate(value: string | null | undefined): Date | null {
  if (!value) return null;
  const d = new Date(value);
  return Number.isNaN(d.getTime()) ? null : d;
}

function fmtDate(d: Date): string {
  return d.toISOString().slice(0, 10);
}

function periodEndOf(data: PolarWebhookEvent["data"]): Date | null {
  return toDate(data.subscription?.current_period_end ?? data.current_period_end);
}

// New expires_at from a period end, done in SQL so concurrent deliveries can't race:
//   NULL    lifetime, never touched
//   past    already lapsed, stays lapsed (a late subscription.updated must not
//           revive it - coming back mints a new license via order.paid)
//   future  only ever moves forward
function expiryFrom(periodEnd: Date): SQL {
  return sql`CASE
    WHEN ${license.expiresAt} IS NULL THEN ${license.expiresAt}
    WHEN ${license.expiresAt} <= now() THEN ${license.expiresAt}
    ELSE GREATEST(${license.expiresAt}, ${periodEnd})
  END`;
}

// Polar's words ('month' / 'year'), null for one-time purchases
function intervalFromMetadata(cadence: string | undefined): string | null {
  if (cadence === "monthly") return "month";
  if (cadence === "yearly") return "year";
  return null;
}

// saves the Polar customer id for the portal's billing link. Best-effort.
async function capturePolarCustomer(
  userId: string | undefined,
  customerId: string | undefined,
): Promise<void> {
  if (!userId || !customerId) return;
  try {
    await db
      .insert(polarCustomer)
      .values({ userId, polarCustomerId: customerId })
      .onConflictDoUpdate({
        target: polarCustomer.userId,
        set: { polarCustomerId: customerId, updatedAt: new Date() },
      });
  } catch (err) {
    console.error("[polar] failed to record customer id", { userId }, err);
  }
}

async function recipientEmail(
  userId: string,
  fallback?: string,
): Promise<string | null> {
  const owners = await db
    .select({ email: user.email })
    .from(user)
    .where(eq(user.id, userId))
    .limit(1);
  return owners[0]?.email ?? fallback ?? null;
}

export async function handleOrderPaid(event: PolarWebhookEvent): Promise<void> {
  const order = event.data;
  const userId = order.metadata?.userId;
  const tier = order.metadata?.tier;

  if (!userId || !isPaidTier(tier)) {
    console.error("[polar] order.paid with missing or invalid metadata", {
      orderId: order.id,
      tier,
    });
    return;
  }

  await capturePolarCustomer(userId, order.customer?.id);

  const subscriptionId = order.subscription_id ?? order.subscription?.id ?? null;
  const isRenewal =
    order.billing_reason === "subscription_cycle" ||
    order.billing_reason === "subscription_update";

  if (isRenewal && subscriptionId) {
    return handleRenewalOrder(order, userId, tier, subscriptionId);
  }

  // supersede first, so a failure can't leave two renewing subscriptions (idempotent)
  const supersedes = order.metadata?.supersedesLicenseId;
  if (supersedes) {
    await supersedeLicense(supersedes, userId);
  }

  // end any running trial, otherwise they'd have two machine slots
  await db
    .update(license)
    .set({ expiresAt: new Date() })
    .where(
      and(
        eq(license.userId, userId),
        eq(license.trial, true),
        isNull(license.revokedAt),
        sql`expires_at > now()`,
      ),
    );

  const periodEnd = periodEndOf(order);
  const key = generateLicenseKey();
  const inserted = await db
    .insert(license)
    .values({
      key,
      keyHash: hashLicenseKey(key),
      tier,
      userId,
      polarOrderId: order.id,
      polarSubscriptionId: subscriptionId,
      billingInterval: subscriptionId
        ? intervalFromMetadata(order.metadata?.cadence)
        : null,
      expiresAt: periodEnd,
    })
    .onConflictDoNothing({
      target: [license.polarOrderId],
      where: sql`polar_order_id IS NOT NULL`,
    })
    .returning({ id: license.id, key: license.key, expiresAt: license.expiresAt });

  const freshlyMinted = inserted.length > 0; // false on a redelivery

  // conflict = an earlier delivery made the license but failed later (usually
  // the email), so load it and only redo the rest
  let provisioned = inserted[0];
  if (!provisioned) {
    const rows = await db
      .select({ id: license.id, key: license.key, expiresAt: license.expiresAt })
      .from(license)
      .where(eq(license.polarOrderId, order.id))
      .limit(1);
    provisioned = rows[0];
  }
  if (!provisioned) {
    throw new Error(`[polar] license lookup failed for order ${order.id}`);
  }

  if (freshlyMinted) {
    await recordSystemAction({
      action: "license_issued",
      targetType: "license",
      targetId: provisioned.id,
      details: {
        userId,
        tier,
        orderId: order.id,
        subscriptionId,
        perpetual: provisioned.expiresAt === null,
        expiresAt: provisioned.expiresAt?.toISOString() ?? null,
      },
    });
  }

  const recipient = await recipientEmail(userId, order.customer?.email);
  if (!recipient) {
    console.error("[polar] no recipient for order", { orderId: order.id });
    return;
  }

  const isSubscription = subscriptionId != null;
  const expiresAt = provisioned.expiresAt;
  const label = tierLabel(tier);
  const renewalNotice = isSubscription
    ? `Your ${label} subscription renews automatically${
        expiresAt ? ` on ${fmtDate(expiresAt)}` : ""
      } unless you cancel. You can see the exact renewal amount and date, update your payment method, or cancel any time at ${BILLING_URL}. If you cancel, ${label} keeps working through the end of the term you've paid for, then Vethida drops to Free.`
    : // LIFETIME_SCOPE only for real lifetime keys (no expiry)
      `This was a one-time purchase. It does not renew and you won't be charged again.${
        expiresAt ? "" : ` ${LIFETIME_SCOPE}`
      }`;

  const rendered = renderTemplate("license-issued", {
    customerFirstName: deriveFirstName(recipient),
    licenseKey: provisioned.key,
    licenseId: provisioned.id,
    tier: label,
    expiryLabel: isSubscription ? "Renews" : expiresAt ? "Expires" : "License type",
    expiryValue: isSubscription
      ? expiresAt
        ? fmtDate(expiresAt)
        : "shown in your dashboard"
      : expiresAt
        ? fmtDate(expiresAt)
        : "Lifetime (one-time purchase)",
    renewalNotice,
    legalEntityAddress: legalEntityAddress(),
  });
  const delivered = await sendEmail({ to: recipient, ...rendered });
  if (!delivered) {
    // throw so Polar retries, the retry only redoes the email
    throw new Error(`[polar] license email delivery failed for order ${order.id}`);
  }
}

async function handleRenewalOrder(
  order: PolarWebhookEvent["data"],
  userId: string,
  tier: PaidTier,
  subscriptionId: string,
): Promise<void> {
  const periodEnd = periodEndOf(order);

  // renewal on a superseded subscription = they got billed for a tier they
  // upgraded away from. Can't undo it here, so flag it.
  const [supersededRenewal] = await db
    .select({ id: license.id, userId: license.userId })
    .from(license)
    .where(
      and(
        eq(license.polarSubscriptionId, subscriptionId),
        isNull(license.revokedAt),
        sql`superseded_at IS NOT NULL`,
      ),
    )
    .limit(1);
  if (supersededRenewal) {
    console.error(
      `[polar] renewal charged on superseded subscription ${subscriptionId}`,
    );
    await raiseBillingAlert(
      "superseded_renewal_charged",
      supersededRenewal.id,
      supersededRenewal.userId,
      {
        subscriptionId,
        orderId: order.id,
        consequence:
          "The customer was charged for a tier they had already upgraded away from. Cancel the subscription in Polar and refund this order.",
      },
    );
  }

  // backfill the interval if it's missing, never overwrite
  const renewalInterval = intervalFromMetadata(order.metadata?.cadence);
  const backfill = renewalInterval
    ? {
        billingInterval: sql`COALESCE(${license.billingInterval}, ${renewalInterval})`,
      }
    : {};

  // A renewal for an already lapsed subscription is a stale redelivery (coming
  // back gets a new subscription id), so it must not extend or un-cancel it.
  const updated = await db
    .update(license)
    .set({
      ...(periodEnd ? { expiresAt: expiryFrom(periodEnd) } : {}),
      cancelAtPeriodEnd: sql`CASE
        WHEN ${license.expiresAt} IS NOT NULL AND ${license.expiresAt} <= now()
          THEN ${license.cancelAtPeriodEnd}
        ELSE false
      END`,
      renewalReminderSentAt: null,
      pastDueNoticeSentAt: null, // so the next failed payment sends a notice again
      ...backfill,
    })
    .where(
      and(
        eq(license.polarSubscriptionId, subscriptionId),
        isNull(license.revokedAt),
      ),
    )
    .returning({ id: license.id, key: license.key, expiresAt: license.expiresAt });

  let lic = updated[0];
  if (!lic) {
    // renewal came before we stored the subscription id, find it by owner + tier
    const found = await db
      .select({ id: license.id, key: license.key, expiresAt: license.expiresAt })
      .from(license)
      .where(
        and(
          eq(license.userId, userId),
          eq(license.tier, tier),
          isNull(license.revokedAt),
        ),
      )
      .limit(1);
    lic = found[0];
    if (lic) {
      await db
        .update(license)
        .set({
          polarSubscriptionId: subscriptionId,
          cancelAtPeriodEnd: false,
          renewalReminderSentAt: null,
          ...(periodEnd ? { expiresAt: periodEnd } : {}),
        })
        .where(eq(license.id, lic.id));
      if (periodEnd) lic.expiresAt = periodEnd;
    }
  }
  if (!lic) {
    console.error("[polar] renewal for unknown subscription", { subscriptionId });
    return;
  }

  await recordSystemAction({
    action: "license_renewed",
    targetType: "license",
    targetId: lic.id,
    details: {
      userId,
      tier,
      orderId: order.id,
      subscriptionId,
      expiresAt: (lic.expiresAt ?? periodEnd)?.toISOString() ?? null,
    },
  });

  const recipient = await recipientEmail(userId, order.customer?.email);
  if (!recipient) {
    console.error("[polar] no recipient for renewal", { subscriptionId });
    return;
  }

  const newExpiry = lic.expiresAt ?? periodEnd;
  const rendered = renderTemplate("license-renewed", {
    customerFirstName: deriveFirstName(recipient),
    tier: tierLabel(tier),
    licenseKeyPrefix: lic.key.slice(0, 5),
    newExpiryDate: newExpiry ? fmtDate(newExpiry) : "your next term",
    legalEntityAddress: legalEntityAddress(),
  });
  const delivered = await sendEmail({ to: recipient, ...rendered });
  if (!delivered) {
    throw new Error(
      `[polar] renewal email delivery failed for subscription ${subscriptionId}`,
    );
  }
}

export async function handleSubscriptionCanceled(
  event: PolarWebhookEvent,
): Promise<void> {
  const sub = event.data;
  const updated = await db
    .update(license)
    .set({ cancelAtPeriodEnd: true })
    .where(
      and(eq(license.polarSubscriptionId, sub.id), isNull(license.revokedAt)),
    )
    .returning({
      tier: license.tier,
      userId: license.userId,
      expiresAt: license.expiresAt,
    });

  const lic = updated[0];
  if (!lic) {
    console.error("[polar] cancel for unknown subscription", {
      subscriptionId: sub.id,
    });
    return;
  }
  if (!lic.userId || !isPaidTier(lic.tier)) return;

  const recipient = await recipientEmail(lic.userId, sub.customer?.email);
  if (!recipient) return;

  const end = toDate(sub.ends_at ?? sub.current_period_end) ?? lic.expiresAt;
  const rendered = renderTemplate("subscription-canceled", {
    customerFirstName: deriveFirstName(recipient),
    tier: tierLabel(lic.tier),
    effectiveEndDate: end ? fmtDate(end) : "the end of your current term",
    legalEntityAddress: legalEntityAddress(),
  });
  const delivered = await sendEmail({ to: recipient, ...rendered });
  if (!delivered) {
    throw new Error(
      `[polar] cancellation email delivery failed for subscription ${sub.id}`,
    );
  }
}

// Applies a subscription event to the license. Events can arrive out of order,
// so nothing moves backwards: see expiryFrom, and cancel_at_period_end is frozen
// once the license has lapsed.
async function syncSubscriptionState(
  subscriptionId: string,
  periodEnd: Date | null,
  cancel: boolean,
): Promise<void> {
  await db
    .update(license)
    .set({
      ...(periodEnd ? { expiresAt: expiryFrom(periodEnd) } : {}),
      cancelAtPeriodEnd: sql`CASE
        WHEN ${license.expiresAt} IS NOT NULL AND ${license.expiresAt} <= now()
          THEN ${license.cancelAtPeriodEnd}
        ELSE ${cancel}
      END`,
      pastDueNoticeSentAt: null, // subscription.active after a retry lands here
    })
    .where(
      and(
        eq(license.polarSubscriptionId, subscriptionId),
        isNull(license.revokedAt),
      ),
    );
}

export async function handleSubscriptionUncanceled(
  event: PolarWebhookEvent,
): Promise<void> {
  const sub = event.data;
  await syncSubscriptionState(sub.id, toDate(sub.current_period_end), false);
}

// subscription.active / subscription.updated: sync period end and cancel flag
export async function handleSubscriptionUpdated(
  event: PolarWebhookEvent,
): Promise<void> {
  const sub = event.data;
  const periodEnd = toDate(sub.current_period_end);
  const cancel = sub.cancel_at_period_end === true;

  // If a superseded license shows up as still renewing, the cancel call to
  // Polar failed. Don't write cancel=false back (that would mean double billing
  // forever), sync the expiry only and flag it.
  if (!cancel) {
    const [superseded] = await db
      .select({ id: license.id, userId: license.userId })
      .from(license)
      .where(
        and(
          eq(license.polarSubscriptionId, sub.id),
          isNull(license.revokedAt),
          sql`superseded_at IS NOT NULL`,
        ),
      )
      .limit(1);

    if (superseded) {
      if (periodEnd) {
        await db
          .update(license)
          .set({ expiresAt: expiryFrom(periodEnd) })
          .where(eq(license.id, superseded.id));
      }
      console.error(
        `[polar] subscription ${sub.id} still renewing despite supersede`,
      );
      await raiseBillingAlert(
        "supersede_not_honoured",
        superseded.id,
        superseded.userId,
        {
          subscriptionId: sub.id,
          consequence:
            "Polar reports this superseded subscription as renewing. Cancel it in Polar or the customer pays for two tiers.",
        },
      );
      return;
    }
  }

  await syncSubscriptionState(sub.id, periodEnd, cancel);
}

// Failed renewal payment. Polar only sends subscription.past_due (no per-retry
// events), so one email per episode, tracked with past_due_notice_sent_at.
// Entitlement doesn't change here, only subscription.revoked ends it.
export async function handleSubscriptionPastDue(
  event: PolarWebhookEvent,
): Promise<void> {
  const sub = event.data;
  const claimed = await db
    .update(license)
    .set({ pastDueNoticeSentAt: new Date() })
    .where(
      and(
        eq(license.polarSubscriptionId, sub.id),
        isNull(license.revokedAt),
        isNull(license.pastDueNoticeSentAt),
      ),
    )
    .returning({
      id: license.id,
      tier: license.tier,
      userId: license.userId,
      expiresAt: license.expiresAt,
    });

  const lic = claimed[0];
  if (!lic) return; // already notified, or unknown subscription

  await recordSystemAction({
    action: "license_past_due",
    targetType: "license",
    targetId: lic.id,
    details: { userId: lic.userId, subscriptionId: sub.id },
  });

  if (!lic.userId || !isPaidTier(lic.tier)) return;
  const recipient = await recipientEmail(lic.userId, sub.customer?.email);
  if (!recipient) return;

  const end = toDate(sub.current_period_end) ?? lic.expiresAt;
  const rendered = renderTemplate("payment-failed", {
    customerFirstName: deriveFirstName(recipient),
    tier: tierLabel(lic.tier),
    // Polar doesn't give us the real decline reason
    declineReason: "the payment was declined",
    expiryDate: end ? fmtDate(end) : "the end of your current term",
    legalEntityAddress: legalEntityAddress(),
  });
  const delivered = await sendEmail({ to: recipient, ...rendered });
  if (!delivered) {
    throw new Error(
      `[polar] past-due email delivery failed for subscription ${sub.id}`,
    );
  }
}

// Subscription is over (retries ran out, or a canceled term ended). Expire the
// license and bump token_generation so existing desktop tokens stop working.
// revoked_at stays null, this is a normal lapse and the account drops to Free.
export async function handleSubscriptionRevoked(
  event: PolarWebhookEvent,
): Promise<void> {
  const sub = event.data;
  const end = toDate(sub.ends_at ?? sub.current_period_end) ?? new Date();
  const lapsed = await db
    .update(license)
    .set({
      expiresAt: end,
      cancelAtPeriodEnd: true,
      tokenGeneration: sql`${license.tokenGeneration} + 1`,
    })
    .where(
      and(eq(license.polarSubscriptionId, sub.id), isNull(license.revokedAt)),
    )
    .returning({
      id: license.id,
      tier: license.tier,
      userId: license.userId,
      // tells failed payments apart from a normal cancel (don't clear it in the SET)
      wasPastDue: license.pastDueNoticeSentAt,
    });

  for (const lic of lapsed) {
    await recordSystemAction({
      action: "license_lapsed",
      targetType: "license",
      targetId: lic.id,
      details: {
        userId: lic.userId,
        subscriptionId: sub.id,
        expiresAt: end.toISOString(),
        reasonCode: lic.wasPastDue ? "payment_failed" : "canceled",
      },
    });

    if (!lic.wasPastDue) continue;
    if (!lic.userId || !isPaidTier(lic.tier)) continue;
    const recipient = await recipientEmail(lic.userId, sub.customer?.email);
    if (!recipient) continue;

    const rendered = renderTemplate("payment-failed-final", {
      customerFirstName: deriveFirstName(recipient),
      tier: tierLabel(lic.tier),
      endDate: fmtDate(end),
      legalEntityAddress: legalEntityAddress(),
    });
    const delivered = await sendEmail({ to: recipient, ...rendered });
    if (!delivered) {
      throw new Error(
        `[polar] lapse email delivery failed for subscription ${sub.id}`,
      );
    }
  }
}

function formatRefundAmount(data: PolarWebhookEvent["data"]): string {
  const cents =
    typeof data.refunded_amount === "number"
      ? data.refunded_amount
      : typeof data.amount === "number"
        ? data.amount
        : null;
  if (cents === null) return "the full amount";
  return (cents / 100).toLocaleString("en-US", {
    style: "currency",
    currency: (data.currency ?? "USD").toUpperCase(),
    // same formatting as the portal's prices
    minimumFractionDigits: cents % 100 === 0 ? 0 : 2,
  });
}

export async function handleOrderRefunded(
  event: PolarWebhookEvent,
): Promise<void> {
  const updated = await db
    .update(license)
    .set({
      revokedAt: new Date(),
      revokedReasonCode: "user_request",
      revokedReason: "Polar order refunded",
      tokenGeneration: sql`${license.tokenGeneration} + 1`,
    })
    .where(
      and(eq(license.polarOrderId, event.data.id), isNull(license.revokedAt)),
    )
    .returning({
      id: license.id,
      key: license.key,
      userId: license.userId,
      createdAt: license.createdAt,
    });

  const lic = updated[0];
  if (!lic) {
    console.error("[polar] refund for unknown or already revoked order", {
      orderId: event.data.id,
    });
    return;
  }

  // audit even if the account is gone (needed for chargebacks)
  await recordSystemAction({
    action: "license_refunded",
    targetType: "license",
    targetId: lic.id,
    details: {
      userId: lic.userId,
      orderId: event.data.id,
      reasonCode: "user_request",
    },
  });

  if (!lic.userId) return;

  const recipient = await recipientEmail(lic.userId, event.data.customer?.email);
  if (!recipient) return;

  const rendered = renderTemplate("refund-issued", {
    customerFirstName: deriveFirstName(recipient),
    refundAmount: formatRefundAmount(event.data),
    originalChargeDate: fmtDate(lic.createdAt),
    licenseKeyPrefix: lic.key.slice(0, 5),
    legalEntityAddress: legalEntityAddress(),
  });
  const delivered = await sendEmail({ to: recipient, ...rendered });
  if (!delivered) {
    throw new Error(
      `[polar] refund email delivery failed for order ${event.data.id}`,
    );
  }
}

export async function dispatchPolarEvent(
  event: PolarWebhookEvent,
): Promise<void> {
  switch (event.type) {
    case "order.paid":
      return handleOrderPaid(event);
    case "order.created":
      return; // before payment, wait for order.paid
    case "order.refunded":
      return handleOrderRefunded(event);
    case "subscription.canceled":
      return handleSubscriptionCanceled(event);
    case "subscription.uncanceled":
      return handleSubscriptionUncanceled(event);
    case "subscription.past_due":
      return handleSubscriptionPastDue(event);
    case "subscription.active":
    case "subscription.updated":
      return handleSubscriptionUpdated(event);
    case "subscription.revoked":
      return handleSubscriptionRevoked(event);
    default:
      // Polar sends more event types than we consume; ignoring them is
      // expected, not an error.
      return;
  }
}
