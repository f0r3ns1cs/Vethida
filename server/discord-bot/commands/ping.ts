import { ChatInputCommandInteraction, SlashCommandBuilder } from "discord.js";

export const data = new SlashCommandBuilder()
  .setName("ping")
  .setDescription("Check the bot's latency");

export async function execute(interaction: ChatInputCommandInteraction): Promise<void> {
  const sent = await interaction.reply({
    content: "Pinging...",
    ephemeral: true,
    fetchReply: true,
  });

  const roundTrip = sent.createdTimestamp - interaction.createdTimestamp;
  const gatewayPing = Math.round(interaction.client.ws.ping);

  await interaction.editReply(
    `Pong. Round-trip **${roundTrip}ms** - Gateway **${gatewayPing}ms**`
  );
}
