const CREDENTIAL_PATTERNS: Array<{ name: string; pattern: RegExp }> = [
  { name: "discord_bot_token", pattern: /[MNOZ][A-Za-z0-9_-]{23,27}\.[A-Za-z0-9_-]{6,7}\.[A-Za-z0-9_-]{27,}/ },
  { name: "discord_mfa_token", pattern: /mfa\.[A-Za-z0-9_-]{84,}/ },
  { name: "aws_access_key", pattern: /\bAKIA[0-9A-Z]{16}\b/ },
  { name: "google_api_key", pattern: /\bAIza[0-9A-Za-z_-]{35}\b/ },
  { name: "github_token", pattern: /\bghp_[A-Za-z0-9]{36}\b/ },
  { name: "stripe_secret", pattern: /\bsk_(live|test)_[A-Za-z0-9]{24,}\b/ },
  { name: "slack_token", pattern: /\bxox[baprs]-[A-Za-z0-9-]{10,}\b/ },
  { name: "private_key_block", pattern: /-----BEGIN (?:RSA |EC |DSA |OPENSSH )?PRIVATE KEY-----/ },
];

export function detectCredentialLeak(content: string): string | null {
  for (const { name, pattern } of CREDENTIAL_PATTERNS) {
    if (pattern.test(content)) {
      return name;
    }
  }
  return null;
}

// no g flag, test() with g keeps lastIndex between calls
const INVITE_REGEX =
  /(?:https?:\/\/)?(?:www\.)?(?:discord(?:\.gg|app\.com\/invite|\.com\/invite)|dsc\.gg|invite\.gg)\/[a-z0-9-]+/i;

export function hasDiscordInvite(content: string): boolean {
  return INVITE_REGEX.test(content);
}

const KNOWN_PHISH_DOMAINS = new Set<string>([
  "discocl.com",
  "discrocl.com",
  "discord-gift.com",
  "discorcl.com",
  "discordapp.io",
  "discord-nitro.co",
  "dlscord.gift",
  "d1scordnitro.com",
  "steamcommunily.com",
  "steamcommmunity.com",
  "stearncommunity.com",
  "steampowercd.com",
  "steampowered-trade.com",
  "discord-airdrop.com",
  "free-nitro.ru",
]);

const URL_REGEX = /\bhttps?:\/\/([^\s<>"'`]+)/gi;

export function extractHosts(content: string): string[] {
  const hosts: string[] = [];
  for (const match of content.matchAll(URL_REGEX)) {
    const raw = match[1];
    if (!raw) {
      continue;
    }
    try {
      const host = new URL(`https://${raw}`).hostname.toLowerCase();
      hosts.push(host);
    } catch {
      // bad url, skip
    }
  }
  return hosts;
}

export function detectPhishing(content: string): string | null {
  for (const host of extractHosts(content)) {
    for (const bad of KNOWN_PHISH_DOMAINS) {
      if (host === bad || host.endsWith(`.${bad}`)) {
        return host;
      }
    }
  }
  return null;
}

export function isExcessiveMentions(
  mentionUserCount: number,
  mentionRoleCount: number,
  mentionEveryone: boolean
): boolean {
  if (mentionEveryone) {
    return true;
  }
  return mentionUserCount + mentionRoleCount > 5;
}

export function isExcessiveCaps(content: string, minLength = 12, ratio = 0.7): boolean {
  const letters = content.replace(/[^A-Za-z]/g, "");
  if (letters.length < minLength) {
    return false;
  }
  const upper = letters.replace(/[^A-Z]/g, "").length;
  return upper / letters.length >= ratio;
}

// strips control/bidi chars and caps the length (keeps newlines and tabs)
export function sanitizeDisplay(input: string, maxLength = 1024): string {
  const cleaned = input
    .replace(/\r\n?/g, "\n")
    // eslint-disable-next-line no-control-regex
    .replace(/[\u0000-\u0008\u000b\u000c\u000e-\u001f\u007f]/g, "")
    .replace(/[\u200B-\u200F\u202A-\u202E\u2060-\u206F\uFEFF]/g, "")
    .replace(/\n{3,}/g, "\n\n")
    .trim();

  if (cleaned.length <= maxLength) {
    return cleaned;
  }
  return maxLength <= 3
    ? cleaned.slice(0, maxLength)
    : `${cleaned.slice(0, maxLength - 3)}...`;
}

export function escapeMarkdown(input: string): string {
  return input
    .replace(/@(everyone|here)/g, "@\u200B$1")
    .replace(/`/g, "\\`")
    .replace(/\*/g, "\\*")
    .replace(/_/g, "\\_")
    .replace(/~/g, "\\~")
    .replace(/\|/g, "\\|");
}
