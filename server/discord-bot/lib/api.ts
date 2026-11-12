import { PAID_TIERS } from "@vethida/db";

import {
  API_BASE_URL,
  INTERNAL_API_TOKEN,
  STATUS_URL,
  type TierSlug,
} from "./config";
import { logger } from "./logger";

const REQUEST_TIMEOUT_MS = 8_000;
let missingTokenWarningShown = false;

function createHeaders(accept: string): Headers {
  const headers = new Headers({
    Accept: accept,
    "User-Agent": "Vethida-Discord-Bot/1.0",
  });

  if (INTERNAL_API_TOKEN) {
    headers.set("Authorization", `Bearer ${INTERNAL_API_TOKEN}`);
  } else if (!missingTokenWarningShown) {
    missingTokenWarningShown = true;
    logger.warn(
      "VETHIDA_INTERNAL_API_TOKEN is not set - internal API calls will fail",
    );
  }

  return headers;
}

async function fetchWithTimeout(
  url: URL | string,
  init: RequestInit,
): Promise<Response | null> {
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), REQUEST_TIMEOUT_MS);

  try {
    return await fetch(url, {
      ...init,
      signal: controller.signal,
    });
  } catch (err) {
    logger.debug({ err, url: String(url) }, "upstream fetch failed");
    return null;
  } finally {
    clearTimeout(timeout);
  }
}

// GET /v1/license/lookup  (api.vethida.com, internal bearer)

export type LicenseStatus = "active" | "suspended" | "revoked" | "expired";

export interface LicenseInfo {
  licenseId: string;
  userId: string | null;
  tier: string; // "pro" | "ultimate"
  status: LicenseStatus;
  expiresAt: string | null;
}

// null if the key isn't found or the api is unreachable
export async function lookupLicense(licenseKey: string): Promise<LicenseInfo | null> {
  const url = new URL("/v1/license/lookup", API_BASE_URL);
  url.searchParams.set("key", licenseKey);

  const response = await fetchWithTimeout(url, {
    headers: createHeaders("application/json"),
  });

  if (!response?.ok) {
    return null;
  }

  try {
    const body = (await response.json()) as {
      license_id?: string;
      user_id?: string | null;
      tier?: string;
      status?: LicenseStatus;
      expires_at?: string | null;
    };
    if (typeof body.license_id !== "string" || typeof body.tier !== "string") {
      return null;
    }
    return {
      licenseId: body.license_id,
      userId: body.user_id ?? null,
      tier: body.tier,
      status: (body.status ?? "active"),
      expiresAt: body.expires_at ?? null,
    };
  } catch {
    return null;
  }
}

export function tierLabelToSlug(tier: string): TierSlug | null {
  const normalized = tier.trim().toLowerCase();
  return PAID_TIERS.find((slug) => slug === normalized) ?? null;
}

export function grantsRole(tier: string): boolean {
  return tierLabelToSlug(tier) !== null;
}

// GET /v1/version  (api.vethida.com)

export interface VethidaRelease {
  version: string;
  publishedAt: string;
  channel?: string;
  sha256?: string;
  sizeBytes?: number;
  downloadUrl?: string;
  notes?: string;
}

// unpublished = 404 (nothing released yet)
export type ReleaseLookup =
  | { state: "ok"; release: VethidaRelease }
  | { state: "unpublished" }
  | { state: "unavailable" };

export async function getLatestVethidaVersion(): Promise<ReleaseLookup> {
  const response = await fetchWithTimeout(
    new URL("/v1/version", API_BASE_URL),
    { headers: createHeaders("application/json") },
  );
  if (!response) {
    return { state: "unavailable" };
  }
  if (response.status === 404) {
    return { state: "unpublished" };
  }
  if (!response.ok) {
    return { state: "unavailable" };
  }

  try {
    const body = (await response.json()) as Partial<VethidaRelease>;
    if (typeof body.version !== "string" || typeof body.publishedAt !== "string") {
      return { state: "unavailable" };
    }
    return { state: "ok", release: body as VethidaRelease };
  } catch {
    return { state: "unavailable" };
  }
}

// external status page

export interface StatusSummary {
  status: "operational" | "degraded" | "down";
  components: Array<{ name: string; status: string }>;
}

export async function getStatusSummary(): Promise<StatusSummary | null> {
  if (!STATUS_URL) {
    return null;
  }

  const response = await fetchWithTimeout(
    new URL("/api/status.json", STATUS_URL),
    { headers: createHeaders("application/json") },
  );
  if (!response?.ok) {
    return null;
  }
  try {
    return (await response.json()) as StatusSummary;
  } catch {
    return null;
  }
}
