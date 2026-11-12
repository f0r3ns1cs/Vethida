#!/usr/bin/env node
// Writes a release into the desktop update feed and release-history/<version>/.
//   node scripts/release/stamp-update-feed.mjs \
//     --version 1.1.0 \
//     --artifact path/to/VethidaSetup.exe \
//     --download-url https://releases.vethida.com/1.1.0/vethida-setup-1.1.0.exe \
//     [--channel stable] [--notes-url https://vethida.com/changelog] \
//     [--security-fix] [--sign-key path/to/update-manifest-p256.pem] \
//     [--feed vethida.com/public/desktop/updates.json] [--no-ledger]
// Run it on the signed installer, signing changes the hash.
// --sign-key adds manifest_jws (ES256). Release builds won't take an update without it.

import { createHash, createPrivateKey, sign as edSign } from "node:crypto";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..", "..");

function parseArgs(argv) {
  const args = { channel: "stable", ledger: true };
  for (let i = 2; i < argv.length; i++) {
    switch (argv[i]) {
      case "--version": args.version = argv[++i]; break;
      case "--artifact": args.artifact = argv[++i]; break;
      case "--download-url": args.downloadUrl = argv[++i]; break;
      case "--notes-url": args.notesUrl = argv[++i]; break;
      case "--channel": args.channel = argv[++i]; break;
      case "--sign-key": args.signKey = argv[++i]; break;
      case "--feed": args.feed = argv[++i]; break;
      case "--security-fix": args.securityFix = true; break;
      case "--no-ledger": args.ledger = false; break;
      default:
        console.error(`Unknown argument: ${argv[i]}`);
        process.exit(2);
    }
  }
  for (const required of ["version", "artifact", "downloadUrl"]) {
    if (!args[required]) {
      console.error(`Missing --${required.replace(/[A-Z]/g, (c) => "-" + c.toLowerCase())}`);
      process.exit(2);
    }
  }
  if (!/^\d+\.\d+(\.\d+)?$/.test(args.version)) {
    console.error(`--version must be numeric dotted (got "${args.version}")`);
    process.exit(2);
  }
  return args;
}

function b64url(buf) {
  return Buffer.from(buf).toString("base64url");
}

function signManifest(keyPath, claims) {
  const key = createPrivateKey(fs.readFileSync(keyPath));
  if (key.asymmetricKeyType !== "ec") {
    console.error(`--sign-key must be a P-256 private key (got ${key.asymmetricKeyType})`);
    process.exit(2);
  }
  const header = b64url(JSON.stringify({ alg: "ES256", typ: "JWT" }));
  const payload = b64url(JSON.stringify(claims));
  // JWS wants raw r||s, not DER
  const signature = edSign("sha256", Buffer.from(`${header}.${payload}`), {
    key,
    dsaEncoding: "ieee-p1363",
  });
  return `${header}.${payload}.${b64url(signature)}`;
}

const args = parseArgs(process.argv);

const artifact = fs.readFileSync(args.artifact);
const sha256 = createHash("sha256").update(artifact).digest("hex");
const sizeBytes = artifact.length;

// same timestamp for the feed and release.json
const stampedAt = new Date().toISOString();

const update = {
  version: args.version,
  channel: args.channel,
  download_url: args.downloadUrl,
  sha256,
  size_bytes: sizeBytes,
  published_at: stampedAt,
};
if (args.notesUrl) update.notes = args.notesUrl;
if (args.securityFix) update.security_fix = true;

if (args.signKey) {
  const nowSec = Math.floor(Date.now() / 1000);
  update.manifest_jws = signManifest(args.signKey, {
    iss: "vethida-release",
    aud: "vethida-updater",
    iat: nowSec,
    // long so people still on this version keep getting updates
    exp: nowSec + 400 * 24 * 60 * 60,
    version: args.version,
    sha256,
    download_url: args.downloadUrl,
  });
}

const feedPath = args.feed
  ? path.resolve(args.feed)
  : path.join(repoRoot, "vethida.com", "public", "desktop", "updates.json");
const feed = JSON.parse(fs.readFileSync(feedPath, "utf-8"));
feed[args.channel] = { ...(feed[args.channel] ?? {}), update };
fs.writeFileSync(feedPath, JSON.stringify(feed, null, 2) + "\n");

if (args.ledger) {
  const dir = path.join(repoRoot, "release-history", args.version);
  fs.mkdirSync(dir, { recursive: true });
  const releaseRecord = {
    version: args.version,
    channel: args.channel,
    artifact: path.basename(args.artifact),
    sha256,
    size_bytes: sizeBytes,
    download_url: args.downloadUrl,
    release_notes_url: args.notesUrl ?? "",
    security_fix: args.securityFix ?? false,
    released_at: stampedAt,
  };
  fs.writeFileSync(
    path.join(dir, "release.json"),
    JSON.stringify(releaseRecord, null, 2) + "\n",
  );
  const avLedger = path.join(dir, "av-submissions.json");
  if (!fs.existsSync(avLedger)) fs.writeFileSync(avLedger, "[]\n");
}

console.log(`Stamped ${args.channel} ${args.version}`);
console.log(`  sha256: ${sha256}`);
console.log(`  size:   ${sizeBytes} bytes`);
console.log(`  feed:   ${feedPath}`);
console.log(`  signed: ${args.signKey ? "yes (manifest_jws)" : "NO - dev builds only"}`);
