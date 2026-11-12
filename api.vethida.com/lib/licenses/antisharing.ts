import crypto from "node:crypto";
import { and, count, eq, isNull } from "drizzle-orm";
import { db, device, license } from "@vethida/db";
import {
  compareHardwareFingerprints,
  MIN_BINDABLE_SOURCES,
  pepperFingerprint,
  recognizedSourceCount,
} from "./fingerprint";

// X-Device-Pubkey-Id: hex of the first 16 bytes of SHA-256(pubkey)
export function derivePubkeyId(devicePublicKey: Buffer): string {
  return crypto
    .createHash("sha256")
    .update(devicePublicKey)
    .digest("hex")
    .slice(0, 32);
}

export interface RegisterDeviceArgs {
  licenseId: string;
  userId: string | null; // null = key not claimed by an account yet
  devicePublicKey: Buffer;
  hardwareFingerprint: unknown;
  ip: string | null;
}

export type RegisterDeviceResult =
  | {
      ok: true;
      deviceId: string;
      rebound: boolean;
      activatedAt: Date;
      resetsRemaining: number;
    }
  | {
      ok: false;
      reason:
        | "license_not_found"
        | "license_revoked"
        | "license_expired"
        | "machine_limit_reached"
        | "fingerprint_mismatch"
        | "fingerprint_unusable"
        | "no_resets_remaining";
    };

export async function registerDevice(
  args: RegisterDeviceArgs,
): Promise<RegisterDeviceResult> {
  if (args.devicePublicKey.length !== 64) {
    throw new TypeError("devicePublicKey must be the 64-byte raw P-256 point");
  }
  // pepper once here, everything below compares/stores the peppered form
  const submittedFingerprint = pepperFingerprint(args.hardwareFingerprint);
  const pubkeyId = derivePubkeyId(args.devicePublicKey);
  const now = new Date();

  return db.transaction(async (tx) => {
    // lock the license row so concurrent activations queue up
    const [lic] = await tx
      .select()
      .from(license)
      .where(eq(license.id, args.licenseId))
      .for("update")
      .limit(1);

    if (!lic) return { ok: false as const, reason: "license_not_found" as const };
    if (lic.revokedAt) return { ok: false as const, reason: "license_revoked" as const };
    if (lic.expiresAt && lic.expiresAt <= now) {
      return { ok: false as const, reason: "license_expired" as const };
    }

    // Too few sources to ever re-verify, binding it would lock the user out of
    // their own machine. Checked after revoked/expired so those answers win.
    if (recognizedSourceCount(args.hardwareFingerprint) < MIN_BINDABLE_SOURCES) {
      return { ok: false as const, reason: "fingerprint_unusable" as const };
    }

    const [existing] = await tx
      .select()
      .from(device)
      .where(eq(device.devicePubkeyId, pubkeyId))
      .limit(1);

    // Same install + license, still active. The stored fingerprint isn't updated
    // here so a copied keypair on other hardware can't take over the binding.
    if (existing && existing.licenseId === lic.id && !existing.deactivatedAt) {
      const cmp = compareHardwareFingerprints(
        existing.hardwareFingerprint,
        submittedFingerprint,
      );
      if (cmp.verdict === "reject") {
        return { ok: false as const, reason: "fingerprint_mismatch" as const };
      }
      await tx
        .update(device)
        .set({ lastSeenAt: now, lastIp: args.ip })
        .where(eq(device.id, existing.id));
      return {
        ok: true as const,
        deviceId: existing.id,
        rebound: false,
        activatedAt: existing.activatedAt,
        resetsRemaining: lic.machineResetsRemaining,
      };
    }

    // one machine per license
    const active = await tx
      .select({ id: device.id })
      .from(device)
      .where(and(eq(device.licenseId, lic.id), isNull(device.deactivatedAt)))
      .limit(1);

    if (active.length > 0) {
      return { ok: false as const, reason: "machine_limit_reached" as const };
    }

    // Moving machines costs a reset. First binding and same-hardware
    // reactivation are free.
    let consumesReset: boolean;
    if (existing && existing.licenseId === lic.id) {
      consumesReset =
        compareHardwareFingerprints(
          existing.hardwareFingerprint,
          submittedFingerprint,
        ).verdict === "reject";
    } else {
      const [{ n: priorDevices }] = await tx
        .select({ n: count() })
        .from(device)
        .where(eq(device.licenseId, lic.id));
      consumesReset = priorDevices > 0;
    }
    if (consumesReset) {
      if (lic.machineResetsRemaining <= 0) {
        return { ok: false as const, reason: "no_resets_remaining" as const };
      }
      await tx
        .update(license)
        .set({ machineResetsRemaining: lic.machineResetsRemaining - 1 })
        .where(eq(license.id, lic.id));
    }

    let deviceId: string;
    if (existing) {
      // only replace the fingerprint on a real move, otherwise repeated
      // reactivations could drift it one part at a time
      const refreshFingerprint = consumesReset || existing.licenseId !== lic.id;
      await tx
        .update(device)
        .set({
          licenseId: lic.id,
          userId: args.userId,
          ...(refreshFingerprint
            ? { hardwareFingerprint: submittedFingerprint }
            : {}),
          lastIp: args.ip,
          deactivatedAt: null,
          activatedAt: now,
          lastSeenAt: now,
        })
        .where(eq(device.id, existing.id));
      deviceId = existing.id;
    } else {
      const [inserted] = await tx
        .insert(device)
        .values({
          licenseId: lic.id,
          userId: args.userId,
          devicePublicKey: args.devicePublicKey,
          devicePubkeyId: pubkeyId,
          hardwareFingerprint: submittedFingerprint,
          lastIp: args.ip,
          activatedAt: now,
          lastSeenAt: now,
        })
        .returning({ id: device.id });
      deviceId = inserted.id;
    }

    return {
      ok: true as const,
      deviceId,
      rebound: !!existing,
      activatedAt: now,
      resetsRemaining: consumesReset
        ? lic.machineResetsRemaining - 1
        : lic.machineResetsRemaining,
    };
  });
}

export async function deactivateDevice(
  licenseId: string,
  devicePubkeyId: string,
): Promise<boolean> {
  const updated = await db
    .update(device)
    .set({ deactivatedAt: new Date() })
    .where(
      and(
        eq(device.licenseId, licenseId),
        eq(device.devicePubkeyId, devicePubkeyId),
        isNull(device.deactivatedAt),
      ),
    )
    .returning({ id: device.id });
  return updated.length > 0;
}
