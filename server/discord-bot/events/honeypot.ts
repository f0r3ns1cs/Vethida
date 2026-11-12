import type { Message, TextChannel } from "discord.js";
import { honeypotAlertEmbed } from "../components/embeds";

export async function handleHoneypot(message: Message) {
  if (!message.guild || message.author.bot) {return;}

  const channel = message.channel;
  if (!("name" in channel) || channel.name !== "free-keys") {return;}

  try {
    await message.delete();
  } catch {
    // already deleted
  }

  const accountAgeDays = Math.floor(
    (Date.now() - message.author.createdTimestamp) / (1000 * 60 * 60 * 24)
  );
  const accountAge = `${accountAgeDays} days`;

  const joinedAt = message.member?.joinedAt
    ? `<t:${Math.floor(message.member.joinedAt.getTime() / 1000)}:R>`
    : "Unknown";

  const logsChannel = message.guild.channels.cache.find(
    (ch) => ch.name === "staff-logs"
  ) as TextChannel | undefined;

  if (logsChannel) {
    await logsChannel.send({
      embeds: [honeypotAlertEmbed(
        message.author.id,
        message.author.tag,
        message.content,
        accountAge,
        joinedAt,
      )],
    });
  }
}
