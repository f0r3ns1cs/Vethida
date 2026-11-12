"use server";

import { redirect } from "next/navigation";
import { headers } from "next/headers";
import { revalidatePath } from "next/cache";
import { auth } from "@/lib/auth";
import { syncDiscordForUser } from "@/lib/discord-link";

export type SyncDiscordResult = { ok: boolean };

// returns whether Discord actually accepted the role changes
export async function syncDiscordAction(): Promise<SyncDiscordResult> {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");
  const ok = await syncDiscordForUser(session.user.id);
  revalidatePath("/dashboard/settings");
  return { ok };
}
