import { desc, eq, isNull } from "drizzle-orm";
import { abuseFlag, db, describeAbuseFlag, device, license, user } from "@vethida/db";
import { requireAdmin } from "@/lib/admin";
import { resolveFlagAction } from "../actions";

export const dynamic = "force-dynamic";
export const metadata = { title: "Admin: abuse flags" };

export default async function AdminFlagsPage() {
  await requireAdmin();

  const flags = await db
    .select({
      flag: abuseFlag,
      userEmail: user.email,
      licenseKey: license.key,
      devicePubkeyId: device.devicePubkeyId,
    })
    .from(abuseFlag)
    .leftJoin(user, eq(abuseFlag.userId, user.id))
    .leftJoin(license, eq(abuseFlag.licenseId, license.id))
    .leftJoin(device, eq(abuseFlag.deviceId, device.id))
    .where(isNull(abuseFlag.resolvedAt))
    .orderBy(desc(abuseFlag.createdAt))
    .limit(100);

  return (
    <div className="space-y-6">
      <header className="page-header">
        <p className="page-pretitle">Admin</p>
        <h1 className="page-title">Flags</h1>
        <p className="page-subtitle">
          Unresolved alerts raised by the licensing and billing paths.
        </p>
      </header>
      <h2 className="text-sm font-semibold">
        Unresolved flags ({flags.length})
      </h2>

      {flags.length === 0 ? (
        <p className="card p-6 text-sm text-foreground-muted">
          Nothing to triage.
        </p>
      ) : (
        <ul className="space-y-3">
          {flags.map(({ flag, userEmail, licenseKey, devicePubkeyId }) => {
            const meta = describeAbuseFlag(flag.type);
            return (
            <li
              key={flag.id}
              className="card flex items-start justify-between gap-4 p-5"
            >
              <div className="min-w-0 space-y-1 text-sm">
                <p className="font-medium">
                  {meta.label}
                  <span className="ml-2 text-xs font-normal text-foreground-muted">
                    {meta.severity}
                    {meta.known ? "" : " - unknown type"}
                  </span>
                </p>
                <p className="text-xs text-foreground-muted">
                  {userEmail ?? "no account"}
                  {licenseKey ? (
                    <>
                      {" - "}
                      <span className="font-mono">{licenseKey}</span>
                    </>
                  ) : null}
                  {devicePubkeyId
                    ? ` - device ${devicePubkeyId.slice(0, 12)}`
                    : ""}
                </p>
                <p className="text-xs text-foreground-faint">
                  <span className="font-mono">{flag.type}</span>
                  {" - "}
                  {flag.createdAt.toISOString().replace("T", " ").slice(0, 16)}
                  {flag.details ? ` - ${JSON.stringify(flag.details)}` : ""}
                </p>
                <p className="text-xs text-foreground-muted">
                  {meta.recommendedAction}
                </p>
              </div>
              <form action={resolveFlagAction}>
                <input type="hidden" name="flagId" value={flag.id} />
                <button
                  type="submit"
                  className="btn btn-secondary h-8 shrink-0 px-3"
                >
                  Resolve
                </button>
              </form>
            </li>
            );
          })}
        </ul>
      )}
    </div>
  );
}
