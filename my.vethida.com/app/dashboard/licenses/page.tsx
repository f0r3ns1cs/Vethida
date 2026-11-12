import Link from "next/link";
import { headers } from "next/headers";
import { redirect } from "next/navigation";
import type { License } from "@vethida/db";
import { auth } from "@/lib/auth";
import { allAvailableCadences, fetchProductPrices } from "@/lib/polar";
import {
  getEntitlement,
  getLiveSubscribedTiers,
  getPerpetualTiers,
  getUserLicenses,
  hasUsedTrial,
} from "@/lib/licenses";
import { LicenseKey } from "@/components/license-key";
import { ActivateInApp } from "@/components/activate-in-app";
import { ClaimLicenseForm } from "@/components/claim-license-form";
import { BuyLicense } from "@/components/buy-license";
import { PlanPill } from "@/components/plan-pill";
import { StartTrial } from "@/components/start-trial";
import { TRIAL_DAYS, isLicensed } from "@vethida/db";
import { deactivateDeviceAction } from "./actions";

export const dynamic = "force-dynamic";
export const metadata = { title: "Licenses" };

type Status = { label: string; badge: string };

function licenseStatus(lic: License): Status {
  if (lic.revokedAt) return { label: "Revoked", badge: "badge-danger" };
  if (lic.expiresAt && lic.expiresAt.getTime() <= Date.now()) {
    return { label: "Expired", badge: "badge-neutral" };
  }
  if (lic.cancelAtPeriodEnd) return { label: "Ending", badge: "badge-warning" };
  return { label: "Active", badge: "badge-success" };
}

function timeAgo(date: Date): string {
  const s = Math.round((Date.now() - date.getTime()) / 1000);
  if (s < 60) return "just now";
  const m = Math.round(s / 60);
  if (m < 60) return `${m}m ago`;
  const h = Math.round(m / 60);
  if (h < 24) return `${h}h ago`;
  const d = Math.round(h / 24);
  return `${d}d ago`;
}

export default async function LicensesPage({
  searchParams,
}: {
  searchParams: Promise<{ status?: string }>;
}) {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  const licenses = await getUserLicenses(session.user.id);
  const justPaid = (await searchParams).status === "success"; // back from Polar checkout

  const [ownedTiers, perpetualTiers, prices, entitlement, trialUsed] =
    await Promise.all([
      getLiveSubscribedTiers(session.user.id),
      getPerpetualTiers(session.user.id),
      fetchProductPrices(),
      getEntitlement(session.user.id),
      hasUsedTrial(session.user.id),
    ]);
  const cadences = allAvailableCadences();

  return (
    <>
      <header className="page-header">
        <h1 className="page-title">Licenses &amp; devices</h1>
        <p className="page-subtitle">
          Each key activates one machine. Moving a key to a new PC means
          deactivating the old device first.
        </p>
      </header>

      {justPaid ? (
        <div role="status" className="alert alert-success mb-4">
          <div>
            <p className="alert-title">Payment received</p>
            <p className="mt-1 text-foreground-muted">
              Your license is being provisioned and appears below within a few
              seconds. Refresh if it has not shown up yet. A receipt is on its
              way to your email.
            </p>
          </div>
        </div>
      ) : null}

      {licenses.length === 0 ? (
        <section className="card mb-4">
          <div className="empty">
            <p className="empty-title">No licenses on this account yet</p>
            <p className="empty-text">
              {trialUsed
                ? "Your trial has ended. Pick a plan below, or claim a key you already have."
                : `Start your ${TRIAL_DAYS}-day trial for the whole of Ultimate, buy a plan below, or claim a key you already have.`}
            </p>
            {!trialUsed && !isLicensed(entitlement) ? (
              <div className="empty-actions">
                <StartTrial days={TRIAL_DAYS} />
              </div>
            ) : null}
          </div>
        </section>
      ) : (
        <div className="mb-4 space-y-4">
          {licenses.map(({ license: lic, devices }) => {
            const status = licenseStatus(lic);
            const revocationReason = lic.revokedAt
              ? (lic.revokedReasonCode ?? "revoked")
              : null;
            return (
              <section key={lic.id} className="card">
                <div className="card-header">
                  <div className="flex min-w-0 items-center gap-2.5">
                    <h2 className="card-title">
                      <PlanPill tier={lic.tier} trial={lic.trial} />
                    </h2>
                    <span className={`badge ${status.badge}`}>
                      {status.label}
                    </span>
                  </div>
                  <Link
                    href={`/dashboard/licenses/${lic.id}`}
                    className="shrink-0 text-sm text-foreground-muted underline-offset-4 hover:text-foreground hover:underline"
                  >
                    Details
                  </Link>
                </div>

                <div className="card-body space-y-5">
                  <div>
                    <p className="label">License key</p>
                    <LicenseKey licenseKey={lic.key} />
                    {revocationReason ? (
                      <p className="mt-2 text-xs text-danger">
                        This license was revoked ({revocationReason}).
                      </p>
                    ) : null}
                  </div>

                  <div>
                    <div className="mb-2 flex items-baseline justify-between gap-3">
                      <p className="label mb-0">Active device</p>
                      <p className="text-xs text-foreground-faint">
                        {lic.machineResetsRemaining} machine{" "}
                        {lic.machineResetsRemaining === 1 ? "reset" : "resets"}{" "}
                        left
                      </p>
                    </div>

                    {devices.length === 0 ? (
                      <div className="rounded-md border border-dashed border-border-strong p-4">
                        <p className="text-sm text-foreground-muted">
                          Not activated on any machine yet.
                        </p>
                        {!lic.revokedAt ? (
                          <div className="mt-3">
                            <ActivateInApp licenseKey={lic.key} />
                          </div>
                        ) : null}
                      </div>
                    ) : (
                      <ul className="space-y-2">
                        {devices.map((d) => (
                          <li
                            key={d.id}
                            className="flex flex-wrap items-center justify-between gap-3 rounded-md border border-border bg-background p-3.5"
                          >
                            <div className="min-w-0">
                              <p className="code text-foreground">
                                {d.devicePubkeyId.slice(0, 12)}
                              </p>
                              <p className="mt-0.5 text-xs text-foreground-faint">
                                Last seen {timeAgo(d.lastSeenAt)}
                                {d.lastIp ? ` from ${d.lastIp}` : ""}
                              </p>
                            </div>
                            <form action={deactivateDeviceAction}>
                              <input
                                type="hidden"
                                name="deviceId"
                                value={d.id}
                              />
                              <button
                                type="submit"
                                className="btn btn-secondary btn-sm"
                                aria-label={`Deactivate device ${d.devicePubkeyId.slice(0, 12)}`}
                              >
                                Deactivate
                              </button>
                            </form>
                          </li>
                        ))}
                      </ul>
                    )}
                  </div>
                </div>
              </section>
            );
          })}
        </div>
      )}

      <div className="space-y-4">
        <BuyLicense
          ownedTiers={ownedTiers}
          perpetualTiers={perpetualTiers}
          availableCadences={cadences}
          prices={prices}
        />
        <ClaimLicenseForm />
      </div>
    </>
  );
}
