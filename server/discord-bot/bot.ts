import {
  Client,
  Collection,
  Events,
  GatewayIntentBits,
  Partials,
  REST,
  Routes,
  type ChatInputCommandInteraction,
} from "discord.js";
import {
  ACTIVATION_PREFIX,
  ALLOW_LEGACY_DISCORD_KEY_DM,
  BOT_TOKEN,
  CLIENT_ID,
  GUILD_ID,
  ROLE_IDS,
  TICKET_CLOSE_HOURS,
  TICKET_WARN_HOURS,
  TIERS,
} from "./lib/config";
import { logger } from "./lib/logger";
import { grantsRole, lookupLicense, tierLabelToSlug } from "./lib/api";

import * as aboutCmd from "./commands/about";
import * as askCmd from "./commands/ask";
import * as bugCmd from "./commands/bug";
import * as changelogCmd from "./commands/changelog";
import * as clearContextCmd from "./commands/clearcontext";
import * as diagnosticCmd from "./commands/diagnostic";
import * as docsCmd from "./commands/docs";
import * as explainCmd from "./commands/explain";
import * as faqCmd from "./commands/faq";
import * as guideCmd from "./commands/guide";
import * as licenseCmd from "./commands/license";
import * as maintenanceCmd from "./commands/maintenance";
import * as pingCmd from "./commands/ping";
import * as purchaseCmd from "./commands/purchase";
import * as purgeCmd from "./commands/purge";
import * as refundCmd from "./commands/refund";
import * as securityCmd from "./commands/security";
import * as statsCmd from "./commands/stats";
import * as statusCmd from "./commands/status";
import * as ticketCmd from "./commands/ticket";
import * as troubleshootCmd from "./commands/troubleshoot";
import * as verifyCmd from "./commands/verify";
import * as versionCmd from "./commands/version";
import * as warnCmd from "./commands/warn";
import * as warningsCmd from "./commands/warnings";

import { logEmbed, ticketWarningEmbed } from "./components/embeds";
import { handleDmAi, handleGuildAi } from "./events/aiChat";
import { handleHoneypot } from "./events/honeypot";
import { handleMemberBoost } from "./events/memberBoost";
import { handleMemberJoin } from "./events/memberJoin";
import { handleMemberLeave } from "./events/memberLeave";
import { handleReady } from "./events/ready";
import { startHealthServer } from "./lib/health";

interface Command {
  data: { name: string; toJSON(): unknown };
  execute: (interaction: ChatInputCommandInteraction) => Promise<void>;
}

const commands = new Collection<string, Command>();
const commandList: Command[] = [
  verifyCmd,
  licenseCmd,
  statsCmd,
  ticketCmd,
  changelogCmd,
  maintenanceCmd,
  warnCmd,
  warningsCmd,
  purgeCmd,
  askCmd,
  explainCmd,
  troubleshootCmd,
  clearContextCmd,
  aboutCmd,
  bugCmd,
  diagnosticCmd,
  docsCmd,
  faqCmd,
  guideCmd,
  pingCmd,
  purchaseCmd,
  refundCmd,
  securityCmd,
  statusCmd,
  versionCmd,
];

for (const command of commandList) {
  commands.set(command.data.name, command);
}

const client = new Client({
  intents: [
    GatewayIntentBits.Guilds,
    GatewayIntentBits.GuildMembers,
    GatewayIntentBits.GuildMessages,
    GatewayIntentBits.DirectMessages,
    GatewayIntentBits.MessageContent,
  ],
  partials: [Partials.Channel],
  allowedMentions: { parse: [] },
});

function findRoleByIdOrName(
  guild: import("discord.js").Guild,
  roleId: string,
  fallbackName: string
) {
  return (
    (roleId && guild.roles.cache.get(roleId)) ||
    guild.roles.cache.find((role) => role.name === fallbackName)
  );
}

client.once(Events.ClientReady, (instance) => handleReady(instance));

