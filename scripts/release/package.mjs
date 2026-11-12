#!/usr/bin/env node
// Stages the install tree (dist/payload) for make-installer.mjs.
//   node scripts/release/package.mjs [--unpacked] [--require-protection] [--out dist]
//
// dumpbin + windeployqt first (both read the import table, which UPX packs),
// then Themida and UPX on Vethida.exe only. Qt DLLs have to stay untouched for
// LGPL, so they get compared against QTDIR at the end.
// Doesn't sign - do that after, packing would break the signature.

import fs from "node:fs";
import path from "node:path";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");

function arg(name, fallback) {
  const i = process.argv.indexOf(name);
  return i !== -1 ? process.argv[i + 1] : fallback;
}
const unpacked = process.argv.includes("--unpacked");
const requireProtection = process.argv.includes("--require-protection");
const outRoot = path.resolve(repoRoot, arg("--out", "dist"));
const payloadDir = path.join(outRoot, unpacked ? "payload-unpacked" : "payload");

function fail(message) {
  console.error(`\npackage: ${message}\n`);
  process.exit(1);
}

function run(label, cmd, args, opts = {}) {
  console.log(`\n=== ${label} ===`);
  const r = spawnSync(cmd, args, { stdio: "inherit", ...opts });
  if (r.error) fail(`${label}: ${cmd} could not be started (${r.error.message})`);
  if (r.status !== 0) fail(`${label} failed with exit code ${r.status}.`);
}

if (process.platform !== "win32") {
  fail("the desktop payload can only be staged on Windows.");
}

// inputs

const releaseDir = path.join(repoRoot, "Vethida", "build", "x64_Release");
const builtExe = path.join(releaseDir, "vethida.exe");
if (!fs.existsSync(builtExe)) {
  fail(
    `${builtExe} does not exist.\n\n` +
      `  Run scripts/release/build-all.mjs first.`,
  );
}

const qtDir = process.env.QTDIR;
if (!qtDir) {
  fail(
    "QTDIR is not set.\n\n" +
      "  Point it at the Qt kit the build links, for example\n" +
      "  C:/Qt/6.11.1/msvc2022_64.",
  );
}
const windeployqt = path.join(qtDir, "bin", "windeployqt.exe");
if (!fs.existsSync(windeployqt)) fail(`windeployqt not found at ${windeployqt}`);

const upx = path.join(repoRoot, "staging", "upx.exe");
if (!unpacked && !fs.existsSync(upx)) fail(`upx not found at ${upx}`);

// imports. We use Schannel, so OpenSSL showing up here is a mistake

function findDumpbin() {
  const roots = [
    "C:\\Program Files\\Microsoft Visual Studio",
    "C:\\Program Files (x86)\\Microsoft Visual Studio",
  ];
  const found = [];
  for (const root of roots) {
    if (!fs.existsSync(root)) continue;
    for (const edition of fs.readdirSync(root)) {
      const tools = path.join(root, edition);
      if (!fs.statSync(tools).isDirectory()) continue;
      for (const sku of fs.readdirSync(tools)) {
        const msvc = path.join(tools, sku, "VC", "Tools", "MSVC");
        if (!fs.existsSync(msvc)) continue;
        for (const ver of fs.readdirSync(msvc)) {
          const candidate = path.join(msvc, ver, "bin", "Hostx64", "x64", "dumpbin.exe");
          if (fs.existsSync(candidate)) found.push(candidate);
        }
      }
    }
  }
  return found.sort().pop() ?? null;
}

const dumpbin = findDumpbin();
if (!dumpbin) {
  fail(
    "dumpbin.exe not found.\n\n" +
      "  It ships with the MSVC toolset that built this binary. Install the\n" +
      "  C++ workload, or run this from a Visual Studio developer prompt.",
  );
}

function importsOf(file) {
  const r = spawnSync(dumpbin, ["/dependents", file], { encoding: "utf8" });
  if (r.status !== 0) fail(`dumpbin exited ${r.status} over ${file}`);
  return [...r.stdout.matchAll(/^\s{4}(\S+\.dll)\s*$/gim)].map((m) => m[1]);
}

