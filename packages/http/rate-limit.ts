type Bucket = { count: number; windowStart: number };

export type RateLimiterOptions = { windowMs: number; max: number };

// Simple in-memory fixed window. Fine with one process per app, but it resets on
// restart and isn't shared, so anything security related (OTP sends) uses the db limiter.
export function createRateLimiter({ windowMs, max }: RateLimiterOptions) {
  const buckets = new Map<string, Bucket>();
  let lastSweep = Date.now();

  return function isLimited(key: string): boolean {
    const now = Date.now();
    if (now - lastSweep > windowMs) {
      for (const [k, b] of buckets) {
        if (now - b.windowStart > windowMs) buckets.delete(k);
      }
      lastSweep = now;
    }
    const bucket = buckets.get(key);
    if (!bucket || now - bucket.windowStart > windowMs) {
      buckets.set(key, { count: 1, windowStart: now });
      return 1 > max; // so max = 0 blocks everything
    }
    bucket.count++;
    return bucket.count > max;
  };
}

// Origin only takes traffic from Cloudflare so CF-Connecting-IP can be trusted.
// For X-Forwarded-For use the last entry, the first one is whatever the client sent.
export function clientIp(req: Request): string {
  const cf = req.headers.get("cf-connecting-ip");
  if (cf) return cf.trim();
  const real = req.headers.get("x-real-ip");
  if (real) return real.trim();
  const xff = req.headers.get("x-forwarded-for");
  if (xff) {
    const parts = xff.split(",");
    return parts[parts.length - 1].trim();
  }
  return "unknown";
}

// just a quick check on Content-Length, nginx enforces the real limit
export function contentLengthExceeds(req: Request, maxBytes: number): boolean {
  const raw = req.headers.get("content-length");
  if (!raw) return false;
  const len = Number(raw);
  return Number.isFinite(len) && len > maxBytes;
}
