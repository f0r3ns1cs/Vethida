import type { Metadata } from "next";
import { notFound } from "next/navigation";

import { LegalPage } from "@/components/legal-page";
import { getLegalDoc } from "@/lib/legal";
import { pageMetadata } from "@/lib/metadata";

// security.txt points here, don't move it (legal/security.md sets `path` so it's
// not also under /legal/)
const SLUG = "security";

export async function generateMetadata(): Promise<Metadata> {
  const doc = getLegalDoc(SLUG);
  if (!doc) return {};
  return pageMetadata({
    title: doc.title,
    description: doc.description,
    path: doc.path,
  });
}

export default function Page() {
  const doc = getLegalDoc(SLUG);
  if (!doc) notFound();

  return (
    <LegalPage title={doc.title} updated={doc.updated || undefined}>
      <div dangerouslySetInnerHTML={{ __html: doc.html }} />
    </LegalPage>
  );
}
