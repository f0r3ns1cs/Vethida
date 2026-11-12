"use client";

import Link from "next/link";
import { usePathname, useRouter } from "next/navigation";
import { useEffect, useState } from "react";
import { signOut } from "@/lib/auth-client";

export interface NavItem {
  href: string;
  label: string;
  section?: string; // group heading
}

export interface PortalNavProps {
  items: NavItem[];
  email: string;
}

// prefix match, except /dashboard itself
function isCurrent(pathname: string, href: string): boolean {
  if (href === "/dashboard") return pathname === "/dashboard";
  return pathname === href || pathname.startsWith(`${href}/`);
}

export function PortalNav({ items, email }: PortalNavProps) {
  const pathname = usePathname();
  const router = useRouter();
  const [open, setOpen] = useState(false);
  const [signingOut, setSigningOut] = useState(false);

  // Esc closes the drawer
  useEffect(() => {
    if (!open) return;
    function onKey(e: KeyboardEvent) {
      if (e.key === "Escape") setOpen(false);
    }
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [open]);

  const sections = items.reduce<Record<string, NavItem[]>>((acc, item) => {
    const key = item.section ?? "";
    (acc[key] ??= []).push(item);
    return acc;
  }, {});

  const list = (
    <nav aria-label="Portal" className="flex h-full flex-col">
      <div className="px-3 py-4">
        <Link
          href="/dashboard"
          className="flex items-center gap-2 px-1 text-sm font-semibold tracking-tight"
        >
          Vethida
        </Link>
      </div>

      <div className="flex-1 overflow-y-auto px-3 pb-4">
        {Object.entries(sections).map(([section, entries]) => (
          <div key={section || "main"}>
            {section ? <p className="nav-section">{section}</p> : null}
            <ul className="space-y-0.5">
              {entries.map((item) => {
                const current = isCurrent(pathname, item.href);
                return (
                  <li key={item.href}>
                    <Link
                      href={item.href}
                      aria-current={current ? "page" : undefined}
                      onClick={() => setOpen(false)}
                      className={`nav-link ${current ? "nav-link-active" : ""}`}
                    >
                      {item.label}
                    </Link>
                  </li>
                );
              })}
            </ul>
          </div>
        ))}
      </div>

      <div className="border-t border-border px-3 py-3">
        <p
          className="truncate px-1 text-xs text-foreground-faint"
          title={email}
        >
          {email}
        </p>
        <button
          type="button"
          disabled={signingOut}
          className="nav-link mt-1 w-full text-left disabled:opacity-60"
          onClick={async () => {
            setSigningOut(true);
            // go to sign-in even if signOut fails
            try {
              await signOut();
            } finally {
              router.push("/sign-in");
              router.refresh();
            }
          }}
        >
          {signingOut ? "Signing out..." : "Sign out"}
        </button>
      </div>
    </nav>
  );

  return (
    <>
      {/* mobile bar */}
      <div className="flex items-center gap-3 border-b border-border bg-surface px-4 py-3 lg:hidden">
        <button
          type="button"
          className="btn btn-secondary btn-sm"
          aria-expanded={open}
          aria-controls="portal-nav-drawer"
          onClick={() => setOpen((v) => !v)}
        >
          {open ? "Close" : "Menu"}
        </button>
        <span className="text-sm font-semibold">Vethida</span>
      </div>

      {open ? (
        <div className="fixed inset-0 z-40 lg:hidden">
          <button
            type="button"
            aria-label="Close menu"
            tabIndex={-1}
            className="fade-in absolute inset-0 bg-black/60"
            onClick={() => setOpen(false)}
          />
          <div
            id="portal-nav-drawer"
            className="drawer-in absolute inset-y-0 left-0 w-64 border-r border-border bg-surface"
          >
            {list}
          </div>
        </div>
      ) : null}

      <aside className="hidden w-(--sidebar-width) shrink-0 border-r border-border bg-surface lg:block">
        <div className="sticky top-0 h-screen">{list}</div>
      </aside>
    </>
  );
}
