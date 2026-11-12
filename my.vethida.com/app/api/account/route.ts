import { NextResponse, type NextRequest } from "next/server";
import { headers } from "next/headers";
import { auth } from "@/lib/auth";
import { requestAccountDeletion } from "@/lib/account-deletion";
import { clientIp, createRateLimiter } from "@/lib/rate-limit";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

const isLimited = createRateLimiter({ windowMs: 60 * 60 * 1000, max: 5 });

export async function DELETE(req: NextRequest) {
  try {
    const session = await auth.api.getSession({ headers: await headers() });
    if (!session) {
      return NextResponse.json({ error: "Sign in first." }, { status: 401 });
    }
    if (isLimited(clientIp(req))) {
      return NextResponse.json({ error: "Too many requests." }, { status: 429 });
    }

    const result = await requestAccountDeletion(session.user.id);
    return NextResponse.json(
      {
        status: "deletion_pending",
        hardDeleteScheduledFor: result.hardDeleteAt.toISOString(),
      },
      { status: result.alreadyPending ? 409 : 200 },
    );
  } catch (err) {
    // delete-account.tsx shows this message
    console.error("[account-delete] unhandled error", err);
    return NextResponse.json(
      {
        error:
          "We couldn't schedule the deletion. Nothing has been deleted. Please try again.",
      },
      { status: 500 },
    );
  }
}
