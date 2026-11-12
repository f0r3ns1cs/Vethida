import { ChatInputCommandInteraction, SlashCommandBuilder } from "discord.js";
import { statsEmbed } from "../components/embeds";
import { ROLE_IDS } from "../lib/config";

export const data = new SlashCommandBuilder()
  .setName("stats")
  .setDescription("View server and product statistics");

export async function execute(interaction: ChatInputCommandInteraction) {
  const guild = interaction.guild;
  if (!guild) {
    await interaction.reply({
      content: "This command can only be used in a server.",
      ephemeral: true,
    });
    return;
  }

  await interaction.deferReply({ ephemeral: true });

  const members = await guild.members.fetch();
  const customers = members.filter((member) =>
    member.roles.cache.some(
      (role) => role.id === ROLE_IDS.customer || role.name === "Customer",
    ),
  ).size;

  await interaction.editReply({
    embeds: [
      statsEmbed({
        totalMembers: guild.memberCount,
        customers,
      }),
    ],
  });
}
