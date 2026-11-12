import type { Metadata } from "next";
import Link from "next/link";

import { PageCta } from "@/components/page-cta";
import { pageMetadata } from "@/lib/metadata";
import { legal, site } from "@/lib/site";

export const metadata: Metadata = pageMetadata({
  title: "About",
  description: "What Vethida is and who makes it.",
  path: "/about",
});

const PRINCIPLES = [
  {
    title: "See what changed",
    body: "Vethida lists the changes it makes, and most can be undone from the Restore page.",
  },
  {
    title: "Protections stay on",
    body: "Nothing in the recommended set turns off a Windows protection.",
  },
  {
    title: "Private by default",
    body: "Crash reports are off until you turn them on. This site has no analytics.",
  },
] as const;

export default function Page() {
  return (
    <main>
      <section className="hero-wash masthead">
        <div className="band__inner band__inner--text">
          <h1 className="masthead__title">About Vethida</h1>
          <p className="masthead__deck">
            Vethida optimizes Windows for the PC it runs on: drivers, power,
            network, privacy and the bloat Windows ships with.
          </p>
        </div>
      </section>

      <section className="band band--tinted">
        <div className="band__inner">
          <div className="card-grid card-grid--three">
            {PRINCIPLES.map((item) => (
              <div key={item.title} className="feature-card gap-3">
                <h2 className="text-subhead font-semibold text-foreground">
                  {item.title}
                </h2>
                <p className="text-meta text-foreground-muted">{item.body}</p>
              </div>
            ))}
          </div>
        </div>
      </section>

      <section className="band band--ruled">
        <div className="band__inner band__inner--text">
          <div className="section-head">
            <h2 className="section-head__title">The company</h2>
            <p className="section-head__deck">
              Vethida is made by {legal.entity}, {legal.jurisdiction}.
            </p>
          </div>

          <dl className="spec-list spec-list--terms">
            <div className="spec-row">
              <dt className="spec-row__key">Support</dt>
              <dd className="spec-row__value">
                <a href={`mailto:${legal.email.support}`}>{legal.email.support}</a>
              </dd>
            </div>
            <div className="spec-row">
              <dt className="spec-row__key">Privacy</dt>
              <dd className="spec-row__value">
                <a href={`mailto:${legal.email.privacy}`}>{legal.email.privacy}</a>
                {", "}
                <Link href="/legal/privacy">Privacy Policy</Link>
              </dd>
            </div>
            <div className="spec-row">
              <dt className="spec-row__key">Security</dt>
              <dd className="spec-row__value">
                <a href={`mailto:${legal.email.security}`}>{legal.email.security}</a>
              </dd>
            </div>
            <div className="spec-row">
              <dt className="spec-row__key">Discord</dt>
              <dd className="spec-row__value">
                <a href={site.links.discord} target="_blank" rel="noopener noreferrer">
                  {site.links.discordDisplay}
                </a>
              </dd>
            </div>
          </dl>
        </div>
      </section>

      <PageCta variant="buy" />
    </main>
  );
}
