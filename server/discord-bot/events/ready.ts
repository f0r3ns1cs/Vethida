import { Client, ActivityType, ChannelType } from "discord.js";
import { GUILD_ID, MEMBER_COUNT_INTERVAL } from "../lib/config";
import { logger } from "../lib/logger";

export function handleReady(client: Client<true>) {
  logger.info({ tag: client.user.tag }, "bot online");

  client.user.setPresence({
    activities: [{ name: "over Vethida", type: ActivityType.Watching }],
    status: "online",
  });

  void updateMemberCount(client);
  setInterval(() => void updateMemberCount(client), MEMBER_COUNT_INTERVAL);
}

async function updateMemberCount(client: Client<true>) {
  try {
    const guild = client.guilds.cache.get(GUILD_ID);
    if (!guild) {return;}

    const countChannel = guild.channels.cache.find(
      (ch) => ch.type === ChannelType.GuildVoice && ch.name.startsWith("Members:")
    );

    if (countChannel) {
      const formatted = guild.memberCount.toLocaleString("en-US");
      const newName = `Members: ${formatted}`;
      if (countChannel.name !== newName) {
        await countChannel.setName(newName);
      }
    }
  } catch (err) {
    logger.error({ err }, "failed to update member count channel");
  }
}
