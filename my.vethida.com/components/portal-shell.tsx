import { PortalNav, type NavItem } from "@/components/portal-nav";

export interface PortalShellProps {
  email: string;
  isAdmin?: boolean; // from the server session
  children: React.ReactNode;
}

const CUSTOMER_NAV: NavItem[] = [
  { href: "/dashboard", label: "Overview" },
  { href: "/dashboard/licenses", label: "Licenses & devices" },
  { href: "/dashboard/billing", label: "Billing" },
  { href: "/dashboard/download-center", label: "Download" },
  { href: "/dashboard/settings", label: "Settings" },
];

const ADMIN_NAV: NavItem[] = [
  { href: "/admin", label: "User lookup", section: "Admin" },
  { href: "/admin/flags", label: "Flags", section: "Admin" },
  { href: "/admin/churn", label: "Churn", section: "Admin" },
];

export function PortalShell({ email, isAdmin, children }: PortalShellProps) {
  const items = isAdmin ? [...CUSTOMER_NAV, ...ADMIN_NAV] : CUSTOMER_NAV;

  return (
    <div className="flex min-h-screen flex-col lg:flex-row">
      <a href="#portal-main" className="skip-link">
        Skip to content
      </a>
      <PortalNav items={items} email={email} />
      <div className="min-w-0 flex-1">
        <main
          id="portal-main"
          className="enter mx-auto w-full max-w-4xl px-4 py-8 sm:px-6 lg:px-8 lg:py-10"
        >
          {children}
        </main>
      </div>
    </div>
  );
}
