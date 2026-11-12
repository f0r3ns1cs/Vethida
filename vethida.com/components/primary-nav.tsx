"use client";

import { useEffect, useId, useRef, useState } from "react";
import Link from "next/link";
import { usePathname } from "next/navigation";
import { AnimatePresence, motion, useReducedMotion } from "motion/react";

import { Chevron } from "@/components/chevron";
import { primaryNav, type NavGroup } from "@/lib/site";

// Desktop nav bar. One shared indicator (layoutId) for the whole row.

// gives the pointer time to cross the gap into the panel
const CLOSE_DELAY_MS = 120;

const CLIP_HIDDEN = "inset(0% 0% 100% 0%)";
const CLIP_VISIBLE = "inset(0% 0% 0% 0%)";
const PANEL_ENTER = { duration: 0.32, ease: [0.22, 1, 0.36, 1] } as const;
const PANEL_EXIT = { duration: 0.2, ease: [0.4, 0, 1, 1] } as const;
const INDICATOR_SPRING = { type: "spring", visualDuration: 0.3, bounce: 0.1 } as const;

function isUnder(pathname: string, href: string) {
  return pathname === href || pathname.startsWith(`${href}/`);
}

function activeGroupLabel(pathname: string, groups: readonly NavGroup[]) {
  const match = groups.find((group) =>
    group.href
      ? isUnder(pathname, group.href)
      : (group.items ?? []).some((item) => isUnder(pathname, item.href)),
  );
  return match?.label ?? null;
}

export function PrimaryNav() {
  const pathname = usePathname();
  const reduced = useReducedMotion();
  const rowRef = useRef<HTMLDivElement>(null);
  const closeTimer = useRef<ReturnType<typeof setTimeout> | null>(null);

  // hovered/focused item, null = indicator shows the current section
  const [pointed, setPointed] = useState<string | null>(null);
  // open panel (not the same as pointed)
  const [open, setOpen] = useState<string | null>(null);

  const active = activeGroupLabel(pathname, primaryNav);
  const marked = pointed ?? active;

  // close on navigation, during render so the panel doesn't flash over the new page
  const [navigatedFrom, setNavigatedFrom] = useState(pathname);
  if (pathname !== navigatedFrom) {
    setNavigatedFrom(pathname);
    setOpen(null);
    setPointed(null);
  }

  useEffect(() => () => clearClose(), []);

  // Esc closes, document listener so it works with focus inside the panel too
  useEffect(() => {
    if (!open) return;
    function onKeyDown(event: KeyboardEvent) {
      if (event.key !== "Escape") return;
      setOpen(null);
    }
    document.addEventListener("keydown", onKeyDown);
    return () => document.removeEventListener("keydown", onKeyDown);
  }, [open]);

  function clearClose() {
    if (closeTimer.current) {
      clearTimeout(closeTimer.current);
      closeTimer.current = null;
    }
  }

  function scheduleClose() {
    clearClose();
    closeTimer.current = setTimeout(() => {
      setOpen(null);
      setPointed(null);
    }, CLOSE_DELAY_MS);
  }

  return (
    <div
      ref={rowRef}
      className="nav-row"
      onPointerLeave={(event) => {
        if (event.pointerType === "mouse") scheduleClose();
      }}
      // close when focus leaves the bar
      onBlur={(event) => {
        if (!rowRef.current?.contains(event.relatedTarget as Node | null)) {
          setOpen(null);
          setPointed(null);
        }
      }}
    >
      {primaryNav.map((group) => (
        <NavItem
          key={group.label}
          group={group}
          pathname={pathname}
          marked={marked === group.label}
          open={open === group.label}
          reduced={reduced ?? false}
          onPoint={() => {
            clearClose();
            setPointed(group.label);
            if (group.items) setOpen(group.label);
          }}
          onFocus={() => {
            clearClose();
            setPointed(group.label);
          }}
          onToggle={() => {
            setOpen((current) =>
              current === group.label ? null : group.label,
            );
            setPointed(group.label);
          }}
          onNavigate={() => setOpen(null)}
        />
      ))}
    </div>
  );
}

function NavItem({
  group,
  pathname,
  marked,
  open,
  reduced,
  onPoint,
  onFocus,
  onToggle,
  onNavigate,
}: {
  group: NavGroup;
  pathname: string;
  marked: boolean;
  open: boolean;
  reduced: boolean;
  onPoint: () => void;
  onFocus: () => void;
  onToggle: () => void;
  onNavigate: () => void;
}) {
  const panelId = useId();
  const items = group.items ?? [];

  const indicator = marked ? (
    <motion.span
      layoutId="nav-indicator"
      className="nav-indicator"
      transition={reduced ? { duration: 0 } : INDICATOR_SPRING}
    />
  ) : null;

  if (group.href) {
    const isActive = isUnder(pathname, group.href);
    return (
      <div className="nav-item" onPointerEnter={onPoint}>
        <Link
          href={group.href}
          className="nav-link"
          aria-current={isActive ? "page" : undefined}
          onFocus={onFocus}
        >
          {group.label}
        </Link>
        {indicator}
      </div>
    );
  }

  return (
    <div className="nav-item" onPointerEnter={onPoint}>
      <button
        type="button"
        aria-expanded={open}
        aria-controls={panelId}
        className="nav-link"
        onFocus={onFocus}
        onClick={onToggle}
      >
        {group.label}
        <Chevron open={open} className="nav-chevron" />
      </button>

      {indicator}

      <AnimatePresence>
        {open ? (
          <motion.div
            id={panelId}
            className="nav-panel"
            initial={reduced ? false : { clipPath: CLIP_HIDDEN }}
            animate={{
              clipPath: CLIP_VISIBLE,
              transition: reduced ? { duration: 0 } : PANEL_ENTER,
            }}
            exit={{
              clipPath: CLIP_HIDDEN,
              transition: reduced ? { duration: 0 } : PANEL_EXIT,
            }}
          >
            <ul>
              {items.map((item) => (
                <li key={item.href}>
                  <Link
                    href={item.href}
                    aria-current={pathname === item.href ? "page" : undefined}
                    className="nav-panel__link"
                    onClick={onNavigate}
                  >
                    <span className="nav-panel__label">{item.label}</span>
                    {item.hint ? (
                      <span className="nav-panel__hint">{item.hint}</span>
                    ) : null}
                  </Link>
                </li>
              ))}
            </ul>
          </motion.div>
        ) : null}
      </AnimatePresence>
    </div>
  );
}
