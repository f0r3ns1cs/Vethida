import { eq } from "drizzle-orm";
import {
  accountDeletion,
  db,
  device,
  emailPreference,
  hasActiveSuspension,
  user,
  type Device,
  type License,
} from "@vethida/db";
import { sendEmail } from "../email";
import { legalEntityAddress } from "../email/identity";
import { renderTemplate } from "../email/render";
import {
  compareHardwareFingerprints,
  MIN_BINDABLE_SOURCES,
  pepperFingerprint,
  recognizedSourceCount,
  type FingerprintComparison,
} from "./fingerprint";

const FLAG_NOTIFY_COOLDOWN_MS = 30 * 24 * 60 * 60 * 1000;

// emails the owner when a heartbeat is flagged, at most once per 30 days per device
async function notifyFingerprintFlag(
  dev: Device,
  lic: License,
  cmp: FingerprintComparison,
  now: Date,
): Promise<void> {
  if (!dev.userId) return;
  const last = dev.fingerprintNotifiedAt;
  if (last && now.getTime() - last.getTime() < FLAG_NOTIFY_COOLDOWN_MS) return;

  // respect the opt-out
  const [prefs] = await db
    .select({ fingerprintAlerts: emailPreference.fingerprintAlerts })
    .from(emailPreference)
    .where(eq(emailPreference.userId, dev.userId))
    .limit(1);
  if (prefs && !prefs.fingerprintAlerts) return;

  const [owner] = await db
    .select({ email: user.email })
    .from(user)
    .where(eq(user.id, dev.userId))
    .limit(1);
  if (!owner?.email) return;

  const first = owner.email.split("@")[0]?.split(/[._-]/)[0] || "there";
  const rendered = renderTemplate("license-fingerprint-flagged", {
    customerFirstName: first.charAt(0).toUpperCase() + first.slice(1),
    matchScore: String(cmp.matchCount),
    // out of what was comparable, not always 6
    comparableCount: String(cmp.comparableCount),
    changedFields: cmp.delta.join(", "),
    detectionDate: now.toISOString().slice(0, 10),
    licenseId: lic.id,
    legalEntityAddress: legalEntityAddress(),
  });
  await sendEmail({ to: owner.email, ...rendered });
  await db
    .update(device)
    .set({ fingerprintNotifiedAt: now })
    .where(eq(device.id, dev.id));
}

export interface HeartbeatArgs {
  // from authenticateDeviceRequest (signature already verified)
  device: Device;
  license: License;
  hardwareFingerprint: unknown;
  ip: string | null;
}

export type HeartbeatResult =
  | {
      ok: true;
      tier: string;
      fingerprint: FingerprintComparison;
      // account deletion pending, the client shows a warning
      pendingDeletion?: { hardDeleteAt: string };
    }
  | {
      ok: false;
      reason:
        | "suspended"
        | "license_revoked"
        | "license_expired"
        | "fingerprint_mismatch"
        | "fingerprint_unusable";
    };

export async function handleHeartbeat(
  args: HeartbeatArgs,
): Promise<HeartbeatResult> {
  const now = new Date();
  const lic = args.license;
  const dev = args.device;

  if (lic.revokedAt) return { ok: false, reason: "license_revoked" };
  if (lic.expiresAt && lic.expiresAt <= now) {
    return { ok: false, reason: "license_expired" };
  }
  if (dev.userId && (await hasActiveSuspension(dev.userId))) {
    return { ok: false, reason: "suspended" };
  }

  // The keypair is just a file and can be copied, the fingerprint is what ties
  // it to the machine. The stored vector (already peppered) never changes here.
  // Too few sources gets its own answer instead of a misleading mismatch.
  if (recognizedSourceCount(args.hardwareFingerprint) < MIN_BINDABLE_SOURCES) {
    return { ok: false, reason: "fingerprint_unusable" };
  }

  const fingerprint = compareHardwareFingerprints(
    dev.hardwareFingerprint,
    pepperFingerprint(args.hardwareFingerprint),
  );
  if (fingerprint.verdict === "reject") {
    return { ok: false, reason: "fingerprint_mismatch" };
  }

  await db
    .update(device)
    .set({ lastSeenAt: now, lastIp: args.ip })
    .where(eq(device.id, dev.id));

  if (fingerprint.verdict === "flag") {
    try {
      await notifyFingerprintFlag(dev, lic, fingerprint, now);
    } catch (err) {
      console.error("[heartbeat] fingerprint-flag notice failed", err);
    }
  }

  let pendingDeletion: { hardDeleteAt: string } | undefined;
  if (dev.userId) {
    const [pending] = await db
      .select({ hardDeleteAt: accountDeletion.hardDeleteAt })
      .from(accountDeletion)
      .where(eq(accountDeletion.userId, dev.userId))
      .limit(1);
    if (pending) {
      pendingDeletion = { hardDeleteAt: pending.hardDeleteAt.toISOString() };
    }
  }

  return { ok: true, tier: lic.tier, fingerprint, pendingDeletion };
}
