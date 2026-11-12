import {
  ChatInputCommandInteraction,
  PermissionFlagsBits,
  SlashCommandBuilder,
  TextChannel,
} from "discord.js";
import { maintenanceEmbed } from "../components/embeds";

export const data = new SlashCommandBuilder()
  .setName("maintenance")
  .setDescription("Post a scheduled maintenance notice (Admin only)")
  .setDefaultMemberPermissions(PermissionFlagsBits.Administrator)
  .addStringOption((option) =>
    option
      .setName("description")
      .setDescription("What is happening")
      .setRequired(true)
      .setMaxLength(500)
  )
  .addIntegerOption((option) =>
    option
      .setName("timestamp")
      .setDescription("Unix timestamp for when maintenance starts")
      .setRequired(true)
  );

export async function execute(interaction: ChatInputCommandInteraction) {
  if (!interaction.memberPermissions?.has(PermissionFlagsBits.Administrator)) {
    await interaction.reply({
      content: "Administrator permission is required.",
      ephemeral: true,
    });
    return;
  }

  const description = interaction.options.getString("description", true);
  const timestamp = interaction.options.getInteger("timestamp", true);
  const guild = interaction.guild;

  if (!guild) {
    return;
  }

  const announcementsChannel = guild.channels.cache.find(
    (channel) => channel.name === "announcements"
  ) as TextChannel | undefined;

  if (!announcementsChannel) {
    await interaction.reply({
      content: "Could not find #announcements channel.",
      ephemeral: true,
    });
    return;
  }

  const announcementsRole = guild.roles.cache.find(
    (role) => role.name === "Announcements"
  );

  await announcementsChannel.send({
    content: announcementsRole ? `<@&${announcementsRole.id}>` : undefined,
    embeds: [maintenanceEmbed(description, timestamp, interaction.user.username)],
    allowedMentions: announcementsRole
      ? { roles: [announcementsRole.id] }
      : { parse: [] },
  });

  await interaction.reply({
    content: "Maintenance notice posted.",
    ephemeral: true,
  });
}
