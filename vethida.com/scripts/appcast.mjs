#!/usr/bin/env node
// public/desktop/appcast.xml, the WinSparkle update feed (AutoUpdate.h in the app).
//   node scripts/appcast.mjs keys
//       new Ed25519 key pair, private key -> ~/.vethida/appcast-ed25519.pem
//       (back it up, never commit it), public key goes in kEdDsaPublicKey
//   node scripts/appcast.mjs add --installer <exe> --url <url> [--version x.y.z] [--key <pem>]
//       signs the installer and adds it at the top
//   node scripts/appcast.mjs --check

import crypto from "node:crypto";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const appcastPath = path.join(here, "..", "public", "desktop", "appcast.xml");
const repoRoot = path.join(here, "..", "..");
const defaultKey = path.join(os.homedir(), ".vethida", "appcast-ed25519.pem");

// silent install, the installer reopens Vethida itself
const INSTALLER_ARGS = "/SILENT /SUPPRESSMSGBOXES /NORESTART /SP-";

function arg(name) {
  const i = process.argv.indexOf(`--${name}`);
  return i > 0 ? process.argv[i + 1] : undefined;
}

function fail(message) {
  console.error(`appcast: ${message}`);
  process.exit(1);
}

function escapeXml(s) {
  return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
}

function emptyAppcast() {
  return `<?xml version="1.0" encoding="utf-8"?>
<rss version="2.0" xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle" xmlns:vethida="https://vethida.com/xmlns/appcast">
  <channel>
    <title>Vethida</title>
    <link>https://vethida.com/desktop/appcast.xml</link>
    <description>Vethida updates</description>
    <language>en</language>
  </channel>
</rss>
`;
}

export function parseItems(xml) {
  return [...xml.matchAll(/<item>([\s\S]*?)<\/item>/g)].map((m) => {
    const item = m[1];
    const attr = (name) => new RegExp(`${name}="([^"]*)"`).exec(item)?.[1] ?? null;
    return {
      version: attr("sparkle:version"),
      url: attr("url"),
      length: Number(attr("length")) || null,
      signature: attr("sparkle:edSignature"),
      sha256: /<vethida:sha256>([0-9a-f]{64})<\/vethida:sha256>/.exec(item)?.[1] ?? null,
      pubDate: /<pubDate>([^<]+)<\/pubDate>/.exec(item)?.[1] ?? null,
    };
  });
}

function keys() {
  if (fs.existsSync(defaultKey)) fail(`${defaultKey} already exists. Move it aside first.`);
  const { publicKey, privateKey } = crypto.generateKeyPairSync("ed25519");
  fs.mkdirSync(path.dirname(defaultKey), { recursive: true });
  fs.writeFileSync(defaultKey, privateKey.export({ format: "pem", type: "pkcs8" }), { mode: 0o600 });
  const raw = publicKey.export({ format: "der", type: "spki" }).subarray(-32);
  console.log(`Private key written to ${defaultKey}. Back it up somewhere safe.`);
  console.log("Public key, for kEdDsaPublicKey in Vethida/vth_core/Runtime/AutoUpdate.h:");
  console.log(raw.toString("base64"));
}

function add() {
  const installer = arg("installer");
  const url = arg("url");
  if (!installer || !url) fail("add needs --installer and --url");
  if (!url.startsWith("https://")) fail("--url must be https");
  const version =
    arg("version") ?? fs.readFileSync(path.join(repoRoot, "VERSION"), "utf8").trim();
  const keyPath = arg("key") ?? defaultKey;
  if (!fs.existsSync(keyPath)) fail(`no private key at ${keyPath}; run 'keys' first`);

  const bytes = fs.readFileSync(installer);
  const signature = crypto
    .sign(null, bytes, crypto.createPrivateKey(fs.readFileSync(keyPath)))
    .toString("base64");
  const sha256 = crypto.createHash("sha256").update(bytes).digest("hex");

  const xml = fs.existsSync(appcastPath) ? fs.readFileSync(appcastPath, "utf8") : emptyAppcast();
  if (parseItems(xml).some((i) => i.version === version)) fail(`${version} is already in the appcast`);

  const item = `    <item>
      <title>Vethida ${escapeXml(version)}</title>
      <pubDate>${new Date().toUTCString()}</pubDate>
      <sparkle:releaseNotesLink>https://vethida.com/changelog</sparkle:releaseNotesLink>
      <vethida:sha256>${sha256}</vethida:sha256>
      <enclosure url="${escapeXml(url)}"
                 sparkle:version="${escapeXml(version)}"
                 sparkle:os="windows-x64"
                 sparkle:installerArguments="${INSTALLER_ARGS}"
                 sparkle:edSignature="${signature}"
                 length="${bytes.length}"
                 type="application/octet-stream" />
    </item>
`;
  const updated = xml.replace(/(<language>[^<]*<\/language>\n)/, `$1${item}`);
  fs.writeFileSync(appcastPath, updated);
  console.log(`Added Vethida ${version} to ${path.relative(repoRoot, appcastPath)}. Commit and deploy the site.`);
}

function check() {
  if (!fs.existsSync(appcastPath)) fail(`${appcastPath} does not exist`);
  const xml = fs.readFileSync(appcastPath, "utf8");
  if (!xml.includes("<channel>") || !xml.includes("</rss>")) fail("not an RSS appcast");
  const items = parseItems(xml);
  for (const i of items) {
    if (!i.version || !/^\d+\.\d+\.\d+$/.test(i.version)) fail(`an item has a bad sparkle:version (${i.version})`);
    if (!i.url?.startsWith("https://")) fail(`${i.version}: the download url must be https`);
    if (!i.signature) fail(`${i.version}: missing sparkle:edSignature`);
    if (!i.length) fail(`${i.version}: missing length`);
  }
  console.log(
    items.length === 0
      ? "appcast: valid. No release has been published yet."
      : `appcast: valid. Latest is ${items[0].version}, ${items.length} release(s) listed.`,
  );
}

if (path.resolve(process.argv[1] ?? "") === fileURLToPath(import.meta.url)) {
  const command = process.argv[2];
  if (command === "keys") keys();
  else if (command === "add") add();
  else if (command === "--check" || command === "check") check();
  else fail("usage: appcast.mjs keys | add --installer <exe> --url <url> | --check");
}
