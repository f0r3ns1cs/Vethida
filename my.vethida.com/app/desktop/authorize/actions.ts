"use server";

import { headers } from "next/headers";
import { redirect } from "next/navigation";
import {
  approveSession,
  denySession,
  findLiveSessionByUserCode,
  normalizeUserCode,
  selectableLicensesFor,
} from "@vethida/db";
import { auth } from "@/lib/auth";

// approve is a server action (POST), never a GET link

export interface AuthorizeState {
  error: string | null;
}

async function requireSession() {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");
  return session;
}

export async function approveDesktopAction(
  _prev: AuthorizeState,
  formData: FormData,
): Promise<AuthorizeState> {
  const session = await requireSession();

  const userCode = normalizeUserCode(String(formData.get("userCode") ?? ""));
  if (!userCode) return { error: "That code is not valid." };

  const requested = await findLiveSessionByUserCode(userCode);
  if (!requested) {
    return {
      error:
        "This sign-in request has expired. Start it again from the Vethida app.",
    };
  }

  // only from the approver's own licenses, don't trust the form id
  const licenses = await selectableLicensesFor(session.user.id);
  const requestedLicenseId = String(formData.get("licenseId") ?? "");
  const chosen =
    licenses.find((l) => l.id === requestedLicenseId) ?? licenses[0];
  if (!chosen) {
    return {
      error:
        "There is no usable license on this account. Buy or claim one first.",
    };
  }

  if (!(await approveSession({
    sessionId: requested.id,
    userId: session.user.id,
    licenseId: chosen.id,
  }))) {
    return {
      error:
        "This sign-in request expired or was already answered. Start it again from the Vethida app.",
    };
  }

  redirect("/desktop/authorize/done");
}

export async function denyDesktopAction(formData: FormData) {
  await requireSession();

  const userCode = normalizeUserCode(String(formData.get("userCode") ?? ""));
  if (!userCode) return;

  const requested = await findLiveSessionByUserCode(userCode);
  if (requested) await denySession(requested.id);

  redirect("/desktop/authorize/denied");
}
