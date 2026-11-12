import { NextResponse, type NextRequest } from "next/server";
import { ABUSE_FLAG_TYPES, abuseFlag, db } from "@vethida/db";
import { apiError, ERROR_CODES, rateLimited } from "@/lib/errors";
import {
  clientIp,
  contentLengthExceeds,
  createRateLimiter,
} from "@/lib/rate-limit";
import { authenticateDeviceRequest } from "@/lib/licenses/device-auth";
import { handleHeartbeat } from "@/lib/licenses/heartbeat";
import { signLicenseToken } from "@/lib/licenses/signing";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

const MAX_BODY_BYTES = 8192;
const TOKEN_TTL_MS = 7 * 24 * 60 * 60 * 1000;
const isLimited = createRateLimiter({ windowMs: 60 * 1000, max: 60 });

export async function POST(req: NextRequest) {
  try {
    if (contentLengthExceeds(req, MAX_BODY_BYTES)) {
      return apiError(413, ERROR_CODES.PAYLOAD_TOO_LARGE, "Request body too large.");
    }
    const ip = clientIp(req);
    if (isLimited(ip)) {
      return rateLimited();
    }

    const rawBody = await req.text();
    if (rawBody.length > MAX_BODY_BYTES) {
      return apiError(413, ERROR_CODES.PAYLOAD_TOO_LARGE, "Request body too large.");
    }

    const auth = await authenticateDeviceRequest(req, rawBody);
    if (!auth.ok) {
      return apiError(auth.status, auth.code, "Device authentication failed.");
    }

    let body: { hardwareFingerprint?: unknown };
    try {
      body = JSON.parse(rawBody);
    } catch {
      return apiError(400, ERROR_CODES.INVALID_JSON, "Request body must be valid JSON.");
    }
    const fingerprint = body.hardwareFingerprint;
    if (
      typeof fingerprint !== "object" ||
      fingerprint === null ||
      Array.isArray(fingerprint)
    ) {
      return apiError(
        400,
        ERROR_CODES.INVALID_FINGERPRINT,
        "hardwareFingerprint must be an object.",
      );
    }

    const result = await handleHeartbeat({
      device: auth.device,
      license: auth.license,
      hardwareFingerprint: fingerprint,
      ip,
    });
    if (!result.ok) {
      switch (result.reason) {
        case "suspended":
          return apiError(
            403,
            ERROR_CODES.ACCOUNT_SUSPENDED,
            "The account that owns this license is suspended.",
          );
        case "license_revoked":
          return apiError(
            403,
            ERROR_CODES.LICENSE_REVOKED,
            "This license has been revoked.",
          );
        case "license_expired":
          return apiError(
            403,
            ERROR_CODES.LICENSE_EXPIRED,
            "This license has expired.",
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
            "Not enough hardware could be identified on this machine. Restart and try again; if it keeps happening, contact support.",
          );
      }
    }

    if (result.fingerprint.verdict === "flag") {
      try {
        await db.insert(abuseFlag).values({
          userId: auth.device.userId,
          licenseId: auth.license.id,
          deviceId: auth.device.id,
          type: ABUSE_FLAG_TYPES.FINGERPRINT_FLAG,
          details: {
            matchCount: result.fingerprint.matchCount,
            delta: result.fingerprint.delta,
          },
        });
      } catch (err) {
        console.error("[heartbeat] abuse flag write failed", err);
      }
    }

    // every heartbeat returns a fresh token, there's no refresh endpoint
    const tokenExpiresAt = new Date(
      Math.min(
        Date.now() + TOKEN_TTL_MS,
        auth.license.expiresAt ? auth.license.expiresAt.getTime() : Infinity,
      ),
    );
    const licenseToken = signLicenseToken({
      keyHash: auth.license.keyHash,
      tier: result.tier,
      devicePubkeyId: auth.device.devicePubkeyId,
      activatedAt: auth.device.activatedAt.toISOString(),
      expiresAt: tokenExpiresAt.toISOString(),
    });

    return NextResponse.json({
      tier: result.tier,
      trial: auth.license.trial,
      licenseExpiresAt: auth.license.expiresAt
        ? auth.license.expiresAt.toISOString()
        : null,
      licenseToken,
      tokenExpiresAt: tokenExpiresAt.toISOString(),
      resetsRemaining: auth.license.machineResetsRemaining,
      fingerprintStatus: result.fingerprint.verdict,
      accountDeleting: result.pendingDeletion ? true : undefined,
      accountHardDeleteAt: result.pendingDeletion?.hardDeleteAt,
    });
  } catch (err) {
    console.error("[heartbeat] unhandled error", err);
    return apiError(500, ERROR_CODES.INTERNAL_ERROR, "Internal server error.");
  }
}
