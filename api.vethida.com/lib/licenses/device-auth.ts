import crypto from "node:crypto";
import { eq } from "drizzle-orm";
import { db, device, license, type Device, type License } from "@vethida/db";

// Headers on every device-signed request:
//   X-Device-Pubkey-Id    hex(SHA-256(pubkey)[:16]), issued at activation
//   X-Request-Timestamp   unix ms when the client built the request
//   X-Device-Signature    base64url P-256 r || s signature over
//                         `${METHOD}\n${pathname}\n${timestamp}\n${sha256_hex(body)}`
// The timestamp is signed, so replays only work inside the skew window.

export const CLOCK_SKEW_WINDOW_MS = 5 * 60 * 1000;

const PUBKEY_ID_RE = /^[0-9a-f]{32}$/;
// r || s = 64 bytes = 86 base64url chars
const SIGNATURE_RE = /^[A-Za-z0-9_-]{86}$/;
const TIMESTAMP_RE = /^\d{1,15}$/;

// devicePublicKey is raw X || Y
export function verifyDeviceSignature(
  devicePublicKey: Buffer,
  canonicalRequest: string,
  signature: Buffer,
): boolean {
  if (devicePublicKey.length !== 64 || signature.length !== 64) return false;
  try {
    const key = crypto.createPublicKey({
      key: {
        kty: "EC",
        crv: "P-256",
        x: devicePublicKey.subarray(0, 32).toString("base64url"),
        y: devicePublicKey.subarray(32).toString("base64url"),
      },
      format: "jwk",
    });
    return crypto.verify(
      "sha256",
      Buffer.from(canonicalRequest, "utf8"),
      { key, dsaEncoding: "ieee-p1363" },
      signature,
    );
  } catch {
    return false;
  }
}

export type SignedRequestCheck =
  | { ok: true }
  | {
      ok: false;
      status: 400 | 401;
      code:
        | "device_auth_missing"
        | "device_auth_malformed"
        | "device_auth_clock_skew"
        | "device_signature_invalid";
    };

// Activation calls this with the pubkey from the body (no device row yet), the
// other routes go through authenticateDeviceRequest.
export function verifySignedRequest(
  req: Request,
  rawBody: string,
  devicePublicKey: Buffer,
): SignedRequestCheck {
  const signatureB64 = req.headers.get("x-device-signature");
  const timestamp = req.headers.get("x-request-timestamp");

  if (!signatureB64 || !timestamp) {
    return { ok: false, status: 401, code: "device_auth_missing" };
  }
  if (!SIGNATURE_RE.test(signatureB64) || !TIMESTAMP_RE.test(timestamp)) {
    return { ok: false, status: 400, code: "device_auth_malformed" };
  }
  if (Math.abs(Date.now() - Number(timestamp)) > CLOCK_SKEW_WINDOW_MS) {
    return { ok: false, status: 401, code: "device_auth_clock_skew" };
  }

  const bodyHash = crypto
    .createHash("sha256")
    .update(rawBody, "utf8")
    .digest("hex");
  // timestamp exactly as sent
  const canonical = [
    req.method.toUpperCase(),
    new URL(req.url).pathname,
    timestamp,
    bodyHash,
  ].join("\n");

  const signature = Buffer.from(signatureB64, "base64url");
  if (!verifyDeviceSignature(devicePublicKey, canonical, signature)) {
    return { ok: false, status: 401, code: "device_signature_invalid" };
  }
  return { ok: true };
}

export type DeviceAuthResult =
  | { ok: true; device: Device; license: License }
  | {
      ok: false;
      status: 400 | 401 | 403;
      code:
        | "device_auth_missing"
        | "device_auth_malformed"
        | "device_auth_clock_skew"
        | "device_not_found"
        | "device_signature_invalid"
        | "device_deactivated";
    };

// doesn't check license state or bump last_seen_at, callers do that
export async function authenticateDeviceRequest(
  req: Request,
  rawBody: string,
): Promise<DeviceAuthResult> {
  const pubkeyId = req.headers.get("x-device-pubkey-id");
  if (!pubkeyId) {
    return { ok: false, status: 401, code: "device_auth_missing" };
  }
  if (!PUBKEY_ID_RE.test(pubkeyId)) {
    return { ok: false, status: 400, code: "device_auth_malformed" };
  }

  const [row] = await db
    .select()
    .from(device)
    .innerJoin(license, eq(device.licenseId, license.id))
    .where(eq(device.devicePubkeyId, pubkeyId))
    .limit(1);

  if (!row) {
    return { ok: false, status: 401, code: "device_not_found" };
  }

  const sig = verifySignedRequest(req, rawBody, row.device.devicePublicKey);
  if (!sig.ok) {
    return { ok: false, status: sig.status, code: sig.code };
  }

  // after the signature check so only the real device learns it's deactivated
  if (row.device.deactivatedAt) {
    return { ok: false, status: 403, code: "device_deactivated" };
  }

  return { ok: true, device: row.device, license: row.license };
}
