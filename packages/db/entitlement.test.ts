import test from "node:test";
import assert from "node:assert/strict";
import { isLicensed, licenseIsLive, resolveEntitlement } from "./entitlement";
import type { EntitlementInput } from "./entitlement";

const FREE = { tier: "free", trial: false, expiresAt: null } as const;

// fixed clock
const NOW = new Date("2026-06-01T12:00:00.000Z");
const days = (n: number) => new Date(NOW.getTime() + n * 86_400_000);

function lic(over: Partial<EntitlementInput> = {}): EntitlementInput {
  return {
    tier: "pro",
    trial: false,
    expiresAt: days(30),
    revokedAt: null,
    ...over,
  };
}

const STATES = {
  pro: lic({ tier: "pro" }),
  ultimate: lic({ tier: "ultimate" }),
  lifetimePro: lic({ tier: "pro", expiresAt: null }),
  lifetimeUltimate: lic({ tier: "ultimate", expiresAt: null }),
  lapsed: lic({ tier: "ultimate", expiresAt: days(-1) }),
  refunded: lic({ tier: "ultimate", revokedAt: days(-5) }),
  revoked: lic({ tier: "ultimate", expiresAt: null, revokedAt: days(-5) }),
  chargeback: lic({ tier: "pro", expiresAt: days(300), revokedAt: days(-2) }),
};

test("a live subscription grants its own tier", () => {
  assert.deepEqual(resolveEntitlement([STATES.pro], NOW), {
    tier: "pro",
    trial: false,
    expiresAt: days(30),
  });
  assert.equal(resolveEntitlement([STATES.ultimate], NOW)?.tier, "ultimate");
});

test("both lifetime SKUs grant their tier with no expiry", () => {
  assert.deepEqual(resolveEntitlement([STATES.lifetimePro], NOW), {
    tier: "pro",
    trial: false,
    expiresAt: null,
  });
  assert.deepEqual(resolveEntitlement([STATES.lifetimeUltimate], NOW), {
    tier: "ultimate",
    trial: false,
    expiresAt: null,
  });
});

test("a lifetime license never lapses, however far the clock moves", () => {
  const farFuture = new Date("2099-01-01T00:00:00.000Z");
  assert.equal(
    resolveEntitlement([STATES.lifetimeUltimate], farFuture)?.tier,
    "ultimate",
  );
  assert.deepEqual(resolveEntitlement([STATES.ultimate], farFuture), FREE);
});

test("lapsed, refunded and revoked all drop to Free, never to nothing", () => {
  for (const [name, row] of [
    ["lapsed", STATES.lapsed],
    ["refunded", STATES.refunded],
    ["revoked", STATES.revoked],
    ["chargeback", STATES.chargeback],
  ] as const) {
    const got = resolveEntitlement([row], NOW);
    assert.deepEqual(got, FREE, `${name} did not resolve to Free`);
    assert.equal(isLicensed(got), false, `${name} counted as licensed`);
    assert.equal(licenseIsLive(row, NOW), false, `${name} counted as live`);
  }
});

test("a revoked lifetime license drops to Free despite having no expiry", () => {
  assert.deepEqual(resolveEntitlement([STATES.revoked], NOW), FREE);
});

test("a live license is licensed and Free is not", () => {
  assert.equal(isLicensed(resolveEntitlement([STATES.pro], NOW)), true);
  assert.equal(isLicensed(resolveEntitlement([], NOW)), false);
});

test("expiry is exclusive at the boundary", () => {
  const exact = lic({ expiresAt: NOW });
  assert.equal(licenseIsLive(exact, NOW), false, "expiring now should be dead");
  assert.equal(licenseIsLive(lic({ expiresAt: days(1) }), NOW), true);
});

test("the highest live tier wins", () => {
  assert.equal(
    resolveEntitlement([STATES.pro, STATES.ultimate], NOW)?.tier,
    "ultimate",
  );
  // order shouldn't matter
  assert.equal(
    resolveEntitlement([STATES.ultimate, STATES.pro], NOW)?.tier,
    "ultimate",
  );
});

