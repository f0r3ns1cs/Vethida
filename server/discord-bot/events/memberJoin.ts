import type { GuildMember, TextChannel } from "discord.js";
import { welcomeEmbed, logEmbed } from "../components/embeds";
import { BOT_COLOR_SUCCESS } from "../lib/config";

export async function handleMemberJoin(member: GuildMember) {
  const welcomeChannel = member.guild.channels.cache.find(
    (ch) => ch.name === "welcome"
  ) as TextChannel | undefined;

  if (welcomeChannel) {
    const embed = welcomeEmbed(
      member.user.username,
      member.user.displayAvatarURL({ size: 128 })
    );
    await welcomeChannel.send({ embeds: [embed] });
  }

  const logsChannel = member.guild.channels.cache.find(
    (ch) => ch.name === "staff-logs"
  ) as TextChannel | undefined;

  if (logsChannel) {
    const accountAge = Math.floor((Date.now() - member.user.createdTimestamp) / (1000 * 60 * 60 * 24));
    await logsChannel.send({
      embeds: [logEmbed(
        "Member Joined",
        `**User:** ${member.user.tag} (<@${member.id}>)\n**Account age:** ${accountAge} days\n**Members:** ${member.guild.memberCount}`,
        BOT_COLOR_SUCCESS,
      )],
    });
  }
}
