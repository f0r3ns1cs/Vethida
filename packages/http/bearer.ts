import crypto from "node:crypto";

// constant time string compares for secrets (cron tokens etc)

export function constantTimeEqualUtf8(a: string, b: string): boolean {
  if (!a || !b) return false;
  const bufA = Buffer.from(a, "utf8");
  const bufB = Buffer.from(b, "utf8");
  if (bufA.length !== bufB.length) return false;
  return crypto.timingSafeEqual(bufA, bufB);
}

export function constantTimeEqualHex(a: string, b: string): boolean {
  if (!a || !b) return false;
  if (a.length !== b.length) return false;
  const bufA = Buffer.from(a, "hex");
  const bufB = Buffer.from(b, "hex");
  // Buffer.from(hex) silently truncates bad input
  if (bufA.length !== a.length / 2 || bufB.length !== b.length / 2) return false;
  return crypto.timingSafeEqual(bufA, bufB);
}

// false if expected is unset too, so a missing env var means denied
export function bearerTokenMatches(
  authorizationHeader: string | null | undefined,
  expected: string | undefined | null,
): boolean {
  if (!expected) return false;
  const auth = authorizationHeader ?? "";
  if (!auth.toLowerCase().startsWith("bearer ")) return false;
  const provided = auth.slice(7).trim();
  if (!provided) return false;
  return constantTimeEqualUtf8(provided, expected);
}
