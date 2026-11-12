import { requireAdmin } from "@/lib/admin";
import { PortalShell } from "@/components/portal-shell";

export const dynamic = "force-dynamic";

export default async function AdminLayout({
  children,
}: Readonly<{ children: React.ReactNode }>) {
  const session = await requireAdmin();

  return (
    <PortalShell email={session.user.email} isAdmin>
      {children}
    </PortalShell>
  );
}
