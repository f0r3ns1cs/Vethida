#!/usr/bin/env node
// Fails if SecurityConfig still has placeholder keys/kids/CN/pins, or if the
// release exe or staged payload looks like a dev/debug build.
//   node scripts/release/check-release-ready.mjs

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const repoRoot = path.resolve(
  path.dirname(fileURLToPath(import.meta.url)),
  "..",
  "..",
);

const RELEASE_EXE = path.join(
  repoRoot,
  "Vethida",
  "build",
  "x64_Release",
  "vethida.exe",
);

// written by package.mjs
const PAYLOAD_DIR = path.join(repoRoot, "dist", "payload");

// minimum, package.mjs works out the full list with dumpbin. Easy to miss since
// the build machine always has the runtime installed.
const CRT_FILES = ["msvcp140.dll", "vcruntime140.dll"];

// any of these in the payload = a Debug build got staged
const DEBUG_CRT = /^(?:msvcp\d+d|vcruntime\d+d|concrt\d+d|ucrtbased)\w*\.dll$/i;

const CONFIG_H = path.join(
  repoRoot,
  "Vethida",
  "vth_core",
  "Services",
  "SecurityConfig.h",
);
const CONFIG_CPP = path.join(
  repoRoot,
  "Vethida",
  "vth_core",
  "Services",
  "SecurityConfig.cpp",
);

function read(file) {
  try {
    return fs.readFileSync(file, "utf8");
  } catch {
    return null;
  }
}

const header = read(CONFIG_H);
if (header === null) {
  console.error(`Cannot read ${CONFIG_H}`);
  process.exit(2);
}
const impl = read(CONFIG_CPP) ?? "";
const both = `${header}\n${impl}`;

// Sentry isn't here on purpose, shipping without it is fine
const checks = [
  {
    name: "License token key ids",
    bad: /LicenseToken(Primary|Backup)Kid\s*=\s*"[^"]*PLACEHOLDER/,
    hint: "Populate from the release keypair; drop the -PLACEHOLDER suffix.",
  },
  {
    name: "Update manifest key ids",
    bad: /UpdateManifest(Primary|Backup)Kid\s*=\s*"[^"]*PLACEHOLDER/,
    hint: "Run `node scripts/release/gen-update-keys.mjs` and paste the output.",
  },
  {
    name: "Authenticode publisher CN",
    bad: /UpdatePublisherCommonName\s*=\s*"PLACEHOLDER/,
    hint: "Set to the certificate's subject CN, exactly as issued.",
  },
  {
    // with placeholder pins the client falls back to normal chain validation
    name: "API certificate pins",
    bad: /apiSpkiPins[\s\S]{0,400}?PLACEHOLDER/,
    hint: "Populate with the SHA-256 SPKI hashes of the leaf and a backup certificate.",
  },
  {
    name: "All-zero embedded keys",
    bad: /(0x00\s*,\s*){16,}|QByteArray\s*\(\s*\d+\s*,\s*'\\0'\s*\)/,
    hint: "An embedded public key is still all zeroes.",
  },
];

// self-test the patterns so they can't silently stop matching
const selfTest = [
  {
    name: "License token key ids",
    matches: ['LicenseTokenPrimaryKid = "vethida-license-v1-PLACEHOLDER";'],
    ignores: ['LicenseTokenPrimaryKid = "vethida-license-2026-08";'],
  },
  {
    name: "Update manifest key ids",
    matches: ['UpdateManifestBackupKid = "PLACEHOLDER";'],
    ignores: ['UpdateManifestBackupKid = "vethida-update-2026-08";'],
  },
  {
    name: "Authenticode publisher CN",
    matches: ['UpdatePublisherCommonName = "PLACEHOLDER";'],
    ignores: ['UpdatePublisherCommonName = "Rens Engineering";'],
  },
  {
    name: "API certificate pins",
    matches: [
      'QStringList SecurityConfig::apiSpkiPins()\n{\n  return { "0000PLACEHOLDER0000" };\n}',
    ],
    ignores: [
      'QStringList SecurityConfig::apiSpkiPins()\n{\n  return { "3b6a27bcceb6a42d62a3a8d02a6f0d73' +
        '653215771de243a63ac048a18b59da29" };\n}',
      // PLACEHOLDER further down the file shouldn't count
      'QStringList SecurityConfig::apiSpkiPins()\n{\n  return { "abc" };\n}\n' +
        '// '.padEnd(420, 'x') +
        '\nstatic const char kOther[] = "PLACEHOLDER";',
    ],
  },
  {
    name: "All-zero embedded keys",
    matches: [
      "return QByteArray(32, '\\0');",
      "{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }",
    ],
    ignores: [
      "return QByteArray::fromHex(\"3b6a27bcceb6a42d62a3a8d02a6f0d73\");",
      "return QByteArray(32, '\\xff');",
    ],
  },
];

