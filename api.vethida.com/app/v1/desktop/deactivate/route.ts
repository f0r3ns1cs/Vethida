import { NextResponse, type NextRequest } from "next/server";
import { apiError, ERROR_CODES, rateLimited } from "@/lib/errors";
import {
  clientIp,
  contentLengthExceeds,
  createRateLimiter,
} from "@/lib/rate-limit";
import { authenticateDeviceRequest } from "@/lib/licenses/device-auth";
import { deactivateDevice } from "@/lib/licenses/antisharing";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

const MAX_BODY_BYTES = 1024;
const isLimited = createRateLimiter({ windowMs: 15 * 60 * 1000, max: 10 });

// frees the machine slot. The reset is only spent when another machine activates.
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

    // false just means it was already deactivated
    await deactivateDevice(auth.license.id, auth.device.devicePubkeyId);

    return NextResponse.json({ deactivated: true });
  } catch (err) {
    console.error("[deactivate] unhandled error", err);
    return apiError(500, ERROR_CODES.INTERNAL_ERROR, "Internal server error.");
  }
}
