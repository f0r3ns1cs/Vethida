import {
  ChatInputCommandInteraction,
  EmbedBuilder,
  SlashCommandBuilder,
} from "discord.js";
import { BOT_COLOR, BOT_COLOR_WARNING } from "../lib/config";
import { chunkForDiscord, isAiEnabled, oneShotPrompt } from "../lib/ai";
import { aiPaidOnlyEmbed, isEntitledToAi } from "../lib/entitlement";

export const data = new SlashCommandBuilder()
  .setName("explain")
  .setDescription("Explain what a Vethida tweak or Windows setting does")
  .addStringOption((option) =>
    option
      .setName("tweak")
      .setDescription("Name of the tweak, setting, or registry path")
      .setRequired(true)
      .setMaxLength(300)
  );

const PROMPT_TEMPLATE = (tweak: string): string =>
  [
    `Explain the following Vethida tweak or Windows setting: ${tweak}`,
    "",
    "Structure your reply with these short sections, each on its own line:",
    "**What it does** - one or two sentences describing the change in plain English.",
    "**Why apply it** - when this is genuinely useful, who benefits most.",
    "**Risks** - known regressions, hardware quirks, or side effects. Say 'none I am aware of' if there genuinely are none.",
    "**Permanent?** - say so plainly if this change cannot be undone. Skip the line otherwise.",
    "",
    "If the tweak name is ambiguous or you do not recognize it, say so and ask",
    "the user to clarify which module it lives in. Do not invent details.",
  ].join("\n");

export async function execute(interaction: ChatInputCommandInteraction): Promise<void> {
  const tweak = interaction.options.getString("tweak", true);

  if (!isAiEnabled()) {
    await interaction.reply({
      embeds: [
        new EmbedBuilder()
          .setColor(BOT_COLOR_WARNING)
          .setTitle("AI assistant unavailable")
          .setDescription(
            "The AI assistant isn't configured on this server. Use `/ticket` for tweak-specific help."
          ),
      ],
      ephemeral: true,
    });
    return;
  }

  if (!isEntitledToAi(interaction.member)) {
    await interaction.reply({ embeds: [aiPaidOnlyEmbed()], ephemeral: true });
    return;
  }

  await interaction.deferReply();

  const result = await oneShotPrompt({
    userId: interaction.user.id,
    username: interaction.user.displayName,
    message: PROMPT_TEMPLATE(tweak),
  });

  if (!result.ok) {
    await interaction.editReply({
      embeds: [
        new EmbedBuilder().setColor(BOT_COLOR_WARNING).setDescription(result.message),
      ],
    });
    return;
  }

  const chunks = chunkForDiscord(result.text);
  const [first, ...rest] = chunks;

  const header = new EmbedBuilder()
    .setColor(BOT_COLOR)
    .setTitle(`Explanation: ${tweak.length > 200 ? `${tweak.slice(0, 199)}...` : tweak}`)
    .setDescription(first ?? "")
    .setFooter({
      text: "AI-generated - always verify before applying changes to your system",
    });

  await interaction.editReply({ embeds: [header] });

  for (const chunk of rest) {
    await interaction.followUp({ content: chunk });
  }
}
