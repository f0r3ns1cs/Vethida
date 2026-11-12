// A trial is a normal Ultimate license with trial = true and an expiry, so the
// existing expiry handling covers it.

export const TRIAL_DAYS = 14;
export const TRIAL_TIER = "ultimate" as const;

export function trialExpiryFrom(start: Date): Date {
  return new Date(start.getTime() + TRIAL_DAYS * 24 * 60 * 60 * 1000);
}

// rounded up, so the last partial day still shows "1 day left"
export function trialDaysRemaining(expiresAt: Date, now: Date): number {
  const ms = expiresAt.getTime() - now.getTime();
  return ms <= 0 ? 0 : Math.ceil(ms / (24 * 60 * 60 * 1000));
}
