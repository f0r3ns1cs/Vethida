import { headers } from "next/headers";
import { redirect } from "next/navigation";
import { auth } from "./auth";

type Session = NonNullable<Awaited<ReturnType<typeof auth.api.getSession>>>;

// Redirects non-admins. Call it in every page as well as the layout, they
// render in parallel so the layout alone doesn't stop the page's queries.
export async function requireAdmin(): Promise<Session> {
  const session = await adminSession();
  if (!session) redirect("/dashboard");
  return session;
}

// for route handlers (return a status instead of redirecting)
export async function adminSession(): Promise<Session | null> {
  const session = await auth.api.getSession({ headers: await headers() });
  if (!session || session.user.role !== "admin") return null;
  return session;
}

// for audit rows from server actions
export async function requestIp(): Promise<string> {
  const h = await headers();
  return (
    h.get("cf-connecting-ip")?.trim() ??
    h.get("x-real-ip")?.trim() ??
    "unknown"
  );
}
