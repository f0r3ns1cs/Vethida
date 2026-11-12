import {
  ChatInputCommandInteraction,
  EmbedBuilder,
  SlashCommandBuilder,
} from "discord.js";
import { BOT_COLOR_INFO, BOT_COLOR_WARNING } from "../lib/config";
import { chunkForDiscord, isAiEnabled, oneShotPrompt } from "../lib/ai";
import { aiPaidOnlyEmbed, isEntitledToAi } from "../lib/entitlement";

export const data = new SlashCommandBuilder()
  .setName("troubleshoot")
  .setDescription("Get triage steps for a Vethida or Windows issue")
  .addStringOption((option) =>
    option
      .setName("issue")
      .setDescription("Describe what's happening (be specific)")
      .setRequired(true)
      .setMaxLength(1500)
  )
  .addStringOption((option) =>
    option
      .setName("os")
      .setDescription("Windows version (e.g. 11 23H2, 10 22H2)")
      .setMaxLength(40)
  );

const PROMPT_TEMPLATE = (issue: string, os: string | null): string =>
  [
    "A Vethida user is reporting a Vethida or Windows issue. Give a short,",
    "practical walk-through. Use this exact structure:",
    "",
    "**Likely cause** - single sentence, or 'unclear, see questions below'.",
    "**Try this first** - numbered list, max 5 steps. Each step <= 1 line.",
    "**If that didn't help** - when to escalate (open `/ticket`, run `/diagnostic`, etc.).",
    "**Questions for the user** - only if you genuinely need more info. Skip this section otherwise.",
    "",
    "Rules:",
    "- Lead with the cheapest, least invasive step.",
    "- Reference Vethida modules by name when relevant.",
    "- Never tell the user to disable Windows Update, Defender, or anti-cheat as a first-line fix.",
    "- If this looks like an account, billing, or license-key issue, say so and tell the user to open `/ticket`.",
    "",
    `Operating system: ${os ?? "unspecified"}`,
    "",
    "Issue:",
    issue,
  ].join("\n");

export async function execute(interaction: ChatInputCommandInteraction): Promise<void> {
  const issue = interaction.options.getString("issue", true);
  const os = interaction.options.getString("os");

  if (!isAiEnabled()) {
    await interaction.reply({
      embeds: [
        new EmbedBuilder()
          .setColor(BOT_COLOR_WARNING)
          .setTitle("AI assistant unavailable")
          .setDescription(
            "AI assistant is offline. Run `/ticket` and the support team will follow up."
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
    message: PROMPT_TEMPLATE(issue, os),
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
    .setColor(BOT_COLOR_INFO)
    .setTitle("Triage")
    .setDescription(first ?? "")
    .addFields({
      name: "Need a human?",
      value:
        "Open `/ticket` and attach a diagnostic bundle (`/diagnostic`). Support replies inside 24h.",
    })
    .setFooter({
      text: "AI-generated triage - not a substitute for /ticket on critical issues",
    });

  await interaction.editReply({ embeds: [header] });

  for (const chunk of rest) {
    await interaction.followUp({ content: chunk });
  }
}
