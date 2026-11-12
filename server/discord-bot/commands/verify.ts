import { ChatInputCommandInteraction, SlashCommandBuilder } from "discord.js";
import { verifyEmbed } from "../components/embeds";

export const data = new SlashCommandBuilder()
  .setName("verify")
  .setDescription("Link your Vethida account to access the full server");

export async function execute(interaction: ChatInputCommandInteraction) {
  const { embed, row } = verifyEmbed();
  await interaction.reply({ embeds: [embed], components: [row], ephemeral: true });
}
