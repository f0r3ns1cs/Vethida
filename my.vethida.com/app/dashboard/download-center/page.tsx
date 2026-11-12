import Link from "next/link";
import { headers } from "next/headers";
import { redirect } from "next/navigation";
import { auth } from "@/lib/auth";
import { getEntitlement, getUserLicenses, hasUsedTrial } from "@/lib/licenses";
import { LicenseKey } from "@/components/license-key";
import { ActivateInApp } from "@/components/activate-in-app";
import { PlanPill } from "@/components/plan-pill";
import { TRIAL_DAYS, tierLabel } from "@vethida/db";

export const dynamic = "force-dynamic";
export const metadata = { title: "Download" };

export default async function DownloadCenterPage() {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  const [entitlement, licenses, trialUsed] = await Promise.all([
    getEntitlement(session.user.id),
    getUserLicenses(session.user.id),
    hasUsedTrial(session.user.id),
  ]);
  const usable = licenses.filter(({ license: l }) => !l.revokedAt);
  // same file as vethida.com/download, unset = not published yet
  const downloadUrl = process.env.NEXT_PUBLIC_DOWNLOAD_URL ?? null;

  return (
    <>
      <header className="page-header">
        <h1 className="page-title">Download</h1>
        <p className="page-subtitle">
          One installer for all three tiers. It runs on Free until you activate
          a key, and the key decides what it unlocks.
        </p>
      </header>

      <section className="card">
        <div className="card-header">
          <div>
            <h2 className="card-title">Vethida for Windows</h2>
            <p className="card-subtitle">
              {entitlement.trial
                ? `Your ${tierLabel(entitlement.tier)} trial is running.`
                : `Your plan is ${tierLabel(entitlement.tier)}.`}
            </p>
          </div>
        </div>
        <div className="card-body">
          {downloadUrl ? (
            <a href={downloadUrl} className="btn btn-primary">
              Download installer
            </a>
          ) : (
            <p className="text-sm text-foreground-muted">
              The installer is not published yet. It appears here as soon as it
              is, and your key already works.
            </p>
          )}
        </div>
        <div className="card-footer">
          <dl className="datalist text-xs">
            <div className="datarow">
              <dt>Requires</dt>
              <dd>Windows 10 version 1903 or later, or Windows 11, 64-bit</dd>
            </div>
            <div className="datarow">
              <dt>Rights</dt>
              <dd>Administrator, requested once at launch</dd>
            </div>
          </dl>
        </div>
      </section>

      <section className="card mt-4">
        <div className="card-header">
          <div>
            <h2 className="card-title">Check what you downloaded</h2>
            <p className="card-subtitle">
              Vethida runs with administrator rights, so it is worth thirty
              seconds to confirm the file is ours.
            </p>
          </div>
        </div>
        <div className="card-body">
          <ol className="space-y-3 text-sm text-foreground-muted">
            <li>
              <span className="font-medium text-foreground">
                Take it from here or from vethida.com/download.
              </span>{" "}
              Those two serve the same file. A copy from a download portal, a
              forum mirror or a torrent is not one we can vouch for.
            </li>
            {/* add a UAC publisher step once releases are signed */}
            <li>
              <span className="font-medium text-foreground">
                Compare the SHA-256.
              </span>{" "}
              vethida.com/download publishes the installer&apos;s hash.
            </li>
            <li>
              <span className="font-medium text-foreground">
                Updates verify themselves.
              </span>{" "}
              Vethida checks each update&apos;s signature before it installs it,
              and refuses the update if the check fails.
            </li>
          </ol>
        </div>
      </section>

      <section className="card mt-4">
        <div className="card-header">
          <h2 className="card-title">
            {usable.length === 1 ? "Your license key" : "Your license keys"}
          </h2>
        </div>
        <div className="card-body">
          {usable.length === 0 ? (
            <div className="empty">
              <p className="empty-title">No license on this account</p>
              <p className="empty-text">
                {trialUsed
                  ? "Your trial has ended. Pick a plan and your key appears here."
                  : `Start your ${TRIAL_DAYS}-day trial and your key appears here.`}
              </p>
              <div className="empty-actions">
                <Link href="/dashboard/licenses" className="btn btn-primary">
                  {trialUsed ? "See plans" : "Start free trial"}
                </Link>
              </div>
            </div>
          ) : (
            <ul className="space-y-5">
              {usable.map(({ license: lic, devices }) => (
                <li key={lic.id}>
                  <div className="mb-2 flex items-center gap-2.5">
                    <PlanPill tier={lic.tier} trial={lic.trial} />
                    {devices.length > 0 ? (
                      <span className="badge badge-success">Activated</span>
                    ) : (
                      <span className="badge badge-neutral">Not activated</span>
                    )}
                  </div>
                  <LicenseKey licenseKey={lic.key} />
                  {devices.length === 0 ? (
                    <div className="mt-3">
                      <ActivateInApp licenseKey={lic.key} />
                    </div>
                  ) : null}
                </li>
              ))}
            </ul>
          )}
        </div>
        {usable.length > 0 ? (
          <div className="card-footer">
            <p className="text-xs text-foreground-faint">
              Paste the key into Vethida&apos;s Account page to activate.
            </p>
          </div>
        ) : null}
      </section>
    </>
  );
}
