import { NextResponse, type NextRequest } from "next/server";
import { headers } from "next/headers";
import { eq } from "drizzle-orm";
import {
  cancellationRequest,
  db,
  emailPreference,
  recordAdminAction,
} from "@vethida/db";
import { auth } from "@/lib/auth";
import { getUserLicenses } from "@/lib/licenses";
import { clientIp, createRateLimiter } from "@/lib/rate-limit";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

// GDPR Art. 15/20 export. No raw fingerprints, fraud signals or tokens.
const isLimited = createRateLimiter({ windowMs: 60 * 60 * 1000, max: 5 });

export async function GET(req: NextRequest) {
  try {
    return await buildExport(req);
  } catch (err) {
    // fail with a clear error, not a half-built file
    console.error("[data-export] unhandled error", err);
    return NextResponse.json(
      {
        error:
          "We couldn't build your export. Nothing was changed. Please try again shortly.",
      },
      { status: 500 },
    );
  }
}

async function buildExport(req: NextRequest): Promise<NextResponse> {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) {
    return NextResponse.json({ error: "Sign in first." }, { status: 401 });
  }
  if (isLimited(clientIp(req))) {
    return NextResponse.json(
      { error: "Too many export requests. Try again later." },
      { status: 429 },
    );
  }

  const licenses = await getUserLicenses(session.user.id);
  const [prefs] = await db
    .select({
      fingerprintAlerts: emailPreference.fingerprintAlerts,
      releaseAnnouncements: emailPreference.releaseAnnouncements,
    })
    .from(emailPreference)
    .where(eq(emailPreference.userId, session.user.id))
    .limit(1);

  // includes the comment, but not offer_detail (our internal ids)
  const cancellations = await db
    .select({
      reason: cancellationRequest.reason,
      comment: cancellationRequest.comment,
      tier: cancellationRequest.tier,
      offerShown: cancellationRequest.offerShown,
      outcome: cancellationRequest.outcome,
      createdAt: cancellationRequest.createdAt,
      resolvedAt: cancellationRequest.resolvedAt,
    })
    .from(cancellationRequest)
    .where(eq(cancellationRequest.userId, session.user.id));

  const data = {
    format: "vethida-data-export-v1",
    exportedAt: new Date().toISOString(),
    account: {
      id: session.user.id,
      email: session.user.email,
      name: session.user.name,
      emailVerified: session.user.emailVerified,
    },
    emailPreferences: prefs ?? {
      fingerprintAlerts: true,
      releaseAnnouncements: false,
    },
    cancellations,
    licenses: licenses.map(({ license: lic, devices }) => ({
      key: lic.key,
      tier: lic.tier,
      status: lic.revokedAt ? "revoked" : "active",
      createdAt: lic.createdAt,
      expiresAt: lic.expiresAt,
      cancelAtPeriodEnd: lic.cancelAtPeriodEnd,
      machineResetsRemaining: lic.machineResetsRemaining,
      devices: devices.map((d) => ({
        deviceId: d.devicePubkeyId,
        lastSeenAt: d.lastSeenAt,
        lastIp: d.lastIp,
      })),
    })),
  };

  // exports get audited, actor = subject here
  await recordAdminAction({
    adminId: session.user.id,
    action: "data_export",
    targetType: "user",
    targetId: session.user.id,
    ipAddress: clientIp(req),
  });

  return new NextResponse(JSON.stringify(data, null, 2), {
    status: 200,
    headers: {
      "Content-Type": "application/json; charset=utf-8",
      "Content-Disposition": `attachment; filename="vethida-data-export-${session.user.id}.json"`,
      "Cache-Control": "no-store",
    },
  });
}
