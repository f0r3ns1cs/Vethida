import pino from "pino";
import { LOG_LEVEL, NODE_ENV } from "./config";

const REDACT_PATHS = [
  "token",
  "bot_token",
  "DISCORD_BOT_TOKEN",
  "authorization",
  "Authorization",
  "x-internal-token",
  "X-Internal-Token",
  "password",
  "activationKey",
  "license_key",
  "apiKey",
  "api_key",
  "*.token",
  "*.password",
  "*.authorization",
];

export const logger = pino({
  level: LOG_LEVEL,
  base: { service: "vethida-discord-bot", env: NODE_ENV },
  redact: { paths: REDACT_PATHS, censor: "[REDACTED]" },
  timestamp: pino.stdTimeFunctions.isoTime,
  transport:
    NODE_ENV === "development"
      ? {
          target: "pino-pretty",
          options: { colorize: true, translateTime: "SYS:standard", ignore: "pid,hostname" },
        }
      : undefined,
});

export type Logger = typeof logger;
