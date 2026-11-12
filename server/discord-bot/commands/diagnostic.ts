import {
  ChatInputCommandInteraction,
  SlashCommandBuilder,
  EmbedBuilder,
} from "discord.js";
import { BOT_COLOR_INFO } from "../lib/config";

export const data = new SlashCommandBuilder()
  .setName("diagnostic")
  .setDescription("Where Vethida's logs live and what to send with a ticket");

export async function execute(
  interaction: ChatInputCommandInteraction,
): Promise<void> {
  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR_INFO)
    .setTitle("Sending diagnostics with a support ticket")
    .setDescription(
      "Vethida keeps its logs on your machine. Attaching them to a ticket " +
        "gets us to the answer far faster than a description does.",
    )
    .addFields(
      {
        name: "Export it from the app",
        value: [
          "In Vethida, open **Settings**, then **Diagnostics**, then **Export**",
          "next to Export support bundle. It writes one zip holding your system",
          "details, the changes Vethida has applied, and its logs, with license",
          "keys masked. Attach that file.",
        ].join(" "),
      },
      {
        name: "If you cannot open the app",
        value: [
          "Paste this into the Explorer address bar:",
          "```%LOCALAPPDATA%\\Vethida```",
          "`logs\\vethida.log` - what the app was doing",
          "`crashes\\` - present only if it has crashed",
          "`tweak-state.json` - which tweaks are currently applied",
          "",
          "`vethida.log` on its own is usually enough. It records file paths and machine details, so it can contain your Windows username. Redact anything you would rather not send.",
        ].join("\n"),
      },
      {
        name: "Where to send it",
        value:
          "Email `support@vethida.com`, or run `/ticket` here and attach the files in the thread.",
      },
    );

  await interaction.reply({ embeds: [embed], ephemeral: true });
}
