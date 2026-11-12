import test from "node:test";
import assert from "node:assert/strict";
import crypto from "node:crypto";

const { privateKey } = crypto.generateKeyPairSync("ec", { namedCurve: "prime256v1" });
process.env.LICENSE_SIGNING_KEY = privateKey
  .export({ format: "der", type: "pkcs8" })
  .toString("base64");
process.env.LICENSE_TOKEN_KID = "vethida-license-v1";

const { signLicenseToken, verifyLicenseToken, signingKid, signingPublicKeyRaw } =
  await import("./signing");

const HOUR = 3600_000;

function payload(over: Partial<Parameters<typeof signLicenseToken>[0]> = {}) {
  return {
    keyHash: "a".repeat(64),
    tier: "ultimate",
    devicePubkeyId: "b".repeat(32),
    activatedAt: new Date("2026-06-01T00:00:00.000Z").toISOString(),
    expiresAt: new Date(Date.now() + 7 * 24 * HOUR).toISOString(),
    ...over,
  };
}

/** Rebuilds a token with a replaced header or claim set, signed with `key`. */
function retoken(
  parts: { header?: object; claims?: object },
  token: string,
  key: crypto.KeyObject = privateKey,
): string {
  const [h, p] = token.split(".");
  const dec = (s: string) => JSON.parse(Buffer.from(s, "base64url").toString());
  const b64 = (o: object) => Buffer.from(JSON.stringify(o)).toString("base64url");
  const input = `${b64({ ...dec(h), ...parts.header })}.${b64({ ...dec(p), ...parts.claims })}`;
  const sig = crypto.sign("sha256", Buffer.from(input), { key, dsaEncoding: "ieee-p1363" });
  return `${input}.${sig.toString("base64url")}`;
}

test("a signed token round-trips its payload", () => {
  const p = payload();
  const got = verifyLicenseToken(signLicenseToken(p));
  assert.ok(got, "a freshly signed token failed to verify");
  assert.equal(got.keyHash, p.keyHash);
  assert.equal(got.tier, "ultimate");
  assert.equal(got.devicePubkeyId, p.devicePubkeyId);
  assert.equal(got.activatedAt, p.activatedAt);
});

test("the header carries the configured kid, which is what makes rotation possible", () => {
  const [h] = signLicenseToken(payload()).split(".");
  const header = JSON.parse(Buffer.from(h, "base64url").toString());
  assert.equal(header.alg, "ES256");
  assert.equal(header.kid, "vethida-license-v1");
  assert.equal(header.kid, signingKid());
});

test("a token naming a kid we do not hold is refused", () => {
  // The rotation failure mode: the client trusts two keys, and a token stamped
  // with a third must not verify against either.
  const token = retoken({ header: { kid: "vethida-license-v9" } }, signLicenseToken(payload()));
  assert.equal(verifyLicenseToken(token), null);
});

test("a token with no kid still verifies, so rotation does not strand old tokens", () => {
  const signed = signLicenseToken(payload());
  const [h, p, s] = signed.split(".");
  const header = JSON.parse(Buffer.from(h, "base64url").toString());
  delete header.kid;
  const rebuilt = `${Buffer.from(JSON.stringify(header)).toString("base64url")}.${p}`;
  const sig = crypto.sign("sha256", Buffer.from(rebuilt), {
    key: privateKey,
    dsaEncoding: "ieee-p1363",
  });
  const token = `${rebuilt}.${sig.toString("base64url")}`;
  assert.ok(verifyLicenseToken(token), "a kid-less token was rejected");
  assert.ok(s.length > 0);
});

test("a tampered tier does not verify", () => {
  // The upgrade an attacker would actually attempt.
  const token = signLicenseToken(payload({ tier: "pro" }));
  const [h, p, s] = token.split(".");
  const claims = JSON.parse(Buffer.from(p, "base64url").toString());
  claims.tier = "ultimate";
  const forged = `${h}.${Buffer.from(JSON.stringify(claims)).toString("base64url")}.${s}`;
  assert.equal(verifyLicenseToken(forged), null);
});

test("a token signed by a different key does not verify", () => {
  const { privateKey: other } = crypto.generateKeyPairSync("ec", { namedCurve: "prime256v1" });
  const token = retoken({}, signLicenseToken(payload()), other);
  assert.equal(verifyLicenseToken(token), null);
});

test("an expired token is refused once past the skew allowance", () => {
  const expired = payload({ expiresAt: new Date(Date.now() - 2 * HOUR).toISOString() });
  assert.equal(verifyLicenseToken(signLicenseToken(expired)), null);
});

test("clock skew is tolerated within the stated window", () => {
  // 30s past expiry with the client's 60s default
  const justExpired = payload({ expiresAt: new Date(Date.now() - 30_000).toISOString() });
  assert.ok(verifyLicenseToken(signLicenseToken(justExpired), 60));
  assert.equal(verifyLicenseToken(signLicenseToken(justExpired), 0), null);
});

test("alg confusion is refused", () => {
  // only ES256
  for (const alg of ["none", "HS256", "RS256", "EdDSA", "es256"]) {
    const token = retoken({ header: { alg } }, signLicenseToken(payload()));
    assert.equal(verifyLicenseToken(token), null, `alg ${alg} was honored`);
  }
});

test("a foreign issuer or audience is refused", () => {
  for (const claims of [{ iss: "https://example.com" }, { aud: "vethida.web" }]) {
    const token = retoken({ claims }, signLicenseToken(payload()));
    assert.equal(verifyLicenseToken(token), null, `${JSON.stringify(claims)} was honored`);
  }
});

test("malformed input is refused rather than throwing", () => {
  for (const bad of ["", "a", "a.b", "a.b.c.d", "....", "not a token at all"]) {
    assert.doesNotThrow(() => verifyLicenseToken(bad), `threw on ${JSON.stringify(bad)}`);
    assert.equal(verifyLicenseToken(bad), null);
  }
});

test("an invalid expiry is rejected at signing rather than producing a bad token", () => {
  assert.throws(() => signLicenseToken(payload({ expiresAt: "not-a-date" })), TypeError);
});

test("the exported raw public key is the 32 bytes the client bakes in", () => {
  const raw = signingPublicKeyRaw();
  assert.equal(raw.length, 64);
  const jwk = crypto.createPublicKey(privateKey).export({ format: "jwk" });
  assert.ok(jwk.x && jwk.y, "the generated key exported no point");
  const derived = Buffer.concat([
    Buffer.from(jwk.x, "base64url"),
    Buffer.from(jwk.y, "base64url"),
  ]);
  assert.deepEqual(raw, derived);

  // the raw point the desktop gets has to verify our tokens
  const [h, p, s] = signLicenseToken(payload()).split(".");
  const key = crypto.createPublicKey({
    key: {
      kty: "EC",
      crv: "P-256",
      x: raw.subarray(0, 32).toString("base64url"),
      y: raw.subarray(32).toString("base64url"),
    },
    format: "jwk",
  });
  assert.ok(
    crypto.verify(
      "sha256",
      Buffer.from(`${h}.${p}`),
      { key, dsaEncoding: "ieee-p1363" },
      Buffer.from(s, "base64url"),
    ),
    "the raw exported point did not verify a token this service signed",
  );
});
