import type { Metadata } from "next";

import { CompareTable } from "@/components/compare-table";
import { PageCta } from "@/components/page-cta";
import { pageMetadata } from "@/lib/metadata";

export const metadata: Metadata = pageMetadata({
  title: "Features",
  description:
    "Every setting Vethida reaches, from the processor and power through graphics, network and debloating, and which plan each one needs.",
  path: "/features",
});

export default function Page() {
  return (
    <main>
      <section className="hero-wash masthead masthead--center">
        <div className="band__inner">
          <h1 className="masthead__title">Features</h1>
          <p className="masthead__deck">
            Compare what each plan includes.
          </p>
        </div>
      </section>

      <section className="band band--tinted band--tight">
        <div className="band__inner" style={{ maxWidth: "60rem" }}>
          <CompareTable />
        </div>
      </section>

      <PageCta variant="buy" />
    </main>
  );
}
