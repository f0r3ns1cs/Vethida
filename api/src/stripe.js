import Stripe from "stripe";

import { openDatabase } from "./db.js";
import { sendLicenseEmail } from "./email.js";
import {
  createLicense,
  findByPayment,
  findBySession,
  findBySubscription,
  setStatus,
} from "./licenses.js";

const GRACE_MS = 3 * 24 * 60 * 60 * 1000;

let client;
export function stripe() {
  if (!client) {
    const key = process.env.STRIPE_SECRET_KEY;
    if (!key) throw new Error("STRIPE_SECRET_KEY is not set");
    client = new Stripe(key);
  }
  return client;
}

export function constructEvent(rawBody, signature) {
  const secret = process.env.STRIPE_WEBHOOK_SECRET;
  if (!secret) throw new Error("STRIPE_WEBHOOK_SECRET is not set");
  return stripe().webhooks.constructEvent(rawBody, signature, secret);
}

const id = (v) => (typeof v === "string" ? v : v?.id ?? null);

// Newer API versions moved the period onto the subscription item.
function periodEnd(subscription) {
  const end =
    subscription.items?.data?.[0]?.current_period_end ?? subscription.current_period_end;
  return end ? new Date(end * 1000 + GRACE_MS).toISOString() : null;
}

function statusOf(subscription) {
  // past_due: Stripe is still retrying the card. The period end plus grace
  // bounds how long that lasts.
  return ["active", "trialing", "past_due"].includes(subscription.status) ? "active" : "expired";
}

function tierOf(price) {
  const tier = price?.metadata?.tier ?? price?.product?.metadata?.tier;
  return tier === "pro" || tier === "ultimate" ? tier : null;
}

function planOf(price) {
  const interval = price?.recurring?.interval;
  if (!interval) return "lifetime";
  return interval === "year" ? "year" : "month";
}

// Creates the license for a completed checkout, once. Called from the webhook
// and from the success page, whichever comes first; the session id makes the
// second call a lookup.
export async function fulfillSession(sessionOrId) {
  const sessionId = id(sessionOrId);
  const existing = findBySession(sessionId);
  if (existing) return { license: existing, created: false };

  const session = await stripe().checkout.sessions.retrieve(sessionId, {
    expand: ["line_items.data.price.product"],
  });
  if (session.payment_status !== "paid" && session.payment_status !== "no_payment_required") {
    return { license: null, created: false, pending: true };
  }

  const price = session.line_items?.data?.[0]?.price;
  const tier = tierOf(price);
  if (!tier) {
    throw new Error(`checkout ${sessionId}: price ${price?.id} has no tier metadata`);
  }
  const email = session.customer_details?.email ?? session.customer_email;
  if (!email) throw new Error(`checkout ${sessionId}: no customer email`);

  let expiresAt = null;
  const subscriptionId = id(session.subscription);
  if (subscriptionId) {
    const subscription = await stripe().subscriptions.retrieve(subscriptionId);
    expiresAt = periodEnd(subscription);
  }

  let created;
  try {
    created = createLicense({
      tier,
      plan: planOf(price),
      email,
      expiresAt,
      customer: id(session.customer),
      subscription: subscriptionId,
      session: sessionId,
      payment: id(session.payment_intent),
    });
  } catch (err) {
    // The webhook and the success page raced, and the other one won.
    const winner = findBySession(sessionId);
    if (winner) return { license: winner, created: false };
    throw err;
  }

  await sendLicenseEmail({ to: email, key: created.key, tier, plan: planOf(price) });
  return { license: created.license, key: created.key, created: true };
}

async function syncSubscription(subscriptionId) {
  const subscription = await stripe().subscriptions.retrieve(subscriptionId);
  const license = findBySubscription(subscription.id);
  if (!license || license.status === "disabled") return;
  // While Stripe retries a failed renewal (past_due), the plan stays on, as the
  // Subscription Terms promise. Access is extended a week at a time, so if the
  // final "canceled" event never arrives the key still lapses and the next
  // check asks Stripe again (refreshIfLapsed).
  const expiresAt =
    subscription.status === "past_due"
      ? new Date(Date.now() + 7 * 24 * 60 * 60 * 1000).toISOString()
      : periodEnd(subscription);
  setStatus(license.id, statusOf(subscription), expiresAt);
}

