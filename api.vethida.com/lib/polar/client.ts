import { Polar } from "@polar-sh/sdk";

// Polar client for the webhook handler (it has to cancel the old subscription
// on upgrades). Sandbox outside production, same as my.vethida.com/lib/polar.ts.
let client: Polar | null = null;

export function getPolar(): Polar {
  if (client) return client;

  const accessToken = process.env.POLAR_ACCESS_TOKEN;
  if (!accessToken) {
    throw new Error("POLAR_ACCESS_TOKEN is not configured");
  }

  const override = process.env.POLAR_SERVER;
  const server: "production" | "sandbox" =
    override === "production" || override === "sandbox"
      ? override
      : process.env.NODE_ENV === "production"
        ? "production"
        : "sandbox";

  client = new Polar({ accessToken, server });
  return client;
}
