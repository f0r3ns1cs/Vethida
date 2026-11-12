#!/usr/bin/env node
// Uploads the installer (and source offer) to R2 and prints the stamp-update-feed
// command. Stamping the feed is left manual since that's what makes it go live.
//   node scripts/release/publish.mjs --installer dist/VethidaSetup.exe \
//       [--source-offer dist/source-offer] [--dry-run]
// env: R2_ACCOUNT_ID, R2_ACCESS_KEY_ID, R2_SECRET_ACCESS_KEY, R2_BUCKET, R2_PUBLIC_BASE

import fs from "node:fs";
import path from "node:path";
import { createHash } from "node:crypto";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");

function arg(name, fallback) {
  const i = process.argv.indexOf(name);
  return i !== -1 ? process.argv[i + 1] : fallback;
}
const dryRun = process.argv.includes("--dry-run");
const installer = arg("--installer");
const sourceOffer = arg("--source-offer");

function fail(message) {
  console.error(`\npublish: ${message}\n`);
  process.exit(1);
}

if (!installer) fail("--installer is required.");
if (!fs.existsSync(installer)) fail(`installer not found: ${installer}`);

const version = fs.readFileSync(path.join(repoRoot, "VERSION"), "utf8").trim();

// unsigned = SmartScreen warnings, and the updater rejects it (publisher mismatch)
if (process.platform === "win32" && !dryRun) {
  const verify = spawnSync(
    process.execPath,
    [path.join(repoRoot, "scripts/release/sign.mjs"), "--verify", installer],
    { encoding: "utf8" },
  );
  if (verify.status !== 0) {
    fail(
      `${installer} is not validly signed.\n\n` +
        `  Run scripts/release/sign.mjs first.`,
    );
  }
  console.log("Signature verified.");
} else if (!dryRun) {
  console.warn(
    "Not on Windows: signature could NOT be verified before upload.\n" +
      "  Verify on the build machine before making this URL public.",
  );
}

const bytes = fs.readFileSync(installer);
const sha256 = createHash("sha256").update(bytes).digest("hex");
const size = bytes.length;
const key = `desktop/${version}/${path.basename(installer)}`;

console.log(`\nVethida ${version}`);
console.log(`  ${path.basename(installer)}`);
console.log(`  ${(size / 1_000_000).toFixed(1)} MB`);
console.log(`  sha256 ${sha256}`);
console.log(`  key    ${key}`);

const required = [
  "R2_ACCOUNT_ID",
  "R2_ACCESS_KEY_ID",
  "R2_SECRET_ACCESS_KEY",
  "R2_BUCKET",
  "R2_PUBLIC_BASE",
];
const missing = required.filter((n) => !process.env[n]);

if (dryRun) {
  console.log("\n--dry-run: nothing uploaded.");
} else {
  if (missing.length > 0) {
    fail(
      `missing object-storage credentials:\n${missing.map((n) => `  ${n}`).join("\n")}\n\n` +
        `  Get them from the Cloudflare dashboard (R2).`,
    );
  }

  // rclone so there's nothing to npm install on the build machine
  const rclone = spawnSync("rclone", ["version"], { encoding: "utf8" });
  if (rclone.error) {
    fail(
      "rclone not found on PATH.\n\n" +
        "  Install rclone and configure an S3-compatible remote for Cloudflare R2,\n" +
        "  or upload the file by hand and stamp the feed with the values above.",
    );
  }

  const uploads = [[installer, key]];
  if (sourceOffer) {
    if (!fs.existsSync(sourceOffer)) fail(`source offer not found: ${sourceOffer}`);
    uploads.push([sourceOffer, `qt-source/${version}/`]);
  }

  for (const [from, to] of uploads) {
    console.log(`\nUploading ${from} -> ${to} ...`);
    const isDir = fs.statSync(from).isDirectory();
    const r = spawnSync(
      "rclone",
      [isDir ? "copy" : "copyto", from, `r2:${process.env.R2_BUCKET}/${to}`, "--progress"],
      { stdio: "inherit" },
    );
    if (r.status !== 0) fail(`rclone exited ${r.status} uploading ${from}.`);
  }
}

const publicUrl = `${(process.env.R2_PUBLIC_BASE ?? "https://<R2_PUBLIC_BASE>").replace(/\/$/, "")}/${key}`;

console.log(`
publish: ${dryRun ? "dry run complete" : "upload complete"}.

  URL   ${publicUrl}

Not yet advertised to anyone. To make this update live, stamp the feed with the
release manifest key:

  node scripts/release/stamp-update-feed.mjs \\
    --channel stable \\
    --version ${version} \\
    --artifact ${installer} \\
    --download-url ${publicUrl} \\
    --sign-key <path to update-manifest-p256.pem>

--artifact has to be the same signed file uploaded above (it gets hashed again).
Without --sign-key release builds will refuse the update.

Then rebuild and deploy vethida.com, it reads the feed at build time.

Set NEXT_PUBLIC_DOWNLOAD_URL on my.vethida.com to ${publicUrl} and redeploy it,
or the download center goes on telling customers the installer is not published.
`);
