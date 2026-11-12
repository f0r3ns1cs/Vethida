import { NextResponse, type NextRequest } from "next/server";
import { db, emailPreference } from "@vethida/db";
import { verifyUnsubscribeSignature } from "@/lib/unsubscribe";
import { clientIp, createRateLimiter } from "@/lib/rate-limit";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

const isLimited = createRateLimiter({ windowMs: 60 * 1000, max: 20 });

function page(title: string, body: string, status: number): NextResponse {
  return new NextResponse(
    `<!doctype html><html><head><meta charset="utf-8"><title>${title}</title></head>` +
      `<body style="font-family:system-ui,sans-serif;max-width:32rem;margin:4rem auto;padding:0 1rem">` +
      `<h1 style="font-size:1.25rem">${title}</h1><p>${body}</p></body></html>`,
    { status, headers: { "content-type": "text/html; charset=utf-8" } },
  );
}

export async function GET(req: NextRequest) {
  if (isLimited(clientIp(req))) {
    return page("Too many requests", "Please try again shortly.", 429);
  }

  const uid = req.nextUrl.searchParams.get("uid");
  const sig = req.nextUrl.searchParams.get("sig");
  if (!uid || !sig || !verifyUnsubscribeSignature(uid, sig)) {
    return page(
      "Invalid link",
      "This unsubscribe link is invalid or has expired. You can manage email preferences from your dashboard settings.",
      400,
    );
  }

  try {
    await db
      .insert(emailPreference)
      .values({ userId: uid, releaseAnnouncements: false })
      .onConflictDoUpdate({
        target: emailPreference.userId,
        set: { releaseAnnouncements: false, updatedAt: new Date() },
      });
  } catch (err) {
    // 23503 = user already deleted, which is fine
    const code =
      typeof err === "object" && err !== null && "code" in err
        ? String((err as { code?: unknown }).code)
        : "";
    if (code !== "23503") {
      console.error("[unsubscribe] could not record preference", err);
      return page(
        "We couldn't record that",
        "Something went wrong on our end and your preference was not saved, so you may still receive release announcements. Please try this link again shortly, or turn them off from your dashboard settings.",
        500,
      );
    }
  }

  return page(
    "Unsubscribed",
    "You will no longer receive release announcements. You can re-enable them any time from your dashboard settings.",
    200,
  );
}
