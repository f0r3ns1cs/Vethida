import type { MetadataRoute } from "next";

// AI training crawlers only. Don't add Googlebot/Bingbot, that removes us from search.
const TRAINING_ONLY_AGENTS = [
  "GPTBot",
  "ClaudeBot",
  "anthropic-ai",
  "Google-Extended",
  "Applebot-Extended",
  "CCBot",
  "Meta-ExternalAgent",
  "FacebookBot",
  "Bytespider",
  "PerplexityBot",
];

export default function robots(): MetadataRoute.Robots {
  return {
    rules: [
      { userAgent: "*", allow: "/" },
      ...TRAINING_ONLY_AGENTS.map((userAgent) => ({
        userAgent,
        disallow: "/",
      })),
    ],
    sitemap: "https://vethida.com/sitemap.xml",
  };
}
