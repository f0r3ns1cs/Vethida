import type { Metadata } from "next";
import { notFound } from "next/navigation";

import { LegalPage } from "@/components/legal-page";
import { getLegalDoc, listLegalRouteDocs } from "@/lib/legal";
import { pageMetadata } from "@/lib/metadata";

// every doc in legal/ except the ones with their own `path`
export const dynamicParams = false;

export function generateStaticParams() {
  return listLegalRouteDocs().map((doc) => ({ slug: doc.slug }));
}

export async function generateMetadata({
  params,
}: {
  params: Promise<{ slug: string }>;
}): Promise<Metadata> {
  const { slug } = await params;
  const doc = getLegalDoc(slug);
  if (!doc) return {};
  return pageMetadata({
    title: doc.title,
    description: doc.description,
    path: `/legal/${doc.slug}`,
  });
}

export default async function Page({
  params,
}: {
  params: Promise<{ slug: string }>;
}) {
  const { slug } = await params;
  const doc = getLegalDoc(slug);
  if (!doc) notFound();

  return (
    <LegalPage title={doc.title} updated={doc.updated || undefined}>
      {/* our own markdown, rendered at build time, never user input */}
      <div dangerouslySetInnerHTML={{ __html: doc.html }} />
    </LegalPage>
  );
}
