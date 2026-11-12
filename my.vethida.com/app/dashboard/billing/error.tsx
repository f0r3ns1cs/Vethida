"use client";

import * as Sentry from "@sentry/nextjs";
import Link from "next/link";
import { useEffect } from "react";

// Billing-specific error page. Actions here only write after Polar confirms,
// so an error means nothing changed.
export default function BillingError({
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
    <main className="mx-auto max-w-3xl px-4 py-10">
      <header className="page-header">
        <h1 className="page-title">Billing is unavailable</h1>
      </header>

      <section className="card p-6">
        <p className="text-sm leading-relaxed text-foreground">
          We could not load your billing details just now.
        </p>
        <ul className="mt-4 space-y-2 text-sm text-foreground-muted">
          <li>
            You have not been charged and nothing has been canceled. This error
            happened before any change could be made.
          </li>
          <li>
            Your subscription is exactly as it was, and your licenses keep
            working on your machines in the meantime.
          </li>
          <li>
            If you were part-way through canceling, that is not lost. Start it
            again once this page loads.
          </li>
        </ul>

        <div className="mt-6 flex items-center gap-3">
          <button onClick={reset} className="btn btn-secondary h-9 px-4">
            Try again
          </button>
          <Link href="/dashboard" className="btn btn-secondary h-9 px-4">
            Back to dashboard
          </Link>
        </div>
      </section>

      <p className="mt-6 text-xs text-foreground-faint">
        Anything to do with a charge, email{" "}
        <a
          href="mailto:support@vethida.com"
          className="underline underline-offset-4"
        >
          support@vethida.com
        </a>{" "}
        and a person will sort it out
        {error.digest ? <>, quoting reference {error.digest}</> : null}.
      </p>
    </main>
  );
}
