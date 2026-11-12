"use client";

import * as Sentry from "@sentry/nextjs";
import { useEffect } from "react";

export default function GlobalError({
  error,
  reset,
}: {
  error: Error & { digest?: string };
  reset: () => void;
}) {
  useEffect(() => {
    Sentry.captureException(error);
  }, [error]);

  return (
    <html lang="en">
      <body
        style={{
          margin: 0,
          minHeight: "100vh",
          display: "flex",
          flexDirection: "column",
          alignItems: "flex-start",
          justifyContent: "center",
          padding: "0 1.5rem",
          background: "#ffffff",
          color: "#0c1424",
          fontFamily:
            "ui-sans-serif, system-ui, -apple-system, 'Segoe UI', sans-serif",
        }}
      >
        <h1
          style={{
            margin: 0,
            fontSize: 32,
            fontWeight: 600,
            letterSpacing: "-0.02em",
          }}
        >
          Something went wrong
        </h1>
        <p style={{ margin: "1rem 0 0", color: "#5b6472", fontSize: 14 }}>
          An unexpected error occurred. Please try again in a moment.
        </p>
        <button
          onClick={reset}
          style={{
            marginTop: 28,
            padding: "10px 18px",
            background: "transparent",
            color: "#0c1424",
            border: "1px solid #d7dbe2",
            borderRadius: 10,
            cursor: "pointer",
            fontSize: 14,
            fontWeight: 500,
          }}
        >
          Try again
        </button>
      </body>
    </html>
  );
}
