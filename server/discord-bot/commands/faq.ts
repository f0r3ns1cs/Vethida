import { ChatInputCommandInteraction, SlashCommandBuilder } from "discord.js";
import { faqEmbed } from "../components/embeds";

export const data = new SlashCommandBuilder()
  .setName("faq")
  .setDescription("Show the Vethida FAQ");

export async function execute(interaction: ChatInputCommandInteraction): Promise<void> {
  await interaction.reply({ embeds: [faqEmbed()], ephemeral: true });
}
