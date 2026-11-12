import type { Metadata } from "next";

import { siteUrl } from "@/lib/site";

// Next replaces nested metadata instead of merging, so the OG image has to be repeated
const OG_IMAGE = {
  url: `${siteUrl}/opengraph-image`,
  width: 1200,
  height: 630,
  alt: "Vethida - optimize, debloat, and speed up your PC", // same as app/opengraph-image.tsx
} as const;

export function pageMetadata(opts: {
  title: string;
  description: string;
  /** Route path, e.g. "/pricing". Resolved against metadataBase. */
  path: string;
}): Metadata {
  const { title, description, path } = opts;
  return {
    title,
    description,
    alternates: { canonical: path },
    openGraph: {
      title,
      description,
      url: path,
      type: "website",
      images: [OG_IMAGE],
    },
    twitter: {
      card: "summary_large_image",
      title,
      description,
      images: [OG_IMAGE.url],
    },
  };
}
