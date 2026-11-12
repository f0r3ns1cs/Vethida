export const dynamic = "force-dynamic";
export const metadata = { title: "Authorization denied" };

export default function DesktopAuthorizeDeniedPage() {
  return (
    <main className="flex min-h-screen items-center justify-center px-4">
      <div className="w-full max-w-md">
        <p className="mb-6 text-center text-sm font-medium tracking-wide text-foreground-muted">
          Vethida
        </p>
        <div className="card p-6 text-center">
          <h1 className="mb-2 text-base font-semibold">Request declined</h1>
          <p className="text-sm text-foreground-muted">
            Nothing was activated and no license was used. If this wasn&apos;t
            you, your account is fine - the request could not do anything
            without this approval.
          </p>
        </div>
      </div>
    </main>
  );
}