test("a dead Ultimate does not outrank a live Pro", () => {
  const rows = [STATES.pro, STATES.lapsed, STATES.refunded];
  assert.deepEqual(resolveEntitlement(rows, NOW), {
    tier: "pro",
    trial: false,
    expiresAt: days(30),
  });
});

test("a paid license beats a trial of the same tier", () => {
  const trial = lic({ tier: "ultimate", trial: true, expiresAt: days(3) });
  const paid = lic({ tier: "ultimate", expiresAt: days(365) });
  for (const rows of [
    [trial, paid],
    [paid, trial],
  ]) {
    const got = resolveEntitlement(rows, NOW);
    assert.equal(got?.trial, false, "trial won over a paid license");
    assert.deepEqual(got?.expiresAt, days(365));
  }
});

test("a trial alone still grants Ultimate, flagged as a trial", () => {
  const trial = lic({ tier: "ultimate", trial: true, expiresAt: days(3) });
  assert.deepEqual(resolveEntitlement([trial], NOW), {
    tier: "ultimate",
    trial: true,
    expiresAt: days(3),
  });
});

test("perpetual outranks dated at the same tier", () => {
  const dated = lic({ tier: "ultimate", expiresAt: days(10) });
  for (const rows of [
    [dated, STATES.lifetimeUltimate],
    [STATES.lifetimeUltimate, dated],
  ]) {
    assert.equal(
      resolveEntitlement(rows, NOW)?.expiresAt,
      null,
      "a dated license beat a perpetual one",
    );
  }
});

test("between two dated licenses the later expiry wins", () => {
  // e.g. renewed early and holds both terms
  const soon = lic({ expiresAt: days(5) });
  const later = lic({ expiresAt: days(400) });
  for (const rows of [
    [soon, later],
    [later, soon],
  ]) {
    assert.deepEqual(resolveEntitlement(rows, NOW)?.expiresAt, days(400));
  }
});

test("an unrecognized tier grants nothing above Free and cannot win", () => {
  const bogus = lic({ tier: "elite", expiresAt: null });
  assert.deepEqual(resolveEntitlement([bogus], NOW), FREE);
  assert.equal(resolveEntitlement([bogus, STATES.pro], NOW).tier, "pro");
});

test("licenseIsLive fails closed on a tier we do not sell", () => {
  for (const tier of ["elite", "free", "", "PRO"]) {
    assert.equal(
      licenseIsLive(lic({ tier, expiresAt: null }), NOW),
      false,
      `${tier || "(empty)"} counted as live`,
    );
  }
  assert.equal(licenseIsLive(lic({ tier: "pro", expiresAt: null }), NOW), true);
});

test("no licenses at all is Free", () => {
  assert.deepEqual(resolveEntitlement([], NOW), FREE);
});

test("every dead license together is still Free", () => {
  const rows = [STATES.lapsed, STATES.refunded, STATES.revoked];
  assert.deepEqual(resolveEntitlement(rows, NOW), FREE);
});

test("revocation outranks a still-running term", () => {
  const refundedEarly = lic({ tier: "ultimate", expiresAt: days(300), revokedAt: days(-1) });
  assert.deepEqual(resolveEntitlement([refundedEarly], NOW), FREE);
});

test("nothing resolves to a missing tier", () => {
  const every = [
    [],
    [STATES.pro],
    [STATES.lapsed],
    [STATES.refunded],
    [STATES.revoked],
    [STATES.chargeback],
    [lic({ tier: "elite" })],
    [lic({ tier: "" })],
    Object.values(STATES),
  ];
  for (const rows of every) {
    const got = resolveEntitlement(rows, NOW);
    assert.ok(
      got.tier === "free" || got.tier === "pro" || got.tier === "ultimate",
      `resolved to ${JSON.stringify(got.tier)}`,
    );
  }
});
