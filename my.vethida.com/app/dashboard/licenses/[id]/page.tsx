import Link from "next/link";
import { headers } from "next/headers";
import { notFound, redirect } from "next/navigation";
import { LIFETIME_SCOPE, tierLabel, type License } from "@vethida/db";
import { auth } from "@/lib/auth";
import { getUserLicenses } from "@/lib/licenses";
import { formatDate } from "@/lib/format";
import { LicenseKey } from "@/components/license-key";
import { PlanPill } from "@/components/plan-pill";
import { deactivateDeviceAction } from "../actions";

export const dynamic = "force-dynamic";
export const metadata = { title: "License" };

function timeAgo(date: Date): string {
  const s = Math.round((Date.now() - date.getTime()) / 1000);
  if (s < 60) return "just now";
  const m = Math.round(s / 60);
  if (m < 60) return `${m}m ago`;
  const h = Math.round(m / 60);
  if (h < 24) return `${h}h ago`;
  return `${Math.round(h / 24)}d ago`;
}

function billingLine(lic: License): string {
  if (lic.revokedAt) return "This license has been revoked.";
  if (lic.trial && lic.expiresAt) {
    return `Free trial. Ends on ${formatDate(lic.expiresAt)}.`;
  }
  if (lic.polarSubscriptionId && lic.expiresAt) {
    return lic.cancelAtPeriodEnd
      ? `Canceled. Access ends on ${formatDate(lic.expiresAt)}.`
      : `Renews on ${formatDate(lic.expiresAt)}.`;
  }
  if (lic.expiresAt) return `Expires on ${formatDate(lic.expiresAt)}.`;
  return `Lifetime license. No renewal required. ${LIFETIME_SCOPE}`;
}

export default async function LicenseDetailPage({
  params,
}: {
  params: Promise<{ id: string }>;
}) {
  const { id } = await params;
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  const entry = (await getUserLicenses(session.user.id)).find(
    (l) => l.license.id === id,
  );
  if (!entry) notFound();
  const { license: lic, devices } = entry;

  return (
    <>
      <header className="page-header">
        <Link
          href="/dashboard/licenses"
          className="page-pretitle underline-offset-4 hover:text-foreground hover:underline"
        >
          &larr; Licenses
        </Link>
        <h1 className="page-title flex items-center gap-2.5">
          {tierLabel(lic.tier)} license
          {lic.trial ? (
            <PlanPill tier={lic.tier} trial />
          ) : null}
        </h1>
      </header>

      <section className="card p-5">
        <dl className="space-y-3 text-sm">
          <div className="flex justify-between gap-4">
            <dt className="text-foreground-muted">Key</dt>
            <dd>
              <LicenseKey licenseKey={lic.key} />
            </dd>
          </div>
          <div className="flex justify-between gap-4">
            <dt className="text-foreground-muted">Billing</dt>
            <dd className="text-right">{billingLine(lic)}</dd>
          </div>
          <div className="flex justify-between gap-4">
            <dt className="text-foreground-muted">Machine resets left</dt>
            <dd>{lic.machineResetsRemaining}</dd>
          </div>
        </dl>
        {lic.polarSubscriptionId ? (
          <Link
            href="/dashboard/billing"
            className="mt-4 inline-block text-sm text-foreground-muted underline-offset-4 hover:text-foreground hover:underline"
          >
            Manage billing
          </Link>
        ) : null}
      </section>

      <section id="machines" className="card mt-6 p-5">
        <h2 className="mb-1 text-sm font-semibold">Machines</h2>
        <p className="mb-4 text-xs text-foreground-muted">
          A license runs on one machine at a time. Deactivate a machine to free
          the slot; you have {lic.machineResetsRemaining} resets left.
        </p>
        {devices.length === 0 ? (
          <p className="text-sm text-foreground-muted">
            No active machine. Activate Vethida with this key to bind it.
          </p>
        ) : (
          <ul className="space-y-3">
            {devices.map((d) => (
              <li
                key={d.id}
                className="flex items-center justify-between gap-4"
              >
                <div className="min-w-0">
                  <p className="font-mono text-sm">
                    {d.devicePubkeyId.slice(0, 12)}
                  </p>
                  <p className="text-xs text-foreground-faint">
                    Last seen {timeAgo(d.lastSeenAt)}
                    {d.lastIp ? ` from ${d.lastIp}` : ""}
                  </p>
                </div>
                <form action={deactivateDeviceAction}>
                  <input type="hidden" name="deviceId" value={d.id} />
                  <button
                    type="submit"
                    className="btn btn-secondary h-8 shrink-0 px-3"
                  >
                    Deactivate
                  </button>
                </form>
              </li>
            ))}
          </ul>
        )}
      </section>
    </>
  );
}
