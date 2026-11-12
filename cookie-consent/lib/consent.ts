// Cookie consent state. Nothing is stored until the visitor picks an option, a
// refusal is stored too, and bumping CONSENT_VERSION re-asks everyone.

const CONSENT_VERSION = 2 as const;
export const CONSENT_COOKIE = "cookie_consent";

// essential cookies don't need consent so they're not a category here.
// No advertising category since there isn't any.
export const CONSENT_CATEGORIES = ["functional", "performance"] as const;

type ConsentCategory = (typeof CONSENT_CATEGORIES)[number];

export type ConsentChoices = Record<ConsentCategory, boolean>;

export interface ConsentState extends ConsentChoices {
  version: number;
}

export const ALL_ON: ConsentChoices = { functional: true, performance: true };
export const ALL_OFF: ConsentChoices = { functional: false, performance: false };

// e.g. v2-f1-p0
const TOKEN = /^v(\d+)-f([01])-p([01])$/;

function encode(choices: ConsentChoices): string {
  return `v${CONSENT_VERSION}-f${choices.functional ? 1 : 0}-p${
    choices.performance ? 1 : 0
  }`;
}

// null = hasn't answered yet (client side only)
export function readConsent(): ConsentState | null {
  if (typeof document === "undefined") return null;

  const raw = document.cookie
    .split("; ")
    .find((c) => c.startsWith(`${CONSENT_COOKIE}=`))
    ?.slice(CONSENT_COOKIE.length + 1);
  if (!raw) return null;

  const m = TOKEN.exec(raw);
  if (!m) return null; // unreadable -> ask again

  const version = Number(m[1]);
  if (version !== CONSENT_VERSION) return null;

  return { version, functional: m[2] === "1", performance: m[3] === "1" };
}

export function writeConsent(choices: ConsentChoices): void {
  if (typeof document === "undefined") return;

  const sixMonths = 60 * 60 * 24 * 183;
  // no Secure flag on http (local dev)
  const secure =
    typeof location !== "undefined" && location.protocol === "https:"
      ? "; Secure"
      : "";

  document.cookie = `${CONSENT_COOKIE}=${encode(
    choices,
  )}; path=/; max-age=${sixMonths}; SameSite=Lax${secure}`;
}

export function clearConsent(): void {
  if (typeof document === "undefined") return;
  document.cookie = `${CONSENT_COOKIE}=; path=/; max-age=0; SameSite=Lax`;
}
