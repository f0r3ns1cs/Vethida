import { NextResponse, type NextRequest } from "next/server";
import {
  DESKTOP_AUTH_POLL_INTERVAL_MS,
  DESKTOP_AUTH_TTL_MS,
  db,
  desktopAuthSession,
  generateDeviceCode,
  generateUserCode,
  hashDeviceCode,
} from "@vethida/db";
import { apiError, ERROR_CODES, rateLimited } from "@/lib/errors";
import {
  clientIp,
  contentLengthExceeds,
  createRateLimiter,
} from "@/lib/rate-limit";
import { verifySignedRequest } from "@/lib/licenses/device-auth";
import { derivePubkeyId } from "@/lib/licenses/antisharing";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

const MAX_BODY_BYTES = 8192;

// looser than /activate, starting a session only costs a row
const isLimited = createRateLimiter({ windowMs: 15 * 60 * 1000, max: 30 });

const PORTAL_ORIGIN =
  process.env.PORTAL_ORIGIN ?? "https://my.vethida.com";

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
      devicePublicKey?: unknown;
      hardwareFingerprint?: unknown;
    };
    try {
      body = JSON.parse(rawBody);
    } catch {
      return apiError(400, ERROR_CODES.INVALID_JSON, "Request body must be valid JSON.");
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

    // proof of possession, same as /activate
    const sig = verifySignedRequest(req, rawBody, devicePublicKey);
    if (!sig.ok) {
      return apiError(
        sig.status,
        sig.code,
        "Request signature verification failed.",
      );
    }

    const deviceCode = generateDeviceCode();
    const expiresAt = new Date(Date.now() + DESKTOP_AUTH_TTL_MS);

    // user codes are unique among pending sessions, retry on the (very rare) collision
    let userCode = "";
    for (let attempt = 0; attempt < 5; attempt++) {
      userCode = generateUserCode();
      try {
        await db.insert(desktopAuthSession).values({
          userCode,
          deviceCodeHash: hashDeviceCode(deviceCode),
          devicePublicKey,
          devicePubkeyId: derivePubkeyId(devicePublicKey),
          hardwareFingerprint: fingerprint,
          startIp: ip,
          expiresAt,
        });
        return NextResponse.json(
          {
            userCode,
            deviceCode,
            verificationUrl: `${PORTAL_ORIGIN}/desktop/authorize?code=${encodeURIComponent(userCode)}`,
            expiresAt: expiresAt.toISOString(),
            pollIntervalMs: DESKTOP_AUTH_POLL_INTERVAL_MS,
          },
          { status: 201 },
        );
      } catch (err) {
        const message = err instanceof Error ? err.message : "";
        if (!message.includes("desktop_auth_user_code_live_unique")) throw err;
      }
    }

    console.error("[desktop-auth/start] user code collisions exhausted");
    return apiError(500, ERROR_CODES.INTERNAL_ERROR, "Internal server error.");
  } catch (err) {
    console.error("[desktop-auth/start] unhandled error", err);
    return apiError(500, ERROR_CODES.INTERNAL_ERROR, "Internal server error.");
  }
}
