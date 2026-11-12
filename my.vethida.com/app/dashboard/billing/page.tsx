import Link from "next/link";
import { headers } from "next/headers";
import { redirect } from "next/navigation";
import { and, eq, isNull } from "drizzle-orm";
import { cancellationRequest, db, type License } from "@vethida/db";
import { auth } from "@/lib/auth";
import { CANCELLATION_REASON_LABELS } from "@/lib/retention";
import { allAvailableCadences, fetchProductPrices } from "@/lib/polar";
import {
  getLiveSubscribedTiers,
  getPerpetualTiers,
  getUserLicenses,
} from "@/lib/licenses";
import { formatDate } from "@/lib/format";
import { BuyLicense } from "@/components/buy-license";
import { PlanPill } from "@/components/plan-pill";
import {
  manageBillingAction,
  submitCancellationFeedbackAction,
} from "./actions";

// ?canceled= is a row id, or "1" if no row was written
const UUID_RE =
  /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;

export const dynamic = "force-dynamic";
export const metadata = { title: "Billing" };

// how it's held (the pill shows the tier)
function planStatus(lic: License): { label: string; detail: string } {
  // trial first or it looks like a fixed term
  if (lic.trial && lic.expiresAt) {
    return { label: "Trial", detail: `Ends ${formatDate(lic.expiresAt)}` };
  }
  if (lic.polarSubscriptionId && lic.expiresAt) {
    return lic.cancelAtPeriodEnd
      ? { label: "Canceled", detail: `Access ends ${formatDate(lic.expiresAt)}` }
      : { label: "Subscription", detail: `Renews ${formatDate(lic.expiresAt)}` };
  }
  if (lic.expiresAt) {
    return { label: "Fixed term", detail: `Expires ${formatDate(lic.expiresAt)}` };
  }
  return { label: "Lifetime", detail: "One payment, no renewal" };
}

