import crypto from "node:crypto";
import { requiredEnv } from "@/lib/env";

// License tokens: compact ES256 JWS the desktop verifies offline (not the same
// thing as the license key the user types in). Signature is raw r || s
// (ieee-p1363), iss/aud have to match LicenseCache.cpp.
// LICENSE_SIGNING_KEY is base64 DER PKCS8, the public key is derived from it.

const ISSUER = "https://vethida.com";
const AUDIENCE = "vethida.desktop";

// kid for the JWS header. Has to match the kid in the client's SecurityConfig.h
// (primary or backup) for the key in LICENSE_SIGNING_KEY. No default on purpose,
// a wrong kid means tokens the client rejects.
// Rotation: ship the new key as the client's backup, wait, then switch key + kid here.
export function signingKid(): string {
  const kid = process.env.LICENSE_TOKEN_KID;
  if (!kid) {
    throw new Error(
      "LICENSE_TOKEN_KID is not set. It must match the kid baked into the " +
        "desktop client's SecurityConfig, or every token this service issues " +
        "will be rejected by the client that receives it.",
    );
  }
  return kid;
}

let privateKey: crypto.KeyObject | null = null;
let publicKey: crypto.KeyObject | null = null;

export function getSigningKeys(): {
  privateKey: crypto.KeyObject;
  publicKey: crypto.KeyObject;
} {
  if (!privateKey || !publicKey) {
    privateKey = crypto.createPrivateKey({
      key: Buffer.from(requiredEnv("LICENSE_SIGNING_KEY"), "base64"),
      format: "der",
      type: "pkcs8",
    });
    publicKey = crypto.createPublicKey(privateKey);
  }
  return { privateKey, publicKey };
}

// raw X || Y (64 bytes), goes into the client's SecurityConfig
export function signingPublicKeyRaw(): Buffer {
  const { publicKey: pk } = getSigningKeys();
  const jwk = pk.export({ format: "jwk" });
  if (jwk.crv !== "P-256" || !jwk.x || !jwk.y) {
    throw new Error(
      "LICENSE_SIGNING_KEY is not a P-256 key. The desktop client verifies " +
        "ES256 and will reject every token signed with anything else.",
    );
  }
  return Buffer.concat([
    Buffer.from(jwk.x, "base64url"),
    Buffer.from(jwk.y, "base64url"),
  ]);
}

export interface LicenseTokenPayload {
  keyHash: string;
  tier: string;
  devicePubkeyId: string;
  activatedAt: string;
  expiresAt: string;
}

interface JwtClaims {
  iss: string;
  aud: string;
  iat: number;
  nbf: number;
  exp: number;
  keyHash: string;
  tier: string;
  devicePubkeyId: string;
  activatedAt: string;
}

function b64url(input: Buffer | string): string {
  return Buffer.from(input).toString("base64url");
}

export function signLicenseToken(payload: LicenseTokenPayload): string {
  const { privateKey: pk } = getSigningKeys();
  const now = Math.floor(Date.now() / 1000);
  const exp = Math.floor(new Date(payload.expiresAt).getTime() / 1000);
  if (!Number.isFinite(exp)) {
    throw new TypeError("license token expiresAt is not a valid date");
  }

  const claims: JwtClaims = {
    iss: ISSUER,
    aud: AUDIENCE,
    iat: now,
    nbf: now,
    exp,
    keyHash: payload.keyHash,
    tier: payload.tier,
    devicePubkeyId: payload.devicePubkeyId,
    activatedAt: payload.activatedAt,
  };

  const signingInput =
    b64url(JSON.stringify({ alg: "ES256", typ: "JWT", kid: signingKid() })) +
    "." +
    b64url(JSON.stringify(claims));
  const signature = crypto.sign("sha256", Buffer.from(signingInput), {
    key: pk,
    dsaEncoding: "ieee-p1363",
  });
  return signingInput + "." + b64url(signature);
}

// server-side verify, only used internally. Same clock skew as the client.
export function verifyLicenseToken(
  token: string,
  clockSkewSec = 60,
): LicenseTokenPayload | null {
  const { publicKey: pk } = getSigningKeys();
  const parts = token.split(".");
  if (parts.length !== 3) return null;
  const [headerB64, payloadB64, sigB64] = parts;

  try {
    const header = JSON.parse(
      Buffer.from(headerB64, "base64url").toString(),
    ) as { alg?: string; kid?: string };
    if (header.alg !== "ES256") return null;
    // tokens without a kid are still accepted
    if (header.kid !== undefined && header.kid !== signingKid()) return null;

    const valid = crypto.verify(
      "sha256",
      Buffer.from(`${headerB64}.${payloadB64}`),
      { key: pk, dsaEncoding: "ieee-p1363" },
      Buffer.from(sigB64, "base64url"),
    );
    if (!valid) return null;

    const c = JSON.parse(
      Buffer.from(payloadB64, "base64url").toString(),
    ) as JwtClaims;
    if (c.iss !== ISSUER || c.aud !== AUDIENCE) return null;
    const now = Math.floor(Date.now() / 1000);
    if (c.exp + clockSkewSec < now) return null;
    if (c.nbf - clockSkewSec > now) return null;

    return {
      keyHash: c.keyHash,
      tier: c.tier,
      devicePubkeyId: c.devicePubkeyId,
      activatedAt: c.activatedAt,
      expiresAt: new Date(c.exp * 1000).toISOString(),
    };
  } catch {
    return null;
  }
}
