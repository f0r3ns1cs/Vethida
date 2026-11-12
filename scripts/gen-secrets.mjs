// Prints new dev secrets for the .env.local files. Nothing is saved.
//   node scripts/gen-secrets.mjs
// The signing key and the public key printed at the bottom have to come from the same run.

import crypto from "node:crypto";

function b64(bytes) {
  return Buffer.from(bytes).toString("base64");
}

function token(bytes = 32) {
  return crypto.randomBytes(bytes).toString("base64url");
}

const { privateKey, publicKey } = crypto.generateKeyPairSync("ec", {
  namedCurve: "prime256v1",
});
const signingKey = b64(privateKey.export({ format: "der", type: "pkcs8" }));
// raw X || Y, same as signing.ts exports
const jwk = publicKey.export({ format: "jwk" });
const publicRaw = Buffer.concat([
  Buffer.from(jwk.x, "base64url"),
  Buffer.from(jwk.y, "base64url"),
]);

const internalApiToken = token();
const cronSecret = token();
const fingerprintPepper = token(); // can't be changed later without breaking stored fingerprints
const out = `
# api.vethida.com/.env.local
LICENSE_SIGNING_KEY=${signingKey}
INTERNAL_API_TOKEN=${internalApiToken}
CRON_SECRET=${cronSecret}
FINGERPRINT_PEPPER=${fingerprintPepper}

# my.vethida.com/.env.local
BETTER_AUTH_SECRET=${b64(crypto.randomBytes(32))}
CRON_SECRET=${cronSecret}

# vethida.com/.env.local
INTERNAL_API_TOKEN=${internalApiToken}

# Public key for LICENSE_SIGNING_KEY (goes into the desktop app, not an env var)
#   base64url : ${publicRaw.toString("base64url")}
#   hex       : ${publicRaw.toString("hex")}
`.trimStart();

process.stdout.write(out);
