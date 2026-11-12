import type { Metadata } from "next";
import { notFound } from "next/navigation";

import { CookieConsentWithdraw } from "@/components/cookie-consent-withdraw";
import { LegalPage } from "@/components/legal-page";
import { getLegalDoc } from "@/lib/legal";
import { pageMetadata } from "@/lib/metadata";

// the withdraw button gets rendered where this marker is in cookies.md
const SPLIT = "<!--consent-withdraw-->";

export async function generateMetadata(): Promise<Metadata> {
  const doc = getLegalDoc("cookies");
  if (!doc) return {};
  return pageMetadata({
    title: doc.title,
    description: doc.description,
    path: "/legal/cookies",
  });
}

export default function Page() {
  const doc = getLegalDoc("cookies");
  if (!doc) notFound();

  const parts = doc.html.split(SPLIT);
  if (parts.length !== 2) {
    throw new Error(
      "legal/cookies.md is missing exactly one <!--consent-withdraw--> marker",
    );
  }
  const [before, after] = parts;

  return (
    <LegalPage title={doc.title} updated={doc.updated || undefined}>
      <div dangerouslySetInnerHTML={{ __html: before }} />
      <CookieConsentWithdraw />
      <div dangerouslySetInnerHTML={{ __html: after }} />
    </LegalPage>
  );
}
