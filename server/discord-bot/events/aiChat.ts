import type { Client, Message } from "discord.js";
import { ChannelType } from "discord.js";
import { AI } from "../lib/config";
import { chatWithHistory, chunkForDiscord, isAiEnabled } from "../lib/ai";
import {
  AI_PAID_ONLY_TEXT,
  isEntitledToAi,
  isUserEntitledToAi,
} from "../lib/entitlement";
import { logger } from "../lib/logger";

// model output can contain anything, so no pings at all
const NO_MENTIONS = { parse: [] as never[], repliedUser: false };

function stripMentions(content: string, botUserId: string): string {
  return content
    .replace(new RegExp(`<@!?${botUserId}>`, "g"), "")
    .replace(/\s+/g, " ")
    .trim();
}

interface ReplyContext {
  cause: "mention" | "support_channel" | "dm";
  client: Client;
  message: Message;
}

async function reply(ctx: ReplyContext): Promise<void> {
  if (!isAiEnabled()) {
    return;
  }

  const { client, message, cause } = ctx;
  const botUserId = client.user?.id;
  if (!botUserId) {
    return;
  }

  const entitled =
    cause === "dm"
      ? await isUserEntitledToAi(client, message.author.id)
      : isEntitledToAi(message.member);
  if (!entitled) {
    // don't spam the upsell in the support channel, only on mentions/DMs
    if (cause !== "support_channel") {
      await message.reply({
        content: AI_PAID_ONLY_TEXT,
        allowedMentions: NO_MENTIONS,
      });
    }
    return;
  }

  const cleaned = stripMentions(message.content ?? "", botUserId);

  if (cleaned.length === 0) {
    if (cause === "mention") {
      await message.reply({
        content:
          "Ask me a Vethida or Windows question - e.g. `@" +
          (client.user?.username ?? "Vethida") +
          " how do I tighten Windows 11 telemetry without breaking updates?`",
        allowedMentions: NO_MENTIONS,
      });
    }
    return;
  }

  if ("sendTyping" in message.channel && typeof message.channel.sendTyping === "function") {
    message.channel.sendTyping().catch(() => {});
  }

  const result = await chatWithHistory({
    userId: message.author.id,
    channelId: message.channelId,
    username: message.author.displayName ?? message.author.username,
    message: cleaned,
  });

  if (!result.ok) {
    await message.reply({
      content: result.message,
      allowedMentions: NO_MENTIONS,
    });
    return;
  }

  const chunks = chunkForDiscord(result.text);
  for (const chunk of chunks) {
    await message.reply({
      content: chunk,
      allowedMentions: NO_MENTIONS,
    });
  }
}

// returns true if the message was handled
export async function handleGuildAi(
  client: Client,
  message: Message
): Promise<boolean> {
  if (!isAiEnabled() || message.author.bot) {
    return false;
  }
  if (message.channel.type === ChannelType.DM) {
    return false;
  }

  const botUserId = client.user?.id;
  if (!botUserId) {
    return false;
  }

  const isMentioned = message.mentions.users.has(botUserId);
  const isInSupportChannel =
    AI.supportChannelId.length > 0 && message.channelId === AI.supportChannelId;

  if (!isMentioned && !isInSupportChannel) {
    return false;
  }

  try {
    await reply({
      cause: isMentioned ? "mention" : "support_channel",
      client,
      message,
    });
  } catch (err) {
    logger.error({ err, channelId: message.channelId }, "ai guild handler failed");
  }
  return true;
}

export async function handleDmAi(client: Client, message: Message): Promise<boolean> {
  if (!isAiEnabled() || message.author.bot) {
    return false;
  }

  try {
    await reply({ cause: "dm", client, message });
  } catch (err) {
    logger.error({ err, userId: message.author.id }, "ai dm handler failed");
  }
  return true;
}
