import Image from "next/image";
import Link from "next/link";

import { DiscordIcon } from "@/components/icons";
import { footerNav, legal, legalNav, site } from "@/lib/site";

const linkClass =
  "text-meta text-foreground-muted underline-offset-4 transition-colors hover:text-foreground hover:underline";

export function Footer() {
  return (
    <footer className="band band--ruled band--tight">
      <div className="band__inner">
        <div className="footer-nav">
          <div className="footer-brand">
            <Link href="/" aria-label="Vethida home">
              <Image src="/vethida-wordmark.svg" alt="" width={107} height={26} />
            </Link>
            <p>Windows optimization for Windows 10 and 11. Free to start, with Pro and Ultimate when you want more.</p>
          </div>
          {footerNav.map((column) => (
            <nav key={column.label} aria-label={column.label}>
              <h2 className="eyebrow eyebrow--ink">{column.label}</h2>
              <ul className="mt-4 flex flex-col gap-3">
                {column.items.map((item) =>
                  item.external ? (
                    <li key={item.href}>
                      <a
                        href={item.href}
                        target="_blank"
                        rel="noopener noreferrer"
                        className={`${linkClass} inline-flex items-center gap-2`}
                      >
                        {item.icon === "discord" ? (
                          <DiscordIcon className="size-4" />
                        ) : null}
                        {item.label}
                      </a>
                    </li>
                  ) : (
                    <li key={item.href}>
                      <Link href={item.href} className={linkClass}>
                        {item.label}
                      </Link>
                    </li>
                  ),
                )}
              </ul>
            </nav>
          ))}
        </div>

        <div className="mt-10 border-t border-border pt-8">
          <nav
            aria-label="Legal and support"
            className="flex flex-wrap gap-x-5 gap-y-2.5"
          >
            {legalNav.map((item) => (
              <Link
                key={item.href}
                href={item.href}
                className="text-micro text-foreground-faint underline-offset-4 transition-colors hover:text-foreground hover:underline"
              >
                {item.label}
              </Link>
            ))}
          </nav>

          <p className="mt-6 text-meta text-foreground-faint">
            &copy; {new Date().getFullYear()} {legal.entity}
          </p>

          {/* Required trademark attribution. The register is the Trademarks
              section of legal/third-party.md. */}
          <p className="mt-2 text-micro text-foreground-faint">
            Windows and Microsoft are trademarks of Microsoft Corporation.{" "}
            {site.product} is not affiliated with or endorsed by Microsoft.
          </p>
        </div>
      </div>
    </footer>
  );
}
