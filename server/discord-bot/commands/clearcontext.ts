import {
  ChatInputCommandInteraction,
  PermissionFlagsBits,
  SlashCommandBuilder,
} from "discord.js";
import { clearHistory, isAiEnabled } from "../lib/ai";

export const data = new SlashCommandBuilder()
  .setName("clearcontext")
  .setDescription("Clear the AI's conversation memory in this channel")
  .setDefaultMemberPermissions(PermissionFlagsBits.ManageMessages)
  .setDMPermission(true);

export async function execute(interaction: ChatInputCommandInteraction): Promise<void> {
  if (!isAiEnabled()) {
    await interaction.reply({
      content: "AI assistant is disabled - there is no context to clear.",
      ephemeral: true,
    });
    return;
  }

  clearHistory(interaction.channelId);

  await interaction.reply({
    content: "Conversation memory cleared for this channel.",
    ephemeral: true,
  });
}
