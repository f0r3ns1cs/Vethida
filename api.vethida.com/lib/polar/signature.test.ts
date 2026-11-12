import test from "node:test";
import assert from "node:assert/strict";
import crypto from "node:crypto";
import { verifyPolarWebhook } from "./signature";

const SECRET = "whsec_qP4kZmLp2vX9nR7tYcB3wS6dF1gH8jK0";
const BODY = JSON.stringify({ type: "order.paid", data: { id: "ord_123" } });
const ID = "msg_2abcXYZ";

const nowSec = () => Math.floor(Date.now() / 1000);

/** A signature the way Polar produces one: HMAC over id.timestamp.body. */
function sign(body: string, id: string, ts: number, secret = SECRET): string {
  const mac = crypto
    .createHmac("sha256", Buffer.from(secret, "utf8"))
    .update(`${id}.${ts}.${body}`, "utf8")
    .digest("base64");
  return `v1,${mac}`;
}

function headers(over: Partial<Record<"id" | "timestamp" | "signature", string | null>> = {}) {
  const ts = nowSec();
  return {
    id: ID,
    timestamp: String(ts),
    signature: sign(BODY, ID, ts),
    ...over,
  };
}

test("a correctly signed delivery verifies", () => {
  assert.equal(verifyPolarWebhook(BODY, headers(), SECRET), true);
});

test("a tampered body is refused", () => {
  // The signature is over the raw body: re-serializing the parsed object would let a changed
  // amount through.
  const tampered = BODY.replace("ord_123", "ord_666");
  assert.equal(verifyPolarWebhook(tampered, headers(), SECRET), false);
});

test("a signature made with a different secret is refused", () => {
  const ts = nowSec();
  const forged = {
    id: ID,
    timestamp: String(ts),
    signature: sign(BODY, ID, ts, "whsec_attacker"),
  };
  assert.equal(verifyPolarWebhook(BODY, forged, SECRET), false);
});

test("the signed id is bound, so a signature cannot be replayed under another id", () => {
  const ts = nowSec();
  const h = { id: "msg_other", timestamp: String(ts), signature: sign(BODY, ID, ts) };
  assert.equal(verifyPolarWebhook(BODY, h, SECRET), false);
});

test("a stale timestamp is refused", () => {
  const old = nowSec() - 6 * 60;
  const h = { id: ID, timestamp: String(old), signature: sign(BODY, ID, old) };
  assert.equal(verifyPolarWebhook(BODY, h, SECRET), false);
});

test("a timestamp far in the future is refused", () => {
  const ahead = nowSec() + 6 * 60;
  const h = { id: ID, timestamp: String(ahead), signature: sign(BODY, ID, ahead) };
  assert.equal(verifyPolarWebhook(BODY, h, SECRET), false);
});

test("a timestamp inside the tolerance is accepted", () => {
  const recent = nowSec() - 4 * 60;
  const h = { id: ID, timestamp: String(recent), signature: sign(BODY, ID, recent) };
  assert.equal(verifyPolarWebhook(BODY, h, SECRET), true);
});

test("missing headers are refused rather than throwing", () => {
  for (const field of ["id", "timestamp", "signature"] as const) {
    assert.equal(
      verifyPolarWebhook(BODY, headers({ [field]: null }), SECRET),
      false,
      `null ${field} was accepted`,
    );
  }
});

test("a non-numeric timestamp is refused", () => {
  // Number("") is 0, Number("1e9") is valid, neither should get through
  for (const timestamp of ["", "abc", "1e9", "-100", " 123", "12.5"]) {
    assert.equal(
      verifyPolarWebhook(BODY, headers({ timestamp }), SECRET),
      false,
      `timestamp ${JSON.stringify(timestamp)} was accepted`,
    );
  }
});

test("during rotation any valid v1 entry accepts", () => {
  // Polar sends space-separated signatures while a secret is being rotated.
  const ts = nowSec();
  const good = sign(BODY, ID, ts);
  const stale = sign(BODY, ID, ts, "whsec_previous_secret");
  assert.equal(
    verifyPolarWebhook(BODY, { id: ID, timestamp: String(ts), signature: `${stale} ${good}` }, SECRET),
    true,
  );
  assert.equal(
    verifyPolarWebhook(BODY, { id: ID, timestamp: String(ts), signature: `${good} ${stale}` }, SECRET),
    true,
  );
  // Neither of them ours.
  assert.equal(
    verifyPolarWebhook(BODY, { id: ID, timestamp: String(ts), signature: `${stale} ${stale}` }, SECRET),
    false,
  );
});

test("a version other than v1 is not honored", () => {
  const ts = nowSec();
  const mac = sign(BODY, ID, ts).slice(3);
  const h = { id: ID, timestamp: String(ts), signature: `v2,${mac}` };
  assert.equal(verifyPolarWebhook(BODY, h, SECRET), false);
});

test("a truncated signature is refused and does not throw", () => {
  // timingSafeEqual throws on a length mismatch, so without the length guard in front of it
  // this is a 500 rather than a 403.
  const ts = nowSec();
  const short = sign(BODY, ID, ts).slice(0, 20);
  assert.doesNotThrow(() => verifyPolarWebhook(BODY, { id: ID, timestamp: String(ts), signature: short }, SECRET));
  assert.equal(verifyPolarWebhook(BODY, { id: ID, timestamp: String(ts), signature: short }, SECRET), false);
});

test("the whsec_ prefix is part of the key, not something to strip", () => {
  // stripping whsec_ and base64-decoding gives the wrong key
  const ts = nowSec();
  const wrongKey = Buffer.from(SECRET.slice("whsec_".length), "base64");
  const mac = crypto
    .createHmac("sha256", wrongKey)
    .update(`${ID}.${ts}.${BODY}`, "utf8")
    .digest("base64");
  assert.equal(
    verifyPolarWebhook(BODY, { id: ID, timestamp: String(ts), signature: `v1,${mac}` }, SECRET),
    false,
    "the stripped-and-decoded derivation verified, which means the key bytes are wrong",
  );
});
