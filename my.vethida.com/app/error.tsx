"use client";

import * as Sentry from "@sentry/nextjs";
import Link from "next/link";
import { useEffect } from "react";

// fallback error page for anything a page/action didn't handle
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
    <main className="mx-auto flex min-h-screen max-w-2xl flex-col justify-center px-4 py-10">
      <h1 className="text-lg font-semibold">Something went wrong</h1>
      <p className="mt-3 max-w-lg text-sm leading-relaxed text-foreground-muted">
        We hit an unexpected error loading this page. Your account, your
        licenses and your billing are unaffected. Nothing was changed.
      </p>

      <div className="mt-8 flex items-center gap-3">
        <button onClick={reset} className="btn btn-secondary h-9 px-4">
          Try again
        </button>
        <Link href="/dashboard" className="btn btn-secondary h-9 px-4">
          Back to dashboard
        </Link>
      </div>

      <p className="mt-8 text-xs text-foreground-faint">
        If this keeps happening, email{" "}
        <a
          href="mailto:support@vethida.com"
          className="underline underline-offset-4"
        >
          support@vethida.com
        </a>
        {error.digest ? <> and quote reference {error.digest}.</> : "."}
      </p>
    </main>
  );
}
