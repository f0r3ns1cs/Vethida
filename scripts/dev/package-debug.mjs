#!/usr/bin/env node
// Debug x64 build zipped up for testing on a clean VM.
//   node scripts/dev/package-debug.mjs [--no-build] [--remote-debugger] [--vm <name>]
//   --no-build          use what's already in Vethida/build/x64_Debug
//   --remote-debugger   include msvsmon
//   --vm <name>         Copy-VMFile to C:\vethida-debug\ on a Hyper-V guest (elevated shell,
//                       Guest Service Interface on)
// Output: dist/debug/Vethida-debug-<version>.zip
// Ships the debug CRT, which isn't redistributable - never give this zip to anyone.

import fs from "node:fs";
import path from "node:path";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");

function arg(name) {
  const i = process.argv.indexOf(name);
  return i !== -1 ? process.argv[i + 1] : undefined;
}
const noBuild = process.argv.includes("--no-build");
const withRemoteDebugger = process.argv.includes("--remote-debugger");
const vmName = arg("--vm");

function fail(message) {
  console.error(`\npackage-debug: ${message}\n`);
  process.exit(1);
}

function run(label, cmd, args, opts = {}) {
  console.log(`\n=== ${label} ===`);
  const r = spawnSync(cmd, args, { stdio: "inherit", ...opts });
  if (r.error) fail(`${label}: ${cmd} could not be started (${r.error.message})`);
  if (r.status !== 0) fail(`${label} failed with exit code ${r.status}.`);
}

function newest(dir, filter = () => true) {
  if (!fs.existsSync(dir)) return null;
  return fs.readdirSync(dir).filter(filter).sort().pop() ?? null;
}

function copyDir(from, to) {
  fs.cpSync(from, to, { recursive: true });
}

if (process.platform !== "win32") fail("the desktop client only builds on Windows.");

// toolchain

const vswhere = "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe";
if (!fs.existsSync(vswhere)) fail(`vswhere not found at ${vswhere}; is Visual Studio installed?`);
const vsRoot = spawnSync(
  vswhere,
  ["-latest", "-prerelease", "-products", "*", "-requires", "Microsoft.Component.MSBuild", "-property", "installationPath"],
  { encoding: "utf8" },
).stdout.trim();
if (!vsRoot) fail("vswhere found no Visual Studio with MSBuild.");

const qtDir = process.env.QTDIR ?? "C:\\Qt\\6.11.1\\msvc2022_64";
const windeployqt = path.join(qtDir, "bin", "windeployqt.exe");
if (!fs.existsSync(windeployqt)) {
  fail(`windeployqt not found at ${windeployqt}\n\n  Set QTDIR to the Qt kit the project links.`);
}

// build

const debugDir = path.join(repoRoot, "Vethida", "build", "x64_Debug");
const builtExe = path.join(debugDir, "vethida.exe");

if (!noBuild) {
  const msbuild = path.join(vsRoot, "MSBuild", "Current", "Bin", "amd64", "MSBuild.exe");
  if (!fs.existsSync(msbuild)) fail(`MSBuild not found at ${msbuild}`);
  // app + vth_core only
  run("build (x64 Debug)", msbuild, [
    path.join(repoRoot, "Vethida", "vethida.sln"),
    "/t:vethida",
    "/p:Configuration=Debug",
    "/p:Platform=x64",
    "/m",
    "/nologo",
    "/verbosity:minimal",
  ]);
}
if (!fs.existsSync(builtExe)) fail(`${builtExe} does not exist. Drop --no-build.`);

// stage

const version = fs.readFileSync(path.join(repoRoot, "VERSION"), "utf8").trim();
const outRoot = path.join(repoRoot, "dist", "debug");
const stageRoot = path.join(outRoot, "stage");
const appDir = path.join(stageRoot, "Vethida");

fs.rmSync(stageRoot, { recursive: true, force: true });
fs.mkdirSync(appDir, { recursive: true });

// pdb keeps its linked name or the debugger won't find it
const stagedExe = path.join(appDir, "Vethida.exe");
fs.copyFileSync(builtExe, stagedExe);
fs.copyFileSync(path.join(debugDir, "vethida.pdb"), path.join(appDir, "vethida.pdb"));

run("windeployqt (debug)", windeployqt, [
  "--debug",
  "--pdb",
  "--no-translations",
  "--no-system-d3d-compiler",
  "--no-opengl-sw",
  "--no-quick-import",
  "--no-compiler-runtime",
  stagedExe,
]);

