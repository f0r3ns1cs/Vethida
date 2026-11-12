import {
  ChatInputCommandInteraction,
  PermissionFlagsBits,
  SlashCommandBuilder,
  TextChannel,
} from "discord.js";
import { BOT_COLOR_WARNING, CHANNEL_NAMES } from "../lib/config";
import { logEmbed, warnEmbed } from "../components/embeds";
import { isModerator, outranks } from "../lib/permissions";
import { modActions, warnings as warningsRepo } from "../lib/db";
import { escapeMarkdown, sanitizeDisplay } from "../lib/security";

export const data = new SlashCommandBuilder()
  .setName("warn")
  .setDescription("Warn a user (Moderator+)")
  .setDefaultMemberPermissions(PermissionFlagsBits.ModerateMembers)
  .addUserOption((option) =>
    option.setName("user").setDescription("User to warn").setRequired(true)
  )
  .addStringOption((option) =>
    option
      .setName("reason")
      .setDescription("Reason for warning")
      .setRequired(true)
      .setMaxLength(300)
  );

export async function execute(interaction: ChatInputCommandInteraction): Promise<void> {
  if (!isModerator(interaction.member)) {
    await interaction.reply({
      content: "Moderate Members permission is required.",
      ephemeral: true,
    });
    return;
  }

  const targetUser = interaction.options.getUser("user", true);
  const reason = escapeMarkdown(
    sanitizeDisplay(interaction.options.getString("reason", true), 300)
  );
  const guild = interaction.guild;

  if (!guild) {
    return;
  }

  if (targetUser.id === interaction.user.id) {
    await interaction.reply({ content: "You cannot warn yourself.", ephemeral: true });
    return;
  }
  if (targetUser.bot) {
    await interaction.reply({ content: "You cannot warn a bot.", ephemeral: true });
    return;
  }

  const targetMember = await guild.members.fetch(targetUser.id).catch(() => null);
  if (targetMember) {
    const actor = await guild.members.fetch(interaction.user.id);
    if (!outranks(actor, targetMember)) {
      await interaction.reply({
        content: "You cannot warn a user with an equal or higher role.",
        ephemeral: true,
      });
      return;
    }
  }

  await warningsRepo.add(guild.id, targetUser.id, interaction.user.id, reason);
  await modActions.record({
    guildId: guild.id,
    userId: targetUser.id,
    action: "warn",
    moderator: interaction.user.id,
    reason,
  });
  const total = await warningsRepo.count(guild.id, targetUser.id);

  try {
    await targetUser.send({
      embeds: [
        logEmbed(
          "You have been warned",
          `You received a warning in Vethida.\n\n**Reason:** ${reason}\n**Total warnings:** ${total}\n\nRepeated violations may result in a mute or ban.`,
          BOT_COLOR_WARNING
        ),
      ],
      allowedMentions: { parse: [] },
    });
  } catch {
    // DMs closed
  }

  const logsChannel = guild.channels.cache.find(
    (channel) => channel.name === CHANNEL_NAMES.staffLogs
  ) as TextChannel | undefined;

  if (logsChannel) {
    await logsChannel.send({
      embeds: [warnEmbed(targetUser.id, reason, interaction.user.username)],
    });
  }

  await interaction.reply({
    content: `Warned <@${targetUser.id}> (total: ${total}) - ${reason}`,
    ephemeral: true,
  });
}
