import {
  ChatInputCommandInteraction,
  EmbedBuilder,
  SlashCommandBuilder,
} from "discord.js";
import { BOT_COLOR, BOT_COLOR_WARNING } from "../lib/config";
import { chunkForDiscord, isAiEnabled, oneShotPrompt } from "../lib/ai";
import { aiPaidOnlyEmbed, isEntitledToAi } from "../lib/entitlement";

export const data = new SlashCommandBuilder()
  .setName("ask")
  .setDescription("Ask the Vethida AI assistant a Vethida or Windows question")
  .addStringOption((option) =>
    option
      .setName("question")
      .setDescription("Your question for the AI")
      .setRequired(true)
      .setMaxLength(1500)
  )
  .addBooleanOption((option) =>
    option
      .setName("private")
      .setDescription("Only you can see the response (default: false)")
  );

export async function execute(interaction: ChatInputCommandInteraction): Promise<void> {
  const question = interaction.options.getString("question", true);
  const ephemeral = interaction.options.getBoolean("private") ?? false;

  if (!isAiEnabled()) {
    await interaction.reply({
      embeds: [
        new EmbedBuilder()
          .setColor(BOT_COLOR_WARNING)
          .setTitle("AI assistant unavailable")
          .setDescription(
            "The AI assistant isn't configured on this server. Use `/ticket` to reach a human."
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

  await interaction.deferReply({ ephemeral });

  const result = await oneShotPrompt({
    userId: interaction.user.id,
    username: interaction.user.displayName,
    message: question,
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
    .setAuthor({ name: `Asked by ${interaction.user.displayName}` })
    .setDescription(first ?? "")
    .setFooter({
      text: "AI assistant - may be wrong, verify before applying",
    });

  await interaction.editReply({ embeds: [header] });

  for (const chunk of rest) {
    await interaction.followUp({ content: chunk, ephemeral });
  }
}
