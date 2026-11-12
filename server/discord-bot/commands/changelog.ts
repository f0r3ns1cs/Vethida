import {
  ChatInputCommandInteraction,
  PermissionFlagsBits,
  SlashCommandBuilder,
  TextChannel,
} from "discord.js";
import { changelogEmbed } from "../components/embeds";

export const data = new SlashCommandBuilder()
  .setName("changelog")
  .setDescription("Post a formatted changelog to #changelog (Admin only)")
  .setDefaultMemberPermissions(PermissionFlagsBits.Administrator)
  .addStringOption((option) =>
    option
      .setName("version")
      .setDescription("Version number (for example v1.2.0)")
      .setRequired(true)
      .setMaxLength(32)
  )
  .addStringOption((option) =>
    option
      .setName("added")
      .setDescription("Added features (separate with |)")
      .setMaxLength(500)
  )
  .addStringOption((option) =>
    option
      .setName("changed")
      .setDescription("Changed features (separate with |)")
      .setMaxLength(500)
  )
  .addStringOption((option) =>
    option
      .setName("fixed")
      .setDescription("Fixed bugs (separate with |)")
      .setMaxLength(500)
  );

export async function execute(interaction: ChatInputCommandInteraction) {
  if (!interaction.memberPermissions?.has(PermissionFlagsBits.Administrator)) {
    await interaction.reply({
      content: "Administrator permission is required.",
      ephemeral: true,
    });
    return;
  }

  const version = interaction.options.getString("version", true);
  const added =
    interaction.options
      .getString("added")
      ?.split("|")
      .map((value) => value.trim())
      .filter(Boolean) ?? [];
  const changed =
    interaction.options
      .getString("changed")
      ?.split("|")
      .map((value) => value.trim())
      .filter(Boolean) ?? [];
  const fixed =
    interaction.options
      .getString("fixed")
      ?.split("|")
      .map((value) => value.trim())
      .filter(Boolean) ?? [];

  if (added.length === 0 && changed.length === 0 && fixed.length === 0) {
    await interaction.reply({
      content: "At least one of added, changed, or fixed is required.",
      ephemeral: true,
    });
    return;
  }

  const guild = interaction.guild;
  if (!guild) {
    return;
  }

  const changelogChannel = guild.channels.cache.find(
    (channel) => channel.name === "changelog"
  ) as TextChannel | undefined;

  if (!changelogChannel) {
    await interaction.reply({
      content: "Could not find #changelog channel.",
      ephemeral: true,
    });
    return;
  }

  const changelogRole = guild.roles.cache.find((role) => role.name === "Changelog");

  await changelogChannel.send({
    content: changelogRole ? `<@&${changelogRole.id}>` : undefined,
    embeds: [changelogEmbed(version, added, changed, fixed, interaction.user.username)],
    allowedMentions: changelogRole ? { roles: [changelogRole.id] } : { parse: [] },
  });

  await interaction.reply({
    content: `Changelog for ${version} posted to ${changelogChannel.toString()}.`,
    ephemeral: true,
  });
}
