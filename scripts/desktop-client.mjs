// Fake desktop client for testing the device routes on the local API.
//   node scripts/desktop-client.mjs activate --license XXXXX-XXXXX-XXXXX-XXXXX-XXXXX
//   node scripts/desktop-client.mjs heartbeat | deactivate | auth-start | drift | newmachine
//   node scripts/desktop-client.mjs auth-poll --device-code <code>
// Signs requests like vth_core: P-256 over METHOD\npath\ntimestamp\nsha256(body).
// The keypair in scripts/dev/.device-key.json is the machine identity, delete it
// (or run `forget`) to act as a new PC.

import crypto from "node:crypto";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const KEY_FILE = path.join(here, "dev", ".device-key.json");

const argv = process.argv.slice(2);
const command = argv[0];
function flag(name, fallback = null) {
  const i = argv.indexOf(`--${name}`);
  return i === -1 || i === argv.length - 1 ? fallback : argv[i + 1];
}

const base = flag("url", "http://localhost:3001");

if (!command || command === "help") {
  process.stdout.write(
    "usage: node scripts/desktop-client.mjs <command> [--license KEY] [--device-code CODE] [--url http://localhost:3001]\n\n" +
      "commands: activate  heartbeat  deactivate  auth-start  auth-poll  drift  newmachine  whoami  forget\n",
  );
  process.exit(command ? 0 : 1);
}

// identity
function loadDevice() {
  try {
    const raw = JSON.parse(fs.readFileSync(KEY_FILE, "utf8"));
    return {
      privateKey: crypto.createPrivateKey({
        key: Buffer.from(raw.privateKeyPkcs8B64, "base64"),
        format: "der",
        type: "pkcs8",
      }),
      publicKeyRaw: Buffer.from(raw.publicKeyRawB64url, "base64url"),
      fingerprint: raw.fingerprint,
    };
  } catch {
    return null;
  }
}

function randomHex(n) {
  return crypto.randomBytes(n).toString("hex").toUpperCase();
}

function newFingerprint() {
  return {
    motherboardUuid: crypto.randomUUID(),
    biosSerial: randomHex(6),
    cpuId: randomHex(8),
    volumeSerial: randomHex(4),
    primaryMac: Array.from({ length: 6 }, () => randomHex(1)).join(":"),
    osInstallDate: "2026-01-14T09:12:00Z",
  };
}

function createDevice() {
  const { privateKey, publicKey } = crypto.generateKeyPairSync("ec", {
    namedCurve: "prime256v1",
  });
  const jwk = publicKey.export({ format: "jwk" });
  const publicKeyRaw = Buffer.concat([
    Buffer.from(jwk.x, "base64url"),
    Buffer.from(jwk.y, "base64url"),
  ]);
  const record = {
    privateKeyPkcs8B64: privateKey
      .export({ format: "der", type: "pkcs8" })
      .toString("base64"),
    publicKeyRawB64url: publicKeyRaw.toString("base64url"),
    fingerprint: newFingerprint(),
  };
  fs.writeFileSync(KEY_FILE, `${JSON.stringify(record, null, 2)}\n`, "utf8");
  console.log(`[sim] new device identity written to ${KEY_FILE}`);
  return loadDevice();
}

const device = loadDevice() ?? createDevice();

function pubkeyId() {
  return crypto
    .createHash("sha256")
    .update(device.publicKeyRaw)
    .digest("hex")
    .slice(0, 32);
}

