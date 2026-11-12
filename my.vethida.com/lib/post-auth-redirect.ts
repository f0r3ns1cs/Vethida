// Remembers ?next= (used by /desktop/authorize) in sessionStorage across the
// sign-in steps so we can send the user back afterwards.

const KEY = "vethida.postAuthRedirect";

export const DEFAULT_POST_AUTH_REDIRECT = "/dashboard";

// relative paths only, no open redirects
export function sanitizeNext(raw: string | null | undefined): string | null {
  if (typeof raw !== "string" || raw.length === 0) return null;
  if (!raw.startsWith("/")) return null;
  if (raw.startsWith("//")) return null;
  // "/\evil.com"
  if (raw.includes("\\")) return null;
  return raw;
}

export function readNextFromLocation(): string | null {
  if (typeof window === "undefined") return null;
  return sanitizeNext(new URLSearchParams(window.location.search).get("next"));
}

export function rememberPostAuthRedirect(next: string | null): void {
  if (typeof window === "undefined") return;
  const safe = sanitizeNext(next);
  if (safe) window.sessionStorage.setItem(KEY, safe);
  else window.sessionStorage.removeItem(KEY);
}

// read once and clear
export function takePostAuthRedirect(): string {
  if (typeof window === "undefined") return DEFAULT_POST_AUTH_REDIRECT;
  const stored = sanitizeNext(window.sessionStorage.getItem(KEY));
  window.sessionStorage.removeItem(KEY);
  return stored ?? DEFAULT_POST_AUTH_REDIRECT;
}