const imported = importsOf(builtExe);
if (imported.length === 0) {
  fail("dumpbin listed no imports, which means this parse no longer matches its output.");
}
const openssl = imported.filter((d) => /^(?:lib)?(?:ssl|crypto)|openssl/i.test(d));
if (openssl.length > 0) {
  fail(
    `OpenSSL linked into vethida.exe: ${openssl.join(", ")} (should be Schannel only)`,
  );
}
console.log(`\n=== dependents ===\n  ${imported.length} imports, no OpenSSL`);

// stage

fs.rmSync(payloadDir, { recursive: true, force: true });
fs.mkdirSync(payloadDir, { recursive: true });

// capital V, InstallEngine and make-installer expect it
const stagedExe = path.join(payloadDir, "Vethida.exe");
fs.copyFileSync(builtExe, stagedExe);

run("windeployqt", windeployqt, [
  "--release",
  "--no-translations",
  "--no-system-d3d-compiler",
  "--no-opengl-sw",
  "--no-quick-import",
  "--compiler-runtime",
  stagedExe,
]);

// AMISCE for the BIOS tab, all three files or none (exe needs both drivers)
const scewinSrc = path.join(repoRoot, "vendors", "ami-scewin");
const scewinFiles = ["SCEWIN_64.exe", "amifldrv64.sys", "amigendrv64.sys"];
if (fs.existsSync(path.join(scewinSrc, scewinFiles[0]))) {
  const scewinOut = path.join(payloadDir, "tools", "ami-scewin");
  fs.mkdirSync(scewinOut, { recursive: true });
  for (const f of scewinFiles) {
    const from = path.join(scewinSrc, f);
    if (!fs.existsSync(from)) fail(`vendors/ami-scewin is incomplete: ${f} is missing.`);
    fs.copyFileSync(from, path.join(scewinOut, f));
  }
  console.log(`\n=== scewin ===\n  ${scewinFiles.length} files staged`);
} else {
  console.log(
    `\n=== scewin ===\n  ${scewinSrc} is not populated; the BIOS tab will fall` +
      `\n  back to a folder the customer picks.`,
  );
}

// sentry.dll is loaded with QLibrary so windeployqt won't pick it up.
// Fails if the DSN is set but the dll is missing.
const sentrySrc = path.join(repoRoot, "vendors", "sentry-native");
const sentryFiles = ["sentry.dll", "crashpad_handler.exe"];
const dsnPopulated = !/SentryDsn\s*=\s*"[^"]*PLACEHOLDER/.test(
  fs.readFileSync(
    path.join(repoRoot, "Vethida", "vth_core", "Services", "SecurityConfig.h"),
    "utf8",
  ),
);

if (fs.existsSync(path.join(sentrySrc, sentryFiles[0]))) {
  for (const f of sentryFiles) {
    const from = path.join(sentrySrc, f);
    if (!fs.existsSync(from)) fail(`vendors/sentry-native is incomplete: ${f} is missing.`);
    fs.copyFileSync(from, path.join(payloadDir, f));
  }
  console.log(`\n=== sentry ===\n  ${sentryFiles.length} files staged`);
} else if (dsnPopulated) {
  fail(
    `SecurityConfig::SentryDsn is populated but ${sentrySrc} is not.\n\n` +
      `  Add ${sentryFiles.join(" and ")} from the sentry-native Windows release,\n` +
      `  or put the DSN back to its placeholder.`,
  );
} else {
  console.log(
    `\n=== sentry ===\n  ${sentrySrc} is not populated and the DSN is still a` +
      `\n  placeholder; crash reporting is off in this build by design.`,
  );
}

// VC++ runtime. windeployqt --compiler-runtime needs VCINSTALLDIR and silently
// does nothing without it, so copy it ourselves from VC\Redist (not System32).
// Has to run after everything else is staged since it dumpbins the whole payload.

