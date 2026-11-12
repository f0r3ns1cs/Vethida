import { NextResponse, type NextRequest } from "next/server";
import { eq } from "drizzle-orm";
import { db, emailPreference, recordAdminAction, user } from "@vethida/db";
import { adminSession, requestIp } from "@/lib/admin";
import { sendEmail } from "@/lib/email";
import { legalEntityAddress } from "@/lib/email/identity";
import { renderTemplate } from "@/lib/email/render";
import { unsubscribeUrl } from "@/lib/unsubscribe";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

const FIELDS = [
  "version",
  "oneLineHighlight",
  "highlightList",
  "downloadUrl",
  "releaseNotesUrl",
  "binarySha256",
  "changelogBody",
  "hazardsBlock",
] as const;

type Payload = Record<(typeof FIELDS)[number], string>;

function firstName(email: string): string {
  const first = email.split("@")[0]?.split(/[._-]/)[0] || "there";
  return first.charAt(0).toUpperCase() + first.slice(1).toLowerCase();
}

// Admin only. Sends to users who opted in under settings#email.
export async function POST(req: NextRequest) {
  const session = await adminSession();
  if (!session) {
    return NextResponse.json({ error: "Forbidden." }, { status: 403 });
  }

  let body: Partial<Payload>;
  try {
    body = await req.json();
  } catch {
    return NextResponse.json({ error: "Invalid JSON body." }, { status: 400 });
  }
  for (const field of FIELDS) {
    if (typeof body[field] !== "string" || !body[field]) {
      return NextResponse.json(
        { error: `Missing field: ${field}` },
        { status: 400 },
      );
    }
  }
  // only copy known fields, don't pass the raw body to the template
  const payload = {} as Payload;
  for (const field of FIELDS) payload[field] = body[field] as string;

  const recipients = await db
    .select({ userId: user.id, email: user.email })
    .from(emailPreference)
    .innerJoin(user, eq(user.id, emailPreference.userId))
    .where(eq(emailPreference.releaseAnnouncements, true));

  let sent = 0;
  let failed = 0;
  for (const r of recipients) {
    try {
      const rendered = renderTemplate("release-announcement", {
        ...payload,
        customerFirstName: firstName(r.email),
        legalEntityAddress: legalEntityAddress(),
        unsubscribeUrl: unsubscribeUrl(r.userId),
        preferencesUrl: "https://my.vethida.com/dashboard/settings#email",
      });
      const ok = await sendEmail({ to: r.email, ...rendered });
      if (ok) sent += 1;
      else failed += 1;
    } catch (err) {
      console.error("[release-announcement] recipient failed", err);
      failed += 1;
    }
  }

  // emails already went out, so don't 500 if the audit insert fails
  try {
    await recordAdminAction({
      adminId: session.user.id,
      action: "release_announcement_sent",
      targetType: "broadcast",
      targetId: payload.version,
      ipAddress: await requestIp(),
      details: { recipients: recipients.length, sent, failed },
    });
  } catch (err) {
    console.error(
      "[release-announcement] sent, but the audit row could not be written",
      err,
    );
  }

  return NextResponse.json({ recipients: recipients.length, sent, failed });
}
