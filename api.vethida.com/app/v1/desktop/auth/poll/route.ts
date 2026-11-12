import { NextResponse, type NextRequest } from "next/server";
import { eq } from "drizzle-orm";
import {
  consumeSession,
  db,
  findSessionByDeviceCode,
  hasActiveSuspension,
  isLive,
  license,
  user,
} from "@vethida/db";
import { apiError, ERROR_CODES, rateLimited } from "@/lib/errors";
import {
  clientIp,
  contentLengthExceeds,
  createRateLimiter,
} from "@/lib/rate-limit";
import { verifySignedRequest } from "@/lib/licenses/device-auth";
import { derivePubkeyId, registerDevice } from "@/lib/licenses/antisharing";
import { signLicenseToken } from "@/lib/licenses/signing";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

const MAX_BODY_BYTES = 4096;
const TOKEN_TTL_MS = 7 * 24 * 60 * 60 * 1000;

// polling every 2s for 10 min is ~300 requests per sign-in
const isLimited = createRateLimiter({ windowMs: 15 * 60 * 1000, max: 500 });

const DEVICE_CODE_RE = /^[A-Za-z0-9_-]{43}$/;

export async function POST(req: NextRequest) {
  try {
    if (contentLengthExceeds(req, MAX_BODY_BYTES)) {
      return apiError(413, ERROR_CODES.PAYLOAD_TOO_LARGE, "Request body too large.");
    }
    if (isLimited(clientIp(req))) {
      return rateLimited();
    }

    const rawBody = await req.text();
    if (rawBody.length > MAX_BODY_BYTES) {
      return apiError(413, ERROR_CODES.PAYLOAD_TOO_LARGE, "Request body too large.");
    }

    let body: { deviceCode?: unknown };
    try {
      body = JSON.parse(rawBody);
    } catch {
      return apiError(400, ERROR_CODES.INVALID_JSON, "Request body must be valid JSON.");
    }

    if (typeof body.deviceCode !== "string" || !DEVICE_CODE_RE.test(body.deviceCode)) {
      return apiError(400, ERROR_CODES.INVALID_DEVICE_CODE, "deviceCode is malformed.");
    }

    const session = await findSessionByDeviceCode(body.deviceCode);
    // same answer for unknown and expired
    if (!session || !isLive(session)) {
      return apiError(
        404,
        ERROR_CODES.AUTH_SESSION_EXPIRED,
        "This sign-in request is no longer valid.",
      );
    }

    // must be signed by the key that started the session, the code alone isn't enough
    const sig = verifySignedRequest(req, rawBody, session.devicePublicKey);
    if (!sig.ok) {
      return apiError(
        sig.status,
        sig.code,
        "Request signature verification failed.",
      );
    }

    if (session.status === "pending") {
      return NextResponse.json({ status: "pending" }, { status: 200 });
    }
    if (session.status === "denied") {
      return apiError(
        403,
        ERROR_CODES.AUTH_DENIED,
        "This sign-in request was declined in the browser.",
      );
    }
    if (session.status === "consumed") {
      return apiError(
        409,
        ERROR_CODES.AUTH_ALREADY_USED,
        "This sign-in request has already been completed.",
      );
    }

    // approved - consume it first so two polls can't both register
    if (!session.userId || !session.licenseId) {
      console.error("[desktop-auth/poll] approved session missing binding", session.id);
      return apiError(500, ERROR_CODES.INTERNAL_ERROR, "Internal server error.");
    }
    if (!(await consumeSession(session.id))) {
      return apiError(
        409,
        ERROR_CODES.AUTH_ALREADY_USED,
        "This sign-in request has already been completed.",
      );
    }

    if (await hasActiveSuspension(session.userId)) {
      return apiError(
        403,
        ERROR_CODES.ACCOUNT_SUSPENDED,
        "The account that owns this license is suspended.",
      );
    }

    const [lic] = await db
      .select()
      .from(license)
      .where(eq(license.id, session.licenseId))
      .limit(1);
    if (!lic) {
      return apiError(401, ERROR_CODES.LICENSE_UNKNOWN, "No license matches this key.");
    }

    // same binding rules as /activate
    const result = await registerDevice({
      licenseId: lic.id,
      userId: lic.userId,
      devicePublicKey: session.devicePublicKey,
      hardwareFingerprint: session.hardwareFingerprint,
      ip: clientIp(req),
    });
    if (!result.ok) {
      switch (result.reason) {
        case "license_not_found":
          return apiError(401, ERROR_CODES.LICENSE_UNKNOWN, "No license matches this key.");
        case "license_revoked":
          return apiError(403, ERROR_CODES.LICENSE_REVOKED, "This license has been revoked.");
        case "license_expired":
          return apiError(403, ERROR_CODES.LICENSE_EXPIRED, "This license has expired.");
        case "machine_limit_reached":
          return apiError(
            409,
            ERROR_CODES.MACHINE_LIMIT_REACHED,
            "This license is active on another machine. Deactivate it first.",
          );
        case "fingerprint_mismatch":
          return apiError(
            409,
            ERROR_CODES.FINGERPRINT_MISMATCH,
            "This machine does not match the installation's hardware record.",
          );
        case "fingerprint_unusable":
          return apiError(
            400,
            ERROR_CODES.INVALID_FINGERPRINT,
            "Not enough hardware could be identified on this machine to activate it. Restart and try again; if it keeps happening, contact support.",
          );
        case "no_resets_remaining":
          return apiError(
            409,
            ERROR_CODES.NO_RESETS_REMAINING,
            "No machine resets remaining on this license. Contact support.",
          );
      }
    }

    const tokenExpiresAt = new Date(
      Math.min(
        Date.now() + TOKEN_TTL_MS,
        lic.expiresAt ? lic.expiresAt.getTime() : Infinity,
      ),
    );
    const licenseToken = signLicenseToken({
      keyHash: lic.keyHash,
      tier: lic.tier,
      devicePubkeyId: derivePubkeyId(session.devicePublicKey),
      activatedAt: result.activatedAt.toISOString(),
      expiresAt: tokenExpiresAt.toISOString(),
    });

    const [owner] = await db
      .select({ email: user.email })
      .from(user)
      .where(eq(user.id, session.userId))
      .limit(1);

    return NextResponse.json(
      {
        status: "approved",
        licenseToken,
        licenseKey: lic.key, // the app shows it on the account page
        tier: lic.tier,
        trial: lic.trial,
        licenseExpiresAt: lic.expiresAt ? lic.expiresAt.toISOString() : null,
        tokenExpiresAt: tokenExpiresAt.toISOString(),
        resetsRemaining: result.resetsRemaining,
        fingerprintStatus: "accept",
        accountEmail: owner?.email ?? null,
      },
      { status: 200 },
    );
  } catch (err) {
    console.error("[desktop-auth/poll] unhandled error", err);
    return apiError(500, ERROR_CODES.INTERNAL_ERROR, "Internal server error.");
  }
}
