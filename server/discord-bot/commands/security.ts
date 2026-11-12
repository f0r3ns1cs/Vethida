import {
  ChatInputCommandInteraction,
  SlashCommandBuilder,
  EmbedBuilder,
} from "discord.js";
import { BOT_COLOR_INFO, WEBSITE_URL } from "../lib/config";

export const data = new SlashCommandBuilder()
  .setName("security")
  .setDescription("How to report a security issue to Vethida");

export async function execute(
  interaction: ChatInputCommandInteraction,
): Promise<void> {
  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR_INFO)
    .setTitle("Reporting a security issue")
    .setDescription(
      [
        "Report vulnerabilities in Vethida, the Vethida portal, or our",
        "infrastructure to the address below.",
      ].join(" "),
    )
    .addFields(
      {
        name: "Contact",
        value: [
          "**Email:** `security@vethida.com`",
          "**Public policy:** [/security](" +
            WEBSITE_URL +
            "/security)",
          "**Disclosure window:** 90 days (negotiable for active exploitation).",
        ].join("\n"),
      },
      {
        name: "Acknowledgment",
        value:
          "Automatic on receipt. If you do not hear anything at all, send it again - " +
          "a missed report is our failure, not yours.",
      },
      {
        name: "Triage",
        value:
          "A human update - severity assessment, planned response, timeline - within **5 business days**.",
      },
      {
        name: "In scope",
        value: [
          "- Vethida desktop client (binary tampering, RCE, privilege escalation, etc.).",
          "- Vethida portal (auth, IDOR, XSS, CSRF, license-server abuse).",
          "- Vethida infrastructure (DNS, TLS, CT-log misissuance).",
        ].join("\n"),
      },
      {
        name: "Out of scope",
        value: [
          "- Theoretical attacks on accepted residual risks (full list in our threat model).",
          "- Issues in third-party services we use (report to them directly).",
        ].join("\n"),
      },
      {
        name: "Good-faith protection",
        value:
          "If you act in good faith and follow our policy, we won't pursue legal action against you for testing, finding, or reporting issues.",
      },
    )
    .setFooter({
      text: "Vethida Security - Coordinated disclosure model - Public credit available with consent",
    });

  await interaction.reply({ embeds: [embed], ephemeral: true });
}
