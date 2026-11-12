import * as Sentry from "@sentry/nextjs";

import { scrubBreadcrumb, scrubEvent } from "@vethida/observability/sentry-scrub";

// edge runtime (proxy.ts)
Sentry.init({
  dsn: process.env.NEXT_PUBLIC_SENTRY_DSN,

  tracesSampleRate: process.env.NODE_ENV === "production" ? 0.1 : 1,
  enableLogs: false,
  sendDefaultPii: false,

  beforeSend: scrubEvent,
  beforeBreadcrumb: scrubBreadcrumb,
});
