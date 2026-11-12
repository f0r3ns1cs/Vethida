import fs from "node:fs";
import path from "node:path";
import { defineConfig } from "drizzle-kit";

// drizzle-kit doesn't load .env on its own. Exported vars still win.
function loadDotEnv(file: string): void {
  let raw: string;
  try {
    raw = fs.readFileSync(file, "utf8");
  } catch {
    return;
  }
  for (const line of raw.split(/\r?\n/)) {
    const match = /^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*)$/.exec(line);
    if (!match) continue;
    const [, key, rest] = match;
    if (process.env[key] !== undefined) continue;
    const value = rest.trim().replace(/^(['"])([\s\S]*)\1$/, "$2");
    process.env[key] = value;
  }
}

loadDotEnv(path.join(process.cwd(), ".env"));

export default defineConfig({
  dialect: "postgresql",
  schema: "./schema.ts",
  out: "./migrations",
  dbCredentials: {
    url: process.env.DATABASE_URL ?? "", // not needed for generate
  },
});
