import { ChatInputCommandInteraction, SlashCommandBuilder, EmbedBuilder } from "discord.js";
import { BOT_COLOR_INFO, BOT_COLOR_WARNING, ROLE_IDS } from "../lib/config";

export const data = new SlashCommandBuilder()
  .setName("license")
  .setDescription("Check whether your Discord account has a linked paid license");

// only the customer role is synced, so we can't tell pro from ultimate here
export async function execute(interaction: ChatInputCommandInteraction) {
  const member = interaction.member;
  if (!member || !("roles" in member)) {
    await interaction.reply({ content: "Could not determine your roles.", ephemeral: true });
    return;
  }

  const roles = member.roles;
  const hasCustomer =
    "cache" in roles
      ? roles.cache.some(
          (r) => r.id === ROLE_IDS.customer || r.name === "Customer",
        )
      : Array.isArray(roles) && ROLE_IDS.customer
        ? roles.includes(ROLE_IDS.customer)
        : false;

  if (!hasCustomer) {
    await interaction.reply({
      embeds: [new EmbedBuilder()
        .setColor(BOT_COLOR_WARNING)
        .setTitle("No paid license found")
        .setDescription("Your Discord account isn't linked to a paid Vethida license.\n\nLink it from https://my.vethida.com/dashboard/settings, or visit [vethida.com/pricing](https://vethida.com/pricing) to get started.")
      ],
      ephemeral: true,
    });
    return;
  }

  await interaction.reply({
    embeds: [new EmbedBuilder()
      .setColor(BOT_COLOR_INFO)
      .setTitle("Paid license active")
      .setDescription("Your Discord account is linked to an active paid Vethida license.")
      .setFooter({ text: "See your tier, devices, and renewal at my.vethida.com" })
    ],
    ephemeral: true,
  });
}