client.on(Events.GuildMemberAdd, (member) => handleMemberJoin(member));

client.on(Events.GuildMemberRemove, (member) => handleMemberLeave(member));

client.on(Events.GuildMemberUpdate, (oldMember, newMember) => {
  if (oldMember.partial || newMember.partial) {
    return;
  }

  void handleMemberBoost(oldMember, newMember);
});

client.on(Events.MessageCreate, async (message) => {
  if (message.author.bot) {
    return;
  }

  if (message.guild) {
    await handleHoneypot(message);
    await handleGuildAi(client, message);
    return;
  }

  const content = message.content.trim();

  if (content.startsWith(ACTIVATION_PREFIX)) {
    if (!ALLOW_LEGACY_DISCORD_KEY_DM) {
      await message.channel.send(
        "Direct key verification in Discord DMs is disabled. Link your Discord account from https://my.vethida.com/dashboard instead."
      );
      return;
    }

    const activationKey = content.slice(ACTIVATION_PREFIX.length).trim().toUpperCase();

    // just a format check, the api does the real validation
    if (!/^[0-9A-HJKMNP-TV-Z]{5}(?:-[0-9A-HJKMNP-TV-Z]{5}){4}$/.test(activationKey)) {
      await message.channel.send("That license key format is invalid.");
      return;
    }

    await message.channel.send("Validating your license key...");

    const license = await lookupLicense(activationKey);
    if (!license) {
      await message.channel.send("Invalid license key.");
      return;
    }
    if (license.status !== "active") {
      await message.channel.send(
        `That key is ${license.status} and cannot be used to claim roles.`,
      );
      return;
    }

    const slug = tierLabelToSlug(license.tier);
    if (!slug) {
      await message.channel.send(
        `Unrecognized tier (${license.tier}). Please contact support.`,
      );
      return;
    }
    const tier = TIERS[slug];

    const guild = client.guilds.cache.get(GUILD_ID);
    if (!guild) {
      await message.channel.send(
        "Could not find the server. Please contact support."
      );
      return;
    }

    if (!grantsRole(license.tier)) {
      await message.channel.send(
        "That key is valid, but I don't recognize its plan. Run `/ticket` and we'll sort the role out."
      );
      return;
    }

    try {
      const member = await guild.members.fetch(message.author.id);
      const customerRole = findRoleByIdOrName(
        guild,
        ROLE_IDS.customer,
        "Customer"
      );
      if (customerRole && !member.roles.cache.has(customerRole.id)) {
        await member.roles.add(customerRole);
      }
      await message.channel.send(
        `That's a valid Vethida ${tier.label} key. Your customer role has been applied.`
      );
    } catch {
      await message.channel.send(
        "Could not assign your role. Make sure you are a member of the Vethida Discord server."
      );
    }

    return;
  }

  // other DMs go to the AI if it's on
  const handled = await handleDmAi(client, message);
  if (handled) {
    return;
  }

  await message.channel.send(
    "Hi, I'm the Vethida Discord Bot. Link your Discord account from https://my.vethida.com/dashboard to receive your roles and access."
  );
});

