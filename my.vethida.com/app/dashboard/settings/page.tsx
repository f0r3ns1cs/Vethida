import { headers } from "next/headers";
import { redirect } from "next/navigation";
import { eq } from "drizzle-orm";
import { db, emailPreference } from "@vethida/db";
import { auth } from "@/lib/auth";
import { discordConfigured } from "@/lib/discord";
import { getLinkedDiscord } from "@/lib/discord-link";
import { getPendingDeletion } from "@/lib/account-deletion";
import { formatDate, formatDateShort } from "@/lib/format";
import { DiscordLink } from "@/components/discord-link";
import { DeleteAccount } from "@/components/delete-account";
import { cancelDeletionAction } from "./account-actions";
import {
  revokeOtherSessionsAction,
  revokeSessionAction,
  updateEmailPreferencesAction,
} from "./preference-actions";

function describeSession(userAgent: string | null | undefined): string {
  if (!userAgent) return "Unknown device";
  const browser = /Firefox\//.test(userAgent)
    ? "Firefox"
    : /Edg\//.test(userAgent)
      ? "Edge"
      : /Chrome\//.test(userAgent)
        ? "Chrome"
        : /Safari\//.test(userAgent)
          ? "Safari"
          : "Browser";
  const os = /Windows/.test(userAgent)
    ? "Windows"
    : /Mac OS/.test(userAgent)
      ? "macOS"
      : /Android/.test(userAgent)
        ? "Android"
        : /iPhone|iPad|iOS/.test(userAgent)
          ? "iOS"
          : /Linux/.test(userAgent)
            ? "Linux"
            : "";
  return os ? `${browser} on ${os}` : browser;
}

export const dynamic = "force-dynamic";
export const metadata = { title: "Settings" };

