#!/usr/bin/env node
// Submits the packed and unpacked installers to VirusTotal and appends the
// result to release-history/<version>/av-submissions.json.
//
//   VT_API_KEY=... node scripts/release/virustotal-submit.mjs \
//       --packed dist/VethidaSetup.exe --unpacked dist/VethidaSetup-unpacked.exe
//
//   --packed     the UPX-packed installer that ships
//   --unpacked   the same build without packing
//   --wait       seconds to wait for analysis (default 180)
//   --no-write   print the result, write nothing
// Both builds so we can see how many detections UPX adds.

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { createHash } from "node:crypto";

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");

function arg(name, fallback) {
  const i = process.argv.indexOf(name);
  return i !== -1 ? process.argv[i + 1] : fallback;
}
const noWrite = process.argv.includes("--no-write");
const packed = arg("--packed");
const unpacked = arg("--unpacked");
const waitSeconds = Number(arg("--wait", "180"));
const apiKey = process.env.VT_API_KEY;

function fail(message) {
  console.error(`\nvirustotal-submit: ${message}\n`);
  process.exit(1);
}

if (!apiKey) {
  fail(
    "VT_API_KEY is not set.\n\n" +
      "  Get one from your VirusTotal profile (free tier is enough).",
  );
}
if (!packed || !unpacked) {
  fail(
    "--packed and --unpacked are both required.",
  );
}
for (const f of [packed, unpacked]) {
  if (!fs.existsSync(f)) fail(`file not found: ${f}`);
}

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

async function vt(pathname, init = {}) {
  const res = await fetch(`https://www.virustotal.com/api/v3${pathname}`, {
    ...init,
    headers: { "x-apikey": apiKey, ...(init.headers ?? {}) },
  });
  if (res.status === 429) {
    fail("VirusTotal rate limit hit. Wait a minute and run again.");
  }
  if (!res.ok && res.status !== 404) {
    fail(`VirusTotal returned ${res.status} for ${pathname}`);
  }
  return { status: res.status, body: res.status === 404 ? null : await res.json() };
}

function sha256(file) {
  return createHash("sha256").update(fs.readFileSync(file)).digest("hex");
}

// check the hash first, no point re-uploading a known file
async function analyze(file, label) {
  const digest = sha256(file);
  const size = fs.statSync(file).size;
  console.log(`\n${label}: ${path.basename(file)}`);
  console.log(`  sha256 ${digest}`);
  console.log(`  ${(size / 1_000_000).toFixed(1)} MB`);

  let report = await vt(`/files/${digest}`);

  if (report.status === 404) {
    console.log("  not seen before, uploading ...");
    const form = new FormData();
    form.append("file", new Blob([fs.readFileSync(file)]), path.basename(file));
    const up = await vt("/files", { method: "POST", body: form });
    const id = up.body?.data?.id;
    if (!id) fail("upload accepted but no analysis id was returned.");

    const deadline = Date.now() + waitSeconds * 1000;
    for (;;) {
      await sleep(15_000);
      const a = await vt(`/analyses/${id}`);
      const state = a.body?.data?.attributes?.status;
      if (state === "completed") break;
      if (Date.now() > deadline) {
        fail(
          `analysis still ${state} after ${waitSeconds}s. Re-run later; the ` +
            `hash is now known so it will return immediately.`,
        );
      }
      console.log(`  ${state} ...`);
    }
    report = await vt(`/files/${digest}`);
  }

  const stats = report.body?.data?.attributes?.last_analysis_stats ?? {};
  const results = report.body?.data?.attributes?.last_analysis_results ?? {};
  const flagged = Object.entries(results)
    .filter(([, r]) => r.category === "malicious" || r.category === "suspicious")
    .map(([vendor, r]) => `${vendor} (${r.result ?? r.category})`)
    .sort();

  console.log(`  ${flagged.length} detection(s) of ${stats.malicious + stats.undetected + stats.harmless || "?"} engines`);
  for (const f of flagged) console.log(`    ${f}`);

  return { file, digest, size, flagged, stats };
}

const packedResult = await analyze(packed, "packed");
const unpackedResult = await analyze(unpacked, "unpacked");

const delta = packedResult.flagged.length - unpackedResult.flagged.length;
const onlyPacked = packedResult.flagged.filter(
  (v) => !unpackedResult.flagged.includes(v),
);

console.log(
  `\ndelta: packing costs ${delta >= 0 ? "+" : ""}${delta} detection(s)`,
);

if (delta > 2) {
  console.log(`\nUPX adds more than 2 detections, consider shipping unpacked.`);
}

if (noWrite) process.exit(0);

const version = fs.readFileSync(path.join(repoRoot, "VERSION"), "utf8").trim();
const now = new Date().toISOString();
const dir = path.join(repoRoot, "release-history", version);
const recordPath = path.join(dir, "av-submissions.json");

const rows = [packedResult, unpackedResult].map((r, i) => ({
  version,
  artifact: path.basename(r.file),
  sha256: r.digest,
  vendor: "VirusTotal",
  submission_url: `https://www.virustotal.com/gui/file/${r.digest}`,
  submitted_at: now,
  case_id: "",
  status: "submitted",
  response_at: now,
  result: `${r.flagged.length} detection(s)`,
  notes:
    (i === 0
      ? `packed; delta vs unpacked ${delta >= 0 ? "+" : ""}${delta}` +
        (onlyPacked.length > 0 ? `; only when packed: ${onlyPacked.join(", ")}` : "")
      : "unpacked") + (r.flagged.length > 0 ? `; flagged by ${r.flagged.join(", ")}` : ""),
}));

let record = [];
try {
  record = JSON.parse(fs.readFileSync(recordPath, "utf8"));
} catch {
  fs.mkdirSync(dir, { recursive: true });
}

fs.writeFileSync(recordPath, `${JSON.stringify([...record, ...rows], null, 2)}\n`, "utf8");
console.log(`\nRecorded in release-history/${version}/av-submissions.json`);
