import type { Metadata } from "next";
import { headers } from "next/headers";
import { redirect } from "next/navigation";
import { auth } from "@/lib/auth";
import { SignInScreen } from "./sign-in-screen";

export const dynamic = "force-dynamic";

export const metadata: Metadata = { title: "Sign in" };

// also sign-up, new emails are registered on first sign in
export default async function SignInPage() {
  const session = await auth.api.getSession({ headers: await headers() });
  if (session) redirect("/dashboard");

  return <SignInScreen />;
}
