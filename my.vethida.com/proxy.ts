import { NextResponse, type NextRequest } from "next/server";

const isDev = process.env.NODE_ENV === "development";

function generateNonce(): string {
  const bytes = crypto.getRandomValues(new Uint8Array(16));
  let bin = "";
  for (const b of bytes) bin += String.fromCharCode(b);
  return btoa(bin);
}

// Turnstile needs script-src, frame-src and connect-src. Without frame-src sign-in silently breaks
const TURNSTILE_ORIGIN = "https://challenges.cloudflare.com";

// nonce based CSP, next picks the nonce up from the request header (pages are dynamic, see layout.tsx)
export function proxy(request: NextRequest) {
  const nonce = generateNonce();

  const csp = [
    "default-src 'self'",
    `script-src 'self' 'nonce-${nonce}' 'strict-dynamic' ${TURNSTILE_ORIGIN}${isDev ? " 'unsafe-eval'" : ""}`,
    "style-src 'self' 'unsafe-inline'",
    "img-src 'self' data:",
    "font-src 'self'",
    `frame-src ${TURNSTILE_ORIGIN}`,
    // sentry ingest
    `connect-src 'self' https://*.ingest.sentry.io https://*.ingest.us.sentry.io ${TURNSTILE_ORIGIN}${isDev ? " ws: wss:" : ""}`,
    "media-src 'none'",
    "worker-src 'self' blob:",
    "object-src 'none'",
    "base-uri 'self'",
    "form-action 'self'",
    "frame-ancestors 'none'",
    "upgrade-insecure-requests",
  ].join("; ");

  const requestHeaders = new Headers(request.headers);
  requestHeaders.set("x-nonce", nonce);
  requestHeaders.set("content-security-policy", csp);

  const response = NextResponse.next({
    request: { headers: requestHeaders },
  });
  response.headers.set("Content-Security-Policy", csp);
  return response;
}

export const config = {
  matcher: [
    {
      source: "/((?!_next/static|_next/image|favicon.ico).*)",
      missing: [
        { type: "header", key: "next-router-prefetch" },
        { type: "header", key: "purpose", value: "prefetch" },
      ],
    },
  ],
};
