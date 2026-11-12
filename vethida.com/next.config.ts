import type { NextConfig } from "next";
import path from "node:path";
import { withSentryConfig } from "@sentry/nextjs";

// No CSP here, proxy.ts sets it per request (two CSP headers get intersected).

const securityHeaders = [
  { key: "X-Content-Type-Options", value: "nosniff" },
  { key: "X-Frame-Options", value: "DENY" },
  { key: "X-DNS-Prefetch-Control", value: "off" },
  { key: "Referrer-Policy", value: "strict-origin-when-cross-origin" },
  { key: "Cross-Origin-Opener-Policy", value: "same-origin" },
  { key: "Cross-Origin-Resource-Policy", value: "same-origin" },
  { key: "Origin-Agent-Cluster", value: "?1" },
  {
    key: "Permissions-Policy",
    value:
      "accelerometer=(), autoplay=(), camera=(), display-capture=(), geolocation=(), gyroscope=(), hid=(), interest-cohort=(), microphone=(), midi=(), payment=(), publickey-credentials-get=(), serial=(), usb=(), bluetooth=(), fullscreen=()",
  },
  {
    key: "Strict-Transport-Security",
    value: "max-age=63072000; includeSubDomains; preload",
  },
];

const nextConfig: NextConfig = {
  // own install + lockfile
  turbopack: {
    root: path.resolve(/*turbopackIgnore: true*/ __dirname),
  },
  poweredByHeader: false,
  output: "standalone", // for the Docker image
  async redirects() {
    return [
      {
        source: "/favicon.ico",
        destination: "/icon-32.png",
        permanent: true,
      },
      { source: "/docs", destination: "/support", permanent: true },
      { source: "/docs/:path*", destination: "/support", permanent: true },
      { source: "/guides", destination: "/support", permanent: true },
      { source: "/guides/:path*", destination: "/support", permanent: true },
      { source: "/blog", destination: "/changelog", permanent: true },
      { source: "/blog/:path*", destination: "/changelog", permanent: true },
      { source: "/why-vethida", destination: "/features", permanent: true },
      // Stripe customer portal
      {
        source: "/billing",
        destination: "https://billing.stripe.com/p/login/REPLACE-PORTAL-LINK",
        permanent: false,
      },
    ];
  },
  async headers() {
    return [
      {
        source: "/(.*)",
        headers: securityHeaders,
      },
      {
        source: "/icon-:size(16|32|64|128|256).png",
        headers: [
          {
            key: "Cache-Control",
            value: "public, max-age=31536000, immutable",
          },
        ],
      },
    ];
  },
};

export default withSentryConfig(nextConfig, {
  // https://www.npmjs.com/package/@sentry/webpack-plugin#options
  org: "rens-engineering",

  project: "rens-engineering",

  silent: !process.env.CI,

  widenClientFileUpload: true,

  webpack: {
    treeshake: {
      removeDebugLogging: true,
    },
  },
});
