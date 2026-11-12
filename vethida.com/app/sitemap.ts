import type { MetadataRoute } from "next";

import { listLegalDocs } from "@/lib/legal";
import { legalNav, siteUrl } from "@/lib/site";

// Static routes, ordered roughly by how much we want them crawled.
const ROUTES: ReadonlyArray<
  [path: string, changeFrequency: "weekly" | "monthly" | "yearly", priority: number]
> = [
  ["/download", "weekly", 0.9],
  ["/features", "weekly", 0.9],
  ["/pricing", "weekly", 0.8],
  ["/faq", "monthly", 0.7],
  ["/support", "monthly", 0.6],
  ["/about", "monthly", 0.5],
  ["/changelog", "weekly", 0.5],
  ["/security", "yearly", 0.4],
];

// legalNav overlaps the table above, and the first entry wins.
function dedupe(entries: MetadataRoute.Sitemap): MetadataRoute.Sitemap {
  const seen = new Set<string>();
  return entries.filter((entry) => {
    if (seen.has(entry.url)) return false;
    seen.add(entry.url);
    return true;
  });
}

export default function sitemap(): MetadataRoute.Sitemap {
  return dedupe([
    {
      url: siteUrl,
      changeFrequency: "weekly",
      priority: 1,
    },
    ...ROUTES.map(([path, changeFrequency, priority]) => ({
      url: `${siteUrl}${path}`,
      changeFrequency,
      priority,
    })),
    // Every document in legal/, whether or not the footer links it.
    ...listLegalDocs().map((doc) => ({
      url: `${siteUrl}${doc.path}`,
      changeFrequency: "yearly" as const,
      priority: 0.3,
    })),
    ...legalNav.map((item) => ({
      url: `${siteUrl}${item.href}`,
      changeFrequency: "yearly" as const,
      priority: 0.3,
    })),
  ]);
}
