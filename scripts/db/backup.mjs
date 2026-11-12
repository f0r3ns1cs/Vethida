#!/usr/bin/env node
// pg_dump (custom format) of the database, checked and pruned.
//   DATABASE_URL=... node scripts/db/backup.mjs [--out <dir>] [--keep <n>]
//   --out   default ./backups
//   --keep  default 14
// Test restores with scripts/db/verify-restore.mjs.

import fs from "node:fs";
import path from "node:path";
import { spawnSync } from "node:child_process";
import { createHash } from "node:crypto";

function arg(name, fallback) {
  const i = process.argv.indexOf(name);
  return i !== -1 ? process.argv[i + 1] : fallback;
}
const outDir = path.resolve(arg("--out", "backups"));
const keep = Number(arg("--keep", "14"));
const url = process.env.DATABASE_URL;

function fail(message) {
  console.error(`\nbackup: ${message}\n`);
  process.exit(1);
}

if (!url) {
  fail(
    "DATABASE_URL is not set.\n\n" +
      "  Set it explicitly so cron doesn't back up the wrong database.",
  );
}
if (!Number.isInteger(keep) || keep < 1) {
  fail("--keep must be a positive integer.");
}

// client major version needs to be >= server
const which = spawnSync("pg_dump", ["--version"], { encoding: "utf8" });
if (which.error) {
  fail(
    "pg_dump not found on PATH.\n\n" +
      "  Install the PostgreSQL client tools. On the VPS this is postgresql-client;\n" +
      "  its major version must be >= the server's.",
  );
}
console.log(`Using ${which.stdout.trim()}`);

fs.mkdirSync(outDir, { recursive: true });

const stamp = new Date().toISOString().replace(/[:.]/g, "-").slice(0, 19);
const outFile = path.join(outDir, `vethida-${stamp}.dump`);

console.log(`Dumping to ${outFile} ...`);
const dump = spawnSync(
  "pg_dump",
  [
    "--format=custom",
    "--compress=9",
    "--no-owner", // otherwise restoring into a scratch db fails on ownership
    "--no-privileges",
    "--file",
    outFile,
    url,
  ],
  { stdio: ["ignore", "inherit", "inherit"] },
);

if (dump.status !== 0) {
  fs.rmSync(outFile, { force: true });
  fail(`pg_dump exited ${dump.status}. No backup was written.`);
}

const size = fs.statSync(outFile).size;
if (size < 1024) {
  fs.rmSync(outFile, { force: true });
  fail(
    `dump is only ${size} bytes, deleted it.`,
  );
}

// cheap integrity check, fails on truncated/corrupt archives
const listed = spawnSync("pg_restore", ["--list", outFile], { encoding: "utf8" });
if (listed.status !== 0) {
  fs.rmSync(outFile, { force: true });
  fail("pg_restore could not read the dump back, deleted it.");
}
const tableCount = (listed.stdout.match(/ TABLE DATA /g) ?? []).length;
if (tableCount === 0) {
  fs.rmSync(outFile, { force: true });
  fail(
    "the dump contains no table data (empty db or failed dump).",
  );
}

const digest = createHash("sha256")
  .update(fs.readFileSync(outFile))
  .digest("hex");
fs.writeFileSync(`${outFile}.sha256`, `${digest}  ${path.basename(outFile)}\n`);

console.log(
  `\nbackup: ${(size / 1_000_000).toFixed(1)} MB, ${tableCount} table(s) with data` +
    `\n  sha256 ${digest}`,
);

// retention - names sort by date
const dumps = fs
  .readdirSync(outDir)
  .filter((f) => /^vethida-.*\.dump$/.test(f))
  .sort();
const excess = dumps.slice(0, Math.max(0, dumps.length - keep));
for (const f of excess) {
  fs.rmSync(path.join(outDir, f), { force: true });
  fs.rmSync(path.join(outDir, `${f}.sha256`), { force: true });
  console.log(`  pruned ${f}`);
}
console.log(`  ${Math.min(dumps.length, keep)} of ${keep} retained`);

console.log(
  `\nThis dump has NOT been restored. Run` +
    `\n  node scripts/db/verify-restore.mjs ${outFile}` +
    `\nagainst a scratch database before relying on it.`,
);
