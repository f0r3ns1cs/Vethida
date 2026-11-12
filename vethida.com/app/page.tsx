import type { Metadata } from "next";
import Link from "next/link";

import { Capabilities } from "@/components/capabilities";
import { FaqAccordion } from "@/components/faq-accordion";
import { FillButton } from "@/components/fill-button";
import { HowItWorks } from "@/components/how-it-works";
import { ArrowRightIcon } from "@/components/icons";
import { PageCta } from "@/components/page-cta";
import { Plans } from "@/components/plans";
import { ProductShot, hasProductShot } from "@/components/product-shot";
import { faqSection } from "@/lib/faqs";

const homeFaqs = faqSection("safety").slice(0, 5);

export const metadata: Metadata = {
  alternates: { canonical: "/" },
};

export default function Page() {
  return (
    <main>
      <section
        className={`hero-wash masthead${hasProductShot ? " masthead--split" : ""}`}
      >
        <div className="band__inner">
          <div>
            <h1 className="masthead__title">
              Optimize, debloat, and speed up your PC with Vethida
            </h1>
            <p className="masthead__deck">
              Vethida optimizes the drivers, power settings and Windows internals
              behind your PC&apos;s performance, and strips out the bloatware and
              telemetry it shipped with.
            </p>
            <div className="masthead__actions">
              <FillButton
                href="/download"
                label="Download free"
                icon={ArrowRightIcon}
              />
              <Link href="/pricing" className="link-arrow link-arrow--lg">
                See plans
                <ArrowRightIcon className="btn-arrow" />
              </Link>
            </div>
          </div>

          <ProductShot />
        </div>
      </section>

      <Capabilities />

      <HowItWorks />

      <Plans />

      <section className="band band--tinted">
        <div className="band__inner band__inner--text">
          <div className="section-head section-head--center">
            <h2 className="section-head__title">Common questions</h2>
          </div>

          <FaqAccordion items={homeFaqs} />

          <p className="band-footer-link">
            <Link href="/faq" className="link-arrow">
              All questions
              <ArrowRightIcon className="btn-arrow" />
            </Link>
          </p>
        </div>
      </section>

      <PageCta variant="buy" />
    </main>
  );
}
