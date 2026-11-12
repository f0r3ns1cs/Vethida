import { NextResponse, type NextRequest } from "next/server";

import { apiError, ERROR_CODES } from "@/lib/errors";

export const runtime = "nodejs";
export const revalidate = 300; // the bot polls this

// GET /v1/version - latest release per channel, used by the Discord bot. Reads
// the same feed as the desktop updater (vethida.com/public/desktop/updates.json,
// written by scripts/release/stamp-update-feed.mjs). manifest_jws isn't returned.

const FEED_URL =
  process.env.UPDATE_FEED_URL ?? "https://vethida.com/desktop/updates.json";

const CHANNELS = new Set(["stable", "beta"]);

interface FeedUpdate {
  version?: unknown;
  channel?: unknown;
  download_url?: unknown;
  sha256?: unknown;
  size_bytes?: unknown;
  published_at?: unknown;
  notes?: unknown;
}

// camelCase for the bot (the feed is snake_case)
interface ReleaseResponse {
  version: string;
  publishedAt: string;
  channel: string;
  sha256?: string;
  sizeBytes?: number;
  downloadUrl?: string;
  notes?: string;
}

function str(value: unknown): string | undefined {
  return typeof value === "string" && value !== "" ? value : undefined;
}

export async function GET(req: NextRequest) {
  const requested = req.nextUrl.searchParams.get("channel") ?? "stable";
  const channel = CHANNELS.has(requested) ? requested : "stable";

  let feed: Record<string, { update?: FeedUpdate }>;
  try {
    const res = await fetch(FEED_URL, {
      headers: { Accept: "application/json" },
      next: { revalidate },
    });
    if (!res.ok) {
      console.error("[version] feed fetch failed", res.status);
      return apiError(
        503,
        ERROR_CODES.INTERNAL_ERROR,
        "Release feed is unavailable.",
      );
    }
    feed = await res.json();
  } catch (err) {
    console.error("[version] feed unreachable", err);
    return apiError(
      503,
      ERROR_CODES.INTERNAL_ERROR,
      "Release feed is unavailable.",
    );
  }

  const update = feed?.[channel]?.update;
  const version = str(update?.version);
  const publishedAt = str(update?.published_at);

  // nothing published on this channel yet
  if (!update || !version || !publishedAt) {
    return apiError(
      404,
      ERROR_CODES.NOT_FOUND,
      `No release is currently published on the ${channel} channel.`,
    );
  }

  const body: ReleaseResponse = {
    version,
    publishedAt,
    channel: str(update.channel) ?? channel,
    ...(str(update.sha256) ? { sha256: str(update.sha256) } : {}),
    ...(typeof update.size_bytes === "number"
      ? { sizeBytes: update.size_bytes }
      : {}),
    ...(str(update.download_url)
      ? { downloadUrl: str(update.download_url) }
      : {}),
    ...(str(update.notes) ? { notes: str(update.notes) } : {}),
  };

  return NextResponse.json(body);
}
