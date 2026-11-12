import { NextResponse } from "next/server";

// The desktop client switches on `code`, so these are stable API. Don't rename
// any, deployed builds depend on them.
export const ERROR_CODES = {
  // generic
  INTERNAL_ERROR: "internal_error",
  INVALID_JSON: "invalid_json",
  PAYLOAD_TOO_LARGE: "payload_too_large",
  RATE_LIMITED: "rate_limited",
  UNAUTHORIZED: "unauthorized",
  NOT_FOUND: "not_found",

  // licensing
  INVALID_LICENSE_KEY: "invalid_license_key",
  LICENSE_UNKNOWN: "license_unknown",
  LICENSE_EXPIRED: "license_expired",
  LICENSE_REVOKED: "license_revoked",
  MACHINE_LIMIT_REACHED: "machine_limit_reached",
  NO_RESETS_REMAINING: "no_resets_remaining",
  // account suspended (the license itself isn't revoked)
  ACCOUNT_SUSPENDED: "account_suspended",

  // devices / fingerprint
  INVALID_DEVICE_CODE: "invalid_device_code",
  INVALID_DEVICE_PUBLIC_KEY: "invalid_device_public_key",
  INVALID_FINGERPRINT: "invalid_fingerprint",
  FINGERPRINT_MISMATCH: "fingerprint_mismatch",
  DEVICE_NOT_FOUND: "device_not_found",
  DEVICE_DEACTIVATED: "device_deactivated",
  DEVICE_AUTH_MISSING: "device_auth_missing",
  DEVICE_AUTH_MALFORMED: "device_auth_malformed",
  DEVICE_AUTH_CLOCK_SKEW: "device_auth_clock_skew",
  DEVICE_SIGNATURE_INVALID: "device_signature_invalid",

  // browser sign-in (RFC 8628 device flow). The client treats the first two the same.
  AUTH_SESSION_EXPIRED: "auth_session_expired",
  AUTH_ALREADY_USED: "auth_already_used",
  AUTH_DENIED: "auth_denied",

  // webhooks (only Polar sees this)
  WEBHOOK_SIGNATURE_INVALID: "webhook_signature_invalid",
} as const;

export type ApiErrorCode = (typeof ERROR_CODES)[keyof typeof ERROR_CODES];

export interface ApiErrorBody {
  error: {
    code: ApiErrorCode;
    message: string;
    details?: Record<string, unknown>;
  };
}

export function apiError(
  status: number,
  code: ApiErrorCode,
  message: string,
  options?: { details?: Record<string, unknown>; headers?: HeadersInit },
): NextResponse<ApiErrorBody> {
  return NextResponse.json(
    {
      error: {
        code,
        message,
        ...(options?.details ? { details: options.details } : {}),
      },
    },
    { status, headers: options?.headers },
  );
}

export function rateLimited(
  retryAfterSeconds?: number,
): NextResponse<ApiErrorBody> {
  return apiError(429, ERROR_CODES.RATE_LIMITED, "Too many requests.", {
    headers: retryAfterSeconds
      ? { "Retry-After": String(Math.ceil(retryAfterSeconds)) }
      : undefined,
  });
}
