import {
  ActionRowBuilder,
  ButtonBuilder,
  ButtonStyle,
  EmbedBuilder,
  StringSelectMenuBuilder,
} from "discord.js";
import {
  BOT_COLOR,
  BOT_COLOR_ERROR,
  BOT_COLOR_INFO,
  BOT_COLOR_SUCCESS,
  BOT_COLOR_WARNING,
  BOT_NAME,
  TICKET_CATEGORIES,
} from "../lib/config";
import * as content from "../embeds/content";

export function rulesEmbed(): EmbedBuilder {
  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR)
    .setTitle(content.RULES_CONTENT.title)
    .setDescription(content.RULES_CONTENT.description)
    .setFooter({ text: content.RULES_CONTENT.footer });

  for (const field of content.RULES_CONTENT.fields) {
    embed.addFields(field);
  }

  return embed;
}

export function aboutEmbed(): EmbedBuilder {
  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR)
    .setTitle(content.ABOUT_CONTENT.title)
    .setDescription(content.ABOUT_CONTENT.description);

  for (const field of content.ABOUT_CONTENT.fields) {
    embed.addFields(field);
  }

  return embed;
}

export function verifyEmbed(): {
  embed: EmbedBuilder;
  row: ActionRowBuilder<ButtonBuilder>;
} {
  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR)
    .setTitle(content.VERIFY_CONTENT.title)
    .setDescription(content.VERIFY_CONTENT.description);

  const row = new ActionRowBuilder<ButtonBuilder>().addComponents(
    new ButtonBuilder()
      .setLabel(content.VERIFY_CONTENT.buttonLabel)
      .setStyle(ButtonStyle.Link)
      .setURL(content.VERIFY_CONTENT.buttonUrl)
  );

  return { embed, row };
}

export function rolesEmbed(): {
  embed: EmbedBuilder;
  row: ActionRowBuilder<ButtonBuilder>;
} {
  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR)
    .setTitle(content.ROLES_CONTENT.title)
    .setDescription(content.ROLES_CONTENT.description);

  const row = new ActionRowBuilder<ButtonBuilder>().addComponents(
    new ButtonBuilder()
      .setCustomId("role_toggle_announcements")
      .setLabel("Announcements")
      .setStyle(ButtonStyle.Primary),
    new ButtonBuilder()
      .setCustomId("role_toggle_changelog")
      .setLabel("Changelog")
      .setStyle(ButtonStyle.Primary)
  );

  return { embed, row };
}

export function purchaseEmbed(): EmbedBuilder {
  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR)
    .setTitle(content.PURCHASE_CONTENT.title)
    .setDescription(content.PURCHASE_CONTENT.description);

  for (const field of content.PURCHASE_CONTENT.fields) {
    embed.addFields(field);
  }

  return embed;
}

export function vethidaInfoEmbed(): EmbedBuilder {
  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR)
    .setTitle(content.VETHIDA_INFO_CONTENT.title)
    .setDescription(content.VETHIDA_INFO_CONTENT.description);

  for (const field of content.VETHIDA_INFO_CONTENT.fields) {
    embed.addFields(field);
  }

  return embed;
}

export function guidesEmbed(): EmbedBuilder {
  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR)
    .setTitle(content.GUIDES_GETTING_STARTED.title)
    .setDescription(content.GUIDES_GETTING_STARTED.description);

  for (const field of content.GUIDES_GETTING_STARTED.fields) {
    embed.addFields(field);
  }

  return embed;
}

export function faqEmbed(): EmbedBuilder {
  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR)
    .setTitle(content.FAQ_CONTENT.title);

  for (const field of content.FAQ_CONTENT.fields) {
    embed.addFields(field);
  }

  return embed;
}

export function welcomeEmbed(username: string, avatarUrl?: string): EmbedBuilder {
  const data = content.WELCOME_MESSAGE(username);
  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR)
    .setTitle(data.title)
    .setDescription(data.description)
    .setTimestamp();

  if (avatarUrl) {
    embed.setThumbnail(avatarUrl);
  }

  return embed;
}

export function ticketOpenEmbed(
  category: string,
  username: string,
  ticketId: string
): EmbedBuilder {
  return new EmbedBuilder()
    .setColor(BOT_COLOR_INFO)
    .setTitle(`Ticket #${ticketId}`)
    .setDescription(
      [
        `Category: ${category}`,
        `Opened by: ${username}`,
        "",
        "A member of our support team will be with you shortly.",
        "Please describe your issue in detail and include your Vethida version, Windows version, and any error messages.",
        "",
        "Staff can close this ticket with /ticket close.",
      ].join("\n")
    )
    .setTimestamp();
}

