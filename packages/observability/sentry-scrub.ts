import type { Breadcrumb, ErrorEvent } from "@sentry/nextjs";

import {
  REDACTED,
  redactString,
  redactValue,
  stripSensitiveQueryParams,
} from "./redact";

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
