import {
  ActionRowBuilder,
  ChatInputCommandInteraction,
  EmbedBuilder,
  ModalBuilder,
  ModalSubmitInteraction,
  SlashCommandBuilder,
  TextChannel,
  TextInputBuilder,
  TextInputStyle,
} from "discord.js";
import { BOT_COLOR_ERROR, CHANNEL_NAMES } from "../lib/config";
import { bugReportLimiter } from "../lib/rateLimit";
import { escapeMarkdown, sanitizeDisplay } from "../lib/security";

export const data = new SlashCommandBuilder()
  .setName("bug")
  .setDescription("File a structured bug report");

export async function execute(interaction: ChatInputCommandInteraction): Promise<void> {
  const limit = bugReportLimiter.check(interaction.user.id);
  if (!limit.allowed) {
    await interaction.reply({
      content: `Slow down. Try again in ${Math.ceil(limit.retryAfterMs / 1000)}s.`,
      ephemeral: true,
    });
    return;
  }

  const modal = new ModalBuilder().setCustomId("bug_modal").setTitle("File a Bug Report");

  const title = new TextInputBuilder()
    .setCustomId("bug_title")
    .setLabel("Short title")
    .setStyle(TextInputStyle.Short)
    .setMinLength(5)
    .setMaxLength(100)
    .setRequired(true);

  const version = new TextInputBuilder()
    .setCustomId("bug_version")
    .setLabel("Vethida version")
    .setStyle(TextInputStyle.Short)
    .setMinLength(1)
    .setMaxLength(32)
    .setPlaceholder("v1.2.0")
    .setRequired(true);

  const os = new TextInputBuilder()
    .setCustomId("bug_os")
    .setLabel("Windows version / build")
    .setStyle(TextInputStyle.Short)
    .setMinLength(1)
    .setMaxLength(64)
    .setPlaceholder("Windows 11 23H2 / 22631")
    .setRequired(true);

  const steps = new TextInputBuilder()
    .setCustomId("bug_steps")
    .setLabel("Steps to reproduce")
    .setStyle(TextInputStyle.Paragraph)
    .setMinLength(10)
    .setMaxLength(1500)
    .setRequired(true);

  const expected = new TextInputBuilder()
    .setCustomId("bug_expected")
    .setLabel("Expected vs actual behavior")
    .setStyle(TextInputStyle.Paragraph)
    .setMinLength(10)
    .setMaxLength(1500)
    .setRequired(true);

  modal.addComponents(
    new ActionRowBuilder<TextInputBuilder>().addComponents(title),
    new ActionRowBuilder<TextInputBuilder>().addComponents(version),
    new ActionRowBuilder<TextInputBuilder>().addComponents(os),
    new ActionRowBuilder<TextInputBuilder>().addComponents(steps),
    new ActionRowBuilder<TextInputBuilder>().addComponents(expected)
  );

  await interaction.showModal(modal);
}

export async function handleModal(interaction: ModalSubmitInteraction): Promise<void> {
  if (interaction.customId !== "bug_modal") {
    return;
  }

  // escaping can push it back over the limit, so check again after
  const EMBED_FIELD_LIMIT = 1024;

  const field = (id: string) => {
    const escaped = escapeMarkdown(
      sanitizeDisplay(interaction.fields.getTextInputValue(id), EMBED_FIELD_LIMIT)
    );
    return escaped.length > EMBED_FIELD_LIMIT
      ? `${escaped.slice(0, EMBED_FIELD_LIMIT - 3)}...`
      : escaped;
  };

  const title = field("bug_title");
  const version = field("bug_version");
  const os = field("bug_os");
  const steps = field("bug_steps");
  const expected = field("bug_expected");

  const guild = interaction.guild;
  const channel =
    (guild?.channels.cache.find((c) => c.name === CHANNEL_NAMES.bugs) as
      | TextChannel
      | undefined) ?? null;

  if (!channel) {
    await interaction.reply({
      content:
        "I can't find the `#bug-reports` channel. Your report was not filed - contact staff directly.",
      ephemeral: true,
    });
    return;
  }

  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR_ERROR)
    .setTitle(`Bug: ${title}`)
    .setAuthor({
      name: interaction.user.tag,
      iconURL: interaction.user.displayAvatarURL(),
    })
    .addFields(
      { name: "Vethida version", value: version, inline: true },
      { name: "OS", value: os, inline: true },
      { name: "Reporter", value: `<@${interaction.user.id}>`, inline: true },
      { name: "Steps to reproduce", value: steps, inline: false },
      { name: "Expected vs actual", value: expected, inline: false }
    )
    .setTimestamp();

  const posted = await channel.send({
    embeds: [embed],
    allowedMentions: { parse: [] },
  });

  try {
    await posted.startThread({
      name: `bug-${title.slice(0, 80)}`,
      autoArchiveDuration: 10080,
    });
  } catch {
    // no thread, not a big deal
  }

  await interaction.reply({
    content: `Bug report filed in ${channel.toString()}. Thanks!`,
    ephemeral: true,
  });
}
