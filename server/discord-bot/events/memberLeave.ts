import type { GuildMember, PartialGuildMember, TextChannel } from "discord.js";
import { logEmbed } from "../components/embeds";
import { BOT_COLOR_ERROR } from "../lib/config";

export async function handleMemberLeave(member: GuildMember | PartialGuildMember) {
  const logsChannel = member.guild.channels.cache.find(
    (ch) => ch.name === "staff-logs"
  ) as TextChannel | undefined;

  if (!logsChannel) {return;}

  const roles = member.roles?.cache
    .filter((r) => r.name !== "@everyone")
    .map((r) => r.name)
    .join(", ") || "None";

  await logsChannel.send({
    embeds: [logEmbed(
      "Member Left",
      `**User:** ${member.user?.tag ?? "Unknown"} (${member.id})\n**Roles:** ${roles}\n**Members:** ${member.guild.memberCount}`,
      BOT_COLOR_ERROR,
    )],
  });
}
