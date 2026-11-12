import crypto from "node:crypto";
import { requiredEnv } from "../env";

// The six fingerprint sources. Values are SHA-256 hashes made by the desktop
// (HardwareFingerprint.cpp), the server never sees raw serials or MACs.
export const FINGERPRINT_SOURCES = [
  "motherboardUuid",
  "biosSerial",
  "cpuId",
  "diskSerial",
  "volumeSerial",
  "primaryMac",
] as const;

export type FingerprintSource = (typeof FINGERPRINT_SOURCES)[number];

// Not cpuId: it's the same for every machine with that CPU model, so it only
// counts toward the total. diskSerial survives a format, volumeSerial doesn't.
export const STRONG_SOURCES: readonly FingerprintSource[] = [
  "motherboardUuid",
  "biosSerial",
  "diskSerial",
  "volumeSerial",
  "primaryMac",
] as const;

export const MATCH_THRESHOLD = 4; // matches needed to accept
export const MIN_STRONG_MATCHES = 3; // of which strong

export type HardwareFingerprint = Partial<
  Record<FingerprintSource, string | null>
>;

export type FingerprintVerdict = "accept" | "flag" | "reject";

export interface FingerprintComparison {
  matchCount: number;
  strongMatchCount: number;
  comparableCount: number; // sources present on both sides
  delta: FingerprintSource[]; // comparable sources that didn't match
  verdict: FingerprintVerdict;
}

// HMACs each client hash with the server pepper (per field, since the compare
// is per field). A volume serial is only 32 bits, so without the pepper a db
// leak could be brute-forced. Stored vectors are already peppered - only ever
// call this on a submitted one.
export function pepperFingerprint(submitted: unknown): HardwareFingerprint {
  const pepper = requiredEnv("FINGERPRINT_PEPPER");

  const out: Record<string, string> = {};
  for (const key of FINGERPRINT_SOURCES) {
    const clientHash = field(submitted, key);
    if (!clientHash) continue;
    out[key] = crypto
      .createHmac("sha256", pepper)
      .update(`${key}|${clientHash}`)
      .digest("hex");
  }
  return out as HardwareFingerprint;
}

function field(vec: unknown, key: FingerprintSource): string | null {
  if (typeof vec !== "object" || vec === null) return null;
  const v = (vec as Record<string, unknown>)[key];
  return typeof v === "string" && v.length > 0 ? v : null;
}

export function recognizedSourceCount(vec: unknown): number {
  let n = 0;
  for (const key of FINGERPRINT_SOURCES) if (field(vec, key)) n += 1;
  return n;
}

// Minimum sources to bind or compare. Below this a vector can't even match
// itself ({} passes the route validation, so this is the real check).
export const MIN_BINDABLE_SOURCES = 3;

// Only compares sources present on both sides. A missing source (e.g. "To be
// filled by O.E.M." boards, which the desktop drops) isn't a mismatch.
// accept: MATCH_THRESHOLD matches with MIN_STRONG_MATCHES strong. One short of
// either is "flag" (still served, but recorded).
export function compareHardwareFingerprints(
  stored: unknown,
  submitted: unknown,
): FingerprintComparison {
  let matchCount = 0;
  let strongMatchCount = 0;
  let comparableCount = 0;
  const delta: FingerprintSource[] = [];

  for (const key of FINGERPRINT_SOURCES) {
    const a = field(stored, key);
    const b = field(submitted, key);
    if (!a || !b) continue;

    comparableCount += 1;
    if (a === b) {
      matchCount += 1;
      if (STRONG_SOURCES.includes(key)) strongMatchCount += 1;
    } else {
      delta.push(key);
    }
  }

  let verdict: FingerprintVerdict;
  if (comparableCount < MIN_BINDABLE_SOURCES) {
    verdict = "reject";
  } else if (
    matchCount >= MATCH_THRESHOLD &&
    strongMatchCount >= MIN_STRONG_MATCHES
  ) {
    verdict = "accept";
  } else if (matchCount >= MATCH_THRESHOLD - 1 && strongMatchCount >= 2) {
    verdict = "flag";
  } else {
    verdict = "reject";
  }

  return { matchCount, strongMatchCount, comparableCount, delta, verdict };
}
