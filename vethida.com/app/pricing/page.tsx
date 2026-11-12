import type { ReactNode } from "react";
import type { Metadata } from "next";
import Link from "next/link";

import { FaqAccordion } from "@/components/faq-accordion";
import { Plans } from "@/components/plans";
import { faqSection } from "@/lib/faqs";
import { pageMetadata } from "@/lib/metadata";
import { requirements } from "@/lib/site";

export const metadata: Metadata = pageMetadata({
  title: "Pricing",
  description:
    "Vethida Free, Pro and Ultimate. What each plan includes, what it runs on, and the terms before you buy.",
  path: "/pricing",
});

const buyingFaqs = faqSection("plans");

const TERMS: Array<{ term: string; body: ReactNode }> = [
  {
    term: "Requirements",
    body: `${requirements.os}, on a ${requirements.arch}.`,
  },
  {
    term: "Installer",
    body: (
      <>
        One build for all three tiers, at{" "}
        <Link href="/download">vethida.com/download</Link>. Your license key
        unlocks the tier you bought.
      </>
    ),
  },
  {
    term: "Price changes",
    body: (
      <>
        We email at least 30 days before any change, and the price shown at
        checkout is the price charged for that term.{" "}
        <Link href="/legal/subscription-terms">Subscription Terms</Link>.
      </>
    ),
  },
];

export default function Page() {
  return (
    <main>
      <section className="hero-wash masthead masthead--center">
        <div className="band__inner">
          <h1 className="masthead__title">Pricing</h1>
          <p className="masthead__deck">
            Start on Free. Pro and Ultimate are billed monthly or yearly, or
            bought once as a lifetime license.
          </p>
        </div>
      </section>

      <Plans variant="full" />

      <section className="band band--tinted">
        <div className="band__inner band__inner--text">
          <div className="section-head">
            <h2 className="section-head__title">Before you buy</h2>
          </div>
          <dl className="spec-list spec-list--terms">
            {TERMS.map((row) => (
              <div key={row.term} className="spec-row">
                <dt className="spec-row__key">{row.term}</dt>
                <dd className="spec-row__value">{row.body}</dd>
              </div>
            ))}
          </dl>
        </div>
      </section>

      <section className="band band--ruled band--tight">
        <div className="band__inner band__inner--text">
          <div className="section-head">
            <h2 className="section-head__title">Questions about plans</h2>
          </div>
          <FaqAccordion items={buyingFaqs} />
        </div>
      </section>
    </main>
  );
}
