#!/usr/bin/env node
// Builds the Qt LGPLv3 source bundle offered in legal/third-party.md: the Qt
// source, build instructions, license texts and a sha256 manifest.
//   node scripts/release/generate-source-offer.mjs --qt-src <dir> [--out <dir>]
//   --qt-src   unpacked Qt source matching the shipped version (not downloaded on purpose)
//   --qt-ver   Qt version, read from vethida.vcxproj if omitted
//   --out      default dist/source-offer
//   --check    verify an existing bundle against its manifest

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { createHash } from "node:crypto";

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");

function arg(name) {
  const i = process.argv.indexOf(name);
  return i !== -1 ? process.argv[i + 1] : undefined;
}
const check = process.argv.includes("--check");
const qtSrc = arg("--qt-src");
const outDir = path.resolve(repoRoot, arg("--out") ?? "dist/source-offer");

function fail(message) {
  console.error(`\ngenerate-source-offer: ${message}\n`);
  process.exit(1);
}

// read from the vcxproj so it can't drift
function shippedQtModules() {
  const proj = path.join(repoRoot, "Vethida", "vethida.vcxproj");
  let src;
  try {
    src = fs.readFileSync(proj, "utf8");
  } catch {
    fail(`cannot read ${proj} to determine which Qt modules are linked.`);
  }
  const m = /<QtModules>([^<]+)<\/QtModules>/.exec(src);
  if (!m) {
    fail(
      "no <QtModules> in vethida.vcxproj. The offer must state which modules " +
        "are covered; fix this script if the project format changed.",
    );
  }
  return m[1]
    .split(";")
    .map((s) => s.trim())
    .filter(Boolean);
}

function shippedQtVersion() {
  const proj = path.join(repoRoot, "Vethida", "vethida.vcxproj");
  const src = fs.readFileSync(proj, "utf8");
  const m = /<QtInstall>Qt\s*([0-9.]+)<\/QtInstall>/.exec(src);
  return m ? m[1] : null;
}

function productVersion() {
  return fs.readFileSync(path.join(repoRoot, "VERSION"), "utf8").trim();
}

function sha256(file) {
  return createHash("sha256").update(fs.readFileSync(file)).digest("hex");
}

if (check) {
  const manifest = path.join(outDir, "MANIFEST.txt");
  if (!fs.existsSync(manifest)) {
    fail(
      `no bundle at ${outDir}. The LGPL written offer in legal/third-party.md ` +
        `promises this exists for three years after a release; generate it before shipping.`,
    );
  }
  const lines = fs.readFileSync(manifest, "utf8").split(/\r?\n/);
  let bad = 0;
  for (const line of lines) {
    const m = /^([0-9a-f]{64})\s{2}(.+)$/.exec(line);
    if (!m) continue;
    const [, want, rel] = m;
    const full = path.join(outDir, rel);
    if (!fs.existsSync(full)) {
      console.error(`  MISSING  ${rel}`);
      bad++;
      continue;
    }
    if (sha256(full) !== want) {
      console.error(`  ALTERED  ${rel}`);
      bad++;
    }
  }
  if (bad > 0) fail(`${bad} file(s) missing or altered in the bundle.`);
  console.log(`source-offer: ok, bundle at ${outDir} matches its manifest.`);
  process.exit(0);
}

if (!qtSrc) {
  fail(
    "--qt-src is required.\n\n" +
      "  Point it at the unpacked Qt source tree the shipped Qt was built from,\n" +
      "  for example from https://download.qt.io/archive/qt/.",
  );
}
if (!fs.existsSync(qtSrc)) fail(`--qt-src path does not exist: ${qtSrc}`);

const modules = shippedQtModules();
const qtVersion = arg("--qt-ver") ?? shippedQtVersion();
if (!qtVersion) {
  fail("could not determine the Qt version; pass --qt-ver explicitly.");
}

fs.rmSync(outDir, { recursive: true, force: true });
fs.mkdirSync(outDir, { recursive: true });

const srcOut = path.join(outDir, `qt-${qtVersion}-src`);
console.log(`Copying Qt source from ${qtSrc} ...`);
fs.cpSync(qtSrc, srcOut, { recursive: true });

// LGPL wants the build scripts too
const buildDoc = `# Rebuilding the Qt libraries shipped with Vethida ${productVersion()}

Qt version: ${qtVersion}
Modules linked by Vethida: ${modules.join(", ")}
Toolchain: MSVC, x64, matching the platform toolset in Vethida/common.props

## Reproducing

From a Visual Studio x64 developer prompt, in a build directory outside the
source tree:

    ..\\qt-${qtVersion}-src\\configure.bat -opensource -confirm-license \\
        -shared -release -nomake examples -nomake tests \\
        -prefix C:\\Qt\\${qtVersion}\\msvc_64

    cmake --build . --parallel

Only the modules listed above are required. Building the full Qt tree also works
and produces a superset.

## Swapping the result into Vethida

Vethida links Qt dynamically. The Qt DLLs sit beside vethida.exe in the
installation directory, and \`qt.conf\` next to the executable declares
\`Prefix = .\` so Qt resolves its libraries and plugins from there.

To use your build, overwrite those DLLs and the \`plugins\` directory with the
equivalents from your \`-prefix\`. Keep the same Qt major version so the ABI
matches.

Vethida applies no technical measure that prevents this. No signature check
gates the Qt libraries, and no integrity or anti-tamper measure refuses to run
when they differ from the ones we shipped.

## Your rights

These libraries are licensed to you under the GNU Lesser General Public License
version 3, not under Vethida's own license. The full LGPLv3 and GPLv3 texts are
in this bundle and also ship inside the application under
Help -> Open-Source Licenses.

Nothing in Vethida's End User License Agreement restricts your rights under
LGPLv3 with respect to these components.
`;
fs.writeFileSync(path.join(outDir, "BUILDING.md"), buildDoc, "utf8");

const licenseSources = [
  ["LGPL-3.0.txt", path.join(repoRoot, "legal", "LGPL-3.0.txt")],
  ["GPL-3.0.txt", path.join(repoRoot, "legal", "GPL-3.0.txt")],
];
for (const [name, from] of licenseSources) {
  if (!fs.existsSync(from)) {
    fail(
      `${name} not found at ${from}. The bundle must carry the license texts; ` +
        `LGPLv3 incorporates GPLv3 by reference and both are required.`,
    );
  }
  fs.copyFileSync(from, path.join(outDir, name));
}

// for --check
const manifestLines = [];
function walk(dir) {
  for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
    const full = path.join(dir, e.name);
    if (e.isDirectory()) walk(full);
    else {
      const rel = path.relative(outDir, full).split(path.sep).join("/");
      if (rel === "MANIFEST.txt") continue;
      manifestLines.push(`${sha256(full)}  ${rel}`);
    }
  }
}
walk(outDir);
manifestLines.sort();
fs.writeFileSync(
  path.join(outDir, "MANIFEST.txt"),
  `# Qt corresponding-source bundle for Vethida ${productVersion()}\n` +
    `# Qt ${qtVersion}; modules: ${modules.join(", ")}\n` +
    `# Verify with: node scripts/release/generate-source-offer.mjs --check --out <dir>\n\n` +
    manifestLines.join("\n") +
    "\n",
  "utf8",
);

console.log(
  `\nsource-offer: ${manifestLines.length} files at ${outDir}` +
    `\n  Qt ${qtVersion}, modules: ${modules.join(", ")}` +
    `\n\nPublish this alongside the installer and keep it reachable for at least` +
    `\nthree years from the release date, as legal/third-party.md promises.`,
);
