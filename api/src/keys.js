import crypto from "node:crypto";

const ALPHABET = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

function luhnMod32(input) {
  let factor = 2;
  let sum = 0;
  for (let i = input.length - 1; i >= 0; i--) {
    let addend = ALPHABET.indexOf(input.charAt(i)) * factor;
    factor = factor === 2 ? 1 : 2;
    addend = Math.floor(addend / 32) + (addend % 32);
    sum += addend;
  }
  return ALPHABET.charAt((32 - (sum % 32)) % 32);
}

export function generateLicenseKey() {
  const groups = [];
  for (let g = 0; g < 5; g++) {
    let group = "";
    for (let i = 0; i < 4; i++) group += ALPHABET[crypto.randomInt(32)];
    groups.push(group + luhnMod32(group));
  }
  return groups.join("-");
}

// Accepts lowercase, spaces, missing hyphens and Crockford's look-alikes
// (I and L read as 1, O as 0, U as V). Returns the canonical key, or null.
export function parseLicenseKey(raw) {
  if (typeof raw !== "string") return null;
  const normalized = raw
    .toUpperCase()
    .replace(/[\s-]/g, "")
    .replace(/[IL]/g, "1")
    .replace(/O/g, "0")
    .replace(/U/g, "V");
  if (!/^[0-9A-HJ-NP-TV-Z]{25}$/.test(normalized)) return null;
  const groups = normalized.match(/.{5}/g);
  if (!groups.every((g) => luhnMod32(g.slice(0, 4)) === g[4])) return null;
  return groups.join("-");
}

export function hashLicenseKey(canonicalKey) {
  return crypto.createHash("sha256").update(canonicalKey).digest("hex");
}

export function maskLicenseKey(canonicalKey) {
  return `${canonicalKey.slice(0, 5)}-*****-*****-*****-*****`;
}

function secretKey() {
  const secret = process.env.KEY_SECRET;
  if (!secret || secret.length < 32) {
    throw new Error("KEY_SECRET must be set to at least 32 characters");
  }
  return crypto.createHash("sha256").update(secret).digest();
}

export function encryptLicenseKey(canonicalKey) {
  const iv = crypto.randomBytes(12);
  const cipher = crypto.createCipheriv("aes-256-gcm", secretKey(), iv);
  const body = Buffer.concat([cipher.update(canonicalKey, "utf8"), cipher.final()]);
  return Buffer.concat([iv, cipher.getAuthTag(), body]).toString("base64");
}

export function decryptLicenseKey(stored) {
  const raw = Buffer.from(stored, "base64");
  const decipher = crypto.createDecipheriv("aes-256-gcm", secretKey(), raw.subarray(0, 12));
  decipher.setAuthTag(raw.subarray(12, 28));
  return Buffer.concat([decipher.update(raw.subarray(28)), decipher.final()]).toString("utf8");
}