export default async function SettingsPage({
  searchParams,
}: {
  searchParams: Promise<{ discord?: string }>;
}) {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  const discordEnabled = discordConfigured();
  // the sync itself is a POST from <DiscordLink/>, not done here
  const justLinked = (await searchParams).discord === "linked";
  const linked = discordEnabled
    ? (await getLinkedDiscord(session.user.id)) !== null
    : false;
  const pendingDeletion = await getPendingDeletion(session.user.id);

  const sessions = await auth.api.listSessions({ headers: await headers() });
  const [prefs] = await db
    .select()
    .from(emailPreference)
    .where(eq(emailPreference.userId, session.user.id))
    .limit(1);
  const fingerprintAlerts = prefs?.fingerprintAlerts ?? true;
  const releaseAnnouncements = prefs?.releaseAnnouncements ?? false;

  return (
    <>
      <header className="page-header">
        <h1 className="page-title">Settings</h1>
      </header>

      <section className="card">
        <div className="card-header">
          <div>
            <h2 className="card-title">How you sign in</h2>
            <p className="card-subtitle">
              Vethida has no passwords. Signing in sends a six-digit code to
              your email address, so keeping that inbox secure is what keeps
              this account secure.
            </p>
          </div>
        </div>
        <div className="card-body">
          <dl className="datalist">
            <div className="datarow">
              <dt>Sign-in address</dt>
              <dd className="break-all">{session.user.email}</dd>
            </div>
            <div className="datarow">
              <dt>Method</dt>
              <dd>Emailed one-time code</dd>
            </div>
          </dl>
        </div>
      </section>

      {discordEnabled ? (
        <section className="card mt-4">
          <div className="card-header">
            <div>
              <h2 className="card-title">Discord</h2>
              <p className="card-subtitle">
                Link your Discord account to receive your customer role in the
                server.
              </p>
            </div>
          </div>
          <div className="card-body">
            <DiscordLink linked={linked} justLinked={justLinked} />
          </div>
        </section>
      ) : null}

      <section id="sessions" className="card mt-4">
        <div className="card-header">
          <div>
            <h2 className="card-title">Sessions</h2>
            <p className="card-subtitle">
              Devices currently signed in to your account. If you see a session
              you don&apos;t recognize, revoke it, then secure the email inbox
              this account signs in with.
            </p>
          </div>
        </div>
        <div className="card-body">
          <ul className="space-y-3">
            {sessions.map((s) => {
              const current = s.token === session.session.token;
              return (
                <li
                  key={s.id}
                  className="flex items-center justify-between gap-4 rounded-md border border-border bg-background p-4"
                >
                  <div>
                    <p className="text-sm text-foreground">
                      {describeSession(s.userAgent)}
                      {current ? (
                        <span className="ml-2 rounded bg-surface px-1.5 py-0.5 text-xs text-foreground-muted">
                          This session
                        </span>
                      ) : null}
                    </p>
                    <p className="mt-1 text-xs text-foreground-muted">
                      {s.ipAddress ? `${s.ipAddress} - ` : ""}signed in{" "}
                      {formatDateShort(s.createdAt)}
                    </p>
                  </div>
                  {!current ? (
                    // session id only, never the token
                    <form action={revokeSessionAction}>
                      <input type="hidden" name="sessionId" value={s.id} />
                      <button
                        type="submit"
                        className="btn btn-secondary h-8 px-3"
                        aria-label={`Revoke session on ${describeSession(s.userAgent)}`}
                      >
                        Revoke
                      </button>
                    </form>
                  ) : null}
                </li>
              );
            })}
          </ul>
          {sessions.length > 1 ? (
            <form action={revokeOtherSessionsAction} className="mt-4">
              <button type="submit" className="btn btn-secondary h-9 px-4">
                Sign out all other sessions
              </button>
            </form>
          ) : null}
        </div>
      </section>

      <section id="notifications" className="card mt-4">
        <div className="card-header">
          <div>
            <h2 className="card-title">Email preferences</h2>
            <p className="card-subtitle" id="email">
              Transactional email (sign-in codes, receipts, and account notices)
              is always sent. Everything else is up to you.
            </p>
          </div>
        </div>
        <div className="card-body">
          <form action={updateEmailPreferencesAction} className="space-y-3">
            <label className="flex items-start gap-3">
              <input
                type="checkbox"
                name="fingerprintAlerts"
                defaultChecked={fingerprintAlerts}
                className="mt-0.5"
              />
              <span className="text-sm text-foreground">
                License security alerts
                <span className="block text-xs text-foreground-muted">
                  Get notified when a license activation shows a substantial
                  hardware change.
                </span>
              </span>
            </label>
            <label className="flex items-start gap-3">
              <input
                type="checkbox"
                name="releaseAnnouncements"
                defaultChecked={releaseAnnouncements}
                className="mt-0.5"
              />
              <span className="text-sm text-foreground">
                Release announcements
                <span className="block text-xs text-foreground-muted">
                  Occasional email when a notable Vethida release ships. Off
                  unless you opt in.
                </span>
              </span>
            </label>
            <button type="submit" className="btn btn-secondary h-9 px-4">
              Save preferences
            </button>
          </form>
        </div>
      </section>

      <section className="card mt-4">
        <div className="card-header">
          <div>
            <h2 className="card-title">Your data</h2>
            <p className="card-subtitle">
              Download a copy of the personal data associated with your account
              (profile, licenses, and activation records) as JSON.
            </p>
          </div>
        </div>
        <div className="card-body">
          <a
            href="/api/account/data-export"
            className="btn btn-secondary h-9 px-4"
            download
          >
            Download my data
          </a>
        </div>
      </section>

      <section className="card mt-4">
        <div className="card-header">
          <div>
            <h2 className="card-title">Delete account</h2>
          </div>
        </div>
        <div className="card-body">
          <p className="mb-2 text-sm text-foreground-muted">
            Deleting your account starts a 30-day grace period, after which your
            personal data is erased. Billing records we must keep for tax and
            dispute purposes are anonymized. Export your data first if you want
            a copy. Active subscriptions are canceled; the unused portion of a
            paid term is not refunded.
          </p>
          {pendingDeletion ? (
            <div className="rounded-md border border-border bg-background p-4">
              <p className="text-sm text-foreground">
                Your account is scheduled for deletion on{" "}
                {formatDate(pendingDeletion.hardDeleteAt)}
                .
              </p>
              <form action={cancelDeletionAction} className="mt-3">
                <button type="submit" className="btn btn-secondary h-9 px-4">
                  Cancel deletion
                </button>
              </form>
            </div>
          ) : (
            <DeleteAccount />
          )}
        </div>
      </section>
    </>
  );
}
