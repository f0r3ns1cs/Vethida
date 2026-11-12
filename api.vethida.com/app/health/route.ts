export const dynamic = "force-dynamic";

// liveness only, no db check (that's /ready)
export function GET() {
  return Response.json({ status: "ok" });
}
