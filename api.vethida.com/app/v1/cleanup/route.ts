import { NextResponse, type NextRequest } from "next/server";
import { lt } from "drizzle-orm";
import {
  db,
  purgeExpiredDesktopAuthSessions,
  session,
  verification,
  webhookEvent,
} from "@vethida/db";
import { apiError, ERROR_CODES } from "@/lib/errors";
import { bearerTokenMatches } from "@vethida/http/bearer";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

// Cron (CRON_SECRET bearer): deletes expired rows from webhook_event, session,
// verification and desktop_auth_session. Safe to run twice. Expiry is enforced
// on read everywhere, this just frees the space.
export async function POST(req: NextRequest) {
  if (!bearerTokenMatches(req.headers.get("authorization"), process.env.CRON_SECRET)) {
    return apiError(401, ERROR_CODES.UNAUTHORIZED, "Invalid cron token.");
  }

  const now = new Date();
  try {
    const [webhooks, sessions, verifications, desktopAuth] = await Promise.all([
      db.delete(webhookEvent).where(lt(webhookEvent.expiresAt, now)).returning({ id: webhookEvent.eventId }),
      db.delete(session).where(lt(session.expiresAt, now)).returning({ id: session.id }),
      db.delete(verification).where(lt(verification.expiresAt, now)).returning({ id: verification.id }),
      purgeExpiredDesktopAuthSessions(),
    ]);

    const purged = {
      webhook_event: webhooks.length,
      session: sessions.length,
      verification: verifications.length,
      desktop_auth_session: desktopAuth,
    };
    return NextResponse.json({ ok: true, purged });
  } catch (err) {
    console.error("[cleanup] failed", err);
    return apiError(500, ERROR_CODES.INTERNAL_ERROR, "Cleanup failed.");
  }
}
