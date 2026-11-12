import {
  bigint,
  index,
  integer,
  jsonb,
  pgTable,
  primaryKey,
  text,
  timestamp,
  uuid,
  varchar,
} from "drizzle-orm/pg-core";

// Discord bot tables. No FKs to users, a discord member isn't necessarily a customer.

export const discordWarning = pgTable(
  "discord_warning",
  {
    id: uuid("id").primaryKey().defaultRandom(),
    guildId: varchar("guild_id", { length: 32 }).notNull(),
    userId: varchar("user_id", { length: 32 }).notNull(),
    moderator: varchar("moderator", { length: 32 }).notNull(),
    reason: text("reason").notNull(),
    createdAt: timestamp("created_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
  },
  (t) => [
    index("discord_warning_guild_user_idx").on(t.guildId, t.userId, t.createdAt),
  ],
);

export const discordModAction = pgTable(
  "discord_mod_action",
  {
    id: uuid("id").primaryKey().defaultRandom(),
    guildId: varchar("guild_id", { length: 32 }).notNull(),
    userId: varchar("user_id", { length: 32 }).notNull(),
    action: varchar("action", { length: 32 }).notNull(),
    moderator: varchar("moderator", { length: 32 }).notNull(),
    reason: text("reason").notNull(),
    metadata: jsonb("metadata"),
    createdAt: timestamp("created_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
  },
  (t) => [
    index("discord_mod_action_guild_user_idx").on(
      t.guildId,
      t.userId,
      t.createdAt,
    ),
  ],
);

export const discordTicket = pgTable(
  "discord_ticket",
  {
    id: text("id").primaryKey(),
    guildId: varchar("guild_id", { length: 32 }).notNull(),
    userId: varchar("user_id", { length: 32 }).notNull(),
    category: varchar("category", { length: 32 }).notNull(),
    threadId: varchar("thread_id", { length: 32 }).notNull(),
    status: varchar("status", { length: 16 }).notNull().default("open"),
    openedAt: timestamp("opened_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
    closedAt: timestamp("closed_at", { withTimezone: true }),
    closedBy: varchar("closed_by", { length: 32 }),
    resolution: text("resolution"),
  },
  (t) => [
    index("discord_ticket_open_idx").on(t.guildId, t.userId, t.status),
  ],
);

export const discordAutomodEvent = pgTable(
  "discord_automod_event",
  {
    id: uuid("id").primaryKey().defaultRandom(),
    guildId: varchar("guild_id", { length: 32 }).notNull(),
    userId: varchar("user_id", { length: 32 }).notNull(),
    kind: varchar("kind", { length: 32 }).notNull(),
    content: text("content"),
    createdAt: timestamp("created_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
  },
  (t) => [
    index("discord_automod_event_user_idx").on(
      t.guildId,
      t.userId,
      t.createdAt,
    ),
  ],
);

// daily AI token usage per user (day = UTC yyyy-mm-dd)
export const discordAiUsage = pgTable(
  "discord_ai_usage",
  {
    userId: varchar("user_id", { length: 32 }).notNull(),
    day: varchar("day", { length: 10 }).notNull(),
    inputTokens: bigint("input_tokens", { mode: "number" })
      .notNull()
      .default(0),
    outputTokens: bigint("output_tokens", { mode: "number" })
      .notNull()
      .default(0),
    requests: integer("requests").notNull().default(0),
    lastUsedAt: timestamp("last_used_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
  },
  (t) => [primaryKey({ columns: [t.userId, t.day] })],
);
