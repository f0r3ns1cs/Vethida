"use server";

import { redirect } from "next/navigation";
import { headers } from "next/headers";
import { revalidatePath } from "next/cache";
import { db, emailPreference } from "@vethida/db";
import { auth } from "@/lib/auth";

export async function updateEmailPreferencesAction(formData: FormData) {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  const fingerprintAlerts = formData.get("fingerprintAlerts") === "on";
  const releaseAnnouncements = formData.get("releaseAnnouncements") === "on";

  await db
    .insert(emailPreference)
    .values({
      userId: session.user.id,
      fingerprintAlerts,
      releaseAnnouncements,
    })
    .onConflictDoUpdate({
      target: emailPreference.userId,
      set: { fingerprintAlerts, releaseAnnouncements, updatedAt: new Date() },
    });

  revalidatePath("/dashboard/settings");
}

// Takes the session id, not the token (tokens shouldn't be in the page). The
// token is looked up from the caller's own sessions.
export async function revokeSessionAction(formData: FormData) {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  const sessionId = formData.get("sessionId");
  if (typeof sessionId !== "string" || !sessionId) return;

  const own = await auth.api.listSessions({ headers: await headers() });
  const target = own.find((s) => s.id === sessionId);
  if (!target) return;

  if (target.token === session.session.token) return;

  await auth.api.revokeSession({
    body: { token: target.token },
    headers: await headers(),
  });
  revalidatePath("/dashboard/settings");
}

export async function revokeOtherSessionsAction() {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  await auth.api.revokeOtherSessions({ headers: await headers() });
  revalidatePath("/dashboard/settings");
}
