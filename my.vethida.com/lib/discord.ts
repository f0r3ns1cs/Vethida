import type { PaidTier } from "@vethida/db";

const API = "https://discord.com/api/v10";

const GUILD_ID = "1258455412471234693";
const CUSTOMER_ROLE_ID = "1488218739613761687";

// extra per-tier roles on top of the customer role (which gates the paid channels)
const TIER_ROLE_IDS: Record<PaidTier, string | undefined> = {
  pro: process.env.DISCORD_ROLE_PRO,
  ultimate: process.env.DISCORD_ROLE_ULTIMATE,
};

function botToken(): string | null {
  return process.env.DISCORD_BOT_TOKEN ?? null;
}

export function discordConfigured(): boolean {
  return Boolean(
    process.env.DISCORD_CLIENT_ID &&
      process.env.DISCORD_CLIENT_SECRET &&
      process.env.DISCORD_BOT_TOKEN,
  );
}

// true only if Discord accepted it (403/404/429 all come back false)
async function botFetch(path: string, init: RequestInit): Promise<boolean> {
  const token = botToken();
  if (!token) return false;
  try {
    const res = await fetch(`${API}${path}`, {
      ...init,
      headers: {
        Authorization: `Bot ${token}`,
        "Content-Type": "application/json",
        ...init.headers,
      },
    });
    if (!res.ok) {
      // the body says which permission/role is the problem
      const body = await res.text().catch(() => "");
      console.error(
        `[discord] ${init.method ?? "GET"} ${path} failed: ${res.status} ${res.statusText} ${body.slice(0, 300)}`,
      );
      return false;
    }
    return true;
  } catch (err) {
    console.error("[discord] request failed", path, err);
    return false;
  }
}

// needs guilds.join. 201 = added, 204 = already in
export async function addToGuild(
  discordUserId: string,
  accessToken: string,
): Promise<boolean> {
  return botFetch(`/guilds/${GUILD_ID}/members/${discordUserId}`, {
    method: "PUT",
    body: JSON.stringify({ access_token: accessToken }),
  });
}

// only touches the customer role
export async function syncCustomerRole(
  discordUserId: string,
  entitled: boolean,
): Promise<boolean> {
  return botFetch(
    `/guilds/${GUILD_ID}/members/${discordUserId}/roles/${CUSTOMER_ROLE_ID}`,
    { method: entitled ? "PUT" : "DELETE" },
  );
}

// Adds the role for the current tier and removes the others. One call per role
// since a member PATCH would replace staff-assigned roles. False if any call failed.
export async function syncTierRoles(
  discordUserId: string,
  tier: string | null,
): Promise<boolean> {
  let allOk = true;
  for (const [name, roleId] of Object.entries(TIER_ROLE_IDS)) {
    if (!roleId) continue;
    const shouldHave = tier === name;
    const ok = await botFetch(
      `/guilds/${GUILD_ID}/members/${discordUserId}/roles/${roleId}`,
      { method: shouldHave ? "PUT" : "DELETE" },
    );
    if (!ok) allOk = false;
  }
  return allOk;
}
