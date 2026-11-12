import test, { before, after } from "node:test";
import assert from "node:assert/strict";
import crypto from "node:crypto";

const HAVE_DB = Boolean(process.env.DATABASE_URL);
const skip = HAVE_DB ? false : "DATABASE_URL is not set; no database to test against";

// Imported lazily so the module graph - and the pg Pool inside it - is never
// constructed on a machine that has no database.
type Mod = {
  db: typeof import("@vethida/db").db;
  license: typeof import("@vethida/db").license;
  device: typeof import("@vethida/db").device;
  user: typeof import("@vethida/db").user;
  webhookEvent: typeof import("@vethida/db").webhookEvent;
  adminAuditLog: typeof import("@vethida/db").adminAuditLog;
  eq: typeof import("drizzle-orm").eq;
  and: typeof import("drizzle-orm").and;
};
let m: Mod;
let handlers: typeof import("./webhook");
let dedup: typeof import("./dedup");

const OWNER = `test-user-${crypto.randomUUID()}`;
const made: { licenses: string[]; events: string[] } = { licenses: [], events: [] };

before(async () => {
  if (!HAVE_DB) return;
  const dbMod = await import("@vethida/db");
  const orm = await import("drizzle-orm");
  m = { ...dbMod, eq: orm.eq, and: orm.and } as unknown as Mod;
  handlers = await import("./webhook");
  dedup = await import("./dedup");

  await m.db.insert(m.user).values({
    id: OWNER,
    name: "Licensing Test",
    email: `${OWNER}@example.invalid`,
    emailVerified: true,
  });
});

after(async () => {
  if (!HAVE_DB || !m) return;
  for (const id of made.licenses) {
    await m.db.delete(m.license).where(m.eq(m.license.id, id));
  }
  for (const id of made.events) {
    await m.db.delete(m.webhookEvent).where(m.eq(m.webhookEvent.eventId, id));
  }
  await m.db.delete(m.adminAuditLog).where(m.eq(m.adminAuditLog.targetId, OWNER));
  await m.db.delete(m.user).where(m.eq(m.user.id, OWNER));
});

const days = (n: number) => new Date(Date.now() + n * 86_400_000);

function orderEvent(over: Record<string, unknown> = {}) {
  const id = `ord_${crypto.randomUUID()}`;
  return {
    type: "order.paid",
    data: {
      id,
      customer: { id: `cus_${crypto.randomUUID()}`, email: `${OWNER}@example.invalid` },
      metadata: { userId: OWNER, tier: "ultimate", cadence: "lifetime" },
      ...over,
    },
  } as Parameters<typeof handlers.handleOrderPaid>[0];
}

/** Every license this account currently holds, newest first. */
async function ownedLicenses() {
  const rows = await m.db
    .select()
    .from(m.license)
    .where(m.eq(m.license.userId, OWNER));
  for (const r of rows) if (!made.licenses.includes(r.id)) made.licenses.push(r.id);
  return rows;
}

test("a lifetime order issues one license with no expiry and no subscription", { skip }, async () => {
  const event = orderEvent();
  await handlers.handleOrderPaid(event);

  const rows = (await ownedLicenses()).filter((r) => r.polarOrderId === event.data.id);
  assert.equal(rows.length, 1, "expected exactly one license for the order");
  const [lic] = rows;
  assert.equal(lic.tier, "ultimate");
  assert.equal(lic.expiresAt, null, "a lifetime license must have no expiry");
  assert.equal(lic.polarSubscriptionId, null, "a lifetime purchase must not create a subscription");
  assert.equal(lic.billingInterval, null, "a one-time purchase has no interval");
  assert.equal(lic.cancelAtPeriodEnd, false);
  assert.equal(lic.trial, false);
});

test("duplicate delivery of the same order issues exactly one license", { skip }, async () => {
  const event = orderEvent();

  // Three deliveries of the identical order, as Polar would retry it.
  await handlers.handleOrderPaid(event);
  await handlers.handleOrderPaid(event);
  await handlers.handleOrderPaid(event);

  const rows = (await ownedLicenses()).filter((r) => r.polarOrderId === event.data.id);
  assert.equal(rows.length, 1, `redelivery minted ${rows.length} licenses; the partial unique index on polar_order_id is what must prevent this`);
});

