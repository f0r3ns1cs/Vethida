"use client";

import { useEffect, useRef, useState } from "react";
import Link from "next/link";
import { usePathname } from "next/navigation";
import { AnimatePresence, motion, useReducedMotion } from "motion/react";

import { DiscordIcon, MenuIcon, XIcon } from "@/components/icons";
import { primaryNav, site } from "@/lib/site";

// absolute, not fixed: .site-header's backdrop-filter makes it the containing block

export function MobileNav() {
  const [open, setOpen] = useState(false);
  const pathname = usePathname();
  const reduced = useReducedMotion();
  const triggerRef = useRef<HTMLButtonElement>(null);
  const panelRef = useRef<HTMLDivElement>(null);

  // close on back/forward (links call close() themselves)
  const [navigatedFrom, setNavigatedFrom] = useState(pathname);
  if (pathname !== navigatedFrom) {
    setNavigatedFrom(pathname);
    setOpen(false);
  }

  useEffect(() => {
    if (!open) return;

    function onKeyDown(event: KeyboardEvent) {
      if (event.key === "Escape") setOpen(false);
    }

    // lock scrolling, restore whatever was there before
    const previousOverflow = document.body.style.overflow;
    document.body.style.overflow = "hidden";
    document.addEventListener("keydown", onKeyDown);

    panelRef.current?.focus();

    return () => {
      document.body.style.overflow = previousOverflow;
      document.removeEventListener("keydown", onKeyDown);
    };
  }, [open]);

  function close() {
    setOpen(false);
    triggerRef.current?.focus();
  }

  return (
    <div className="lg:hidden">
      <button
        ref={triggerRef}
        type="button"
        aria-expanded={open}
        aria-controls="mobile-nav-panel"
        aria-label={open ? "Close menu" : "Open menu"}
        onClick={() => (open ? close() : setOpen(true))}
        className="flex h-10 w-10 items-center justify-center rounded-md text-foreground-muted transition-colors hover:text-foreground"
      >
        {open ? (
          <XIcon className="h-5 w-5" />
        ) : (
          <MenuIcon className="h-5 w-5" />
        )}
      </button>

      <AnimatePresence>
        {open ? (
          <>
            <motion.div
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              exit={{ opacity: 0 }}
              transition={{ duration: reduced ? 0 : 0.15 }}
              onClick={close}
              className="absolute inset-x-0 top-full z-30 h-[100dvh] bg-foreground/20"
              aria-hidden
            />

            <motion.div
              ref={panelRef}
              id="mobile-nav-panel"
              tabIndex={-1}
              role="dialog"
              aria-modal="true"
              aria-label="Site menu"
              initial={reduced ? { opacity: 0 } : { opacity: 0, y: -8 }}
              animate={{ opacity: 1, y: 0 }}
              exit={reduced ? { opacity: 0 } : { opacity: 0, y: -8 }}
              transition={{ duration: reduced ? 0 : 0.18, ease: "easeOut" }}
              className="absolute inset-x-0 top-full z-40 max-h-[calc(100dvh-4rem)] overflow-y-auto overscroll-contain border-b border-border bg-background outline-none"
            >
              <nav aria-label="Primary" className="band__inner py-5">
                {primaryNav.map((group) => (
                  <div key={group.label} className="mb-6 last:mb-0">
                    {group.href ? (
                      <Link
                        href={group.href}
                        onClick={close}
                        className="block py-1 text-body font-semibold text-foreground"
                      >
                        {group.label}
                      </Link>
                    ) : (
                      <>
                        <p className="eyebrow eyebrow--quiet pb-1">
                          {group.label}
                        </p>
                        <ul className="flex flex-col">
                          {(group.items ?? []).map((item) => (
                            <li key={item.href}>
                              <Link
                                href={item.href}
                                onClick={close}
                                className="block py-2.5 text-body text-foreground-muted transition-colors hover:text-foreground"
                              >
                                {item.label}
                              </Link>
                            </li>
                          ))}
                        </ul>
                      </>
                    )}
                  </div>
                ))}

                <div className="mt-6 flex flex-col gap-3 border-t border-border pt-6">
                  <a
                    href={site.links.billing}
                    target="_blank"
                    rel="noopener noreferrer"
                    onClick={close}
                    className="btn btn-lg btn-secondary"
                  >
                    Manage plan
                  </a>
                  <a
                    href={site.links.discord}
                    target="_blank"
                    rel="noopener noreferrer"
                    onClick={close}
                    className="inline-flex items-center gap-2 pt-1 text-meta text-foreground-muted transition-colors hover:text-foreground"
                  >
                    <DiscordIcon className="h-5 w-5" />
                    Join the Discord
                  </a>
                </div>
              </nav>
            </motion.div>
          </>
        ) : null}
      </AnimatePresence>
    </div>
  );
}
