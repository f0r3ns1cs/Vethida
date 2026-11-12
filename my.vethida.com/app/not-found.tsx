import Link from "next/link";

export default function NotFound() {
  return (
    <main className="mx-auto flex min-h-screen max-w-2xl flex-col justify-center px-4 py-10">
      <p className="text-sm font-medium tracking-wide text-foreground-muted">
        Error 404
      </p>
      <h1 className="mt-1 text-lg font-semibold">Page not found</h1>
      <p className="mt-3 max-w-lg text-sm leading-relaxed text-foreground-muted">
        This page doesn&apos;t exist or has moved. Your account is fine. Pick up
        from the dashboard.
      </p>
      <div className="mt-8">
        <Link href="/dashboard" className="btn btn-secondary h-9 px-4">
          Back to dashboard
        </Link>
      </div>
    </main>
  );
}
