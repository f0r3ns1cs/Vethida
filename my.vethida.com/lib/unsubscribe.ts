import { createHmac, timingSafeEqual } from "node:crypto";

// unsubscribe links work signed out, so they carry an HMAC of the user id
function secret(): string {
  const s = process.env.BETTER_AUTH_SECRET;
  if (!s) throw new Error("BETTER_AUTH_SECRET is not configured");
  return s;
}

export function unsubscribeSignature(userId: string): string {
  return createHmac("sha256", secret())
    .update(`unsubscribe:release-announcements:${userId}`)
    .digest("base64url");
}

export function verifyUnsubscribeSignature(
  userId: string,
  signature: string,
): boolean {
  const expected = Buffer.from(unsubscribeSignature(userId));
  const given = Buffer.from(signature);
  return expected.length === given.length && timingSafeEqual(expected, given);
}

export function unsubscribeUrl(userId: string): string {
  const base = process.env.BETTER_AUTH_URL ?? "https://my.vethida.com";
  const sig = unsubscribeSignature(userId);
  return `${base}/api/unsubscribe?uid=${encodeURIComponent(userId)}&sig=${sig}`;
}