test("the delivery ledger admits an event id once", { skip }, async () => {
  const eventId = `msg_${crypto.randomUUID()}`;
  made.events.push(eventId);
  const key = { provider: "polar", eventId, eventType: "order.paid" };

  assert.equal(await dedup.tryRecordDelivery(key), true, "first delivery was refused");
  assert.equal(await dedup.tryRecordDelivery(key), false, "second delivery was admitted");
  assert.equal(await dedup.tryRecordDelivery(key), false, "third delivery was admitted");

  // A handler that throws must release the slot, or the retry is dropped.
  await dedup.clearDelivery({ provider: "polar", eventId });
  assert.equal(await dedup.tryRecordDelivery(key), true, "a cleared slot was not reusable");
});

test("issuing a license writes a system audit row", { skip }, async () => {
  const event = orderEvent();
  await handlers.handleOrderPaid(event);
  const [lic] = (await ownedLicenses()).filter((r) => r.polarOrderId === event.data.id);

  const rows = await m.db
    .select()
    .from(m.adminAuditLog)
    .where(m.eq(m.adminAuditLog.targetId, lic.id));
  made.licenses.push(lic.id);

  const issued = rows.filter((r) => r.action === "license_issued");
  assert.equal(issued.length, 1, "expected exactly one license_issued row");
  assert.equal(issued[0].actor, "system");
  assert.equal(issued[0].adminId, null, "a webhook has no admin behind it");
  assert.equal(issued[0].ipAddress, null);
});

test("a redelivered order does not write a second issuance row", { skip }, async () => {
  const event = orderEvent();
  await handlers.handleOrderPaid(event);
  await handlers.handleOrderPaid(event);
  const [lic] = (await ownedLicenses()).filter((r) => r.polarOrderId === event.data.id);

  const rows = await m.db
    .select()
    .from(m.adminAuditLog)
    .where(m.eq(m.adminAuditLog.targetId, lic.id));
  const issued = rows.filter((r) => r.action === "license_issued");
  assert.equal(issued.length, 1, "redelivery claimed a second issuance");
});

test("a refund revokes the license and records why", { skip }, async () => {
  const event = orderEvent();
  await handlers.handleOrderPaid(event);

  await handlers.handleOrderRefunded({
    type: "order.refunded",
    data: { id: event.data.id, refunded_amount: 15999, currency: "usd" },
  } as Parameters<typeof handlers.handleOrderRefunded>[0]);

  const [lic] = (await ownedLicenses()).filter((r) => r.polarOrderId === event.data.id);
  assert.ok(lic.revokedAt, "a refunded license must be revoked");
  assert.equal(lic.revokedReasonCode, "user_request");
  assert.ok(lic.tokenGeneration > 0, "revocation must bump token_generation");
});

test("a subscription order records the interval and an expiry", { skip }, async () => {
  const event = orderEvent({
    subscription_id: `sub_${crypto.randomUUID()}`,
    current_period_end: days(30).toISOString(),
    metadata: { userId: OWNER, tier: "pro", cadence: "monthly" },
  });
  await handlers.handleOrderPaid(event);

  const [lic] = (await ownedLicenses()).filter((r) => r.polarOrderId === event.data.id);
  assert.equal(lic.tier, "pro");
  assert.equal(lic.billingInterval, "month");
  assert.ok(lic.expiresAt, "a subscription license must carry an expiry");
  assert.ok(lic.polarSubscriptionId, "a subscription license must record its subscription");
});

test("canceling marks the term as ending without revoking it", { skip }, async () => {
  const subscriptionId = `sub_${crypto.randomUUID()}`;
  const event = orderEvent({
    subscription_id: subscriptionId,
    current_period_end: days(30).toISOString(),
    metadata: { userId: OWNER, tier: "pro", cadence: "monthly" },
  });
  await handlers.handleOrderPaid(event);

  await handlers.handleSubscriptionCanceled({
    type: "subscription.canceled",
    data: { id: subscriptionId, current_period_end: days(30).toISOString() },
  } as Parameters<typeof handlers.handleSubscriptionCanceled>[0]);

  const [lic] = (await ownedLicenses()).filter((r) => r.polarSubscriptionId === subscriptionId);
  assert.equal(lic.cancelAtPeriodEnd, true, "cancel must set cancel_at_period_end");
  assert.equal(lic.revokedAt, null, "canceling is not revoking; the paid term runs on");
  assert.ok(lic.expiresAt && lic.expiresAt > new Date(), "access must continue to the period end");
});

