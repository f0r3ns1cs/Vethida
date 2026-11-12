import { sql } from "drizzle-orm";
import { db } from "@vethida/db";

export const dynamic = "force-dynamic";

// Readiness: checks the database with SELECT 1. The supervisor watches /health,
// the load balancer watches this.

const TIMEOUT_MS = 3_000;

class ReadyTimeout extends Error {}

export async function GET() {
  const started = Date.now();

  try {
    // own timeout, the pool's 10s is too long for a probe
    await Promise.race([
      db.execute(sql`select 1`),
      new Promise((_, reject) =>
        setTimeout(
          () =>
            reject(
              new ReadyTimeout(
                `database did not answer within ${TIMEOUT_MS}ms`,
              ),
            ),
          TIMEOUT_MS,
        ),
      ),
    ]);
  } catch (err) {
    // Don't return the driver's message, this route is public and pg errors
    // include the db host and port. Only our own timeout text goes out.
    console.error("[ready] database check failed", err);
    return Response.json(
      {
        status: "unavailable",
        detail:
          err instanceof ReadyTimeout ? err.message : "database unreachable",
      },
      { status: 503, headers: { "cache-control": "no-store" } },
    );
  }

  return Response.json(
    { status: "ready", database_ms: Date.now() - started },
    { headers: { "cache-control": "no-store" } },
  );
}
