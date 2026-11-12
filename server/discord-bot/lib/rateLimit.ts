interface Bucket {
  timestamps: number[];
}

export class RateLimiter {
  private readonly buckets = new Map<string, Bucket>();

  constructor(
    private readonly limit: number,
    private readonly windowMs: number,
    private readonly maxKeys = 50_000
  ) {
    const pruneInterval = Math.min(windowMs, 60_000);
    setInterval(() => this.prune(), pruneInterval).unref();
  }

  check(key: string): { allowed: true } | { allowed: false; retryAfterMs: number } {
    const now = Date.now();
    const bucket = this.buckets.get(key) ?? { timestamps: [] };
    bucket.timestamps = bucket.timestamps.filter((ts) => now - ts < this.windowMs);

    if (bucket.timestamps.length >= this.limit) {
      const oldest = bucket.timestamps[0] ?? now;
      return { allowed: false, retryAfterMs: Math.max(0, this.windowMs - (now - oldest)) };
    }

    bucket.timestamps.push(now);
    this.buckets.set(key, bucket);
    return { allowed: true };
  }

  reset(key: string): void {
    this.buckets.delete(key);
  }

  private prune(): void {
    const now = Date.now();
    for (const [key, bucket] of this.buckets.entries()) {
      bucket.timestamps = bucket.timestamps.filter((ts) => now - ts < this.windowMs);
      if (bucket.timestamps.length === 0) {
        this.buckets.delete(key);
      }
    }

    if (this.buckets.size <= this.maxKeys) {
      return;
    }
    const excess = this.buckets.size - this.maxKeys;
    const iterator = this.buckets.keys();
    for (let i = 0; i < excess; i++) {
      const next = iterator.next();
      if (next.done || next.value === undefined) {
        break;
      }
      this.buckets.delete(next.value);
    }
  }
}

export const commandLimiter = new RateLimiter(10, 10_000); // 10 commands / 10s / user
export const ticketLimiter = new RateLimiter(1, 60_000); // 1 ticket / 60s / user
export const bugReportLimiter = new RateLimiter(3, 5 * 60_000); // 3 reports / 5min / user
