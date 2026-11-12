import crypto from "crypto";
import {
  ChannelType,
  ChatInputCommandInteraction,
  PermissionFlagsBits,
  SlashCommandBuilder,
  StringSelectMenuInteraction,
  TextChannel,
  ThreadAutoArchiveDuration,
} from "discord.js";
import { BOT_COLOR_INFO } from "../lib/config";
import {
  logEmbed,
  ticketCloseEmbed,
  ticketOpenEmbed,
  ticketSelectMenu,
} from "../components/embeds";

const OPEN_TICKET_COOLDOWN_MS = 60 * 1000;

const globalTicketStore = globalThis as typeof globalThis & {
  __vethidaTicketOpenStore?: Map<string, number>;
};

const ticketOpenStore =
  globalTicketStore.__vethidaTicketOpenStore ??
  (globalTicketStore.__vethidaTicketOpenStore = new Map<string, number>());

function buildTicketIdentity(username: string) {
  const ticketId = crypto.randomBytes(3).toString("hex");
  const safeUser = username
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, "-")
    .replace(/^-+|-+$/g, "")
    .slice(0, 16) || "user";

  return {
    ticketId,
    threadName: `ticket-${ticketId}-${safeUser}`,
  };
}

function checkTicketOpenCooldown(userId: string): {
  ok: boolean;
  retryAfterSeconds: number;
} {
  const now = Date.now();
  const lastOpenedAt = ticketOpenStore.get(userId);

  if (!lastOpenedAt || now - lastOpenedAt >= OPEN_TICKET_COOLDOWN_MS) {
    ticketOpenStore.set(userId, now);
    return { ok: true, retryAfterSeconds: 0 };
  }

  return {
    ok: false,
    retryAfterSeconds: Math.max(
      1,
      Math.ceil((OPEN_TICKET_COOLDOWN_MS - (now - lastOpenedAt)) / 1000)
    ),
  };
}

async function findExistingOpenTicket(
  guild: import("discord.js").Guild,
  userId: string
) {
  const activeThreads = await guild.channels.fetchActiveThreads();

  for (const [, thread] of activeThreads.threads) {
    if (!thread.name.startsWith("ticket-") || thread.archived) {
      continue;
    }

    try {
      await thread.members.fetch(userId);
      return thread;
    } catch {
      // not in this one
    }
  }

  return null;
}

export const data = new SlashCommandBuilder()
  .setName("ticket")
  .setDescription("Open or close a support ticket")
  .addSubcommand((subcommand) =>
    subcommand.setName("open").setDescription("Open a new support ticket")
  )
  .addSubcommand((subcommand) =>
    subcommand
      .setName("close")
      .setDescription("Close the current ticket")
      .addStringOption((option) =>
        option
          .setName("resolution")
          .setDescription("Resolution note")
          .setMaxLength(300)
      )
  );

export async function execute(interaction: ChatInputCommandInteraction) {
  const subcommand = interaction.options.getSubcommand();

  if (subcommand === "open") {
    await interaction.reply({
      content: "Select a category for your ticket:",
      components: [ticketSelectMenu()],
      ephemeral: true,
    });
    return;
  }

  await handleClose(interaction);
}

export async function handleTicketCategorySelect(
  interaction: StringSelectMenuInteraction,
  category: string
) {
  const guild = interaction.guild;
  if (!guild) {
    return;
  }

  const ticketsChannel = guild.channels.cache.find(
    (channel) =>
      channel.type === ChannelType.GuildText && channel.name === "tickets"
  ) as TextChannel | undefined;

  if (!ticketsChannel) {
    await interaction.reply({
      content: "Ticket system is not set up. Please contact staff directly.",
      ephemeral: true,
    });
    return;
  }

  const existingThread = await findExistingOpenTicket(guild, interaction.user.id);
  if (existingThread) {
    await interaction.reply({
      content: `You already have an open ticket: ${existingThread.toString()}`,
      ephemeral: true,
    });
    return;
  }

  const ticketBudget = checkTicketOpenCooldown(interaction.user.id);
  if (!ticketBudget.ok) {
    await interaction.reply({
      content: `Please wait ${ticketBudget.retryAfterSeconds} seconds before opening another ticket.`,
      ephemeral: true,
    });
    return;
  }

  const { ticketId, threadName } = buildTicketIdentity(interaction.user.username);

  const thread = await ticketsChannel.threads.create({
    name: threadName,
    type: ChannelType.PrivateThread,
    autoArchiveDuration: ThreadAutoArchiveDuration.OneWeek,
    invitable: false,
  });

  await thread.members.add(interaction.user.id);

  await thread.send({
    embeds: [ticketOpenEmbed(category, interaction.user.username, ticketId)],
  });

  await interaction.update({
    content: `Your ticket has been opened: ${thread.toString()}`,
    components: [],
  });

  const staffTickets = guild.channels.cache.find(
    (channel) => channel.name === "staff-tickets"
  ) as TextChannel | undefined;

  if (staffTickets) {
    await staffTickets.send({
      embeds: [
        logEmbed(
          `Ticket #${ticketId} Opened`,
          `User: <@${interaction.user.id}>\nCategory: ${category}\nThread: ${thread.toString()}`,
          BOT_COLOR_INFO
        ),
      ],
    });
  }
}

async function handleClose(interaction: ChatInputCommandInteraction) {
  const channel = interaction.channel;
  if (!channel || !channel.isThread() || !channel.name.startsWith("ticket-")) {
    await interaction.reply({
      content: "This command can only be used inside a ticket thread.",
      ephemeral: true,
    });
    return;
  }

  const canClose =
    interaction.memberPermissions?.has(PermissionFlagsBits.ManageThreads) ||
    interaction.memberPermissions?.has(PermissionFlagsBits.ManageChannels) ||
    interaction.memberPermissions?.has(PermissionFlagsBits.Administrator);

  if (!canClose) {
    await interaction.reply({
      content: "Only staff can close tickets.",
      ephemeral: true,
    });
    return;
  }

  const resolution = interaction.options.getString("resolution") ?? undefined;
  const ticketId = channel.name.split("-")[1] ?? "unknown";

  await channel.send({
    embeds: [ticketCloseEmbed(ticketId, interaction.user.username, resolution)],
  });

  await channel.setArchived(true);

  const guild = interaction.guild;
  if (guild) {
    const staffTickets = guild.channels.cache.find(
      (candidate) => candidate.name === "staff-tickets"
    ) as TextChannel | undefined;

    if (staffTickets) {
      await staffTickets.send({
        embeds: [
          logEmbed(
            `Ticket #${ticketId} Closed`,
            `Closed by: ${interaction.user.username}\nResolution: ${resolution ?? "No resolution provided"}`,
            0xf1c40f
          ),
        ],
      });
    }
  }

  await interaction.reply({
    content: "Ticket closed and archived.",
    ephemeral: true,
  });
}
