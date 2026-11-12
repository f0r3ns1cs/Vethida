import { ChatInputCommandInteraction, SlashCommandBuilder } from "discord.js";
import { aboutEmbed } from "../components/embeds";

export const data = new SlashCommandBuilder()
  .setName("about")
  .setDescription("About Vethida and Rens Engineering");

export async function execute(interaction: ChatInputCommandInteraction): Promise<void> {
  await interaction.reply({ embeds: [aboutEmbed()], ephemeral: true });
}