test("a lapse expires the license and kills outstanding tokens", { skip }, async () => {
  const subscriptionId = `sub_${crypto.randomUUID()}`;
  await handlers.handleOrderPaid(
    orderEvent({
      subscription_id: subscriptionId,
      current_period_end: days(30).toISOString(),
      metadata: { userId: OWNER, tier: "pro", cadence: "monthly" },
    }),
  );

  await handlers.handleSubscriptionRevoked({
    type: "subscription.revoked",
    data: { id: subscriptionId, ends_at: new Date().toISOString() },
  } as Parameters<typeof handlers.handleSubscriptionRevoked>[0]);

  const [lic] = (await ownedLicenses()).filter((r) => r.polarSubscriptionId === subscriptionId);
  assert.ok(lic.expiresAt && lic.expiresAt <= new Date(), "a lapsed license must be expired");
  assert.equal(lic.revokedAt, null, "a lapse is not a revocation");
  assert.ok(lic.tokenGeneration > 0, "a lapse must bump token_generation");

  const audit = await m.db
    .select()
    .from(m.adminAuditLog)
    .where(m.eq(m.adminAuditLog.targetId, lic.id));
  assert.ok(audit.some((r) => r.action === "license_lapsed"), "a lapse must be audited");
});

test("a stale update arriving after a lapse cannot resurrect the license", { skip }, async () => {
  // The out-of-order case: a correctly signed event passes signature verification and the
  // delivery ledger, and arrives after one generated later.
  const subscriptionId = `sub_${crypto.randomUUID()}`;
  const periodEnd = days(30);
  await handlers.handleOrderPaid(
    orderEvent({
      subscription_id: subscriptionId,
      current_period_end: periodEnd.toISOString(),
      metadata: { userId: OWNER, tier: "pro", cadence: "monthly" },
    }),
  );

  await handlers.handleSubscriptionRevoked({
    type: "subscription.revoked",
    data: { id: subscriptionId, ends_at: new Date().toISOString() },
  } as Parameters<typeof handlers.handleSubscriptionRevoked>[0]);

  // Generated BEFORE the revoke, delivered after it: still renewing, term
  // running to the original period end.
  await handlers.handleSubscriptionUpdated({
    type: "subscription.updated",
    data: {
      id: subscriptionId,
      current_period_end: periodEnd.toISOString(),
      cancel_at_period_end: false,
    },
  } as Parameters<typeof handlers.handleSubscriptionUpdated>[0]);

  const [lic] = (await ownedLicenses()).filter((r) => r.polarSubscriptionId === subscriptionId);
  assert.ok(
    lic.expiresAt && lic.expiresAt <= new Date(),
    "a stale update extended a lapsed term back into the future",
  );
  assert.equal(
    lic.cancelAtPeriodEnd,
    true,
    "a stale update marked a lapsed license as renewing",
  );
});

test("an out-of-order update cannot shorten a term the customer paid for", { skip }, async () => {
  const subscriptionId = `sub_${crypto.randomUUID()}`;
  await handlers.handleOrderPaid(
    orderEvent({
      subscription_id: subscriptionId,
      current_period_end: days(60).toISOString(),
      metadata: { userId: OWNER, tier: "pro", cadence: "monthly" },
    }),
  );

  // An older event, carrying the previous period end.
  await handlers.handleSubscriptionUpdated({
    type: "subscription.updated",
    data: {
      id: subscriptionId,
      current_period_end: days(30).toISOString(),
      cancel_at_period_end: false,
    },
  } as Parameters<typeof handlers.handleSubscriptionUpdated>[0]);

  const [lic] = (await ownedLicenses()).filter((r) => r.polarSubscriptionId === subscriptionId);
  assert.ok(
    lic.expiresAt && lic.expiresAt > days(59),
    `a stale update rewound the expiry to ${lic.expiresAt?.toISOString()}`,
  );
});