client.on(Events.InteractionCreate, async (interaction) => {
  if (interaction.isChatInputCommand()) {
    const command = commands.get(interaction.commandName);
    if (!command) {
      return;
    }

    try {
      await command.execute(interaction);
    } catch (error) {
      console.error(`Error executing /${interaction.commandName}:`, error);

      const reply = {
        content: "Something went wrong. Please try again.",
        ephemeral: true,
      };

      if (interaction.replied || interaction.deferred) {
        await interaction.followUp(reply);
      } else {
        await interaction.reply(reply);
      }
    }

    return;
  }

  if (interaction.isButton()) {
    if (
      interaction.customId === "role_toggle_announcements" ||
      interaction.customId === "role_toggle_changelog"
    ) {
      const roleName =
        interaction.customId === "role_toggle_announcements"
          ? "Announcements"
          : "Changelog";

      const guild = interaction.guild;
      if (!guild) {
        return;
      }

      const role = guild.roles.cache.find((candidate) => candidate.name === roleName);
      if (!role) {
        await interaction.reply({
          content: `Could not find the ${roleName} role.`,
          ephemeral: true,
        });
        return;
      }

      const member = await guild.members.fetch(interaction.user.id);
      if (member.roles.cache.has(role.id)) {
        await member.roles.remove(role);
        await interaction.reply({
          content: `Removed the ${roleName} role.`,
          ephemeral: true,
        });
      } else {
        await member.roles.add(role);
        await interaction.reply({
          content: `Added the ${roleName} role.`,
          ephemeral: true,
        });
      }
    }

    return;
  }

  // /bug modal
  if (interaction.isModalSubmit()) {
    try {
      await bugCmd.handleModal(interaction);
    } catch (error) {
      logger.error({ err: error, customId: interaction.customId }, "modal handler failed");
      if (!interaction.replied && !interaction.deferred) {
        await interaction.reply({
          content: "Something went wrong filing that. Please try again.",
          ephemeral: true,
        });
      }
    }

    return;
  }

  if (interaction.isStringSelectMenu()) {
    if (interaction.customId === "ticket_category_select") {
      const category = interaction.values[0];
      if (!category) {
        return;
      }
      const displayCategory =
        category.charAt(0).toUpperCase() + category.slice(1);

      await ticketCmd.handleTicketCategorySelect(interaction, displayCategory);
    }
  }
});

setInterval(async () => {
  try {
    const guild = client.guilds.cache.get(GUILD_ID);
    if (!guild) {
      return;
    }

    const activeThreads = await guild.channels.fetchActiveThreads();

    for (const [, thread] of activeThreads.threads) {
      if (!thread.name.startsWith("ticket-") || thread.archived) {
        continue;
      }

      // count from the last non-bot message (bot's own warning shouldn't reset it)
      const messages = await thread.messages.fetch({ limit: 50 });
      const lastHuman = messages.find((message) => !message.author.bot);
      const lastActivityAt =
        lastHuman?.createdTimestamp ??
        (messages.size < 50 ? thread.createdTimestamp : null);
      if (lastActivityAt === null) {
        continue;
      }

      const hoursSinceLastMessage =
        (Date.now() - lastActivityAt) / (1000 * 60 * 60);

      if (hoursSinceLastMessage >= TICKET_CLOSE_HOURS) {
        await thread.send({
          embeds: [
            logEmbed(
              "Ticket Auto-Closed",
              "This ticket was automatically closed due to inactivity.",
              0xf1c40f
            ),
          ],
        });
        await thread.setArchived(true);
      } else if (hoursSinceLastMessage >= TICKET_WARN_HOURS) {
        const recentMessages = await thread.messages.fetch({ limit: 5 });
        const alreadyWarned = recentMessages.some(
          (message) =>
            message.author.id === client.user?.id &&
            message.embeds.some((embed) => embed.title === "Inactivity Notice")
        );

        if (!alreadyWarned) {
          await thread.send({ embeds: [ticketWarningEmbed()] });
        }
      }
    }
  } catch (error) {
    console.error("Ticket inactivity check error:", error);
  }
}, 60 * 60 * 1000);

async function main() {
  const rest = new REST().setToken(BOT_TOKEN);
  const commandData = commandList.map((command) => command.data.toJSON());

  logger.info({ count: commandData.length }, "registering slash commands");
  await rest.put(Routes.applicationGuildCommands(CLIENT_ID, GUILD_ID), {
    body: commandData,
  });
  logger.info("slash commands registered");

  startHealthServer(client); // 503 until the client is ready

  await client.login(BOT_TOKEN);
}

main().catch((error) => {
  console.error("Failed to start Vethida Discord Bot:", error);
  process.exit(1);
});
