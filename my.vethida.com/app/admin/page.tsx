import { desc, eq } from "drizzle-orm";
import { db, license, user, userSuspension } from "@vethida/db";
import { requireAdmin } from "@/lib/admin";
import {
  liftSuspensionsAction,
  lookupUserAction,
  mintLicenseAction,
  revokeLicenseAction,
  suspendUserAction,
  unrevokeLicenseAction,
} from "./actions";

export const dynamic = "force-dynamic";
export const metadata = { title: "Admin: user lookup" };

// by id so the email never ends up in the URL
async function loadUser(userId: string) {
  const [account] = await db
    .select()
    .from(user)
    .where(eq(user.id, userId))
    .limit(1);
  if (!account) return null;

  const [licenses, suspensions] = await Promise.all([
    db
      .select()
      .from(license)
      .where(eq(license.userId, account.id))
      .orderBy(desc(license.createdAt)),
    db
      .select()
      .from(userSuspension)
      .where(eq(userSuspension.userId, account.id))
      .orderBy(desc(userSuspension.createdAt)),
  ]);

  return { account, licenses, suspensions };
}

export default async function AdminLookupPage({
  searchParams,
}: {
  searchParams: Promise<{ u?: string; notfound?: string; error?: string }>;
}) {
  await requireAdmin();

  const { u, notfound, error } = await searchParams;
  const result = u ? await loadUser(u) : null;
  const activeSuspensions =
    result?.suspensions.filter((s) => !s.liftedAt) ?? [];

  return (
    <div className="space-y-6">
      <header className="page-header">
        <p className="page-pretitle">Admin</p>
        <h1 className="page-title">User lookup</h1>
        <p className="page-subtitle">
          Find a customer by email, then act on their account or licenses.
        </p>
      </header>
      <form action={lookupUserAction} className="flex gap-2">
        <input
          name="email"
          type="email"
          autoComplete="off"
          spellCheck={false}
          placeholder="user@example.com"
          required
          className="input max-w-sm"
          aria-label="Customer email address"
        />
        <button type="submit" className="btn btn-primary shrink-0">
          Look up
        </button>
      </form>

      {notfound && !result ? (
        <p className="text-sm text-foreground-muted">
          No user with that email.
        </p>
      ) : null}

      {error === "perpetual" ? (
        <p role="alert" className="text-sm text-danger">
          That term is not available. Nothing was
          minted. Pick a duration, or switch the tier to free.
        </p>
      ) : null}

      {result ? (
        <>
          <section className="card p-6">
            <div className="mb-4 flex items-start justify-between gap-4">
              <div>
                <p className="text-sm font-semibold">{result.account.email}</p>
                <p className="mt-1 text-xs text-foreground-faint">
                  {result.account.id} - {result.account.role} - joined{" "}
                  {result.account.createdAt.toISOString().slice(0, 10)}
                  {result.account.emailVerified ? "" : " - email unverified"}
                </p>
              </div>
              {activeSuspensions.length > 0 ? (
                <span className="shrink-0 text-xs font-medium text-danger">
                  Suspended
                </span>
              ) : null}
            </div>

            <div className="flex flex-wrap items-end gap-2">
              {activeSuspensions.length > 0 ? (
                <form action={liftSuspensionsAction}>
                  <input
                    type="hidden"
                    name="userId"
                    value={result.account.id}
                  />
                  <button type="submit" className="btn btn-secondary">
                    Lift all suspensions
                  </button>
                </form>
              ) : (
                <form
                  action={suspendUserAction}
                  className="flex items-end gap-2"
                >
                  <input
                    type="hidden"
                    name="userId"
                    value={result.account.id}
                  />
                  <input
                    name="reasonDetail"
                    type="text"
                    placeholder="Reason (internal note)"
                    maxLength={256}
                    className="input w-64"
                  />
                  <button type="submit" className="btn btn-secondary shrink-0">
                    Suspend
                  </button>
                </form>
              )}
            </div>
          </section>

          <section className="card p-6">
            <h2 className="mb-3 text-sm font-semibold">
              Licenses ({result.licenses.length})
            </h2>
            {result.licenses.length === 0 ? (
              <p className="text-sm text-foreground-muted">None.</p>
            ) : (
              <ul className="space-y-4 text-sm">
                {result.licenses.map((lic) => (
                  <li key={lic.id} className="space-y-2">
                    <div className="flex items-center justify-between gap-4">
                      <span className="font-mono">{lic.key}</span>
                      <span className="text-xs text-foreground-faint">
                        {lic.tier}
                        {lic.revokedAt
                          ? ` - revoked (${lic.revokedReasonCode})`
                          : ""}{" "}
                        -{" "}
                        {lic.expiresAt
                          ? `expires ${lic.expiresAt.toISOString().slice(0, 10)}`
                          : "no expiry"}{" "}
                        - {lic.machineResetsRemaining} resets
                      </span>
                    </div>
                    {lic.revokedAt ? (
                      <form action={unrevokeLicenseAction}>
                        <input type="hidden" name="licenseId" value={lic.id} />
                        <button
                          type="submit"
                          className="btn btn-secondary h-8 px-3"
                        >
                          Unrevoke
                        </button>
                      </form>
                    ) : (
                      <form
                        action={revokeLicenseAction}
                        className="flex flex-wrap items-center gap-2"
                      >
                        <input type="hidden" name="licenseId" value={lic.id} />
                        <select
                          name="reasonCode"
                          required
                          defaultValue=""
                          className="input h-8 w-40"
                        >
                          <option value="" disabled>
                            Reason
                          </option>
                          <option value="chargeback">chargeback</option>
                          <option value="tos_violation">tos_violation</option>
                          <option value="fraud">fraud</option>
                          <option value="user_request">user_request</option>
                          <option value="admin_error">admin_error</option>
                          <option value="other">other</option>
                        </select>
                        <input
                          name="reason"
                          type="text"
                          placeholder="Internal note"
                          maxLength={256}
                          className="input h-8 w-56"
                        />
                        <button
                          type="submit"
                          className="btn btn-secondary h-8 shrink-0 px-3"
                        >
                          Revoke
                        </button>
                      </form>
                    )}
                  </li>
                ))}
              </ul>
            )}

            <form
              action={mintLicenseAction}
              className="mt-5 flex items-center gap-2 border-t border-border pt-4"
            >
              <input type="hidden" name="userId" value={result.account.id} />
              <select
                name="tier"
                required
                defaultValue=""
                className="input h-8 w-32"
              >
                <option value="" disabled>
                  Tier
                </option>
                <option value="pro">pro</option>
                <option value="ultimate">ultimate</option>
              </select>
              {/* required, no term = lifetime */}
              <select
                name="term"
                required
                defaultValue=""
                className="input h-8 w-40"
              >
                <option value="" disabled>
                  Term
                </option>
                <option value="30">30 days</option>
                <option value="90">90 days</option>
                <option value="365">365 days</option>
                <option value="perpetual">Lifetime</option>
              </select>
              <button
                type="submit"
                className="btn btn-secondary h-8 shrink-0 px-3"
              >
                Mint license
              </button>
            </form>
          </section>

          <section className="card p-6">
            <h2 className="mb-3 text-sm font-semibold">
              Suspension history ({result.suspensions.length})
            </h2>
            {result.suspensions.length === 0 ? (
              <p className="text-sm text-foreground-muted">None.</p>
            ) : (
              <ul className="space-y-2 text-sm">
                {result.suspensions.map((s) => (
                  <li key={s.id}>
                    <span
                      className={
                        s.liftedAt ? "text-foreground-faint" : "text-danger"
                      }
                    >
                      {s.reasonCode}
                    </span>{" "}
                    <span className="text-xs text-foreground-faint">
                      via {s.source} on {s.createdAt.toISOString().slice(0, 10)}
                      {s.liftedAt
                        ? `; lifted ${s.liftedAt.toISOString().slice(0, 10)}`
                        : " (active)"}
                      {s.reasonDetail ? ` - ${s.reasonDetail}` : ""}
                    </span>
                  </li>
                ))}
              </ul>
            )}
          </section>
        </>
      ) : null}
    </div>
  );
}
