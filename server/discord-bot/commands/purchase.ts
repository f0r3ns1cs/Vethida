import { ChatInputCommandInteraction, SlashCommandBuilder } from "discord.js";
import { purchaseEmbed } from "../components/embeds";

export const data = new SlashCommandBuilder()
  .setName("purchase")
  .setDescription("See Vethida tiers and pricing");

export async function execute(interaction: ChatInputCommandInteraction): Promise<void> {
  await interaction.reply({ embeds: [purchaseEmbed()], ephemeral: true });
}
