import { headers } from "next/headers";
import { redirect } from "next/navigation";
import { auth } from "@/lib/auth";
import { PortalShell } from "@/components/portal-shell";

export const dynamic = "force-dynamic";

// Redirects signed-out users. Pages still check the session themselves since
// layouts and pages render in parallel.
export default async function DashboardLayout({
  children,
}: Readonly<{ children: React.ReactNode }>) {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session) redirect("/sign-in");

  return (
    <PortalShell
      email={session.user.email}
      isAdmin={session.user.role === "admin"}
    >
      {children}
    </PortalShell>
  );
}
