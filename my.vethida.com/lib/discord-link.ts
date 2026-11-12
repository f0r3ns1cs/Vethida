import { and, eq } from "drizzle-orm";
import { account, db, isLicensed } from "@vethida/db";
import { addToGuild, syncCustomerRole, syncTierRoles } from "./discord";
import { getEntitlement } from "./licenses";

export interface LinkedDiscord {
  discordUserId: string;
  accessToken: string | null;
}

export async function getLinkedDiscord(
  userId: string,
): Promise<LinkedDiscord | null> {
  const [row] = await db
    .select({
      accountId: account.accountId,
      accessToken: account.accessToken,
    })
    .from(account)
    .where(and(eq(account.userId, userId), eq(account.providerId, "discord")))
    .limit(1);
  if (!row) return null;
  return { discordUserId: row.accountId, accessToken: row.accessToken };
}

// joins the guild if we have a token, then syncs roles. Trials count as licensed.
export async function syncDiscordForUser(userId: string): Promise<boolean> {
  const linked = await getLinkedDiscord(userId);
  if (!linked) return false;

  if (linked.accessToken) {
    // result ignored, they might have left on purpose
    await addToGuild(linked.discordUserId, linked.accessToken);
  }
  const entitlement = await getEntitlement(userId);
  const roleOk = await syncCustomerRole(
    linked.discordUserId,
    isLicensed(entitlement),
  );
  // no-op until the role ids are set
  const tiersOk = await syncTierRoles(linked.discordUserId, entitlement.tier);

  if (!roleOk || !tiersOk) {
    console.error(
      `[discord-link] role sync incomplete for user ${userId}: customer role ${roleOk ? "ok" : "FAILED"}, tier roles ${tiersOk ? "ok" : "FAILED"}`,
    );
    return false;
  }
  return true;
}
