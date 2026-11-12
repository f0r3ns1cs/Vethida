"use server";

import { headers } from "next/headers";
import { redirect } from "next/navigation";
import { revalidatePath } from "next/cache";
import { and, eq } from "drizzle-orm";
import { cancellationRequest, db, isUuid, license } from "@vethida/db";

import { auth } from "@/lib/auth";
import { getPolar } from "@/lib/polar";

// One-step cancel (has to be as easy as subscribing).
export async function cancelSubscriptionAction(formData: FormData) {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  const licenseId = formData.get("licenseId");
  if (typeof licenseId !== "string" || !isUuid(licenseId)) {
    redirect("/dashboard/billing");
  }

  // scoped to the session user
  const [lic] = await db
    .select()
    .from(license)
    .where(and(eq(license.id, licenseId), eq(license.userId, session.user.id)))
    .limit(1);

  // not theirs / not a subscription / already canceling
  if (!lic || !lic.polarSubscriptionId || lic.cancelAtPeriodEnd) {
    redirect("/dashboard/billing");
  }
  const subscriptionId = lic.polarSubscriptionId;

  try {
    await getPolar().subscriptions.update({
      id: subscriptionId,
      subscriptionUpdate: {
        cancelAtPeriodEnd: true, // they keep the term they paid for
        customerCancellationReason: "other", // Polar needs one of its enum values
      },
    });
  } catch (err) {
    console.error(`[cancel] Polar cancel failed for ${subscriptionId}`, err);
    // don't record anything locally if Polar didn't accept it
    redirect(`/dashboard/billing/cancel?license=${lic.id}&error=1`);
  }

  // churn row for /admin/churn, reason gets filled in later if they answer
  let requestId: string | null = null;
  try {
    const [created] = await db
      .insert(cancellationRequest)
      .values({
        userId: session.user.id,
        licenseId: lic.id,
        polarSubscriptionId: subscriptionId,
        tier: lic.tier,
        reason: "other",
        outcome: "canceled",
        resolvedAt: new Date(),
      })
      .returning({ id: cancellationRequest.id });
    requestId = created?.id ?? null;
  } catch (err) {
    // the cancel already went through, don't fail over a missing analytics row
    console.error("[cancel] failed to record churn row", err);
  }

  revalidatePath("/dashboard/billing");
  redirect(
    requestId
      ? `/dashboard/billing?canceled=${requestId}`
      : "/dashboard/billing?canceled=1",
  );
}
