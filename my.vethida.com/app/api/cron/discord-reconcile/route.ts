import { NextResponse, type NextRequest } from "next/server";
import { bearerTokenMatches } from "@vethida/http/bearer";
import { discordConfigured } from "@/lib/discord";
import { reconcileDiscordRoles } from "@/lib/discord-reconcile";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

// Re-syncs Discord roles for every linked account so expired licenses lose them.
// Called by the systemd timer with the CRON_SECRET bearer.
export async function GET(req: NextRequest) {
  if (!bearerTokenMatches(req.headers.get("authorization"), process.env.CRON_SECRET)) {
    return NextResponse.json({ error: "Unauthorized." }, { status: 401 });
  }

  if (!discordConfigured()) {
    return NextResponse.json(
      { skipped: "discord is not configured; no bot token" },
      { status: 200 },
    );
  }

  try {
    const result = await reconcileDiscordRoles();

    if (result.truncated) {
      console.warn(
        "[discord-reconcile] hit the per-run limit; the remainder waits for the next run",
      );
    }
    if (result.failed > 0) {
      console.warn(
        `[discord-reconcile] ${result.failed} account(s) failed to sync`,
      );
    }

    return NextResponse.json(result);
  } catch (err) {
    console.error("[discord-reconcile] run failed", err);
    return NextResponse.json({ error: "Run failed." }, { status: 500 });
  }
}
