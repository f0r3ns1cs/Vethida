import { NextResponse, type NextRequest } from "next/server";
import { apiError, ERROR_CODES } from "@/lib/errors";
import { contentLengthExceeds } from "@/lib/rate-limit";
import { requiredEnv } from "@/lib/env";
import { verifyPolarWebhook } from "@/lib/polar/signature";
import {
  clearDelivery,
  markDeliveryProcessed,
  tryRecordDelivery,
} from "@/lib/polar/dedup";
import {
  dispatchPolarEvent,
  type PolarWebhookEvent,
} from "@/lib/polar/webhook";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

const MAX_BODY_BYTES = 64 * 1024;

export async function POST(req: NextRequest) {
  try {
    if (contentLengthExceeds(req, MAX_BODY_BYTES)) {
      return apiError(413, ERROR_CODES.PAYLOAD_TOO_LARGE, "Request body too large.");
    }
    const rawBody = await req.text();
    if (rawBody.length > MAX_BODY_BYTES) {
      return apiError(413, ERROR_CODES.PAYLOAD_TOO_LARGE, "Request body too large.");
    }

    const deliveryId = req.headers.get("webhook-id");
    const verified = verifyPolarWebhook(
      rawBody,
      {
        id: deliveryId,
        timestamp: req.headers.get("webhook-timestamp"),
        signature: req.headers.get("webhook-signature"),
      },
      requiredEnv("POLAR_WEBHOOK_SECRET"),
    );
    if (!verified || !deliveryId) {
      return apiError(
        403,
        ERROR_CODES.WEBHOOK_SIGNATURE_INVALID,
        "Webhook signature verification failed.",
      );
    }

    let event: PolarWebhookEvent;
    try {
      event = JSON.parse(rawBody) as PolarWebhookEvent;
    } catch {
      event = { type: "", data: { id: "" } };
    }
    if (typeof event?.type !== "string" || typeof event?.data?.id !== "string") {
      // signed but not something we understand, ack it so Polar stops retrying
      console.error("[polar-webhook] unrecognized payload shape", {
        deliveryId,
      });
      return NextResponse.json({ received: true });
    }

    const key = { provider: "polar", eventId: deliveryId };
    const first = await tryRecordDelivery({ ...key, eventType: event.type });
    if (!first) {
      return NextResponse.json({ received: true, duplicate: true });
    }

    try {
      await dispatchPolarEvent(event);
    } catch (err) {
      await clearDelivery(key); // so the retry isn't treated as a duplicate
      throw err;
    }
    await markDeliveryProcessed(key);

    return NextResponse.json({ received: true });
  } catch (err) {
    console.error("[polar-webhook] unhandled error", err);
    return apiError(500, ERROR_CODES.INTERNAL_ERROR, "Internal server error.");
  }
}
