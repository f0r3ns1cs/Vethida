import { NextResponse, type NextRequest } from "next/server";
import { and, eq, gt, isNull, lte, sql } from "drizzle-orm";
import { db, license, tierLabel, trialDaysRemaining, user } from "@vethida/db";
import { sendEmail } from "@/lib/email";
import { legalEntityAddress } from "@/lib/email/identity";
import { renderTemplate } from "@/lib/email/render";
import { bearerTokenMatches } from "@vethida/http/bearer";
import { apiError, ERROR_CODES } from "@/lib/errors";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

const REMIND_WITHIN_DAYS = 3; // monthly + trials
// yearly: California's ARL wants the notice 15-45 days before renewal
const REMIND_WITHIN_DAYS_ANNUAL = 30;

function fmtDate(d: Date): string {
  return d.toLocaleDateString("en-US", {
    year: "numeric",
    month: "long",
    day: "numeric",
  });
}

// Daily cron (systemd timer, CRON_SECRET bearer): "renews soon" and "trial
// ending" emails. One reminder per term, tracked with renewal_reminder_sent_at.
export async function GET(req: NextRequest) {
  if (!bearerTokenMatches(req.headers.get("authorization"), process.env.CRON_SECRET)) {
    return apiError(401, ERROR_CODES.UNAUTHORIZED, "Invalid cron token.");
  }

  try {
    const renewals = await sendDueReminders();
    const trials = await sendDueTrialReminders();
    return NextResponse.json({ renewals, trials });
  } catch (err) {
    console.error("[license-reminders] unhandled error", err);
    return apiError(
      500,
      ERROR_CODES.INTERNAL_ERROR,
      "Reminder sweep failed.",
    );
  }
}

interface SweepResult {
  due: number;
  sent: number;
  failed: number;
}

async function sendDueReminders(): Promise<SweepResult> {
  const now = new Date();
  // query the widest window, then filter per billing interval
  const cutoff = new Date(
    now.getTime() + Math.max(REMIND_WITHIN_DAYS, REMIND_WITHIN_DAYS_ANNUAL) * 86_400_000,
  );

  const candidates = await db
    .select({
      id: license.id,
      tier: license.tier,
      userId: license.userId,
      expiresAt: license.expiresAt,
      billingInterval: license.billingInterval,
    })
    .from(license)
    .where(
      and(
        gt(license.expiresAt, now),
        lte(license.expiresAt, cutoff),
        isNull(license.revokedAt),
        eq(license.cancelAtPeriodEnd, false),
        isNull(license.renewalReminderSentAt),
        sql`polar_subscription_id IS NOT NULL`,
      ),
    );

  const due = candidates.filter((lic) => {
    if (!lic.expiresAt) return false;
    const days = lic.billingInterval === "year" ? REMIND_WITHIN_DAYS_ANNUAL : REMIND_WITHIN_DAYS;
    return lic.expiresAt.getTime() <= now.getTime() + days * 86_400_000;
  });

  let sent = 0;
  let failed = 0;
  for (const lic of due) {
    if (!lic.userId || !lic.expiresAt) continue;
    try {
      const [owner] = await db
        .select({ email: user.email })
        .from(user)
        .where(eq(user.id, lic.userId))
        .limit(1);
      if (!owner?.email) continue;

      const first = owner.email.split("@")[0]?.split(/[._-]/)[0] || "there";
      const rendered = renderTemplate("license-expiring-soon", {
        customerFirstName: first.charAt(0).toUpperCase() + first.slice(1),
        tier: tierLabel(lic.tier),
        renewalDate: fmtDate(lic.expiresAt),
        legalEntityAddress: legalEntityAddress(),
      });
      // only mark it sent on success so the next run retries
      if (await sendEmail({ to: owner.email, ...rendered })) {
        await db
          .update(license)
          .set({ renewalReminderSentAt: now })
          .where(eq(license.id, lic.id));
        sent += 1;
      }
    } catch (err) {
      failed += 1;
      console.error(`[license-reminders] license ${lic.id} failed`, err);
    }
  }

  return { due: due.length, sent, failed };
}

// Trials have no polar_subscription_id so they need their own query. Reuses
// renewal_reminder_sent_at (a trial only has one term).
async function sendDueTrialReminders(): Promise<SweepResult> {
  const now = new Date();
  const cutoff = new Date(now.getTime() + REMIND_WITHIN_DAYS * 86_400_000);

  const due = await db
    .select({
      id: license.id,
      userId: license.userId,
      expiresAt: license.expiresAt,
    })
    .from(license)
    .where(
      and(
        eq(license.trial, true),
        gt(license.expiresAt, now),
        lte(license.expiresAt, cutoff),
        isNull(license.revokedAt),
        isNull(license.renewalReminderSentAt),
      ),
    );

  let sent = 0;
  let failed = 0;
  for (const lic of due) {
    if (!lic.userId || !lic.expiresAt) continue;
    try {
      const [owner] = await db
        .select({ email: user.email })
        .from(user)
        .where(eq(user.id, lic.userId))
        .limit(1);
      if (!owner?.email) continue;

      const first = owner.email.split("@")[0]?.split(/[._-]/)[0] || "there";
      const rendered = renderTemplate("trial-ending", {
        customerFirstName: first.charAt(0).toUpperCase() + first.slice(1),
        endDate: fmtDate(lic.expiresAt),
        daysLeft: describeDaysLeft(lic.expiresAt, now),
        legalEntityAddress: legalEntityAddress(),
      });
      if (await sendEmail({ to: owner.email, ...rendered })) {
        await db
          .update(license)
          .set({ renewalReminderSentAt: now })
          .where(eq(license.id, lic.id));
        sent += 1;
      }
    } catch (err) {
      failed += 1;
      console.error(`[license-reminders] trial ${lic.id} failed`, err);
    }
  }

  return { due: due.length, sent, failed };
}

// "3 days", "1 day", "less than a day" (rounded up, same as the portal and the app)
function describeDaysLeft(expiresAt: Date, now: Date): string {
  const days = trialDaysRemaining(expiresAt, now);
  if (days <= 0) return "less than a day";
  return days === 1 ? "1 day" : `${days} days`;
}