function stagedBinaries(dir) {
  const out = [];
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) out.push(...stagedBinaries(full));
    else if (/\.(?:dll|exe)$/i.test(entry.name)) out.push(full);
  }
  return out;
}

function findVcRedistCrt() {
  const roots = [
    "C:\\Program Files\\Microsoft Visual Studio",
    "C:\\Program Files (x86)\\Microsoft Visual Studio",
  ];
  const found = [];
  for (const root of roots) {
    if (!fs.existsSync(root)) continue;
    for (const edition of fs.readdirSync(root)) {
      const editionDir = path.join(root, edition);
      if (!fs.statSync(editionDir).isDirectory()) continue;
      for (const sku of fs.readdirSync(editionDir)) {
        const msvc = path.join(editionDir, sku, "VC", "Redist", "MSVC");
        if (!fs.existsSync(msvc)) continue;
        for (const ver of fs.readdirSync(msvc)) {
          const x64 = path.join(msvc, ver, "x64");
          if (!fs.existsSync(x64)) continue;
          for (const entry of fs.readdirSync(x64)) {
            if (!/^Microsoft\.VC\d+\.CRT$/.test(entry)) continue;
            const dir = path.join(x64, entry);
            if (fs.existsSync(path.join(dir, "msvcp140.dll"))) found.push(dir);
          }
        }
      }
    }
  }
  return found.sort().pop() ?? null;
}

// not api-ms-win-crt-*, that's the UCRT which comes with Windows
const CRT_IMPORT = /^(?:msvcp|vcruntime|concrt|vccorlib)\d+\w*\.dll$/i;
const DEBUG_CRT = /^(?:msvcp\d+d|vcruntime\d+d|concrt\d+d|ucrtbased)\w*\.dll$/i;

const neededCrt = new Set();
for (const binary of stagedBinaries(payloadDir)) {
  for (const dep of importsOf(binary)) {
    if (CRT_IMPORT.test(dep)) neededCrt.add(dep.toLowerCase());
  }
}
if (neededCrt.size === 0) {
  fail(
    "no CRT imports found across the payload, which cannot be true of an MSVC\n" +
      "  build. Either dumpbin's output no longer matches this parse, or nothing\n" +
      "  was staged for it to read.",
  );
}

const crtDir = findVcRedistCrt();
if (!crtDir) {
  fail(
    "no Visual C++ redistributable directory holding the CRT was found.\n\n" +
      "  Looked for VC/Redist/MSVC/<version>/x64/Microsoft.VC<n>.CRT holding\n" +
      "  msvcp140.dll.\n\n" +
      "  Install the C++ workload's redistributable component (don't copy them\n" +
      "  from System32).",
  );
}

// import tables say VCRUNTIME140.dll
const available = new Map(
  fs.readdirSync(crtDir).map((f) => [f.toLowerCase(), f]),
);
const staged = [];
const unavailable = [];
for (const name of [...neededCrt].sort()) {
  const actual = available.get(name);
  if (!actual) {
    unavailable.push(name);
    continue;
  }
  fs.copyFileSync(path.join(crtDir, actual), path.join(payloadDir, actual));
  staged.push(actual);
}
if (unavailable.length > 0) {
  fail(
    `the payload imports CRT libraries that ${crtDir}\n` +
      `  does not hold: ${unavailable.join(", ")}\n\n` +
      "  The build links a toolset newer than the redistributable installed here.",
  );
}

const stagedDebugCrt = fs.readdirSync(payloadDir).filter((f) => DEBUG_CRT.test(f));
if (stagedDebugCrt.length > 0) {
  fail(
    `debug CRT libraries in the payload: ${stagedDebugCrt.join(", ")}\n\n` +
      "  These are not redistributable and the machine they land on will not have\n" +
      "  the debug UCRT they need. Something staged a Debug build.",
  );
}

console.log(
  `\n=== runtime ===\n  ${staged.length} CRT libraries from ${crtDir}` +
    `\n  ${staged.join(", ")}`,
);

