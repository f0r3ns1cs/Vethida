import { ChatInputCommandInteraction, EmbedBuilder, SlashCommandBuilder } from "discord.js";
import { BOT_COLOR, PORTAL_URL, WEBSITE_URL } from "../lib/config";

export const data = new SlashCommandBuilder()
  .setName("docs")
  .setDescription("Show Vethida links and where to get help");

export async function execute(interaction: ChatInputCommandInteraction): Promise<void> {
  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR)
    .setTitle("Vethida links")
    .setDescription(
      [
        `**Website** - [${WEBSITE_URL}](${WEBSITE_URL})`,
        `**Dashboard** - [${PORTAL_URL}/dashboard](${PORTAL_URL}/dashboard)`,
        `**Download** - [${WEBSITE_URL}/download](${WEBSITE_URL}/download)`,
        `**FAQ** - [${WEBSITE_URL}/faq](${WEBSITE_URL}/faq)`,
        `**Support** - open a ticket with \`/ticket open\``,
        "",
        "Report a bug with `/bug` - feature requests go in #feature-requests",
      ].join("\n")
    );

  await interaction.reply({ embeds: [embed], ephemeral: true });
}