test("the first binding is free, a second machine is refused, a move costs a reset", { skip }, async () => {
  const { registerDevice } = await import("../licenses/antisharing");
  const event = orderEvent();
  await handlers.handleOrderPaid(event);
  const [lic] = (await ownedLicenses()).filter((r) => r.polarOrderId === event.data.id);

  // use the real source names, made-up keys get dropped by pepperFingerprint
  const { FINGERPRINT_SOURCES } = await import("../licenses/fingerprint");
  const machine = (seed: string) => ({
    devicePublicKey: crypto.createHash("sha256").update(seed).digest(),
    hardwareFingerprint: Object.fromEntries(
      FINGERPRINT_SOURCES.map((k) => [
        k,
        crypto.createHash("sha256").update(`${seed}:${k}`).digest("hex"),
      ]),
    ),
  });

  // First activation on a fresh license: no reset spent.
  const first = await registerDevice({
    licenseId: lic.id,
    userId: OWNER,
    ...machine("machine-one"),
    ip: "203.0.113.10",
  });
  assert.equal(first.ok, true, "the first activation was refused");
  assert.equal(first.ok && first.resetsRemaining, 3, "the first binding must be free");

  // A second machine while the first is still active: one machine per license.
  const second = await registerDevice({
    licenseId: lic.id,
    userId: OWNER,
    ...machine("machine-two"),
    ip: "203.0.113.11",
  });
  assert.equal(second.ok, false, "a second concurrent machine was allowed");
  assert.equal(second.ok === false && second.reason, "machine_limit_reached");

  // Re-activating the SAME install on the same hardware is free and idempotent.
  const again = await registerDevice({
    licenseId: lic.id,
    userId: OWNER,
    ...machine("machine-one"),
    ip: "203.0.113.10",
  });
  assert.equal(again.ok, true);
  assert.equal(again.ok && again.resetsRemaining, 3, "a same-hardware reactivation must not cost a reset");

  // Free the slot, then move to different hardware: that costs one reset.
  const { deactivateDevice, derivePubkeyId } = await import("../licenses/antisharing");
  await deactivateDevice(lic.id, derivePubkeyId(machine("machine-one").devicePublicKey));

  const moved = await registerDevice({
    licenseId: lic.id,
    userId: OWNER,
    ...machine("machine-two"),
    ip: "203.0.113.11",
  });
  assert.equal(moved.ok, true, "the move was refused");
  assert.equal(moved.ok && moved.resetsRemaining, 2, "a machine move must consume exactly one reset");

  const [after] = (await ownedLicenses()).filter((r) => r.id === lic.id);
  assert.equal(after.machineResetsRemaining, 2, "the quota must be decremented on the license row");
});

test("a revoked license cannot be activated", { skip }, async () => {
  const { registerDevice } = await import("../licenses/antisharing");
  const { FINGERPRINT_SOURCES } = await import("../licenses/fingerprint");
  const event = orderEvent();
  await handlers.handleOrderPaid(event);
  await handlers.handleOrderRefunded({
    type: "order.refunded",
    data: { id: event.data.id },
  } as Parameters<typeof handlers.handleOrderRefunded>[0]);
  const [lic] = (await ownedLicenses()).filter((r) => r.polarOrderId === event.data.id);

  const got = await registerDevice({
    licenseId: lic.id,
    userId: OWNER,
    devicePublicKey: crypto.createHash("sha256").update("revoked-machine").digest(),
    hardwareFingerprint: Object.fromEntries(
      FINGERPRINT_SOURCES.map((k) => [k, "x"]),
    ),
    ip: "203.0.113.20",
  });
  assert.equal(got.ok, false, "a refunded license activated");
  assert.equal(got.ok === false && got.reason, "license_revoked");
});