// Themida. THEMIDA_PROJECT is a .tmd made in the Themida GUI against
// dist/payload/Vethida.exe, it decides what gets protected.

const themida = path.join(repoRoot, "staging", "themida", "Themida64.exe");
const themidaProject = process.env.THEMIDA_PROJECT;
const beforeProtect = fs.readFileSync(stagedExe);

if (themidaProject) {
  if (!fs.existsSync(themida)) fail(`Themida not found at ${themida}`);
  if (!fs.existsSync(themidaProject)) {
    fail(`THEMIDA_PROJECT does not exist: ${themidaProject}`);
  }
  run("themida", themida, ["/protect", themidaProject]);
  if (fs.readFileSync(stagedExe).equals(beforeProtect)) {
    fail(
      `Themida reported success but ${stagedExe} is unchanged.\n\n` +
        `  The project's input and output paths have to name this file, or the\n` +
        `  release ships the unprotected build under a protected build's name.`,
    );
  }
} else if (requireProtection) {
  fail(
    "THEMIDA_PROJECT is not set and --require-protection was passed.\n\n" +
      "  Set it to the .tmd project that protects dist/payload/Vethida.exe.",
  );
} else {
  console.log(
    "\n=== themida ===\n  THEMIDA_PROJECT unset; shipping unprotected." +
      "\n  Release builds pass --require-protection so this cannot happen quietly.",
  );
}

// UPX

if (unpacked) {
  console.log("\n=== upx ===\n  --unpacked: skipped, this is the comparison build");
} else {
  const before = fs.statSync(stagedExe).size;
  console.log("\n=== upx ===");
  const r = spawnSync(upx, ["-9", "--overlay=strip", "--compress-exports=0", stagedExe], {
    encoding: "utf8",
  });
  const output = (r.stdout ?? "") + (r.stderr ?? "");
  process.stdout.write(output);
  if (r.status !== 0) {
    // UPX refuses CFG images, and --force would leave broken CFG metadata
    if (/GUARD_CF/.test(output)) {
      fail(
        "UPX will not pack a Control Flow Guard image, and this build has CFG on\n" +
          "  (ControlFlowGuard in Vethida/common.props).\n\n" +
          "  Either pass --unpacked (on 1.0.0 packing made the installer ~958 KB\n" +
          "  bigger anyway since the payload is already LZMA'd), or turn CFG off\n" +
          "  in common.props. Don't use --force.",
      );
    }
    fail(`upx exited ${r.status}.`);
  }
  const after = fs.statSync(stagedExe).size;
  console.log(`  ${(before / 1_000_000).toFixed(1)} MB -> ${(after / 1_000_000).toFixed(1)} MB`);
}

// Qt DLLs must match QTDIR byte for byte (LGPL: users can swap in their own Qt)

const qtBin = path.join(qtDir, "bin");
let compared = 0;
for (const entry of fs.readdirSync(payloadDir)) {
  if (!/^Qt6[A-Za-z]+\.dll$/.test(entry)) continue;
  const shipped = path.join(payloadDir, entry);
  const original = path.join(qtBin, entry);
  if (!fs.existsSync(original)) {
    fail(`${entry} is in the payload but not in ${qtBin}; it did not come from this kit.`);
  }
  if (!fs.readFileSync(shipped).equals(fs.readFileSync(original))) {
    fail(
      `${entry} differs from ${original}.\n\n` +
        `  Something in this pipeline rewrote a Qt library. LGPLv3 s.4 requires\n` +
        `  the customer to be able to replace it with their own build.`,
    );
  }
  compared += 1;
}
if (compared === 0) {
  fail("no Qt6*.dll in the payload; windeployqt did not stage the runtime.");
}
console.log(`\n=== qt ===\n  ${compared} Qt libraries staged unmodified from ${qtBin}`);

console.log(
  `\npackage: ${payloadDir}` +
    `\n\nNothing here is signed. Next:` +
    `\n  scripts/release/sign.mjs ${path.join(payloadDir, "Vethida.exe")}` +
    `\n  scripts/release/make-installer.mjs`,
);
