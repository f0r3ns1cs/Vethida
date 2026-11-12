// Writes scripts/eula.txt (the installer's license page) from legal/eula.md.
//   node scripts/make-eula.mjs           write it
//   node scripts/make-eula.mjs --check   exit 1 if it's out of date

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const src = path.join(root, "legal", "eula.md");
const out = path.join(root, "scripts", "eula.txt");
const check = process.argv.includes("--check");

// token values come from the same place the website uses
const site = fs.readFileSync(path.join(root, "vethida.com", "lib", "site.ts"), "utf8");
function siteValue(key) {
  const m = site.match(new RegExp(`\\b${key}:\\s*"([^"]+)"`));
  if (!m) {
    console.error(`make-eula: ${key} not found in vethida.com/lib/site.ts`);
    process.exit(1);
  }
  return m[1];
}
const tokens = {
  entity: siteValue("entity"),
  jurisdiction: siteValue("jurisdiction"),
  effectiveDate: siteValue("effectiveDate"),
  "email.support": siteValue("support"),
  "email.privacy": siteValue("privacy"),
  "email.security": siteValue("security"),
  "email.legal": siteValue("legal"),
};

let md = fs.readFileSync(src, "utf8").replace(/\r\n/g, "\n");
let title = "End User License Agreement";
if (md.startsWith("---\n")) {
  const end = md.indexOf("\n---\n", 4);
  const front = md.slice(4, end);
  const t = front.match(/^title:\s*"?(.+?)"?$/m);
  if (t) title = t[1];
  md = md.slice(end + 5);
}

md = md.replace(/\{\{([a-zA-Z.]+)\}\}/g, (whole, key) => tokens[key] ?? whole);

const unknown = md.match(/\{\{[a-zA-Z.]+\}\}/);
if (unknown) {
  console.error(`make-eula: unknown token ${unknown[0]} in legal/eula.md`);
  process.exit(1);
}

const body = md
  .split("\n")
  .map((line) =>
    line
      .replace(/^#{1,6}\s+(.*)$/, (_, h) => h.toUpperCase())
      // same as proseParagraph() in check-integrity.mjs
      .replace(/\[([^\]]+)\]\([^)]*\)/g, "$1")
      .replace(/\*\*/g, ""),
  )
  .join("\n")
  .replace(/\n{3,}/g, "\n\n")
  .trim();

const text = `VETHIDA ${title.toUpperCase()}\nEffective ${tokens.effectiveDate}\n\n${body}\n`.replace(/\n/g, "\r\n");

if (check) {
  const current = fs.existsSync(out) ? fs.readFileSync(out, "utf8") : "";
  if (current !== text) {
    console.error("make-eula: scripts/eula.txt is out of date, run node scripts/make-eula.mjs");
    process.exit(1);
  }
  console.log("make-eula: up to date");
} else {
  fs.writeFileSync(out, text);
  console.log(`make-eula: wrote ${path.relative(root, out)}`);
}
