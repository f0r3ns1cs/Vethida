import { NextResponse, type NextRequest } from "next/server";
import { eq } from "drizzle-orm";
import {
  db,
  hasActiveSuspension,
  hashLicenseKey,
  license,
  parseLicenseKey,
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

const MAX_BODY_BYTES = 8192;
const TOKEN_TTL_MS = 7 * 24 * 60 * 60 * 1000;
// keys are 100 bits so brute force isn't realistic, this just cuts noise
const isLimited = createRateLimiter({ windowMs: 15 * 60 * 1000, max: 10 });

function decodeDevicePublicKey(raw: unknown): Buffer | null {
  if (typeof raw !== "string" || !/^[A-Za-z0-9_-]{86}$/.test(raw)) return null;
  const buf = Buffer.from(raw, "base64url");
  return buf.length === 64 ? buf : null;
}

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

    let body: {
      licenseKey?: unknown;
      devicePublicKey?: unknown;
      hardwareFingerprint?: unknown;
    };
    try {
      body = JSON.parse(rawBody);
    } catch {
      return apiError(400, ERROR_CODES.INVALID_JSON, "Request body must be valid JSON.");
    }

    const licenseKey =
      typeof body.licenseKey === "string"
        ? parseLicenseKey(body.licenseKey)
        : null;
    if (!licenseKey) {
      return apiError(
        400,
        ERROR_CODES.INVALID_LICENSE_KEY,
        "License key format is invalid.",
      );
    }

    const devicePublicKey = decodeDevicePublicKey(body.devicePublicKey);
    if (!devicePublicKey) {
      return apiError(
        400,
        ERROR_CODES.INVALID_DEVICE_PUBLIC_KEY,
        "devicePublicKey must be 64 bytes, base64url encoded.",
      );
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

    // proof of possession for the key being registered
    const sig = verifySignedRequest(req, rawBody, devicePublicKey);
    if (!sig.ok) {
      return apiError(
        sig.status,
        sig.code,
        "Request signature verification failed.",
      );
    }

    const [lic] = await db
      .select()
      .from(license)
      .where(eq(license.keyHash, hashLicenseKey(licenseKey)))
      .limit(1);
    if (!lic) {
      return apiError(401, ERROR_CODES.LICENSE_UNKNOWN, "No license matches this key.");
    }
    if (lic.userId && (await hasActiveSuspension(lic.userId))) {
      return apiError(
        403,
        ERROR_CODES.ACCOUNT_SUSPENDED,
        "The account that owns this license is suspended.",
      );
    }

    const result = await registerDevice({
      licenseId: lic.id,
      userId: lic.userId,
      devicePublicKey,
      hardwareFingerprint: fingerprint,
      ip,
    });
    if (!result.ok) {
      switch (result.reason) {
        case "license_not_found":
          return apiError(
            401,
            ERROR_CODES.LICENSE_UNKNOWN,
            "No license matches this key.",
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

    // token never outlives the license
    const tokenExpiresAt = new Date(
      Math.min(
        Date.now() + TOKEN_TTL_MS,
        lic.expiresAt ? lic.expiresAt.getTime() : Infinity,
      ),
    );
    const licenseToken = signLicenseToken({
      keyHash: lic.keyHash,
      tier: lic.tier,
      devicePubkeyId: derivePubkeyId(devicePublicKey),
      activatedAt: result.activatedAt.toISOString(),
      expiresAt: tokenExpiresAt.toISOString(),
    });

    return NextResponse.json(
      {
        licenseToken,
        tier: lic.tier,
        // trial so the client can show "Trial - N days left". licenseExpiresAt
        // is the entitlement, tokenExpiresAt is just the 7 day token.
        trial: lic.trial,
        licenseExpiresAt: lic.expiresAt ? lic.expiresAt.toISOString() : null,
        tokenExpiresAt: tokenExpiresAt.toISOString(),
        rebound: result.rebound,
        resetsRemaining: result.resetsRemaining,
        fingerprintStatus: "accept",
      },
      { status: 201 },
    );
  } catch (err) {
    console.error("[activate] unhandled error", err);
    return apiError(500, ERROR_CODES.INTERNAL_ERROR, "Internal server error.");
  }
}
