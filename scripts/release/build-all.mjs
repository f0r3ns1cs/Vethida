#!/usr/bin/env node
// Runs the release checks, then builds the desktop app and the three Next apps.
//   node scripts/release/build-all.mjs [--skip-desktop] [--skip-web]
// Doesn't sign, package or publish (see the other scripts in this folder).

import fs from "node:fs";
import path from "node:path";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");
const skipDesktop = process.argv.includes("--skip-desktop");
const skipWeb = process.argv.includes("--skip-web");

function fail(message) {
  console.error(`\nbuild-all: ${message}\n`);
  process.exit(1);
}

function run(label, cmd, args, cwd) {
  console.log(`\n=== ${label} ===`);
  const r = spawnSync(cmd, args, {
    cwd: cwd ?? repoRoot,
    stdio: "inherit",
    shell: process.platform === "win32",
  });
  if (r.error) fail(`${label}: ${cmd} could not be started (${r.error.message})`);
  if (r.status !== 0) fail(`${label} failed with exit code ${r.status}.`);
}

// checks

run("version agreement", process.execPath, [
  path.join(repoRoot, "scripts/release/bump-version.mjs"),
  "--check",
]);

run("release readiness", process.execPath, [
  path.join(repoRoot, "scripts/release/check-release-ready.mjs"),
]);

run("integrity", process.execPath, [path.join(repoRoot, "scripts/check-integrity.mjs")]);

run("voice", process.execPath, [path.join(repoRoot, "scripts/check-voice.mjs")]);

run("preflight", process.execPath, [path.join(repoRoot, "scripts/preflight.mjs")]);

const version = fs.readFileSync(path.join(repoRoot, "VERSION"), "utf8").trim();
console.log(`\nBuilding Vethida ${version}`);

// desktop

if (!skipDesktop) {
  if (process.platform !== "win32") {
    fail(
      "the desktop client only builds on Windows with MSVC.\n\n" +
        "  Pass --skip-desktop to build the web surfaces only.",
    );
  }

  const msbuild = spawnSync("msbuild", ["-version"], { encoding: "utf8", shell: true });
  if (msbuild.error || msbuild.status !== 0) {
    fail(
      "msbuild not found.\n\n" +
        "  Run this from a Visual Studio Developer Command Prompt, or from a shell\n" +
        "  where VsDevCmd.bat has been run.",
    );
  }

  run(
    "desktop (x64 Release)",
    "msbuild",
    [
      path.join(repoRoot, "Vethida", "vethida.sln"),
      "/p:Configuration=Release",
      "/p:Platform=x64",
      "/m",
      "/nologo",
      "/verbosity:minimal",
    ],
  );

  const exe = path.join(repoRoot, "Vethida", "build", "x64_Release", "vethida.exe");
  if (!fs.existsSync(exe)) {
    fail(
      `msbuild reported success but ${exe} does not exist.\n` +
        `  The output path in common.props may have changed.`,
    );
  }
  console.log(`  ${exe}  ${(fs.statSync(exe).size / 1_000_000).toFixed(1)} MB`);

  // run again so the dev-build marker scan sees the new exe
  run("release readiness (built binary)", process.execPath, [
    path.join(repoRoot, "scripts/release/check-release-ready.mjs"),
  ]);
}

// web

if (!skipWeb) {
  // vethida.com isn't in the workspace
  run("marketing site", "pnpm", ["build"], path.join(repoRoot, "vethida.com"));
  run("portal", "pnpm", ["--filter", "my.vethida.com", "build"]);
  run("api", "pnpm", ["--filter", "api.vethida.com", "build"]);
}

console.log(
  `\nbuild-all: ${version} built.` +
    `\n\nNothing here is staged, packed or signed. Next, in order:` +
    `\n  scripts/release/package.mjs        stage Qt, protect, pack` +
    `\n  scripts/release/sign.mjs           sign the staged Vethida.exe` +
    `\n  scripts/release/make-installer.mjs build VethidaSetup.exe` +
    `\n  scripts/release/sign.mjs           sign the installer` +
    `\n\nThen generate-source-offer.mjs, virustotal-submit.mjs and publish.mjs.`,
);
