"use client";

import { useEffect, useState } from "react";
import Image from "next/image";
import Link from "next/link";

import { DiscordIcon } from "@/components/icons";
import { MobileNav } from "@/components/mobile-nav";
import { PrimaryNav } from "@/components/primary-nav";
import { downloadCta, site } from "@/lib/site";

const SCROLL_MARK_PX = 8;

export function Navbar() {
  const [scrolled, setScrolled] = useState(false);

  useEffect(() => {
    // also on mount, a reload mid-page doesn't fire scroll
    const onScroll = () => setScrolled(window.scrollY > SCROLL_MARK_PX);
    onScroll();
    window.addEventListener("scroll", onScroll, { passive: true });
    return () => window.removeEventListener("scroll", onScroll);
  }, []);

  return (
    <header className="site-header" data-scrolled={scrolled || undefined}>
      <div className="band__inner band__inner--full flex h-16 items-center gap-8">
        <Link href="/" className="flex shrink-0 items-center" aria-label="Vethida home">
          <Image
            src="/vethida-wordmark.svg"
            alt=""
            width={107}
            height={26}
            priority
          />
        </Link>

        <nav aria-label="Primary" className="hidden h-full flex-1 items-center lg:flex">
          <PrimaryNav />
        </nav>

        <div className="ml-auto flex items-center gap-2 lg:gap-3">
          <a
            href={site.links.discord}
            target="_blank"
            rel="noopener noreferrer"
            aria-label="Vethida on Discord"
            className="hidden size-9 items-center justify-center rounded-md text-foreground-muted transition-colors hover:bg-surface hover:text-foreground sm:flex"
          >
            <DiscordIcon className="size-5" />
          </a>
          <a
            href={site.links.billing}
            target="_blank"
            rel="noopener noreferrer"
            className="hidden text-meta font-medium text-foreground-muted transition-colors hover:text-foreground sm:block"
          >
            Manage plan
          </a>
          <Link
            href={downloadCta.href}
            className="btn btn-primary whitespace-nowrap"
          >
            {downloadCta.label}
          </Link>
          <MobileNav />
        </div>
      </div>
    </header>
  );
}
