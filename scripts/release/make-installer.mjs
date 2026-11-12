#!/usr/bin/env node
// Builds VethidaSetup.exe: setup stub + 7z payload + footer.
//   node scripts/release/make-installer.mjs \
//     --stub Vethida/build/x64_Release/vethida-setup.exe \
//     --payload-dir dist/payload \
//     --uninstall Vethida/build/x64_Release/uninstall.exe \
//     --out dist/VethidaSetup.exe
// Sign the output afterwards, and stamp the feed from the signed file.

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";

function parseArgs(argv) {
  const args = {};
  for (let i = 2; i < argv.length; i++) {
    switch (argv[i]) {
      case "--stub": args.stub = argv[++i]; break;
      case "--payload-dir": args.payloadDir = argv[++i]; break;
      case "--uninstall": args.uninstall = argv[++i]; break;
      case "--out": args.out = argv[++i]; break;
      case "--sevenzip": args.sevenZip = argv[++i]; break;
      default:
        console.error(`Unknown argument: ${argv[i]}`);
        process.exit(2);
    }
  }
  for (const required of ["stub", "payloadDir", "uninstall", "out"]) {
    if (!args[required]) {
      console.error(`Missing --${required.replace(/[A-Z]/g, (c) => "-" + c.toLowerCase())}`);
      process.exit(2);
    }
  }
  return args;
}

function findSevenZip(override) {
  const candidates = [
    override,
    "C:\\Program Files\\7-Zip\\7z.exe",
    "C:\\Program Files (x86)\\7-Zip\\7z.exe",
    "7z",
  ].filter(Boolean);
  for (const c of candidates) {
    try {
      execFileSync(c, ["i"], { stdio: "ignore" });
      return c;
    } catch {
      continue;
    }
  }
  console.error("7z.exe not found; install 7-Zip or pass --sevenzip.");
  process.exit(2);
}

const args = parseArgs(process.argv);
const sevenZip = findSevenZip(args.sevenZip);

for (const f of [args.stub, args.uninstall]) {
  if (!fs.existsSync(f)) {
    console.error(`Not found: ${f}`);
    process.exit(2);
  }
}
if (!fs.existsSync(path.join(args.payloadDir, "Vethida.exe"))) {
  console.error(`${args.payloadDir} does not look like a staged payload (no Vethida.exe).`);
  process.exit(2);
}

// archive root = install dir
const work = fs.mkdtempSync(path.join(os.tmpdir(), "vth-pkg-"));
const archive = path.join(work, "payload.7z");
try {
  execFileSync(sevenZip, [
    "a", "-t7z", "-mx=7", "-ms=on", archive,
    path.join(path.resolve(args.payloadDir), "*"),
  ], { stdio: "inherit" });
  execFileSync(sevenZip, ["a", archive, path.resolve(args.uninstall)], {
    stdio: "inherit",
  });

  const stub = fs.readFileSync(args.stub);
  const payload = fs.readFileSync(archive);

  // "VTHPKG01" + u64le offset + u64le size (the stub stores the magic xor'd so it
  // doesn't find itself)
  const footer = Buffer.alloc(24);
  footer.write("VTHPKG01", 0, "ascii");
  footer.writeBigUInt64LE(BigInt(stub.length), 8);
  footer.writeBigUInt64LE(BigInt(payload.length), 16);

  fs.mkdirSync(path.dirname(path.resolve(args.out)), { recursive: true });
  fs.writeFileSync(args.out, Buffer.concat([stub, payload, footer]));

  console.log(`Wrote ${args.out}`);
  console.log(`  stub:    ${stub.length} bytes`);
  console.log(`  payload: ${payload.length} bytes (7z)`);
  console.log("Next: sign it, then stamp the update feed from the signed file.");
} finally {
  fs.rmSync(work, { recursive: true, force: true });
}
