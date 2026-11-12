import {
  ChatInputCommandInteraction,
  SlashCommandBuilder,
  EmbedBuilder,
} from "discord.js";
import { BOT_COLOR_INFO, REFUND_WINDOW_DAYS } from "../lib/config";

export const data = new SlashCommandBuilder()
  .setName("refund")
  .setDescription("Vethida refund policy and how to request one");

export async function execute(
  interaction: ChatInputCommandInteraction,
): Promise<void> {
  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR_INFO)
    .setTitle("Refund policy")
    .setDescription(
      "Payments are non-refundable except as stated here or as required by law.",
    )
    .addFields(
      {
        name: `${REFUND_WINDOW_DAYS}-day refund window`,
        value:
          `Request a full refund within ${REFUND_WINDOW_DAYS} days of the initial purchase of a paid license. ` +
          `It applies to the first purchase and does not reset on renewal. After ${REFUND_WINDOW_DAYS} days, ` +
          "purchases are non-refundable except where the law requires otherwise.",
      },
      {
        name: "Cancellation is not a refund",
        value:
          "Canceling stops future renewals. The license stays active until the end of the term already paid for, then Vethida drops to Free. The unused portion of a term is not refunded and access is not cut off early. Renewal charges fall outside the refund window.",
      },
      {
        name: "Statutory rights",
        value:
          `Consumers in the EU, EEA, and UK have a ${REFUND_WINDOW_DAYS}-day right of withdrawal on digital ` +
          "purchases. The window above matches it and is applied to every customer, wherever they are.",
      },
      {
        name: "How to request",
        value: [
          "1. Email support@vethida.com from the address on your account.",
          "2. Refunds are issued by Polar, our merchant of record, to the original payment method (5-10 business days).",
          "3. On refund, the license is deactivated and Vethida drops to Free.",
        ].join("\n"),
      },
      // matches the Exceptions section in legal/refunds.md
      {
        name: "Not refundable",
        value: [
          "- Purchases outside the windows above.",
          "- We may decline a refund for conduct aimed at the purchase itself: payment fraud, defeating license validation or the device limit, sharing or reselling a license or account, and repeated refund-and-repurchase patterns.",
          "- A person reviews the case before a refund is declined, and you can ask for that decision to be looked at again.",
          "- Any other breach of the EULA is dealt with under the EULA, and your refund window is unaffected.",
        ].join("\n"),
      },
      {
        name: "Chargebacks",
        value:
          "Contact us before filing one; we can usually settle it faster than a bank dispute. Filing a chargeback may suspend the account and license while the dispute is investigated.",
      },
    );

  await interaction.reply({ embeds: [embed], ephemeral: true });
}
