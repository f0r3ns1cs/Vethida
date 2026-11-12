import { NextResponse, type NextRequest } from "next/server";
import { eq } from "drizzle-orm";
import {
  db,
  hashLicenseKey,
  hasActiveSuspension,
  license,
  parseLicenseKey,
} from "@vethida/db";
import { apiError, ERROR_CODES } from "@/lib/errors";
import { bearerTokenMatches } from "@vethida/http/bearer";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

export async function GET(req: NextRequest) {
  try {
    if (
      !bearerTokenMatches(
        req.headers.get("authorization"),
        process.env.INTERNAL_API_TOKEN,
      )
    ) {
      return apiError(401, ERROR_CODES.UNAUTHORIZED, "Invalid internal token.");
    }

    const canonicalKey = parseLicenseKey(
      req.nextUrl.searchParams.get("key") ?? "",
    );
    if (!canonicalKey) {
      return apiError(400, ERROR_CODES.INVALID_LICENSE_KEY, "Not a valid license key.");
    }

    const [row] = await db
      .select({
        id: license.id,
        userId: license.userId,
        tier: license.tier,
        expiresAt: license.expiresAt,
        revokedAt: license.revokedAt,
      })
      .from(license)
      .where(eq(license.keyHash, hashLicenseKey(canonicalKey)))
      .limit(1);

    if (!row) {
      return apiError(404, ERROR_CODES.NOT_FOUND, "No license for that key.");
    }

    const status = row.revokedAt
      ? "revoked"
      : row.expiresAt && row.expiresAt.getTime() < Date.now()
        ? "expired"
        : row.userId && (await hasActiveSuspension(row.userId))
          ? "suspended"
          : "active";

    return NextResponse.json({
      license_id: row.id,
      user_id: row.userId,
      tier: row.tier,
      status,
      expires_at: row.expiresAt?.toISOString() ?? null,
    });
  } catch (err) {
    console.error("[license-lookup] unhandled error", err);
    return apiError(500, ERROR_CODES.INTERNAL_ERROR, "Internal server error.");
  }
}
