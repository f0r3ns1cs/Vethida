#!/usr/bin/env node
// Authenticode signing with signtool. The key stays on the token/HSM, never a file.
//   node scripts/release/sign.mjs <file> [<file> ...]
//   node scripts/release/sign.mjs --verify <file> [<file> ...]
//
// env (signing only):
//   SIGNING_CERT_THUMBPRINT   SHA-1 thumbprint of the cert in the Windows store
//   SIGNING_TIMESTAMP_URL     RFC 3161 TSA, defaults to digicert
//   SIGNING_CSP               optional CSP/KSP name
//   SIGNING_KEY_CONTAINER     key container, required with SIGNING_CSP
//   SIGNTOOL_PATH             optional

import fs from "node:fs";
import path from "node:path";
import { spawnSync } from "node:child_process";

const DEFAULT_TIMESTAMP_URL = "http://timestamp.digicert.com";

const verifyOnly = process.argv.includes("--verify");
const files = process.argv.slice(2).filter((a) => !a.startsWith("--"));

function fail(message) {
  console.error(`\nsign: ${message}\n`);
  process.exit(1);
}

if (files.length === 0) {
  fail("no files given.\n\n  Usage: node scripts/release/sign.mjs <file> [<file> ...]");
}
for (const f of files) {
  if (!fs.existsSync(f)) fail(`file not found: ${f}`);
}

if (process.platform !== "win32") {
  fail("signtool is Windows-only; run this on the build machine.");
}

function findSignTool() {
  if (process.env.SIGNTOOL_PATH) return process.env.SIGNTOOL_PATH;
  const roots = [
    "C:\\Program Files (x86)\\Windows Kits\\10\\bin",
    "C:\\Program Files\\Windows Kits\\10\\bin",
  ];
  const found = [];
  for (const root of roots) {
    if (!fs.existsSync(root)) continue;
    for (const ver of fs.readdirSync(root)) {
      const candidate = path.join(root, ver, "x64", "signtool.exe");
      if (fs.existsSync(candidate)) found.push(candidate);
    }
  }
  return found.sort().pop() ?? null;
}

const signtool = findSignTool();
if (!signtool) {
  fail(
    "signtool.exe not found.\n\n" +
      "  Install the Windows SDK, or set SIGNTOOL_PATH to its full path.",
  );
}

function verify(file) {
  const r = spawnSync(signtool, ["verify", "/pa", "/all", file], {
    encoding: "utf8",
  });
  return { ok: r.status === 0, output: (r.stdout ?? "") + (r.stderr ?? "") };
}

if (verifyOnly) {
  let bad = 0;
  for (const f of files) {
    const r = verify(f);
    console.log(`${r.ok ? "ok    " : "FAILED"}  ${f}`);
    if (!r.ok) {
      console.log(r.output.trim().split("\n").map((l) => `        ${l}`).join("\n"));
      bad++;
    }
  }
  if (bad > 0) fail(`${bad} file(s) failed signature verification.`);
  process.exit(0);
}

// certmgr shows it with spaces
const thumbprint = (process.env.SIGNING_CERT_THUMBPRINT ?? "")
  .replace(/[\s:]/g, "")
  .toUpperCase();

if (!thumbprint) {
  fail(
    "SIGNING_CERT_THUMBPRINT is not set.\n\n" +
      "  It is the SHA-1 thumbprint of the code-signing certificate, which must\n" +
      "  already be present in the Windows certificate store on this machine\n" +
      "  with its private key held by the token or HSM provider.",
  );
}
if (!/^[0-9A-F]{40}$/.test(thumbprint)) {
  fail(
    `SIGNING_CERT_THUMBPRINT is not a SHA-1 thumbprint.\n\n` +
      `  Expected 40 hex characters after stripping spaces and colons; got ` +
      `${thumbprint.length}.`,
  );
}

const csp = process.env.SIGNING_CSP;
const keyContainer = process.env.SIGNING_KEY_CONTAINER;
if (csp && !keyContainer) {
  fail("SIGNING_CSP is set but SIGNING_KEY_CONTAINER is not. Set both or neither.");
}
if (keyContainer && !csp) {
  fail("SIGNING_KEY_CONTAINER is set but SIGNING_CSP is not. Set both or neither.");
}

const timestampUrl = process.env.SIGNING_TIMESTAMP_URL ?? DEFAULT_TIMESTAMP_URL;

const providerArgs = csp ? ["/csp", csp, "/kc", keyContainer] : [];

let failures = 0;
for (const file of files) {
  console.log(`\nSigning ${file} ...`);
  const r = spawnSync(
    signtool,
    [
      "sign",
      "/v",
      "/debug",
      "/sha1", thumbprint, // not /n, renewals leave two certs with the same CN
      "/fd", "SHA256",
      ...providerArgs,
      "/tr", timestampUrl,
      "/td", "SHA256",
      file,
    ],
    { stdio: "inherit" },
  );
  if (r.status !== 0) {
    console.error(`  signtool exited ${r.status}`);
    failures++;
    continue;
  }
  // signtool can exit 0 with a signature Windows won't accept
  const v = verify(file);
  if (!v.ok) {
    console.error(`  signed, but verification failed:\n${v.output}`);
    failures++;
    continue;
  }
  console.log(`  signed and verified`);
}

if (failures > 0) fail(`${failures} file(s) failed.`);

console.log(
  `\nsign: ${files.length} artifact(s) signed and verified.` +
    `\n  timestamp authority: ${timestampUrl}` +
    `\n  key provider:        ${csp ? `${csp} / ${keyContainer}` : "certificate store"}` +
    `\n\nConfirm the certificate's subject CN matches UpdatePublisherCommonName in` +
    `\nVethida/vth_core/Services/SecurityConfig.h. If they differ the updater will` +
    `\nreject our own builds (check-release-ready.mjs catches the placeholder).`,
);
