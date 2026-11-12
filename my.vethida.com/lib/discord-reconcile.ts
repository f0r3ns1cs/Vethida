import { eq } from "drizzle-orm";
import { account, db, isLicensed } from "@vethida/db";
import { syncCustomerRole, syncTierRoles } from "./discord";
import { getEntitlement } from "./licenses";

// Periodic sweep (cron) that resets Discord roles to match licenses, since
// licenses expiring doesn't fire any event. DB -> Discord only.

export interface ReconcileResult {
  checked: number;
  corrected: number;
  failed: number;
  truncated: boolean; // stopped at limit
}

// limit because of Discord rate limits (2-4 calls per member)
export async function reconcileDiscordRoles(
  options: { limit?: number; dryRun?: boolean } = {},
): Promise<ReconcileResult> {
  const limit = options.limit ?? 500;

  const linked = await db
    .select({ userId: account.userId, discordUserId: account.accountId })
    .from(account)
    .where(eq(account.providerId, "discord"))
    .limit(limit + 1);

  const truncated = linked.length > limit;
  const batch = truncated ? linked.slice(0, limit) : linked;

  let corrected = 0;
  let failed = 0;

  for (const row of batch) {
    try {
      const entitlement = await getEntitlement(row.userId);
      const entitled = isLicensed(entitlement);

      if (options.dryRun) {
        corrected += 1;
        continue;
      }

      // both idempotent, no need to diff first
      const roleOk = await syncCustomerRole(row.discordUserId, entitled);
      const tiersOk = await syncTierRoles(row.discordUserId, entitlement.tier);
      if (roleOk && tiersOk) {
        corrected += 1;
      } else {
        console.error(
          `[discord-reconcile] could not sync ${row.discordUserId}: customer role ${roleOk ? "ok" : "FAILED"}, tier roles ${tiersOk ? "ok" : "FAILED"}`,
        );
        failed += 1;
      }
    } catch (err) {
      // usually someone who left the server, keep going
      console.error(
        "[discord-reconcile] sync failed for a linked account",
        err,
      );
      failed += 1;
    }
  }

  return { checked: batch.length, corrected, failed, truncated };
}

