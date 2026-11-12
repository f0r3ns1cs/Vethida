import { NextResponse, type NextRequest } from "next/server";
import { bearerTokenMatches } from "@vethida/http/bearer";
import { findHardDeleteEligible, hardDeleteUser } from "@/lib/account-deletion";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

// Hard-deletes accounts past the 30 day grace period (GDPR Art. 17). Dry run
// unless HARD_DELETE_ENABLED=true. Called by the systemd timer with the CRON_SECRET bearer.
export async function GET(req: NextRequest) {
  if (!bearerTokenMatches(req.headers.get("authorization"), process.env.CRON_SECRET)) {
    return NextResponse.json({ error: "Unauthorized." }, { status: 401 });
  }

  // otherwise Next returns an empty 500
  try {
    const eligible = await findHardDeleteEligible();

    if (process.env.HARD_DELETE_ENABLED !== "true") {
      console.log(
        `[delete-accounts] DRY RUN - ${eligible.length} account(s) eligible for hard-delete`,
      );
      return NextResponse.json({ mode: "dry-run", eligible: eligible.length });
    }

    let deleted = 0;
    const failed: string[] = [];
    for (const userId of eligible) {
      try {
        await hardDeleteUser(userId);
        deleted += 1;
      } catch (err) {
        console.error("[delete-accounts] failed to hard-delete a user", err);
        failed.push(userId);
      }
    }
    return NextResponse.json({ mode: "live", deleted, failed: failed.length });
  } catch (err) {
    console.error("[delete-accounts] run failed", err);
    return NextResponse.json({ error: "Run failed." }, { status: 500 });
  }
}
