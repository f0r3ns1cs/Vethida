"use server";

import { redirect } from "next/navigation";
import { headers } from "next/headers";
import { revalidatePath } from "next/cache";
import { auth } from "@/lib/auth";
import { cancelAccountDeletion } from "@/lib/account-deletion";

export async function cancelDeletionAction() {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  await cancelAccountDeletion(session.user.id);
  revalidatePath("/dashboard/settings");
  revalidatePath("/dashboard");
}
