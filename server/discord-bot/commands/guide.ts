import { ChatInputCommandInteraction, SlashCommandBuilder } from "discord.js";
import { guidesEmbed } from "../components/embeds";

export const data = new SlashCommandBuilder()
  .setName("guide")
  .setDescription("Show the Vethida getting-started guide");

export async function execute(interaction: ChatInputCommandInteraction): Promise<void> {
  await interaction.reply({ embeds: [guidesEmbed()], ephemeral: true });
}
