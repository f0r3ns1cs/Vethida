import type { Metadata, Viewport } from "next";
import { headers } from "next/headers";
import { IBM_Plex_Sans, IBM_Plex_Mono } from "next/font/google";
import "./globals.css";

import { Banner } from "@/components/banner";
import { Footer } from "@/components/footer";
import { Navbar } from "@/components/navbar";
import { isPublished, latestRelease } from "@/lib/download";
import { pricingAggregateOffer } from "@/lib/pricing";
import {
  requirements,
  siteDescription,
  siteLogo,
  siteTitle,
  siteUrl,
} from "@/lib/site";

// dynamic because of the per-request CSP nonce (proxy.ts)
export const dynamic = "force-dynamic";

const plexSans = IBM_Plex_Sans({
  subsets: ["latin"],
  weight: ["400", "500", "600"],
  display: "swap",
  variable: "--font-plex-sans",
});

const plexMono = IBM_Plex_Mono({
  subsets: ["latin"],
  weight: ["400"],
  display: "swap",
  variable: "--font-plex-mono",
});

const SITE = {
  name: "Rens Engineering",
  url: siteUrl,
  title: siteTitle,
  description: siteDescription,
};

export const metadata: Metadata = {
  metadataBase: new URL(SITE.url),
  title: { default: SITE.title, template: "%s: Vethida" },
  description: SITE.description,
  applicationName: SITE.name,
  authors: [{ name: SITE.name, url: SITE.url }],
  creator: SITE.name,
  publisher: SITE.name,
  icons: {
    icon: [
      { url: "/icon-16.png", sizes: "16x16", type: "image/png" },
      { url: "/icon-32.png", sizes: "32x32", type: "image/png" },
      { url: "/icon-64.png", sizes: "64x64", type: "image/png" },
    ],
    apple: [{ url: "/icon-256.png", sizes: "256x256", type: "image/png" }],
  },
  openGraph: {
    title: SITE.title,
    description: SITE.description,
    url: SITE.url,
    siteName: SITE.name,
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    site: "@f0r3ns1cs",
    creator: "@f0r3ns1cs",
    title: SITE.title,
    description: SITE.description,
  },
  // No site-wide canonical, because every page would inherit it.
  robots: { index: true, follow: true },
};

export const viewport: Viewport = {
  themeColor: "#ffffff",
  colorScheme: "light",
};

const jsonLd = {
  "@context": "https://schema.org",
  "@graph": [
    {
      "@type": "Organization",
      "@id": `${SITE.url}#org`,
      name: SITE.name,
      url: SITE.url,
      logo: siteLogo,
      description: SITE.description,
      sameAs: ["https://discord.gg/CjnYVU9cFb"],
    },
    {
      "@type": "SoftwareApplication",
      "@id": `${SITE.url}#vethida`,
      name: "Vethida",
      applicationCategory: "UtilitiesApplication",
      applicationSubCategory: "System Performance",
      operatingSystem: "Windows 10, Windows 11",
      operatingSystemRequirements: requirements.combined,
      processorRequirements: "x64",
      ...(latestRelease ? { softwareVersion: latestRelease.version } : {}),
      url: SITE.url,
      description: SITE.description,
      author: { "@id": `${SITE.url}#org` },
      publisher: { "@id": `${SITE.url}#org` },
      offers: pricingAggregateOffer(isPublished),
    },
  ],
};

export default async function RootLayout({
  children,
}: Readonly<{ children: React.ReactNode }>) {
  const nonce = (await headers()).get("x-nonce") ?? undefined;

  return (
    <html
      lang="en"
      className={`${plexSans.variable} ${plexMono.variable}`}
    >
      <body className="antialiased">
        <script
          type="application/ld+json"
          nonce={nonce}
          dangerouslySetInnerHTML={{
            __html: JSON.stringify(jsonLd).replace(/</g, "\\u003c"),
          }}
        />
        <a href="#content" className="skip-link">
          Skip to content
        </a>
        <Banner />
        <Navbar />
        {/* tabIndex makes the skip target focusable. */}
        <div id="content" tabIndex={-1}>
          {children}
        </div>
        <Footer />
      </body>
    </html>
  );
}
