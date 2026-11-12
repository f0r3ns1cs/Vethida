import type { GuildMember, TextChannel } from "discord.js";
import { logEmbed } from "../components/embeds";
import { BOT_COLOR_SUCCESS } from "../lib/config";

export async function handleMemberBoost(oldMember: GuildMember, newMember: GuildMember) {
  const wasBoosting = oldMember.premiumSince !== null;
  const isBoosting = newMember.premiumSince !== null;

  if (!wasBoosting && isBoosting) {
    const boosterRole = newMember.guild.roles.cache.find((r) => r.name === "Booster");
    if (boosterRole) {
      await newMember.roles.add(boosterRole);
    }

    const general = newMember.guild.channels.cache.find(
      (ch) => ch.name === "general"
    ) as TextChannel | undefined;

    if (general) {
      await general.send(
        `Thanks for boosting the server, <@${newMember.id}>.`
      );
    }

    const logs = newMember.guild.channels.cache.find(
      (ch) => ch.name === "staff-logs"
    ) as TextChannel | undefined;

    if (logs) {
      await logs.send({
        embeds: [logEmbed("Server Boost", `<@${newMember.id}> boosted the server.`, BOT_COLOR_SUCCESS)],
      });
    }
  } else if (wasBoosting && !isBoosting) {
    const boosterRole = newMember.guild.roles.cache.find((r) => r.name === "Booster");
    if (boosterRole) {
      await newMember.roles.remove(boosterRole);
    }
  }
}