for (const t of selfTest) {
  const check = checks.find((c) => c.name === t.name);
  if (!check) {
    console.error(`self-test: no check named "${t.name}" - rename or remove it.`);
    process.exit(2);
  }
  for (const s of t.matches) {
    if (!check.bad.test(s)) {
      console.error(
        `self-test FAILED: "${t.name}" no longer matches placeholder material.\n` +
          `  sample: ${s}\n` +
          `  This check would report a false all-clear. Refusing to run.`,
      );
      process.exit(2);
    }
  }
  for (const s of t.ignores) {
    if (check.bad.test(s)) {
      console.error(
        `self-test FAILED: "${t.name}" matches real material and would block a good build.\n` +
          `  sample: ${s}\n  Refusing to run.`,
      );
      process.exit(2);
    }
  }
}

// Title bar / startup log strings that only exist under VETHIDA_DEV_BUILD.
// Searched as both UTF-16LE and ASCII. Must match the app's strings exactly.
const DEV_MARKERS = ["Dev build", "Development build"];

// only in builds with VETHIDA_TIER_OVERRIDE (unlocks every tier from a file) -
// most important check here
const OVERRIDE_MARKER = "TIER OVERRIDE ACTIVE";

// left in the repo root while the override is enabled
const OVERRIDE_NOTE = path.join(repoRoot, "DEBUG-TIER-OVERRIDE.md");

function markersIn(buffer, markers) {
  const found = [];
  for (const marker of markers) {
    const utf16 = Buffer.from(marker, "utf16le");
    const ascii = Buffer.from(marker, "latin1");
    if (buffer.includes(utf16) || buffer.includes(ascii)) found.push(marker);
  }
  return found;
}

function devMarkersIn(buffer) {
  return markersIn(buffer, DEV_MARKERS);
}

// self-test the marker scan
{
  const clean = Buffer.concat([
    Buffer.alloc(64),
    Buffer.from("Vethida 1.0.0", "utf16le"),
    Buffer.alloc(64),
  ]);
  if (devMarkersIn(clean).length !== 0) {
    console.error(
      "self-test FAILED: dev-marker scan fires on a clean binary and would " +
        "block a good release. Refusing to run.",
    );
    process.exit(2);
  }
  for (const marker of DEV_MARKERS) {
    for (const encoding of ["utf16le", "latin1"]) {
      const dirty = Buffer.concat([
        Buffer.alloc(64),
        Buffer.from(marker, encoding),
        Buffer.alloc(64),
      ]);
      if (!devMarkersIn(dirty).includes(marker)) {
        console.error(
          `self-test FAILED: dev-marker scan misses "${marker}" as ${encoding}.\n` +
            `  This check would report a false all-clear. Refusing to run.`,
        );
        process.exit(2);
      }
    }
  }
}

// self-test DEBUG_CRT both ways
{
  const mustMatch = [
    "msvcp140d.dll",
    "vcruntime140d.dll",
    "concrt140d.dll",
    "ucrtbased.dll",
    "MSVCP140D.DLL",
  ];
  const mustNotMatch = [
    ...CRT_FILES,
    "msvcp140_1.dll",
    "msvcp140_atomic_wait.dll",
    "Qt6Core.dll",
    "Vethida.exe",
  ];
  for (const name of mustMatch) {
    if (!DEBUG_CRT.test(name)) {
      console.error(
        `self-test FAILED: debug-CRT pattern misses "${name}".\n` +
          `  This check would report a false all-clear. Refusing to run.`,
      );
      process.exit(2);
    }
  }
  for (const name of mustNotMatch) {
    if (DEBUG_CRT.test(name)) {
      console.error(
        `self-test FAILED: debug-CRT pattern matches "${name}", which is correct\n` +
          `  to ship. This check would fail every clean release. Refusing to run.`,
      );
      process.exit(2);
    }
  }
}

