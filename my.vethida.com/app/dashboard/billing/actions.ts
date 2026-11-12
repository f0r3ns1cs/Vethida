"use server";

import { headers } from "next/headers";
import { redirect } from "next/navigation";
import { and, eq, isNull } from "drizzle-orm";
import {
  CANCELLATION_REASONS,
  cancellationRequest,
  db,
  isUuid,
  polarCustomer,
  type CancellationReason,
} from "@vethida/db";
import { auth } from "@/lib/auth";
import { getPolar } from "@/lib/polar";

// Polar customer portal. Uses the stored customer id, or our user id as the
// external id if the webhook hasn't stored one yet.
export async function manageBillingAction() {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  const [mapped] = await db
    .select({ customerId: polarCustomer.polarCustomerId })
    .from(polarCustomer)
    .where(eq(polarCustomer.userId, session.user.id))
    .limit(1);

  const polar = getPolar();
  let portalUrl: string;
  try {
    const created = mapped
      ? await polar.customerSessions.create({ customerId: mapped.customerId })
      : await polar.customerSessions.create({
          externalCustomerId: session.user.id,
        });
    portalUrl = created.customerPortalUrl;
  } catch (err) {
    console.error("[billing] customer portal session failed", err);
    redirect("/dashboard/billing?portal=unavailable");
  }
  redirect(portalUrl);
}

function isReason(v: unknown): v is CancellationReason {
  return (
    typeof v === "string" &&
    (CANCELLATION_REASONS as readonly string[]).includes(v)
  );
}

// Optional "why did you cancel" form, shown only AFTER the cancellation is done.
// Don't move it before the cancel button (click-to-cancel rules).
export async function submitCancellationFeedbackAction(formData: FormData) {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  const requestId = formData.get("requestId");
  const reason = formData.get("reason");
  const rawComment = formData.get("comment");

  // bad input just goes back to billing (isUuid avoids a 22P02 throw)
  if (!isUuid(requestId) || !isReason(reason)) {
    redirect("/dashboard/billing");
  }

  const comment =
    typeof rawComment === "string" && rawComment.trim()
      ? rawComment.trim().slice(0, 2000)
      : null;

  // owner only, and only once
  await db
    .update(cancellationRequest)
    .set({ reason, comment, feedbackAt: new Date() })
    .where(
      and(
        eq(cancellationRequest.id, requestId),
        eq(cancellationRequest.userId, session.user.id),
        isNull(cancellationRequest.feedbackAt),
      ),
    );

  // not sent to Polar, theirs stays 'other'
  redirect("/dashboard/billing?thanks=1");
}
