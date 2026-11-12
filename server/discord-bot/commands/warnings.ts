import {
  ChatInputCommandInteraction,
  EmbedBuilder,
  PermissionFlagsBits,
  SlashCommandBuilder,
} from "discord.js";
import { BOT_COLOR_WARNING } from "../lib/config";
import { isModerator } from "../lib/permissions";
import { warnings } from "../lib/db";
import { escapeMarkdown } from "../lib/security";

export const data = new SlashCommandBuilder()
  .setName("warnings")
  .setDescription("List a user's warnings (Moderator+)")
  .setDefaultMemberPermissions(PermissionFlagsBits.ModerateMembers)
  .addUserOption((option) =>
    option.setName("user").setDescription("User to inspect").setRequired(true)
  );

export async function execute(interaction: ChatInputCommandInteraction): Promise<void> {
  if (!isModerator(interaction.member)) {
    await interaction.reply({
      content: "Missing Moderate Members permission.",
      ephemeral: true,
    });
    return;
  }

  const user = interaction.options.getUser("user", true);
  const guild = interaction.guild;
  if (!guild) {
    return;
  }

  const records = await warnings.list(guild.id, user.id, 10);
  const total = await warnings.count(guild.id, user.id);

  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR_WARNING)
    .setTitle(`Warnings for ${user.tag}`)
    .setDescription(
      records.length === 0
        ? "No warnings on record."
        : records
            .map((record, index) => {
              const when = `<t:${Math.floor(record.createdAt.getTime() / 1000)}:R>`;
              const reason = escapeMarkdown(record.reason);
              return `**${index + 1}.** ${when} by <@${record.moderator}> - ${reason}`;
            })
            .join("\n")
    )
    .setFooter({ text: `${total} total warning(s)` });

  await interaction.reply({ embeds: [embed], ephemeral: true });
}