test("a fingerprint too thin to re-verify is refused rather than bound", { skip }, async () => {
  const { registerDevice } = await import("../licenses/antisharing");
  const { FINGERPRINT_SOURCES } = await import("../licenses/fingerprint");
  const event = orderEvent();
  await handlers.handleOrderPaid(event);
  const [lic] = (await ownedLicenses()).filter((r) => r.polarOrderId === event.data.id);

  const attempt = (hardwareFingerprint: unknown, seed: string) =>
    registerDevice({
      licenseId: lic.id,
      userId: OWNER,
      devicePublicKey: crypto.createHash("sha256").update(seed).digest(),
      hardwareFingerprint,
      ip: "203.0.113.30",
    });

  // An object with no recognized source is what a client sending well-formed
  // nonsense produces, and what the routes' type check lets through.
  const empty = await attempt({}, "thin-empty");
  assert.equal(empty.ok === false && empty.reason, "fingerprint_unusable");

  const wrongNames = await attempt(
    { cpu: "x", board: "x", disk: "x", mac: "x", bios: "x", gpu: "x" },
    "thin-names",
  );
  assert.equal(
    wrongNames.ok === false && wrongNames.reason,
    "fingerprint_unusable",
    "unrecognized key names must not bind as an empty vector",
  );

  // Two sources cannot reach the comparison's accept or flag thresholds, so a
  // binding made from them could never be re-verified.
  const two = await attempt(
    Object.fromEntries(FINGERPRINT_SOURCES.slice(0, 2).map((k) => [k, "x"])),
    "thin-two",
  );
  assert.equal(two.ok === false && two.reason, "fingerprint_unusable");

  // three is the minimum and still activates
  const three = await attempt(
    Object.fromEntries(FINGERPRINT_SOURCES.slice(0, 3).map((k) => [k, "x"])),
    "thin-three",
  );
  assert.equal(three.ok, true, "three recognized sources must be bindable");

  // Nothing was bound by the three refusals, so the slot is still free.
  const devices = await m.db
    .select()
    .from(m.device)
    .where(m.eq(m.device.licenseId, lic.id));
  assert.equal(devices.length, 1, "a refused activation must not leave a device row");
});

test("uncanceling a live subscription clears the flag", { skip }, async () => {
  const subscriptionId = `sub_${crypto.randomUUID()}`;
  await handlers.handleOrderPaid(
    orderEvent({
      subscription_id: subscriptionId,
      current_period_end: days(30).toISOString(),
      metadata: { userId: OWNER, tier: "pro", cadence: "monthly" },
    }),
  );
  await handlers.handleSubscriptionCanceled({
    type: "subscription.canceled",
    data: { id: subscriptionId, current_period_end: days(30).toISOString() },
  } as Parameters<typeof handlers.handleSubscriptionCanceled>[0]);

  await handlers.handleSubscriptionUncanceled({
    type: "subscription.uncanceled",
    data: { id: subscriptionId, current_period_end: days(30).toISOString() },
  } as Parameters<typeof handlers.handleSubscriptionUncanceled>[0]);

  const [lic] = (await ownedLicenses()).filter((r) => r.polarSubscriptionId === subscriptionId);
  assert.equal(lic.cancelAtPeriodEnd, false, "changing your mind must restore renewal");
});

// --- payment failure
// one notice per past_due episode, entitlement untouched until revoked, and the
// final notice only when the lapse came from a failed payment

test("a past-due renewal notifies once and leaves entitlement alone", { skip }, async () => {
  const subscriptionId = `sub_${crypto.randomUUID()}`;
  await handlers.handleOrderPaid(
    orderEvent({
      subscription_id: subscriptionId,
      current_period_end: days(30).toISOString(),
      metadata: { userId: OWNER, tier: "ultimate", cadence: "monthly" },
    }),
  );

  const pastDue = {
    type: "subscription.past_due",
    data: { id: subscriptionId, current_period_end: days(30).toISOString() },
  } as Parameters<typeof handlers.handleSubscriptionPastDue>[0];

  await handlers.handleSubscriptionPastDue(pastDue);
  const [first] = (await ownedLicenses()).filter((r) => r.polarSubscriptionId === subscriptionId);
  assert.ok(first.pastDueNoticeSentAt, "the first past-due event must stamp the notice");
  assert.equal(first.revokedAt, null, "Polar is still retrying; the license must not be revoked");
  assert.equal(
    first.cancelAtPeriodEnd,
    false,
    "past due is not a cancellation; renewal is still expected to happen",
  );
  const stampedAt = first.pastDueNoticeSentAt;

  // Redelivery. Polar retries webhooks, and the customer does not need the same
  // email twice for one declined card.
  await handlers.handleSubscriptionPastDue(pastDue);
  const [second] = (await ownedLicenses()).filter((r) => r.polarSubscriptionId === subscriptionId);
  assert.deepEqual(
    second.pastDueNoticeSentAt,
    stampedAt,
    "a redelivered past-due event must not re-notify",
  );
});

