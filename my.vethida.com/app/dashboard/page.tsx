import Link from "next/link";
import { headers } from "next/headers";
import { redirect } from "next/navigation";
import { auth } from "@/lib/auth";
import { getEntitlement, getUserLicenses, hasUsedTrial } from "@/lib/licenses";
import { getPendingDeletion } from "@/lib/account-deletion";
import { TRIAL_DAYS, isLicensed, tierLabel, trialDaysRemaining } from "@vethida/db";
import { formatDate } from "@/lib/format";
import { PlanPill } from "@/components/plan-pill";

export const dynamic = "force-dynamic";
export const metadata = { title: "Overview" };

// days is already rounded up
function daysLeftLabel(days: number): string {
  if (days <= 0) return "Ends today";
  return days === 1 ? "1 day left" : `${days} days left`;
}

export default async function DashboardPage() {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  const [entitlement, licenses, pendingDeletion, trialUsed] = await Promise.all([
    getEntitlement(session.user.id),
    getUserLicenses(session.user.id),
    getPendingDeletion(session.user.id),
    hasUsedTrial(session.user.id),
  ]);

  const live = licenses.filter(({ license: l }) => !l.revokedAt);
  const activeDevices = live.reduce((n, { devices }) => n + devices.length, 0);

  // the license for the displayed tier, not just the newest one (upgrades
  // can leave an old Pro next to Ultimate)
  const licensed = isLicensed(entitlement);
  const current = licensed
    ? (live.find(
        ({ license: l }) =>
          l.tier === entitlement.tier && l.trial === entitlement.trial,
      )?.license ?? live.find(({ license: l }) => l.tier === entitlement.tier)
        ?.license)
    : undefined;

  return (
    <>
      <header className="page-header">
        <p className="page-pretitle">Account</p>
        <h1 className="page-title">Overview</h1>
      </header>

      {pendingDeletion ? (
        <div role="alert" className="alert alert-danger mb-4">
          <div>
            <p className="alert-title">
              This account is scheduled for deletion
            </p>
            <p className="mt-1">
              Everything is removed on {formatDate(pendingDeletion.hardDeleteAt)}.
              You can stop this any time before then from{" "}
              <Link
                href="/dashboard/settings"
                className="underline underline-offset-4"
              >
                Settings
              </Link>
              .
            </p>
          </div>
        </div>
      ) : null}

      <div className="grid gap-4 sm:grid-cols-3">
        <section className="card">
          <div className="card-body">
            <p className="text-xs text-foreground-muted">Current plan</p>
            <div className="mt-1 flex items-center gap-2">
              <p className="text-lg font-semibold">
                {tierLabel(entitlement.tier)}
              </p>
              <PlanPill tier={entitlement.tier} trial={entitlement.trial} />
            </div>
          </div>
        </section>

        <section className="card">
          <div className="card-body">
            <p className="text-xs text-foreground-muted">
              {!licensed
                ? "Expires"
                : entitlement.trial
                  ? "Trial ends"
                  : current?.cancelAtPeriodEnd
                    ? "Access ends"
                    : "Renews"}
            </p>
            <p className="mt-1 text-lg font-semibold">
              {entitlement.trial && current?.expiresAt
                ? daysLeftLabel(trialDaysRemaining(current.expiresAt, new Date()))
                : current?.expiresAt
                  ? formatDate(current.expiresAt)
                  : "Never"}
            </p>
            {entitlement.trial && current?.expiresAt ? (
              <p className="mt-1 text-xs text-foreground-faint">
                {formatDate(current.expiresAt)}
              </p>
            ) : null}
          </div>
        </section>

        <section className="card">
          <div className="card-body">
            <p className="text-xs text-foreground-muted">Active devices</p>
            <p className="mt-1 text-lg font-semibold">{activeDevices}</p>
            <p className="mt-1 text-xs text-foreground-faint">
              {live.length === 1 ? "1 license" : `${live.length} licenses`}
            </p>
          </div>
        </section>
      </div>

      <section className="card mt-4">
        <div className="card-header">
          <h2 className="card-title">
            {live.length === 0
              ? "Get started"
              : activeDevices === 0
                ? "Activate Vethida on your PC"
                : "You're set up"}
          </h2>
        </div>
        <div className="card-body">
          {live.length === 0 ? (
            <>
              <p className="text-sm text-foreground-muted">
                {trialUsed
                  ? "Your trial has ended. Pick a plan to carry on where you left off."
                  : `Start your ${TRIAL_DAYS}-day trial and Vethida unlocks in full, or buy a plan outright.`}
              </p>
              <div className="mt-4 flex flex-wrap gap-3">
                <Link href="/dashboard/licenses" className="btn btn-primary">
                  {trialUsed ? "See plans" : "Start free trial"}
                </Link>
                <Link
                  href="/dashboard/download-center"
                  className="btn btn-secondary"
                >
                  Download Vethida
                </Link>
              </div>
            </>
          ) : activeDevices === 0 ? (
            <>
              <p className="text-sm text-foreground-muted">
                Your license isn&apos;t activated on a machine yet. Install
                Vethida, then paste your key into its Account page.
              </p>
              <div className="mt-4 flex flex-wrap gap-3">
                <Link
                  href="/dashboard/download-center"
                  className="btn btn-primary"
                >
                  Download Vethida
                </Link>
                <Link href="/dashboard/licenses" className="btn btn-secondary">
                  View your key
                </Link>
              </div>
            </>
          ) : (
            <>
              <p className="text-sm text-foreground-muted">
                Vethida is activated and reporting in. Everything you&apos;ve
                applied stays applied. Nothing here needs your attention.
              </p>
              <div className="mt-4 flex flex-wrap gap-3">
                <Link href="/dashboard/licenses" className="btn btn-secondary">
                  Licenses &amp; devices
                </Link>
                <Link href="/dashboard/billing" className="btn btn-secondary">
                  Billing
                </Link>
              </div>
            </>
          )}
        </div>
      </section>

      <section className="card mt-4">
        <div className="card-header">
          <h2 className="card-title">Account</h2>
        </div>
        <div className="card-body">
          <dl className="datalist">
            <div className="datarow">
              <dt>Email</dt>
              <dd className="break-all">{session.user.email}</dd>
            </div>
            <div className="datarow">
              <dt>Plan</dt>
              <dd>
                {entitlement.trial
                  ? `${tierLabel(entitlement.tier)} trial`
                  : tierLabel(entitlement.tier)}
              </dd>
            </div>
          </dl>
        </div>
      </section>
    </>
  );
}
