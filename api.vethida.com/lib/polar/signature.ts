import crypto from "node:crypto";

// Standard-webhooks signature check. Not using the SDK's validateEvent because
// it throws on event types newer than the SDK version.
const TIMESTAMP_TOLERANCE_S = 5 * 60;

export interface WebhookSignatureHeaders {
  id: string | null;
  timestamp: string | null;
  signature: string | null;
}

export function verifyPolarWebhook(
  rawBody: string,
  headers: WebhookSignatureHeaders,
  secret: string,
): boolean {
  if (!headers.id || !headers.timestamp || !headers.signature) return false;
  if (!/^\d{1,12}$/.test(headers.timestamp)) return false;
  if (
    Math.abs(Date.now() / 1000 - Number(headers.timestamp)) >
    TIMESTAMP_TOLERANCE_S
  ) {
    return false;
  }

  // The key is the secret string's raw bytes, including any whsec_ prefix. Don't
  // strip it or base64-decode (the SDK encodes and standardwebhooks decodes, so
  // it nets out to the plain string).
  const key = Buffer.from(secret, "utf8");
  const expected = crypto
    .createHmac("sha256", key)
    .update(`${headers.id}.${headers.timestamp}.${rawBody}`, "utf8")
    .digest();

  // can be several signatures, space separated (secret rotation)
  for (const part of headers.signature.split(" ")) {
    const [version, sig] = part.split(",");
    if (version !== "v1" || !sig) continue;
    const provided = Buffer.from(sig, "base64");
    if (
      provided.length === expected.length &&
      crypto.timingSafeEqual(provided, expected)
    ) {
      return true;
    }
  }
  return false;
}