test("recovering from past due clears the stamp so next term can notify", { skip }, async () => {
  const subscriptionId = `sub_${crypto.randomUUID()}`;
  await handlers.handleOrderPaid(
    orderEvent({
      subscription_id: subscriptionId,
      current_period_end: days(30).toISOString(),
      metadata: { userId: OWNER, tier: "pro", cadence: "monthly" },
    }),
  );
  await handlers.handleSubscriptionPastDue({
    type: "subscription.past_due",
    data: { id: subscriptionId, current_period_end: days(30).toISOString() },
  } as Parameters<typeof handlers.handleSubscriptionPastDue>[0]);

  await handlers.handleSubscriptionUpdated({
    type: "subscription.active",
    data: { id: subscriptionId, current_period_end: days(60).toISOString() },
  } as Parameters<typeof handlers.handleSubscriptionUpdated>[0]);

  const [lic] = (await ownedLicenses()).filter((r) => r.polarSubscriptionId === subscriptionId);
  assert.equal(
    lic.pastDueNoticeSentAt,
    null,
    "a card that succeeds on retry must leave the next failure able to notify",
  );
});

test("a lapse after a payment failure is told apart from a deliberate cancel", { skip }, async () => {
  const failed = `sub_${crypto.randomUUID()}`;
  const quit = `sub_${crypto.randomUUID()}`;
  for (const [id, tier] of [
    [failed, "ultimate"],
    [quit, "pro"],
  ] as const) {
    await handlers.handleOrderPaid(
      orderEvent({
        subscription_id: id,
        current_period_end: days(30).toISOString(),
        metadata: { userId: OWNER, tier, cadence: "monthly" },
      }),
    );
  }

  await handlers.handleSubscriptionPastDue({
    type: "subscription.past_due",
    data: { id: failed, current_period_end: days(30).toISOString() },
  } as Parameters<typeof handlers.handleSubscriptionPastDue>[0]);

  for (const id of [failed, quit]) {
    await handlers.handleSubscriptionRevoked({
      type: "subscription.revoked",
      data: { id, ends_at: new Date().toISOString() },
    } as Parameters<typeof handlers.handleSubscriptionRevoked>[0]);
  }

  const rows = await ownedLicenses();
  for (const id of [failed, quit]) {
    const [lic] = rows.filter((r) => r.polarSubscriptionId === id);
    assert.ok(lic.expiresAt && lic.expiresAt <= new Date(), `${id} must have lapsed`);
  }
  // the stamp has to survive the revoke, the handler reads it from RETURNING
  assert.ok(
    rows.filter((r) => r.polarSubscriptionId === failed)[0].pastDueNoticeSentAt,
    "the past-due stamp must outlive the revoke that reads it",
  );

  // The audit trail is where the distinction is recorded for a human.
  const audit = await m.db
    .select()
    .from(m.adminAuditLog)
    .where(m.eq(m.adminAuditLog.action, "license_lapsed"));
  const reasons = new Map(
    audit
      .filter((r) => [failed, quit].includes((r.details as { subscriptionId?: string })?.subscriptionId ?? ""))
      .map((r) => [
        (r.details as { subscriptionId?: string }).subscriptionId,
        (r.details as { reasonCode?: string }).reasonCode,
      ]),
  );
  assert.equal(reasons.get(failed), "payment_failed", "a lapse after past due is a payment failure");
  assert.equal(reasons.get(quit), "canceled", "a lapse with no past-due history is a cancellation");
});
