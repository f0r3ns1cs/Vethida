import Link from "next/link";
import { headers } from "next/headers";
import { redirect } from "next/navigation";
import { and, eq } from "drizzle-orm";
import { db, isUuid, license, tierLabel } from "@vethida/db";

import { auth } from "@/lib/auth";
import { formatDate } from "@/lib/format";
import { cancelSubscriptionAction } from "./actions";

export const dynamic = "force-dynamic";
export const metadata = { title: "Cancel your plan" };

// Keep this one button, no extra steps or questions (click-to-cancel rules).
export default async function CancelPage({
  searchParams,
}: {
  searchParams: Promise<{ license?: string; error?: string }>;
}) {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  const { license: licenseId, error } = await searchParams;
  if (!licenseId || !isUuid(licenseId)) redirect("/dashboard/billing");

  const [lic] = await db
    .select()
    .from(license)
    .where(and(eq(license.id, licenseId), eq(license.userId, session.user.id)))
    .limit(1);

  if (!lic || !lic.polarSubscriptionId || lic.cancelAtPeriodEnd) {
    redirect("/dashboard/billing");
  }

  const plan = tierLabel(lic.tier);
  const endsOn = lic.expiresAt ? formatDate(lic.expiresAt) : null;

  return (
    <>
      <header className="page-header">
        <h1 className="page-title">Cancel {plan}</h1>
      </header>

      {error ? (
        <div role="alert" className="alert alert-danger mb-4">
          <div>
            <p className="alert-title">Nothing was canceled</p>
            <p className="mt-1 text-foreground-muted">
              We couldn&apos;t reach our payment provider, so your subscription
              is unchanged and you have not been charged anything extra. Try
              again, or email support and we&apos;ll cancel it by hand.
            </p>
          </div>
        </div>
      ) : null}

      <section className="card p-6">
        <p className="text-sm leading-relaxed text-foreground-muted">
          Your {plan} access continues
          {endsOn ? ` until ${endsOn}` : " to the end of the term you have already paid for"}
          , then it ends. You will not be charged again.
        </p>
        <p className="mt-3 text-sm leading-relaxed text-foreground-muted">
          Changes you have already applied to your PC stay applied. The unused
          part of the current term is not refunded, except where the law
          requires it.
        </p>

        <form action={cancelSubscriptionAction} className="mt-6">
          <input type="hidden" name="licenseId" value={lic.id} />
          <button type="submit" className="btn btn-danger h-9 px-4">
            Cancel subscription
          </button>
        </form>
      </section>

      <p className="mt-4 text-sm">
        <Link
          href="/dashboard/billing"
          className="text-foreground-muted underline-offset-4 hover:text-foreground hover:underline"
        >
          Back to billing
        </Link>
      </p>
    </>
  );
}