// debug CRT + ucrtbased, a clean VM doesn't have them
const redistRoot = path.join(vsRoot, "VC", "Redist", "MSVC");
let debugCrt = null;
for (const ver of fs.existsSync(redistRoot) ? fs.readdirSync(redistRoot).sort().reverse() : []) {
  const base = path.join(redistRoot, ver, "debug_nonredist", "x64");
  const crt = newest(base, (d) => /^Microsoft\.VC\d+\.DebugCRT$/.test(d));
  if (crt) {
    debugCrt = path.join(base, crt);
    break;
  }
}
if (!debugCrt) fail(`no Microsoft.VC*.DebugCRT under ${redistRoot}\\*\\debug_nonredist\\x64`);
for (const f of fs.readdirSync(debugCrt)) fs.copyFileSync(path.join(debugCrt, f), path.join(appDir, f));

const kitsBin = "C:\\Program Files (x86)\\Windows Kits\\10\\bin";
const sdk = newest(kitsBin, (d) => /^10\./.test(d) && fs.existsSync(path.join(kitsBin, d, "x64", "ucrt", "ucrtbased.dll")));
if (!sdk) fail(`ucrtbased.dll not found under ${kitsBin}\\<sdk>\\x64\\ucrt`);
fs.copyFileSync(path.join(kitsBin, sdk, "x64", "ucrt", "ucrtbased.dll"), path.join(appDir, "ucrtbased.dll"));
console.log(`\n=== runtime ===\n  debug CRT from ${debugCrt}\n  ucrtbased.dll from SDK ${sdk}`);

// optional extras, same layout as package.mjs
const scewinSrc = path.join(repoRoot, "vendors", "ami-scewin");
const scewinFiles = ["SCEWIN_64.exe", "amifldrv64.sys", "amigendrv64.sys"];
if (scewinFiles.every((f) => fs.existsSync(path.join(scewinSrc, f)))) {
  const out = path.join(appDir, "tools", "ami-scewin");
  fs.mkdirSync(out, { recursive: true });
  for (const f of scewinFiles) fs.copyFileSync(path.join(scewinSrc, f), path.join(out, f));
  console.log("\n=== scewin ===\n  staged");
}
const sentrySrc = path.join(repoRoot, "vendors", "sentry-native");
for (const f of ["sentry.dll", "crashpad_handler.exe"]) {
  if (fs.existsSync(path.join(sentrySrc, f))) fs.copyFileSync(path.join(sentrySrc, f), path.join(appDir, f));
}

if (withRemoteDebugger) {
  const msvsmon = path.join(vsRoot, "Common7", "IDE", "Remote Debugger", "x64");
  if (!fs.existsSync(msvsmon)) fail(`Remote Debugger not found at ${msvsmon}`);
  copyDir(msvsmon, path.join(stageRoot, "RemoteDebugger"));
  console.log(`\n=== remote debugger ===\n  staged from ${msvsmon}`);
}

fs.writeFileSync(
  path.join(stageRoot, "README.txt"),
  [
    `Vethida ${version} - DEBUG build (VETHIDA_DEV_BUILD), not for distribution.`,
    ``,
    `Run Vethida\\Vethida.exe (it asks for elevation).`,
    `Entitlement is bypassed; data lives in %LOCALAPPDATA%\\Rens Engineering\\Vethida (dev)\\.`,
    ``,
    ...(withRemoteDebugger
      ? [
          `Remote debugging:`,
          `  1. On the VM, run RemoteDebugger\\msvsmon.exe as administrator`,
          `     (Vethida runs elevated, so msvsmon has to be too).`,
          `  2. In Visual Studio on the host: Debug > Attach to Process,`,
          `     Connection target <vm-name>:<port msvsmon shows>, attach to Vethida.exe.`,
          `  Symbols: vethida.pdb sits beside the exe. Qt's own PDBs come from`,
          `  the "Qt Debug Information Files" component in the Qt maintenance tool.`,
          ``,
        ]
      : []),
  ].join("\r\n"),
);

// zip (System32 bsdtar, Git's tar can't write zips)
const zipPath = path.join(outRoot, `Vethida-debug-${version}.zip`);
fs.rmSync(zipPath, { force: true });
run("zip", "C:\\Windows\\System32\\tar.exe", ["-a", "-c", "-f", zipPath, "-C", stageRoot, "."]);
console.log(`  ${zipPath}  ${(fs.statSync(zipPath).size / 1_000_000).toFixed(1)} MB`);

if (vmName) {
  const dest = `C:\\vethida-debug\\${path.basename(zipPath)}`;
  run(`copy to VM '${vmName}'`, "powershell.exe", [
    "-NoProfile",
    "-Command",
    `Copy-VMFile -Name '${vmName.replace(/'/g, "''")}' -SourcePath '${zipPath}' ` +
      `-DestinationPath '${dest}' -CreateFullPath -FileSource Host -Force -ErrorAction Stop`,
  ]);
  console.log(`  ${dest} on ${vmName}`);
}

console.log(`\npackage-debug: done.`);