export function ticketCloseEmbed(
  ticketId: string,
  closedBy: string,
  resolution?: string
): EmbedBuilder {
  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR_WARNING)
    .setTitle(`Ticket #${ticketId} - Closed`)
    .addFields({ name: "Closed by", value: closedBy, inline: true })
    .setTimestamp();

  if (resolution) {
    embed.addFields({ name: "Resolution", value: resolution, inline: false });
  }

  return embed;
}

export function ticketWarningEmbed(): EmbedBuilder {
  return new EmbedBuilder()
    .setColor(BOT_COLOR_WARNING)
    .setTitle("Inactivity Notice")
    .setDescription(
      "This ticket will be closed in 24 hours if there is no further activity."
    );
}

export function ticketSelectMenu(): ActionRowBuilder<StringSelectMenuBuilder> {
  const descriptions: Record<(typeof TICKET_CATEGORIES)[number], string> = {
    Billing: "Purchasing, refunds, license upgrades, and payments",
    Technical: "Bugs, crashes, performance issues, and broken tweaks",
    Account: "Sign-in codes, email, HWID, and activation problems",
    General: "Anything else not covered above",
  };

  return new ActionRowBuilder<StringSelectMenuBuilder>().addComponents(
    new StringSelectMenuBuilder()
      .setCustomId("ticket_category_select")
      .setPlaceholder("Select a category")
      .addOptions(
        TICKET_CATEGORIES.map((category) => ({
          label: category,
          value: category.toLowerCase(),
          description: descriptions[category],
        }))
      )
  );
}

export function logEmbed(
  title: string,
  description: string,
  color = BOT_COLOR_INFO
): EmbedBuilder {
  return new EmbedBuilder()
    .setColor(color)
    .setTitle(title)
    .setDescription(description)
    .setFooter({ text: BOT_NAME })
    .setTimestamp();
}

export function statsEmbed(data: {
  totalMembers: number;
  customers: number;
}): EmbedBuilder {
  return new EmbedBuilder()
    .setColor(BOT_COLOR)
    .setTitle("Vethida Server Stats")
    .addFields(
      { name: "Members", value: `${data.totalMembers} total members`, inline: true },
      { name: "Customers", value: `${data.customers} paid license holders`, inline: true }
    )
    .setTimestamp();
}

export function maintenanceEmbed(
  description: string,
  timestamp: number,
  postedBy: string
): EmbedBuilder {
  return new EmbedBuilder()
    .setColor(BOT_COLOR_WARNING)
    .setTitle("Scheduled Maintenance")
    .setDescription(description)
    .addFields(
      { name: "When", value: `<t:${timestamp}:F> (<t:${timestamp}:R>)`, inline: false },
      { name: "Posted by", value: postedBy, inline: true }
    )
    .setTimestamp();
}

export function changelogEmbed(
  version: string,
  added: string[],
  changed: string[],
  fixed: string[],
  postedBy: string
): EmbedBuilder {
  const embed = new EmbedBuilder()
    .setColor(BOT_COLOR_SUCCESS)
    .setTitle(`Vethida ${version}`)
    .setTimestamp()
    .setFooter({ text: `Posted by ${postedBy}` });

  if (added.length > 0) {
    embed.addFields({
      name: "Added",
      value: added.map((line) => `+ ${line}`).join("\n"),
      inline: false,
    });
  }

  if (changed.length > 0) {
    embed.addFields({
      name: "Changed",
      value: changed.map((line) => `~ ${line}`).join("\n"),
      inline: false,
    });
  }

  if (fixed.length > 0) {
    embed.addFields({
      name: "Fixed",
      value: fixed.map((line) => `- ${line}`).join("\n"),
      inline: false,
    });
  }

  return embed;
}

export function warnEmbed(
  userId: string,
  reason: string,
  warnedBy: string
): EmbedBuilder {
  return new EmbedBuilder()
    .setColor(BOT_COLOR_ERROR)
    .setTitle("Warning Issued")
    .addFields(
      { name: "User", value: `<@${userId}>`, inline: true },
      { name: "Warned by", value: warnedBy, inline: true },
      { name: "Reason", value: reason, inline: false }
    )
    .setTimestamp();
}

export function honeypotAlertEmbed(
  userId: string,
  username: string,
  messageContent: string,
  accountAge: string,
  joinedAt: string
): EmbedBuilder {
  return new EmbedBuilder()
    .setColor(BOT_COLOR_ERROR)
    .setTitle("Honeypot Triggered")
    .setDescription(`<@${userId}> sent a message in the honeypot channel.`)
    .addFields(
      { name: "User", value: `${username} (${userId})`, inline: true },
      { name: "Account Age", value: accountAge, inline: true },
      { name: "Joined Server", value: joinedAt, inline: true },
      {
        name: "Message",
        value: messageContent.substring(0, 1024) || "(empty)",
        inline: false,
      }
    )
    .setTimestamp();
}
