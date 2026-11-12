#!/usr/bin/env node
// P-256 keypair for signing update manifests.
//   node scripts/release/gen-update-keys.mjs --out ./update-manifest-v1.pem
// Not the same key as LICENSE_SIGNING_KEY (gen-secrets.mjs) - keep this one offline.
// Run it twice for the primary and backup keys, paste the printed C++ into
// SecurityConfig and drop -PLACEHOLDER from the kids.

import crypto from "node:crypto";
import fs from "node:fs";
import path from "node:path";

const argv = process.argv.slice(2);
function flag(name, fallback = null) {
  const i = argv.indexOf(`--${name}`);
  return i === -1 || i === argv.length - 1 ? fallback : argv[i + 1];
}

if (argv.includes("--help")) {
  process.stdout.write(
    "usage: node scripts/release/gen-update-keys.mjs [--out <file.pem>] [--kid vethida-update-v1]\n" +
      "  --out   where to write the PRIVATE key. Omit to print it instead.\n" +
      "  --kid   key id to embed in the printed C++ (default vethida-update-v1)\n",
  );
  process.exit(0);
}

const kid = flag("kid", "vethida-update-v1");
const out = flag("out");

const { privateKey, publicKey } = crypto.generateKeyPairSync("ec", {
  namedCurve: "prime256v1",
});
const privatePem = privateKey.export({ format: "pem", type: "pkcs8" });
// raw X || Y (64 bytes)
const jwk = publicKey.export({ format: "jwk" });
const publicRaw = Buffer.concat([
  Buffer.from(jwk.x, "base64url"),
  Buffer.from(jwk.y, "base64url"),
]);

if (out) {
  const resolved = path.resolve(out);
  if (fs.existsSync(resolved)) {
    console.error(
      `${resolved} already exists, not overwriting a signing key.`,
    );
    process.exit(1);
  }
  fs.writeFileSync(resolved, privatePem, { mode: 0o600 });
  console.log(`private key -> ${resolved}`);
  console.log(
    "Keep it offline and back it up. If it's lost, installed clients can't take\n" +
      "a signed update until they reinstall.\n",
  );
} else {
  console.log("---- PRIVATE KEY (store offline, never commit) ----");
  process.stdout.write(privatePem);
  console.log();
}

const bytes = [...publicRaw]
  .map((b) => `0x${b.toString(16).padStart(2, "0")}`)
  .reduce((rows, b, i) => {
    const row = Math.floor(i / 8);
    (rows[row] ??= []).push(b);
    return rows;
  }, [])
  .map((row) => "            " + row.join(", "))
  .join(",\n");

console.log(`---- SecurityConfig.cpp (updateManifestPrimaryPublicKey or ...BackupPublicKey) ----

    // kid: ${kid}
    static const unsigned char key[64] = {
${bytes}
    };
    return QByteArray(reinterpret_cast<const char*>(key), sizeof(key));

---- SecurityConfig.h (UpdateManifestPrimaryKid or ...BackupKid) ----

"${kid}"

base64url: ${publicRaw.toString("base64url")}
hex:       ${publicRaw.toString("hex")}
`);
