import { and, eq } from "drizzle-orm";
import { db, webhookEvent } from "@vethida/db";

// has to be longer than Polar keeps retrying (a few days)
const RETENTION_MS = 30 * 24 * 60 * 60 * 1000;

export interface DeliveryKey {
  provider: string;
  eventId: string; // webhook-id header, NOT data.id (repeats across order.created / order.paid)
}

// true = first delivery, go ahead. false = duplicate, just return 200.
export async function tryRecordDelivery(
  args: DeliveryKey & { eventType: string },
): Promise<boolean> {
  const inserted = await db
    .insert(webhookEvent)
    .values({
      provider: args.provider,
      eventId: args.eventId,
      eventType: args.eventType,
      expiresAt: new Date(Date.now() + RETENTION_MS),
    })
    .onConflictDoNothing({
      target: [webhookEvent.provider, webhookEvent.eventId],
    })
    .returning({ eventId: webhookEvent.eventId });
  return inserted.length > 0;
}

export async function markDeliveryProcessed(key: DeliveryKey): Promise<void> {
  await db
    .update(webhookEvent)
    .set({ status: "processed", processedAt: new Date() })
    .where(
      and(
        eq(webhookEvent.provider, key.provider),
        eq(webhookEvent.eventId, key.eventId),
      ),
    );
}

// call when the handler throws, otherwise the retry gets treated as a duplicate
export async function clearDelivery(key: DeliveryKey): Promise<void> {
  await db
    .delete(webhookEvent)
    .where(
      and(
        eq(webhookEvent.provider, key.provider),
        eq(webhookEvent.eventId, key.eventId),
      ),
    );
}
