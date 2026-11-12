import fs from "node:fs";
import path from "node:path";

// Latest release from public/desktop/appcast.xml (same file WinSparkle reads,
// written by scripts/appcast.mjs). Newest is the first <item>. Server only.

export interface DesktopRelease {
  version: string;
  url: string;
  sha256: string;
  sizeBytes: number | null;
  publishedAt: string | null;
}

function readAppcast(): DesktopRelease | null {
  const appcastPath = path.join(process.cwd(), "public", "desktop", "appcast.xml");
  let xml: string;
  try {
    xml = fs.readFileSync(appcastPath, "utf8");
  } catch {
    return null; // nothing released yet
  }
  const item = /<item>([\s\S]*?)<\/item>/.exec(xml)?.[1];
  if (!item) return null;
  const attr = (name: string) => new RegExp(`${name}="([^"]*)"`).exec(item)?.[1] ?? null;
  const version = attr("sparkle:version");
  const url = attr("url");
  const sha256 = /<vethida:sha256>([0-9a-f]{64})<\/vethida:sha256>/.exec(item)?.[1] ?? null;
  if (!version || !url || !sha256) return null;
  const length = Number(attr("length"));
  const pubDate = /<pubDate>([^<]+)<\/pubDate>/.exec(item)?.[1];
  const published = pubDate ? new Date(pubDate) : null;
  return {
    version,
    url: url.replace(/&amp;/g, "&"),
    sha256,
    sizeBytes: Number.isFinite(length) && length > 0 ? length : null,
    publishedAt: published && !Number.isNaN(published.getTime()) ? published.toISOString() : null,
  };
}

export const latestRelease: DesktopRelease | null = readAppcast();

export const isPublished = latestRelease !== null;

export function formatSize(bytes: number | null): string | null {
  if (bytes === null || bytes <= 0) return null;
  return `${(bytes / 1_000_000).toFixed(1)} MB`;
}