// transport
async function signedPost(pathname, payload, { withPubkeyId = true } = {}) {
  const body = JSON.stringify(payload);
  const timestamp = String(Date.now());
  const bodyHash = crypto.createHash("sha256").update(body, "utf8").digest("hex");
  const canonical = ["POST", pathname, timestamp, bodyHash].join("\n");
  const signature = crypto
    .sign("sha256", Buffer.from(canonical, "utf8"), {
      key: device.privateKey,
      dsaEncoding: "ieee-p1363",
    })
    .toString("base64url");

  const headers = {
    "content-type": "application/json",
    "x-request-timestamp": timestamp,
    "x-device-signature": signature,
  };
  // activate and auth/start send the key in the body instead
  if (withPubkeyId) headers["x-device-pubkey-id"] = pubkeyId();

  const res = await fetch(`${base}${pathname}`, { method: "POST", headers, body });
  const text = await res.text();
  let parsed;
  try {
    parsed = JSON.parse(text);
  } catch {
    parsed = text;
  }
  console.log(`POST ${pathname} -> ${res.status}`);
  console.dir(parsed, { depth: null });
  return { status: res.status, body: parsed };
}

function mutate(fingerprint, keys) {
  const next = { ...fingerprint };
  for (const k of keys) next[k] = randomHex(6);
  return next;
}

// commands
switch (command) {
  case "whoami":
    console.log(`pubkeyId    ${pubkeyId()}`);
    console.log(`publicKey   ${device.publicKeyRaw.toString("base64url")}`);
    console.dir(device.fingerprint, { depth: null });
    break;

  case "forget":
    fs.rmSync(KEY_FILE, { force: true });
    console.log("[sim] device identity deleted; the next command is a new machine");
    break;

  case "activate": {
    const licenseKey = flag("license");
    if (!licenseKey) {
      console.error("--license is required, e.g. --license ABCDE-FGHIJ-KLMNO-PQRST-UVWXY");
      process.exit(1);
    }
    const r = await signedPost(
      "/v1/desktop/activate",
      {
        licenseKey,
        devicePublicKey: device.publicKeyRaw.toString("base64url"),
        hardwareFingerprint: device.fingerprint,
      },
      { withPubkeyId: false },
    );
    process.exitCode = r.status === 201 ? 0 : 1;
    break;
  }

  case "heartbeat": {
    const r = await signedPost("/v1/desktop/heartbeat", {
      hardwareFingerprint: device.fingerprint,
    });
    process.exitCode = r.status === 200 ? 0 : 1;
    break;
  }

  // 3 of 6 still match -> served but flagged (abuse_flag row)
  case "drift": {
    const r = await signedPost("/v1/desktop/heartbeat", {
      hardwareFingerprint: mutate(device.fingerprint, [
        "volumeSerial",
        "primaryMac",
        "biosSerial",
      ]),
    });
    process.exitCode = r.status === 200 ? 0 : 1;
    break;
  }

  // should get 409 fingerprint_mismatch
  case "newmachine": {
    const r = await signedPost("/v1/desktop/heartbeat", {
      hardwareFingerprint: newFingerprint(),
    });
    process.exitCode = r.status === 409 ? 0 : 1;
    break;
  }

  case "deactivate": {
    const r = await signedPost("/v1/desktop/deactivate", {});
    process.exitCode = r.status === 200 ? 0 : 1;
    break;
  }

  case "auth-start": {
    const r = await signedPost(
      "/v1/desktop/auth/start",
      {
        devicePublicKey: device.publicKeyRaw.toString("base64url"),
        hardwareFingerprint: device.fingerprint,
      },
      { withPubkeyId: false },
    );
    if (r.status === 201) {
      console.log(
        `\nOpen ${r.body.verificationUrl} while signed in to the portal, approve it,\n` +
          `then: node scripts/desktop-client.mjs auth-poll --device-code ${r.body.deviceCode}`,
      );
    }
    process.exitCode = r.status === 201 ? 0 : 1;
    break;
  }

  case "auth-poll": {
    const deviceCode = flag("device-code");
    if (!deviceCode) {
      console.error("--device-code is required; auth-start prints it");
      process.exit(1);
    }
    const r = await signedPost("/v1/desktop/auth/poll", { deviceCode });
    process.exitCode = r.status === 200 ? 0 : 1;
    break;
  }

  default:
    console.error(`Unknown command "${command}". Run without arguments for the list.`);
    process.exit(1);
}
