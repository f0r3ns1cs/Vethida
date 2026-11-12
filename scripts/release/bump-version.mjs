#!/usr/bin/env node
// Copies /VERSION into every file that has a version number. Only edit /VERSION.
//   node scripts/release/bump-version.mjs            apply
//   node scripts/release/bump-version.mjs --check    check only
//   node scripts/release/bump-version.mjs 1.2.0      set /VERSION, then apply

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");
const VERSION_FILE = path.join(repoRoot, "VERSION");

const args = process.argv.slice(2);
const check = args.includes("--check");
const explicit = args.find((a) => /^\d+\.\d+\.\d+$/.test(a));

function fail(message) {
  console.error(`bump-version: ${message}`);
  process.exit(1);
}

if (explicit && check) fail("--check and an explicit version are mutually exclusive.");

if (explicit) {
  fs.writeFileSync(VERSION_FILE, `${explicit}\n`, "utf8");
  console.log(`VERSION set to ${explicit}`);
}

let version;
try {
  version = fs.readFileSync(VERSION_FILE, "utf8").trim();
} catch {
  fail(`cannot read ${VERSION_FILE}. It is the source of truth and must exist.`);
}
if (!/^\d+\.\d+\.\d+$/.test(version)) {
  fail(`"${version}" in /VERSION is not MAJOR.MINOR.PATCH.`);
}
const [major, minor, patch] = version.split(".");

// A pattern that doesn't match is an error so a file can't get left behind.
// The other package.json files stay at 0.0.0.
const FOUR_PART_ASSEMBLY_VERSION = /(<assemblyIdentity\s+version=")\d+\.\d+\.\d+\.\d+(")/;

const targets = [
  {
    file: "Vethida/version.h",
    subs: [
      {
        find: /(#define VETHIDA_VERSION_FILE\s+)\d+,\d+,\d+,\d+/,
        to: `$1${major},${minor},${patch},0`,
      },
      {
        find: /(#define VETHIDA_VERSION_STRING\s+")[^"]*(")/,
        to: `$1${major}.${minor}.${patch}$2`,
      },
    ],
  },
  {
    file: "Vethida/common.props",
    subs: [
      { find: /(<VethidaVersionMajor>)\d+(<\/VethidaVersionMajor>)/, to: `$1${major}$2` },
      { find: /(<VethidaVersionMinor>)\d+(<\/VethidaVersionMinor>)/, to: `$1${minor}$2` },
      { find: /(<VethidaVersionPatch>)\d+(<\/VethidaVersionPatch>)/, to: `$1${patch}$2` },
    ],
  },
  {
    file: "vethida.com/package.json",
    subs: [{ find: /("version":\s*")[^"]*(")/, to: `$1${version}$2` }],
  },
  // manifests are four-part, revision is always 0
  {
    file: "Vethida/app.manifest",
    subs: [{ find: FOUR_PART_ASSEMBLY_VERSION, to: `$1${major}.${minor}.${patch}.0$2` }],
  },
  {
    file: "installer-updater/Vethida/setup/installer.manifest",
    subs: [{ find: FOUR_PART_ASSEMBLY_VERSION, to: `$1${major}.${minor}.${patch}.0$2` }],
  },
];

let changed = 0;
let drift = 0;

for (const target of targets) {
  const full = path.join(repoRoot, target.file);
  let src;
  try {
    src = fs.readFileSync(full, "utf8");
  } catch {
    fail(`${target.file} is missing. Update this script if the file moved.`);
  }

  let next = src;
  for (const sub of target.subs) {
    if (!sub.find.test(next)) {
      fail(
        `${target.file}: pattern ${sub.find} matched nothing.\n` +
          `  The file changed, update this script.`,
      );
    }
    next = next.replace(sub.find, sub.to);
  }

  if (next === src) {
    console.log(`  ${target.file}  already ${version}`);
    continue;
  }
  drift++;
  if (check) {
    console.error(`  ${target.file}  OUT OF DATE (expected ${version})`);
    continue;
  }
  fs.writeFileSync(full, next, "utf8");
  changed++;
  console.log(`  ${target.file}  -> ${version}`);
}

if (check) {
  if (drift > 0) {
    console.error(
      `\n${drift} file(s) disagree with /VERSION (${version}).` +
        `\nRun: node scripts/release/bump-version.mjs`,
    );
    process.exit(1);
  }
  console.log(`\nbump-version: ok, everything at ${version}`);
} else {
  console.log(
    `\nbump-version: ${version} applied to ${changed} file(s).` +
      `\n\nThe desktop version resource is compiled in, so the binary only carries` +
      `\nthis after a rebuild. The update feed is stamped separately by` +
      `\nscripts/release/stamp-update-feed.mjs, which takes the version as an` +
      `\nargument - use the same one.`,
  );
}
