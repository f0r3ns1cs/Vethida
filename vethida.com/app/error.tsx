"use client";

import * as Sentry from "@sentry/nextjs";
import Link from "next/link";
import { useEffect } from "react";

export default function Error({
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
    <main className="masthead masthead--center">
      <div className="band__inner band__inner--prose">
        <h1 className="masthead__title">Something went wrong</h1>
        <p className="masthead__deck">
          An unexpected error occurred while loading this page. Try again, or
          return home and pick up where you left off.
        </p>

        <div className="masthead__actions">
          <button onClick={reset} className="btn btn-lg btn-primary">
            Try again
          </button>
          <Link href="/" className="btn btn-lg btn-secondary">
            Back to home
          </Link>
        </div>

        {error.digest ? (
          <p className="mt-8 text-micro text-foreground-faint">
            Reference code: {error.digest}
          </p>
        ) : null}
      </div>
    </main>
  );
}
