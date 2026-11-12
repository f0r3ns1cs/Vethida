import { and, desc, eq, gte, sql } from "drizzle-orm";
import {
  db,
  discordAiUsage,
  discordAutomodEvent,
  discordModAction,
  discordTicket,
  discordWarning,
} from "@vethida/db";

export const warnings = {
  async add(
    guildId: string,
    userId: string,
    moderator: string,
    reason: string,
  ): Promise<void> {
    await db
      .insert(discordWarning)
      .values({ guildId, userId, moderator, reason });
  },

  async list(
    guildId: string,
    userId: string,
    limit = 25,
  ): Promise<
    Array<{ id: string; moderator: string; reason: string; createdAt: Date }>
  > {
    return db
      .select({
        id: discordWarning.id,
        moderator: discordWarning.moderator,
        reason: discordWarning.reason,
        createdAt: discordWarning.createdAt,
      })
      .from(discordWarning)
      .where(
        and(
          eq(discordWarning.guildId, guildId),
          eq(discordWarning.userId, userId),
        ),
      )
      .orderBy(desc(discordWarning.createdAt))
      .limit(limit);
  },

  async count(guildId: string, userId: string): Promise<number> {
    const [row] = await db
      .select({ n: sql<number>`count(*)::int` })
      .from(discordWarning)
      .where(
        and(
          eq(discordWarning.guildId, guildId),
          eq(discordWarning.userId, userId),
        ),
      );
    return row?.n ?? 0;
  },
};

export const modActions = {
  async record(input: {
    guildId: string;
    userId: string;
    action:
      | "ban"
      | "kick"
      | "mute"
      | "unmute"
      | "warn"
      | "quarantine"
      | "unquarantine";
    moderator: string;
    reason: string;
    metadata?: Record<string, unknown>;
  }): Promise<void> {
    await db.insert(discordModAction).values({
      guildId: input.guildId,
      userId: input.userId,
      action: input.action,
      moderator: input.moderator,
      reason: input.reason,
      metadata: input.metadata ?? null,
    });
  },
};

export const tickets = {
  async create(
    id: string,
    guildId: string,
    userId: string,
    category: string,
    threadId: string,
  ): Promise<void> {
    await db
      .insert(discordTicket)
      .values({ id, guildId, userId, category, threadId, status: "open" });
  },

  async close(
    id: string,
    closedBy: string,
    resolution?: string,
  ): Promise<void> {
    await db
      .update(discordTicket)
      .set({
        status: "closed",
        closedAt: new Date(),
        closedBy,
        resolution: resolution ?? null,
      })
      .where(and(eq(discordTicket.id, id), eq(discordTicket.status, "open")));
  },

  async findOpen(
    guildId: string,
    userId: string,
  ): Promise<{ id: string; threadId: string } | null> {
    const [row] = await db
      .select({ id: discordTicket.id, threadId: discordTicket.threadId })
      .from(discordTicket)
      .where(
        and(
          eq(discordTicket.guildId, guildId),
          eq(discordTicket.userId, userId),
          eq(discordTicket.status, "open"),
        ),
      )
      .limit(1);
    return row ?? null;
  },
};

export const automod = {
  async record(
    guildId: string,
    userId: string,
    kind: string,
    content: string | null,
  ): Promise<void> {
    await db
      .insert(discordAutomodEvent)
      .values({ guildId, userId, kind, content });
  },

  async countRecent(
    guildId: string,
    userId: string,
    sinceMs: number,
  ): Promise<number> {
    const [row] = await db
      .select({ n: sql<number>`count(*)::int` })
      .from(discordAutomodEvent)
      .where(
        and(
          eq(discordAutomodEvent.guildId, guildId),
          eq(discordAutomodEvent.userId, userId),
          gte(discordAutomodEvent.createdAt, new Date(Date.now() - sinceMs)),
        ),
      );
    return row?.n ?? 0;
  },
};

// yyyy-mm-dd (UTC), cap resets at midnight UTC
function todayKey(): string {
  return new Date().toISOString().slice(0, 10);
}

export const aiUsage = {
  async record(
    userId: string,
    inputTokens: number,
    outputTokens: number,
  ): Promise<void> {
    await db
      .insert(discordAiUsage)
      .values({
        userId,
        day: todayKey(),
        inputTokens,
        outputTokens,
        requests: 1,
        lastUsedAt: new Date(),
      })
      .onConflictDoUpdate({
        target: [discordAiUsage.userId, discordAiUsage.day],
        set: {
          inputTokens: sql`${discordAiUsage.inputTokens} + excluded.input_tokens`,
          outputTokens: sql`${discordAiUsage.outputTokens} + excluded.output_tokens`,
          requests: sql`${discordAiUsage.requests} + 1`,
          lastUsedAt: sql`excluded.last_used_at`,
        },
      });
  },

  async totalTokensToday(userId: string): Promise<number> {
    const [row] = await db
      .select({
        inputTokens: discordAiUsage.inputTokens,
        outputTokens: discordAiUsage.outputTokens,
      })
      .from(discordAiUsage)
      .where(
        and(
          eq(discordAiUsage.userId, userId),
          eq(discordAiUsage.day, todayKey()),
        ),
      );
    if (!row) {return 0;}
    return row.inputTokens + row.outputTokens;
  },
};