// A subscription whose paid period has run out on our side is checked against
// Stripe before the app is told it expired: a renewal webhook may have been
// missed. At most once an hour per license.
const lastRefresh = new Map();
export async function refreshIfLapsed(license) {
  if (!license?.stripe_subscription || license.status !== "active") return license;
  if (!license.expires_at || Date.parse(license.expires_at) > Date.now()) return license;
  const last = lastRefresh.get(license.id) ?? 0;
  if (Date.now() - last < 60 * 60 * 1000) return license;
  lastRefresh.set(license.id, Date.now());
  try {
    await syncSubscription(license.stripe_subscription);
  } catch (err) {
    console.error("subscription refresh failed", license.id, err.message);
  }
  return findBySubscription(license.stripe_subscription) ?? license;
}

function alreadyHandled(event) {
  const d = openDatabase();
  const seen = d.prepare("SELECT 1 FROM stripe_events WHERE id = ?").get(event.id);
  return Boolean(seen);
}

function markHandled(event) {
  openDatabase()
    .prepare("INSERT OR IGNORE INTO stripe_events (id, type) VALUES (?, ?)")
    .run(event.id, event.type);
}

// Lifetime purchases are found by their payment; subscription payments by the
// invoice the charge belongs to, where Stripe includes it.
async function licenseForCharge(charge) {
  const license = findByPayment(id(charge.payment_intent));
  if (license) return license;
  const invoiceId = id(charge.invoice);
  if (!invoiceId) return null;
  const invoice = await stripe().invoices.retrieve(invoiceId);
  const subscriptionId =
    id(invoice.subscription) ?? id(invoice.parent?.subscription_details?.subscription);
  return subscriptionId ? findBySubscription(subscriptionId) : null;
}

// A full refund or a dispute ends the license. A partial refund does not. If
// the charge matches no license, cancel the subscription in Stripe (that
// expires the license) or run `npm run admin -- revoke <key>`.
async function disableForCharge(charge) {
  const license = await licenseForCharge(charge);
  if (license) setStatus(license.id, "disabled");
  else console.warn("refund or dispute matched no license:", charge.id);
}

// The Refund Policy suspends a license only while a dispute is investigated, so
// a dispute decided in our favor gives it back. A lost dispute leaves it
// disabled, and so does a charge that was refunded in the meantime.
async function restoreAfterDispute(dispute) {
  if (dispute.status !== "won") return;
  const charge = await stripe().charges.retrieve(id(dispute.charge));
  if (charge.refunded) return;
  const license = await licenseForCharge(charge);
  if (!license || license.status !== "disabled") return;
  setStatus(license.id, "active");
  if (license.stripe_subscription) await syncSubscription(license.stripe_subscription);
}

export async function handleEvent(event) {
  if (alreadyHandled(event)) return;
  const object = event.data.object;

  switch (event.type) {
    case "checkout.session.completed":
    case "checkout.session.async_payment_succeeded":
      await fulfillSession(object.id);
      break;

    case "customer.subscription.created":
    case "customer.subscription.updated":
    case "customer.subscription.deleted":
    case "customer.subscription.paused":
    case "customer.subscription.resumed":
      // By id rather than from the payload: Stripe does not deliver events in
      // order, and an older update arriving last must not overwrite a newer one.
      await syncSubscription(object.id);
      break;

    case "invoice.paid": {
      const subscriptionId =
        id(object.subscription) ?? id(object.parent?.subscription_details?.subscription);
      if (subscriptionId) await syncSubscription(subscriptionId);
      break;
    }

    case "charge.refunded":
      if (object.refunded) await disableForCharge(object);
      break;

    case "charge.dispute.created": {
      const charge = await stripe().charges.retrieve(id(object.charge));
      await disableForCharge(charge);
      break;
    }

    case "charge.dispute.closed":
      await restoreAfterDispute(object);
      break;

    default:
      break;
  }
  markHandled(event);
}
