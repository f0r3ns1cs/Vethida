import http from "http";
import type { Client } from "discord.js";
import { HEALTH_HOST, HEALTH_PORT } from "./config";
import { logger } from "./logger";

export function startHealthServer(client: Client): http.Server {
  const server = http.createServer((req, res) => {
    if (!req.url) {
      res.statusCode = 400;
      res.end();
      return;
    }

    const url = new URL(req.url, "http://localhost");

    if (req.method === "GET" && (url.pathname === "/health" || url.pathname === "/healthz")) {
      const ready = client.isReady();
      res.statusCode = ready ? 200 : 503;
      res.setHeader("Content-Type", "application/json");
      res.end(
        JSON.stringify({
          status: ready ? "ok" : "starting",
          uptime: process.uptime(),
          guilds: client.guilds.cache.size,
          ws: client.ws.status,
          ping: client.ws.ping,
        })
      );
      return;
    }

    if (req.method === "GET" && url.pathname === "/metrics") {
      res.statusCode = 200;
      res.setHeader("Content-Type", "text/plain");
      res.end(
        [
          `# HELP vethida_bot_up 1 if the Discord gateway is ready`,
          `# TYPE vethida_bot_up gauge`,
          `vethida_bot_up ${client.isReady() ? 1 : 0}`,
          `# HELP vethida_bot_ping_ms Discord gateway ping in ms`,
          `# TYPE vethida_bot_ping_ms gauge`,
          `vethida_bot_ping_ms ${client.ws.ping}`,
          `# HELP vethida_bot_guilds Number of guilds the bot is in`,
          `# TYPE vethida_bot_guilds gauge`,
          `vethida_bot_guilds ${client.guilds.cache.size}`,
          `# HELP vethida_bot_uptime_seconds Process uptime`,
          `# TYPE vethida_bot_uptime_seconds counter`,
          `vethida_bot_uptime_seconds ${Math.floor(process.uptime())}`,
          "",
        ].join("\n")
      );
      return;
    }

    res.statusCode = 404;
    res.end();
  });

  server.listen(HEALTH_PORT, HEALTH_HOST, () => {
    logger.info({ host: HEALTH_HOST, port: HEALTH_PORT }, "health server listening");
  });

  return server;
}
