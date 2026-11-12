import type { Breadcrumb, ErrorEvent } from "@sentry/nextjs";

const REDACTED = "[redacted]";
const MAX_DEPTH = 10;
const MAX_NODES = 10_000;

const SENSITIVE_KEYS: ReadonlyArray<RegExp> = [
  /password/i,
  /secret/i,
  /token/i,
  /api[_-]?key/i,
  /auth(?:orization)?/i,
  /bearer/i,
  /session/i,
  /cookie/i,
  /signature/i,
  /private[_-]?key/i,
  /email/i,
  /ip[_-]?address/i,
  /x[_-]?forwarded[_-]?for/i,
  /fingerprint/i,
  /hwid/i,
  /license/i,
];

// Replaced anywhere inside a string (breadcrumbs join all console args into one
// message). JWT pattern needs 10+ chars per segment so hostnames don't match.
const EMBEDDED_SECRETS: ReadonlyArray<RegExp> = [
  // license keys, 5x5 Crockford base32 (two alphabets, old and new)
  /\b[0-9A-HJ-NP-TV-Z]{5}(?:-[0-9A-HJ-NP-TV-Z]{5}){4}\b/g,
  /\b[0-9A-HJKMNP-TV-Z]{5}(?:-[0-9A-HJKMNP-TV-Z]{5}){4}\b/g,
  /\bBearer\s+[A-Za-z0-9._~+/=-]{8,}/gi,
  /\b[A-Za-z0-9_-]{10,}\.[A-Za-z0-9_-]{10,}\.[A-Za-z0-9_-]{5,}\b/g,
  /\b[A-Za-z0-9._%+-]+@[A-Za-z0-9-]+(?:\.[A-Za-z0-9-]+)+\b/g,
  /\b[a-f0-9]{32,}\b/gi,
];

// query params to redact from URLs
const URL_SENSITIVE_PARAMS: ReadonlySet<string> = new Set([
  "token",
  "code",
  "state",
  "email",
  "key",
  "access_token",
  "refresh_token",
  "api_key",
  "secret",
]);

function isSensitiveKey(key: string): boolean {
  return SENSITIVE_KEYS.some((re) => re.test(key));
}

function redactString(value: string): string {
  if (value.length < 8) return value;
  let out = value;
  for (const re of EMBEDDED_SECRETS) {
    out = out.replace(re, REDACTED);
  }
  return out;
}

interface WalkState {
  visited: WeakSet<object>;
  nodes: number;
}

function redactValue<T>(value: T, depth = 0, state?: WalkState): T {
  const s: WalkState = state ?? { visited: new WeakSet(), nodes: 0 };
  s.nodes += 1;
  if (s.nodes > MAX_NODES || depth > MAX_DEPTH) return REDACTED as unknown as T;

  if (value === null || value === undefined) return value;
  if (typeof value === "string") return redactString(value) as unknown as T;
  if (
    typeof value === "number" ||
    typeof value === "boolean" ||
    typeof value === "bigint"
  ) {
    return value;
  }
  if (typeof value === "function" || typeof value === "symbol") {
    return REDACTED as unknown as T;
  }

  if (Array.isArray(value)) {
    if (s.visited.has(value)) return REDACTED as unknown as T;
    s.visited.add(value);
    return value.map((v) => redactValue(v, depth + 1, s)) as unknown as T;
  }

  if (value instanceof Error) {
    return {
      name: value.name,
      message: redactString(value.message ?? ""),
      stack: value.stack ? redactString(value.stack) : undefined,
    } as unknown as T;
  }

  if (typeof value === "object") {
    if (s.visited.has(value as object)) return REDACTED as unknown as T;
    s.visited.add(value as object);
    const out: Record<string, unknown> = {};
    for (const [k, v] of Object.entries(value as Record<string, unknown>)) {
      out[k] = isSensitiveKey(k) ? REDACTED : redactValue(v, depth + 1, s);
    }
    return out as unknown as T;
  }

  return value;
}

function stripSensitiveQueryParams(url: string): string {
  try {
    const u = new URL(url, "http://scrub.invalid");
    let mutated = false;
    for (const key of Array.from(u.searchParams.keys())) {
      if (URL_SENSITIVE_PARAMS.has(key.toLowerCase())) {
        u.searchParams.set(key, REDACTED);
        mutated = true;
      }
    }
    if (!mutated) return url;
    return u.toString().replace(/^http:\/\/scrub\.invalid/, "");
  } catch {
    return url;
  }
}

export function scrubBreadcrumb(breadcrumb: Breadcrumb): Breadcrumb | null {
  const out: Breadcrumb = { ...breadcrumb };

  if (typeof out.message === "string") {
    out.message = redactString(out.message);
  }
  if (out.data) {
    const data = redactValue(out.data as Record<string, unknown>);
    for (const field of ["url", "to", "from"] as const) {
      if (typeof data[field] === "string") {
        data[field] = stripSensitiveQueryParams(data[field] as string);
      }
    }
    out.data = data;
  }
  return out;
}

export function scrubEvent(event: ErrorEvent): ErrorEvent | null {
  const out: ErrorEvent = { ...event };

  if (out.request) {
    out.request = redactValue(out.request);
    if (out.request.url) {
      out.request.url = stripSensitiveQueryParams(out.request.url);
    }
    if (typeof out.request.query_string === "string") {
      out.request.query_string = stripSensitiveQueryParams(
        "?" + out.request.query_string,
      ).replace(/^\?/, "");
    }
  }
  if (out.extra) out.extra = redactValue(out.extra);
  if (out.contexts) out.contexts = redactValue(out.contexts);
  if (out.tags) {
    const tags: typeof out.tags = {};
    for (const [k, v] of Object.entries(out.tags)) {
      tags[k] = typeof v === "string" ? redactString(v) : v;
    }
    out.tags = tags;
  }
  if (out.exception?.values) {
    out.exception.values = out.exception.values.map((ex) => ({
      ...ex,
      value: ex.value ? redactString(ex.value) : ex.value,
    }));
  }
  // in case something calls setUser
  if (out.user) {
    const user = { ...out.user };
    if (user.email) user.email = REDACTED;
    if (user.ip_address) user.ip_address = REDACTED;
    if (user.username) user.username = REDACTED;
    out.user = user;
  }
  return out;
}