export default async function BillingPage({
  searchParams,
}: {
  searchParams: Promise<{
    portal?: string;
    canceled?: string;
    thanks?: string;
  }>;
}) {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  const params = await searchParams;
  const portalUnavailable = params.portal === "unavailable";
  const justCancelled = Boolean(params.canceled);
  const thanked = params.thanks === "1";

  // unanswered cancellation row for this user, if any
  const feedbackTarget =
    params.canceled && UUID_RE.test(params.canceled)
      ? ((
          await db
            .select({ id: cancellationRequest.id })
            .from(cancellationRequest)
            .where(
              and(
                eq(cancellationRequest.id, params.canceled),
                eq(cancellationRequest.userId, session.user.id),
                isNull(cancellationRequest.feedbackAt),
              ),
            )
            .limit(1)
        )[0]?.id ?? null)
      : null;
  const paid = (await getUserLicenses(session.user.id)).filter(
    ({ license: lic }) => !lic.revokedAt,
  );

  const ownedTiers = await getLiveSubscribedTiers(session.user.id);
  const perpetualTiers = await getPerpetualTiers(session.user.id);
  const prices = await fetchProductPrices();
  const cadences = allAvailableCadences();

  return (
    <>
      <header className="page-header">
        <h1 className="page-title">Billing</h1>
        <p className="page-subtitle">
          Your plan, your renewal date, and where to change either. Payments are
          handled by Polar, our merchant of record.
        </p>
      </header>

      {thanked ? (
        <div role="status" className="alert alert-success mb-4">
          <p>Thanks - that is genuinely useful.</p>
        </div>
      ) : null}

      {justCancelled ? (
        <div role="status" className="alert alert-warning mb-4">
          <div>
            <p className="alert-title">Subscription canceled</p>
            <p className="mt-1 text-foreground-muted">
              Your access runs to the end of the term you have paid for, and
              everything already applied to your PC stays applied. You can turn
              auto-renew back on any time before then.
            </p>
          </div>
        </div>
      ) : null}

      {/* only after the cancel is done, never before it */}
      {feedbackTarget ? (
        <section className="card mb-4 p-6">
          <h2 className="text-sm font-semibold">
            One optional question: what made you cancel?
          </h2>
          <p className="mt-1 text-xs text-foreground-muted">
            Your subscription is already canceled. This changes nothing and you
            can ignore it.
          </p>

          <form action={submitCancellationFeedbackAction} className="mt-4">
            <input type="hidden" name="requestId" value={feedbackTarget} />

            <fieldset>
              <legend className="sr-only">Reason for canceling</legend>
              <div className="space-y-2">
                {CANCELLATION_REASON_LABELS.map((reason) => (
                  <label
                    key={reason.id}
                    className="flex cursor-pointer gap-3 rounded-md border border-border bg-background p-3 transition-colors hover:border-border-strong"
                  >
                    <input
                      type="radio"
                      name="reason"
                      value={reason.id}
                      required
                      className="mt-0.5 shrink-0"
                    />
                    <span className="text-sm text-foreground">
                      {reason.label}
                    </span>
                  </label>
                ))}
              </div>
            </fieldset>

            <div className="mt-4">
              <label
                htmlFor="cancel-comment"
                className="block text-sm font-semibold"
              >
                Anything else?{" "}
                <span className="font-normal text-foreground-muted">
                  (optional)
                </span>
              </label>
              <textarea
                id="cancel-comment"
                name="comment"
                rows={3}
                maxLength={2000}
                spellCheck
                className="textarea mt-2"
              />
            </div>

            <div className="mt-4 flex flex-wrap items-center gap-3">
              <button type="submit" className="btn btn-secondary h-9 px-4">
                Send
              </button>
              <Link
                href="/dashboard/billing"
                className="text-sm text-foreground-muted underline-offset-4 hover:text-foreground hover:underline"
              >
                No thanks
              </Link>
            </div>
          </form>
        </section>
      ) : null}

      {paid.length > 0 ? (
        <section className="card mb-4">
          <div className="card-header">
            <h2 className="card-title">Your plan</h2>
          </div>
          <ul>
            {paid.map(({ license: lic }, i) => {
              const status = planStatus(lic);
              const cancellable =
                Boolean(lic.polarSubscriptionId) && !lic.cancelAtPeriodEnd;
              return (
                <li
                  key={lic.id}
                  className={`flex flex-wrap items-center justify-between gap-4 px-5 py-4 ${
                    i > 0 ? "border-t border-border" : ""
                  }`}
                >
                  <div className="min-w-0">
                    <div className="flex items-center gap-2.5">
                      <PlanPill tier={lic.tier} trial={lic.trial} />
                      <p className="text-sm font-semibold">{status.label}</p>
                      {lic.cancelAtPeriodEnd ? (
                        <span className="badge badge-warning">Ending</span>
                      ) : (
                        <span className="badge badge-success">Active</span>
                      )}
                    </div>
                    <p className="mt-1 text-xs text-foreground-muted">
                      {status.detail}
                    </p>
                  </div>
                  <div className="flex shrink-0 items-center gap-2">
                    <Link
                      href={`/dashboard/licenses/${lic.id}`}
                      className="btn btn-secondary btn-sm"
                    >
                      Details
                    </Link>
                    {cancellable ? (
                      <Link
                        href={`/dashboard/billing/cancel?license=${lic.id}`}
                        className="btn btn-secondary btn-sm text-foreground-muted"
                      >
                        Cancel
                      </Link>
                    ) : null}
                  </div>
                </li>
              );
            })}
          </ul>
        </section>
      ) : (
        <section className="card mb-4">
          <div className="empty">
            <p className="empty-title">No paid plan on this account</p>
            <p className="empty-text">
              Pick one below. Monthly and yearly renew until you cancel; a
              lifetime license is one payment and never expires.
            </p>
          </div>
        </section>
      )}

      <BuyLicense
        ownedTiers={ownedTiers}
        perpetualTiers={perpetualTiers}
        availableCadences={cadences}
        prices={prices}
      />

      <section className="card mt-4">
        <div className="card-header">
          <h2 className="card-title">Payment method &amp; invoices</h2>
        </div>
        <div className="card-body">
          <p className="text-sm leading-relaxed text-foreground-muted">
            Manage billing opens Polar&apos;s secure portal, where you can
            update your card, download invoices, or cancel a subscription.
            Canceling keeps your plan through the end of the paid term; the
            unused portion is not refunded. Need help?{" "}
            <a
              href="mailto:support@vethida.com"
              className="text-accent-on-surface underline-offset-4 hover:underline"
            >
              support@vethida.com
            </a>
            .
          </p>
          {portalUnavailable ? (
            <div role="alert" className="alert alert-warning mt-4">
              <p>
                The billing portal is unavailable right now. Use the manage link
                in your Polar receipt emails, or try again shortly.
              </p>
            </div>
          ) : null}
        </div>
        {paid.length > 0 ? (
          <div className="card-footer">
            <form action={manageBillingAction}>
              <button type="submit" className="btn btn-primary btn-sm">
                Manage billing
              </button>
            </form>
          </div>
        ) : null}
      </section>
    </>
  );
}
