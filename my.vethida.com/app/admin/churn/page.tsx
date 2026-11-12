import { desc, sql } from "drizzle-orm";
import { cancellationRequest, db } from "@vethida/db";

import { requireAdmin } from "@/lib/admin";
import { reasonLabelFor } from "@/lib/retention";

export const dynamic = "force-dynamic";
export const metadata = { title: "Admin: churn" };

// reason is 'other' unless the customer answered afterwards

export default async function AdminChurnPage() {
  await requireAdmin();

  const byTier = await db
    .select({
      tier: cancellationRequest.tier,
      total: sql<number>`count(*)::int`,
      answered: sql<number>`count(*) filter (where feedback_at is not null)::int`,
    })
    .from(cancellationRequest)
    .where(sql`outcome = 'canceled'`)
    .groupBy(cancellationRequest.tier)
    .orderBy(desc(sql`count(*)`));

  // answered only, unanswered rows are all 'other'
  const byReason = await db
    .select({
      reason: cancellationRequest.reason,
      total: sql<number>`count(*)::int`,
    })
    .from(cancellationRequest)
    .where(sql`outcome = 'canceled' and feedback_at is not null`)
    .groupBy(cancellationRequest.reason)
    .orderBy(desc(sql`count(*)`));

  const recent = await db
    .select({
      id: cancellationRequest.id,
      reason: cancellationRequest.reason,
      comment: cancellationRequest.comment,
      tier: cancellationRequest.tier,
      outcome: cancellationRequest.outcome,
      createdAt: cancellationRequest.createdAt,
      feedbackAt: cancellationRequest.feedbackAt,
    })
    .from(cancellationRequest)
    .orderBy(desc(cancellationRequest.createdAt))
    .limit(50);

  const total = byTier.reduce((acc, r) => acc + r.total, 0);
  const answered = byTier.reduce((acc, r) => acc + r.answered, 0);
  const responseRate =
    total > 0 ? Math.round((answered / total) * 100) : null;

  return (
    <div className="space-y-6">
      <header className="page-header">
        <p className="page-pretitle">Admin</p>
        <h1 className="page-title">Churn</h1>
        <p className="page-subtitle">
          Completed cancellations. There is no save rate: canceling is one button
          and makes no offer, by law.
        </p>
      </header>

      <section className="card p-6">
        <h2 className="text-sm font-semibold">By tier</h2>
        {byTier.length === 0 ? (
          <p className="mt-2 text-sm text-foreground-muted">Nothing yet.</p>
        ) : (
          <>
            <p className="mt-2 text-sm text-foreground-muted">
              <span className="text-lg font-semibold tabular-nums text-foreground">
                {total}
              </span>{" "}
              {total === 1 ? "cancellation" : "cancellations"} in total.
              {responseRate !== null ? (
                <>
                  {" "}
                  {answered} answered the optional question ({responseRate}%).
                </>
              ) : null}
            </p>
            <table className="mt-4 w-full text-left text-sm">
              <thead>
                <tr className="border-b border-border text-xs text-foreground-faint">
                  <th scope="col" className="pb-2 font-medium">
                    Tier
                  </th>
                  <th scope="col" className="pb-2 text-right font-medium">
                    Canceled
                  </th>
                </tr>
              </thead>
              <tbody>
                {byTier.map((row) => (
                  <tr
                    key={row.tier}
                    className="border-b border-border last:border-0"
                  >
                    <td className="py-2 text-foreground-muted">{row.tier}</td>
                    <td className="py-2 text-right tabular-nums">{row.total}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </>
        )}
      </section>

      <section className="card p-6">
        <h2 className="mb-1 text-sm font-semibold">By reason</h2>
        <p className="mb-3 text-xs text-foreground-muted">
          Answered cancellations only. Rows nobody replied to are excluded rather
          than counted as &ldquo;Something else&rdquo;.
        </p>
        {byReason.length === 0 ? (
          <p className="text-sm text-foreground-muted">
            No one has answered yet.
          </p>
        ) : (
          <table className="w-full text-left text-sm">
            <thead>
              <tr className="border-b border-border text-xs text-foreground-faint">
                <th scope="col" className="pb-2 font-medium">
                  Reason
                </th>
                <th scope="col" className="pb-2 text-right font-medium">
                  Count
                </th>
              </tr>
            </thead>
            <tbody>
              {byReason.map((row) => (
                <tr
                  key={row.reason}
                  className="border-b border-border last:border-0"
                >
                  <td className="py-2 text-foreground-muted">
                    {reasonLabelFor(row.reason, true)}
                  </td>
                  <td className="py-2 text-right tabular-nums">{row.total}</td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </section>

      <section className="card p-6">
        <h2 className="mb-3 text-sm font-semibold">Recent, with comments</h2>
        {recent.length === 0 ? (
          <p className="text-sm text-foreground-muted">Nothing yet.</p>
        ) : (
          <ul className="space-y-3 text-sm">
            {recent.map((row) => (
              <li
                key={row.id}
                className="border-b border-border pb-3 last:border-0"
              >
                <p className="text-xs text-foreground-faint">
                  {row.createdAt.toISOString().slice(0, 10)} - {row.tier} -{" "}
                  {row.outcome}
                </p>
                <p className="mt-1 text-foreground-muted">
                  {reasonLabelFor(row.reason, row.feedbackAt !== null)}
                </p>
                {row.comment ? (
                  <p className="mt-1 text-foreground">
                    &ldquo;{row.comment}&rdquo;
                  </p>
                ) : null}
              </li>
            ))}
          </ul>
        )}
      </section>
    </div>
  );
}
