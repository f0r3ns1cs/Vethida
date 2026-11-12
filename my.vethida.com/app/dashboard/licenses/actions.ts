"use server";

import { redirect } from "next/navigation";
import { headers } from "next/headers";
import { revalidatePath } from "next/cache";
import { auth } from "@/lib/auth";
import { formatLicenseKey, TRIAL_DAYS } from "@vethida/db";
import { claimLicense, deactivateOwnedDevice, startTrial } from "@/lib/licenses";
import { sendEmail } from "@/lib/email";
import { renderTemplate } from "@/lib/email/render";
import { legalEntityAddress } from "@/lib/email/identity";
import { formatDate } from "@/lib/format";

export async function deactivateDeviceAction(formData: FormData) {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  const deviceId = formData.get("deviceId");
  if (typeof deviceId !== "string" || !deviceId) return;

  await deactivateOwnedDevice(session.user.id, deviceId);
  revalidatePath("/dashboard/licenses");
}

export interface ClaimState {
  error: string | null;
}

export async function claimLicenseAction(
  _prev: ClaimState,
  formData: FormData,
): Promise<ClaimState> {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  const rawKey = formData.get("licenseKey");
  if (typeof rawKey !== "string" || !rawKey.trim()) {
    return { error: "Enter a license key." };
  }

  const result = await claimLicense(session.user.id, rawKey.trim());
  if (result === "not_claimable") {
    return { error: "That key is invalid or already claimed." };
  }

  revalidatePath("/dashboard/licenses");
  revalidatePath("/dashboard");
  return { error: null };
}

export interface TrialState {
  error: string | null;
}

// email failures are ignored, the key shows on the licenses page anyway
export async function startTrialAction(): Promise<TrialState> {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  const result = await startTrial(session.user.id);
  if (!result.ok) {
    return {
      error:
        result.reason === "already_used"
          ? "This account has already used its trial."
          : "This account already has a license.",
    };
  }

  const rendered = renderTemplate("trial-started", {
    customerFirstName: session.user.name?.split(" ")[0] ?? "there",
    trialDays: String(TRIAL_DAYS),
    licenseKey: formatLicenseKey(result.key),
    expiryDate: formatDate(result.expiresAt),
    legalEntityAddress: legalEntityAddress(),
  });
  await sendEmail({ to: session.user.email, ...rendered }).catch(() => {});

  revalidatePath("/dashboard/licenses");
  revalidatePath("/dashboard");
  revalidatePath("/dashboard/download-center");
  return { error: null };
}
