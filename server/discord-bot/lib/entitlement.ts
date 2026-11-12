import {
  EmbedBuilder,
  PermissionFlagsBits,
  PermissionsBitField,
} from "discord.js";
import type {
  APIInteractionGuildMember,
  Client,
  GuildMember,
} from "discord.js";
import { BOT_COLOR_WARNING, GUILD_ID, PORTAL_URL, ROLE_IDS, WEBSITE_URL } from "./config";
import { logger } from "./logger";

type MemberLike = GuildMember | APIInteractionGuildMember | null | undefined;

function entitledRoleIds(): string[] {
  return [
    ROLE_IDS.customer,
    ROLE_IDS.founder,
    ROLE_IDS.team,
    ROLE_IDS.partner,
  ].filter((id): id is string => id.length > 0);
}

function memberHasEntitledRole(member: MemberLike): boolean {
  if (!member) {return false;}
  const allowed = entitledRoleIds();
  if (allowed.length === 0) {return false;}

  const roles = member.roles;
  if (Array.isArray(roles)) {
    return roles.some((id) => allowed.includes(id));
  }
  if (roles && "cache" in roles) {
    return roles.cache.some((role) => allowed.includes(role.id));
  }
  return false;
}

function memberIsAdmin(member: MemberLike): boolean {
  if (!member) {return false;}
  const perms = member.permissions;
  if (perms instanceof PermissionsBitField) {
    return perms.has(PermissionFlagsBits.Administrator);
  }
  if (typeof perms === "string") {
    return new PermissionsBitField(BigInt(perms)).has(
      PermissionFlagsBits.Administrator,
    );
  }
  return false;
}

export function isEntitledToAi(member: MemberLike): boolean {
  return memberHasEntitledRole(member) || memberIsAdmin(member);
}

// for DMs, look the user up in the guild. Not found = no access
export async function isUserEntitledToAi(
  client: Client,
  userId: string,
): Promise<boolean> {
  try {
    const guild =
      client.guilds.cache.get(GUILD_ID) ??
      (await client.guilds.fetch(GUILD_ID));
    const member = await guild.members.fetch(userId);
    return isEntitledToAi(member);
  } catch (err) {
    logger.debug({ err, userId }, "ai entitlement dm lookup failed");
    return false;
  }
}

export const AI_PAID_ONLY_TEXT =
  "The AI assistant comes with Vethida Pro and Ultimate. Link your license at " +
  `${PORTAL_URL}/dashboard to unlock it, or see ${WEBSITE_URL}/pricing.`;

export function aiPaidOnlyEmbed(): EmbedBuilder {
  return new EmbedBuilder()
    .setColor(BOT_COLOR_WARNING)
    .setTitle("Pro and Ultimate")
    .setDescription(AI_PAID_ONLY_TEXT);
}
