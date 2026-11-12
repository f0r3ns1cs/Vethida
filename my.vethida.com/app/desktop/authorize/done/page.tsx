export const dynamic = "force-dynamic";
export const metadata = { title: "PC authorized" };

export default function DesktopAuthorizedPage() {
  return (
    <main className="flex min-h-screen items-center justify-center px-4">
      <div className="w-full max-w-md">
        <p className="mb-6 text-center text-sm font-medium tracking-wide text-foreground-muted">
          Vethida
        </p>
        <div className="card p-6 text-center">
          <h1 className="mb-2 text-base font-semibold">
            You&apos;re signed in
          </h1>
          <p className="text-sm text-foreground-muted">
            Go back to Vethida on your PC. It picks this up on its own within a
            few seconds.
          </p>
        </div>
      </div>
    </main>
  );
}
