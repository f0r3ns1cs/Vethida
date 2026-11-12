import {
  ChatInputCommandInteraction,
  PermissionFlagsBits,
  SlashCommandBuilder,
  TextChannel,
} from "discord.js";

export const data = new SlashCommandBuilder()
  .setName("purge")
  .setDescription("Bulk delete messages in this channel (Moderator+)")
  .setDefaultMemberPermissions(PermissionFlagsBits.ManageMessages)
  .addIntegerOption((option) =>
    option
      .setName("count")
      .setDescription("Number of messages to delete (1-100)")
      .setRequired(true)
      .setMinValue(1)
      .setMaxValue(100)
  );

export async function execute(interaction: ChatInputCommandInteraction) {
  if (!interaction.memberPermissions?.has(PermissionFlagsBits.ManageMessages)) {
    await interaction.reply({
      content: "Manage Messages permission is required.",
      ephemeral: true,
    });
    return;
  }

  const count = interaction.options.getInteger("count", true);
  const channel = interaction.channel;

  if (!channel || !("bulkDelete" in channel)) {
    await interaction.reply({
      content: "Cannot delete messages in this channel.",
      ephemeral: true,
    });
    return;
  }

  await interaction.deferReply({ ephemeral: true });

  try {
    const deleted = await (channel as TextChannel).bulkDelete(count, true);
    await interaction.editReply({ content: `Deleted ${deleted.size} messages.` });
  } catch {
    await interaction.editReply({
      content: "Bulk delete failed. Messages older than 14 days cannot be purged.",
    });
  }
}
