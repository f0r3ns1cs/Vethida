import type { Metadata } from "next";
import { headers } from "next/headers";

import { FaqAccordion } from "@/components/faq-accordion";
import { PageCta } from "@/components/page-cta";
import { pageMetadata } from "@/lib/metadata";
import { allFaqs, faqSections } from "@/lib/faqs";
import { site } from "@/lib/site";

export const metadata: Metadata = pageMetadata({
  title: "Frequently asked questions",
  description:
    "What Vethida optimizes, whether it touches Windows Update or Defender, how licensing and machine limits work, and what happens if something goes wrong.",
  path: "/faq",
});

const faqJsonLd = {
  "@context": "https://schema.org",
  "@type": "FAQPage",
  mainEntity: allFaqs.map((f) => ({
    "@type": "Question",
    name: f.q,
    acceptedAnswer: { "@type": "Answer", text: f.a },
  })),
};

export default async function Page() {
  const nonce = (await headers()).get("x-nonce") ?? undefined;

  return (
    <>
      <script
        type="application/ld+json"
        nonce={nonce}
        // Escaping `<` keeps an answer from closing the script element.
        dangerouslySetInnerHTML={{
          __html: JSON.stringify(faqJsonLd).replace(/</g, "\\u003c"),
        }}
      />

      <main>
        <section className="hero-wash masthead">
          <div className="band__inner band__inner--text">
            <h1 className="masthead__title">Frequently asked questions</h1>
            <p className="masthead__deck">
              Licensing, safety, what Vethida changes, and what happens if
              something goes wrong.
            </p>
          </div>
        </section>

        <section className="band band--ruled">
          <div className="band__inner band__inner--text stack">
            {faqSections.map((section) => (
              <div key={section.id}>
                <h2 className="section-head__title">{section.title}</h2>
                <div className="stack__body">
                  <FaqAccordion items={section.faqs} />
                </div>
              </div>
            ))}
          </div>
        </section>

        <PageCta
          title="Still have questions?"
          body="Write to us and a person answers."
          primary={{ href: "/support", label: "Support" }}
          secondary={{
            href: site.links.discord,
            label: "Join the Discord",
            external: true,
          }}
        />
      </main>
    </>
  );
}