let failed = 0;
for (const c of checks) {
  const hit = c.bad.test(both);
  console.log(`${hit ? "FAIL" : "ok  "}  ${c.name}`);
  if (hit) {
    console.log(`        ${c.hint}`);
    failed += 1;
  }
}

// build-all runs this before and after the build, so the exe might not exist yet
const releaseExe = (() => {
  try {
    return fs.readFileSync(RELEASE_EXE);
  } catch {
    return null;
  }
})();

if (releaseExe === null) {
  console.log(`note  no release binary at ${RELEASE_EXE} - dev-build scan not run.`);
} else {
  const markers = devMarkersIn(releaseExe);
  console.log(`${markers.length ? "FAIL" : "ok  "}  Development build markers in vethida.exe`);
  if (markers.length) {
    console.log(`        found: ${markers.join(", ")}`);
    console.log(
      "        This binary was not built from the Release configuration. It " +
        "unlocks\n        every tier without a license. Rebuild Release and do " +
        "not publish this one.",
    );
    failed += 1;
  }
}

// tier override: note, props macro and binary are each checked separately
{
  const noteExists = fs.existsSync(OVERRIDE_NOTE);
  console.log(`${noteExists ? "FAIL" : "ok  "}  No debug tier override note in the repository root`);
  if (noteExists) {
    console.log(`        ${OVERRIDE_NOTE} exists.`);
    console.log(
      "        That note is only written while the tier override is in the tree.\n" +
        "        Follow it, take the override out, then delete the note.",
    );
    failed += 1;
  }

  const props = read(path.join(repoRoot, "Vethida", "common.props")) ?? "";
  const macroSet = /VETHIDA_TIER_OVERRIDE/.test(props);
  console.log(`${macroSet ? "FAIL" : "ok  "}  VETHIDA_TIER_OVERRIDE not defined in common.props`);
  if (macroSet) {
    console.log(
      "        Every build from this sheet unlocks every tier from a file on\n" +
        "        disk. Remove the definition.",
    );
    failed += 1;
  }

  if (releaseExe !== null) {
    const hit = markersIn(releaseExe, [OVERRIDE_MARKER]);
    console.log(`${hit.length ? "FAIL" : "ok  "}  No tier override compiled into vethida.exe`);
    if (hit.length) {
      console.log(
        "        This binary unlocks Pro and Ultimate for anyone who drops a text\n" +
          "        file beside it. It must not be signed, published or kept.",
      );
      failed += 1;
    }
  }
}

// CRT check runs on the payload since that's what actually ships
const payloadFiles = (() => {
  try {
    return fs.readdirSync(PAYLOAD_DIR);
  } catch {
    return null;
  }
})();

if (payloadFiles === null) {
  console.log(`note  no staged payload at ${PAYLOAD_DIR} - runtime check not run.`);
} else {
  const present = new Set(payloadFiles.map((f) => f.toLowerCase()));
  const missing = CRT_FILES.filter((f) => !present.has(f.toLowerCase()));
  console.log(`${missing.length ? "FAIL" : "ok  "}  Visual C++ runtime beside Vethida.exe`);
  if (missing.length) {
    console.log(`        missing: ${missing.join(", ")}`);
    console.log(
      "        A clean Windows install cannot start this build. The loader fails\n" +
        "        with a dialog naming the DLL before any Vethida code runs.",
    );
    failed += 1;
  }

  const debug = payloadFiles.filter((f) => DEBUG_CRT.test(f));
  console.log(`${debug.length ? "FAIL" : "ok  "}  No debug CRT in the payload`);
  if (debug.length) {
    console.log(`        found: ${debug.join(", ")}`);
    console.log(
      "        Debug CRT libraries are not redistributable and the target machine\n" +
        "        will not have the debug UCRT they need. A Debug build was staged.",
    );
    failed += 1;
  }
}

// just a note
if (/SentryDsn\s*=\s*"[^"]*PLACEHOLDER/.test(header)) {
  console.log("note  Sentry DSN unset - desktop telemetry stays a local no-op.");
}

if (failed > 0) {
  console.error(
    `\n${failed} blocking item(s). This build must not be published.`,
  );
  process.exitCode = 1;
} else {
  console.log("\nSecurity material populated.");
}
