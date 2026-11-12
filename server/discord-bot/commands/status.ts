import { ChatInputCommandInteraction, EmbedBuilder, SlashCommandBuilder } from "discord.js";
import { BOT_COLOR_ERROR, BOT_COLOR_SUCCESS, BOT_COLOR_WARNING, STATUS_URL } from "../lib/config";
import { getStatusSummary } from "../lib/api";

export const data = new SlashCommandBuilder()
  .setName("status")
  .setDescription("Show live Vethida service status");

export async function execute(interaction: ChatInputCommandInteraction): Promise<void> {
  await interaction.deferReply({ ephemeral: true });

  if (!STATUS_URL) {
    await interaction.editReply(
      "There is no public status page yet. If something looks broken, run " +
        "`/ticket` and it will reach a human.",
    );
    return;
  }

  const summary = await getStatusSummary();

  if (!summary) {
    await interaction.editReply(
      `Status service is unreachable. Check [${STATUS_URL}](${STATUS_URL}) directly.`,
    );
    return;
  }

  const color =
    summary.status === "operational"
      ? BOT_COLOR_SUCCESS
      : summary.status === "degraded"
        ? BOT_COLOR_WARNING
        : BOT_COLOR_ERROR;

  const embed = new EmbedBuilder()
    .setColor(color)
    .setTitle(`Vethida - ${summary.status.toUpperCase()}`)
    .setURL(STATUS_URL)
    .addFields(
      summary.components.slice(0, 24).map((component) => ({
        name: component.name,
        value: component.status,
        inline: true,
      }))
    )
    .setTimestamp();

  await interaction.editReply({ embeds: [embed] });
}
