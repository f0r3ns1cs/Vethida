#!/usr/bin/env node

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");

const TARGETS = [
  {
    file: "Vethida/version.h",
    subs: [
      [/(#define VETHIDA_VERSION_STRING\s+")[^"]+(")/, (v) => `$1${v}$2`],
      [/(#define VETHIDA_VERSION_FILE\s+)[\d,]+/, (v) => `$1${v.split(".").join(",")},0`],
    ],
  },
  {
    file: "Vethida/app.manifest",
    subs: [[/(<assemblyIdentity version=")[^"]+(")/, (v) => `$1${v}.0$2`]],
  },
  {
    file: "vethida.com/package.json",
    subs: [[/("version":\s*")[^"]+(")/, (v) => `$1${v}$2`]],
  },
];

function apply(version, write) {
  const stale = [];
  for (const t of TARGETS) {
    const full = path.join(root, t.file);
    const before = fs.readFileSync(full, "utf8");
    let after = before;
    for (const [re, to] of t.subs) {
      if (!re.test(after)) throw new Error(`${t.file}: pattern ${re} not found`);
      after = after.replace(re, to(version));
    }
    if (after !== before) {
      stale.push(t.file);
      if (write) fs.writeFileSync(full, after);
    }
  }
  return stale;
}

const argv = process.argv.slice(2);
if (argv[0] === "--check") {
  const version = fs.readFileSync(path.join(root, "VERSION"), "utf8").trim();
  const stale = apply(version, false);
  if (stale.length) {
    console.error(`bump-version: ${stale.join(", ")} disagree with /VERSION (${version}).`);
    console.error("Run node scripts/bump-version.mjs " + version);
    process.exit(1);
  }
  console.log(`bump-version: every target says ${version}`);
} else if (/^\d+\.\d+\.\d+$/.test(argv[0] ?? "")) {
  fs.writeFileSync(path.join(root, "VERSION"), `${argv[0]}\n`);
  const changed = apply(argv[0], true);
  console.log(`bump-version: ${argv[0]} written to VERSION${changed.length ? ", " + changed.join(", ") : ""}`);
} else {
  console.error("usage: bump-version.mjs <MAJOR.MINOR.PATCH> | --check");
  process.exit(1);
}
