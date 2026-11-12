import crypto from "node:crypto";

// Keys are 5 groups of 5: 4 random Crockford base32 chars + 1 Luhn mod 32 check char.
// Always hash the canonical form (uppercase, with hyphens).
// Note: the check char misses a 0<->Z swap, same blind spot decimal Luhn has with 0/9.
const BASE32_CHARS = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

function luhnMod32(input: string): string {
  let factor = 2;
  let sum = 0;
  for (let i = input.length - 1; i >= 0; i--) {
    let addend = BASE32_CHARS.indexOf(input.charAt(i)) * factor;
    factor = factor === 2 ? 1 : 2;
    addend = Math.floor(addend / 32) + (addend % 32);
    sum += addend;
  }
  return BASE32_CHARS.charAt((32 - (sum % 32)) % 32);
}

export function generateLicenseKey(): string {
  const groups: string[] = [];
  for (let g = 0; g < 5; g++) {
    let group = "";
    for (let i = 0; i < 4; i++) {
      group += BASE32_CHARS[crypto.randomInt(32)];
    }
    groups.push(group + luhnMod32(group));
  }
  return groups.join("-");
}

export function hashLicenseKey(canonicalKey: string): string {
  return crypto.createHash("sha256").update(canonicalKey).digest("hex");
}

// lenient: lowercase, spaces, no hyphens, I/L->1, O->0, U->V
export function parseLicenseKey(raw: string): string | null {
  if (typeof raw !== "string") return null;

  const normalized = raw
    .toUpperCase()
    .replace(/[\s-]/g, "")
    .replace(/[IL]/g, "1")
    .replace(/O/g, "0")
    .replace(/U/g, "V");

  if (!/^[0-9A-HJ-NP-TV-Z]{25}$/.test(normalized)) return null;

  const groups = normalized.match(/.{5}/g)!;
  if (!groups.every((g) => luhnMod32(g.slice(0, 4)) === g[4])) return null;

  return groups.join("-");
}

export function formatLicenseKey(raw: string): string {
  return parseLicenseKey(raw) ?? raw;
}
