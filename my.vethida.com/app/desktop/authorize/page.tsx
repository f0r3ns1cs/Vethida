import { headers } from "next/headers";
import Link from "next/link";
import { redirect } from "next/navigation";
import {
  findLiveSessionByUserCode,
  normalizeUserCode,
  selectableLicensesFor,
  tierLabel,
} from "@vethida/db";
import { auth } from "@/lib/auth";
import { AuthorizeForm } from "./authorize-form";

export const dynamic = "force-dynamic";
export const metadata = { title: "Authorize this PC" };

// simpler frame than AuthCard, people land here from the desktop app
function Shell({
  title,
  subtitle,
  children,
}: {
  title: string;
  subtitle?: string;
  children: React.ReactNode;
}) {
  return (
    <main className="flex min-h-screen flex-col items-center justify-center px-4 py-10">
      <div className="enter w-full max-w-md">
        <div className="mb-6 text-center">
          <p className="text-sm font-semibold tracking-tight">Vethida</p>
          <p className="mt-0.5 text-xs text-foreground-faint">
            Device authorization
          </p>
        </div>
        <div className="card">
          <div className="card-body">
            <h1 className="text-base font-semibold">{title}</h1>
            {subtitle ? (
              <p className="mt-1.5 text-sm leading-relaxed text-foreground-muted">
                {subtitle}
              </p>
            ) : null}
            <div className="mt-5">{children}</div>
          </div>
        </div>
      </div>
    </main>
  );
}

export default async function AuthorizeDesktopPage({
  searchParams,
}: {
  searchParams: Promise<{ code?: string }>;
}) {
  const { code } = await searchParams;
  const userCode = normalizeUserCode(code ?? "");

  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) {
    // come back here with the code after signing in
    const next = `/desktop/authorize?code=${encodeURIComponent(code ?? "")}`;
    redirect(`/sign-in?next=${encodeURIComponent(next)}`);
  }

  if (!userCode) {
    return (
      <Shell
        title="Nothing to authorize"
        subtitle="This page opens itself when your PC asks to sign in."
      >
        <p className="text-sm text-foreground-muted">
          Open Vethida on your PC and choose <strong>Sign in</strong>. Your
          browser will come back here with the request already filled in.
        </p>
      </Shell>
    );
  }

  const requested = await findLiveSessionByUserCode(userCode);
  if (!requested) {
    return (
      <Shell title="This request has expired">
        <div role="alert" className="alert alert-warning">
          <p>
            Sign-in requests are only good for ten minutes, so nothing was
            activated. Open Vethida on your PC and choose{" "}
            <strong>Sign in</strong> again.
          </p>
        </div>
      </Shell>
    );
  }

  const licenses = await selectableLicensesFor(session.user.id);
  if (licenses.length === 0) {
    return (
      <Shell
        title="No license on this account"
        subtitle="Signing in activates one of your licenses on that PC, and this account has none that can be used yet."
      >
        <Link href="/dashboard/licenses" className="btn btn-primary w-full">
          Get a license
        </Link>
      </Shell>
    );
  }

  return (
    <Shell title="Sign in to Vethida on your PC">
      <p className="mb-4 text-sm text-foreground-muted">
        A Vethida installation is asking to sign in as{" "}
        <strong className="text-foreground">{session.user.email}</strong>.
        Approving activates the license you pick on that PC.
      </p>

      {/* anti-phishing, has to match what the app shows */}
      <div className="mb-4 rounded-md border border-border-strong bg-background p-4 text-center">
        <p className="text-xs text-foreground-faint">
          Check this matches the code shown in Vethida
        </p>
        <p className="code mt-2 text-2xl font-semibold tracking-[0.35em] text-foreground">
          {requested.userCode}
        </p>
      </div>

      <AuthorizeForm
        userCode={requested.userCode}
        licenses={licenses.map((l) => ({
          id: l.id,
          key: l.key,
          tier: tierLabel(l.tier),
        }))}
      />

      <p className="mt-4 text-xs text-foreground-faint">
        If you didn&apos;t start this from your own PC, decline. Nothing is
        activated until you approve.
      </p>
    </Shell>
  );
}
