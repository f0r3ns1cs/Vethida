import { ChatInputCommandInteraction, EmbedBuilder, SlashCommandBuilder } from "discord.js";
import { BOT_COLOR, WEBSITE_URL } from "../lib/config";
import { getLatestVethidaVersion } from "../lib/api";

const DOWNLOAD_URL = `${WEBSITE_URL}/download`;

export const data = new SlashCommandBuilder()
  .setName("version")
  .setDescription("Show the latest Vethida release");

export async function execute(interaction: ChatInputCommandInteraction): Promise<void> {
  await interaction.deferReply({ ephemeral: true });

  const lookup = await getLatestVethidaVersion();

  // 404 = nothing published yet
  if (lookup.state === "unpublished") {
    await interaction.editReply(
      `No Vethida release has been published yet. Watch this server for the announcement.`
    );
    return;
  }
  if (lookup.state === "unavailable") {
    await interaction.editReply(
      `Couldn't reach the release feed just now. Check [${DOWNLOAD_URL}](${DOWNLOAD_URL}) for the latest version.`
    );
    return;
  }

  const release = lookup.release;
  const publishedAt = Math.floor(new Date(release.publishedAt).getTime() / 1000);

  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR)
    .setTitle(`Vethida ${release.version}`)
    .setURL(release.downloadUrl ?? DOWNLOAD_URL)
    .addFields(
      { name: "Published", value: `<t:${publishedAt}:F> (<t:${publishedAt}:R>)`, inline: false },
      ...(release.sha256
        ? [{ name: "SHA-256", value: `\`${release.sha256}\``, inline: false }]
        : [])
    );

  await interaction.editReply({ embeds: [embed] });
}
