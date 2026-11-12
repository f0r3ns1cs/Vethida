/** Canonical origin, no trailing slash. */
export const siteUrl = "https://vethida.com";

/** Organization logo for structured data. */
export const siteLogo = `${siteUrl}/icon-256.png`;

export const siteTitle = "Vethida: Advanced Windows Optimization Tool";
// meta description, OG card, JSON-LD and manifest
export const siteDescription =
  "Vethida optimizes the drivers, power settings and Windows internals behind a PC's performance, then strips out the bloatware and telemetry it shipped with.";

export const site = {
  name: "Rens Engineering",
  product: "Vethida",
  links: {
    discord: "https://discord.gg/CjnYVU9cFb",
    discordDisplay: "discord.gg/CjnYVU9cFb",
    // /billing goes to the Stripe customer portal, lost keys get resent by the API
    billing: "https://vethida.com/billing",
    licenses: "https://api.vethida.com/licenses/lost",
  },
} as const;

const BUY_VERB = "Buy";

export const buyLabel = `${BUY_VERB} now`;

// plan cards
export function buyPlanLabel(plan: string): string {
  return `${BUY_VERB} ${plan}`;
}

export const codeSigned = false; // the FAQ reads this

// has to match api/src/licenses.js
export const activationLimit = 3;

export const requirements = {
  os: "Windows 10 version 1903 or later, or Windows 11",
  arch: "64-bit (x64) processor",
  combined: "Windows 10 (version 1903 or later); Windows 11; x64", // JSON-LD wants one string
} as const;

// days, must match legal/refunds.md
export const refundWindowDays = 14;

export const legal = {
  entity: "Rens Engineering",
  jurisdiction: "Pennsylvania, United States",
  // used by every legal doc unless its frontmatter sets `effective`
  effectiveDate: "September 30, 2026",
  email: {
    contact: "contact@vethida.com",
    support: "support@vethida.com",
    privacy: "privacy@vethida.com",
    security: "security@vethida.com",
    legal: "legal@vethida.com",
  },
} as const;

// nav for both the desktop bar and the mobile menu
type NavItem = {
  href: string;
  label: string;
  hint?: string; // dropdown subtitle
};

export type NavGroup = {
  label: string;
  href?: string; // plain link, no items
  items?: readonly NavItem[];
};

export const primaryNav: readonly NavGroup[] = [
  {
    label: "Product",
    items: [
      { href: "/features", label: "Features", hint: "What the app does" },
      { href: "/changelog", label: "Release notes", hint: "What changed" },
    ],
  },
  { label: "Pricing", href: "/pricing" },
  { label: "Download", href: "/download" },
  {
    label: "Resources",
    items: [
      { href: "/faq", label: "FAQ", hint: "Common questions" },
      { href: "/support", label: "Support", hint: "Get help" },
    ],
  },
  { label: "About", href: "/about" },
] as const;

function navItems(label: string): readonly NavItem[] {
  const group = primaryNav.find((g) => g.label === label);
  if (!group?.items) throw new Error(`primaryNav has no group named "${label}"`);
  return group.items;
}

function navLink(label: string): NavItem {
  const group = primaryNav.find((g) => g.label === label);
  if (!group?.href) throw new Error(`primaryNav has no link named "${label}"`);
  return { href: group.href, label: group.label };
}

// the header button, reuses the nav's Download entry
export const downloadCta: NavItem = navLink("Download");

function navItem(label: string): NavItem {
  const item = primaryNav.flatMap((g) => g.items ?? []).find((i) => i.label === label);
  if (!item) throw new Error(`primaryNav has no item named "${label}"`);
  return item;
}

export type FooterLink = NavItem & {
  external?: boolean; // new tab
  icon?: "discord";
};

export type FooterColumn = { label: string; items: readonly FooterLink[] };

// built from primaryNav so labels/hrefs stay the same, just grouped differently
export const footerNav: readonly FooterColumn[] = [
  {
    label: "Product",
    items: [navItem("Features"), navLink("Pricing"), navItem("Release notes")],
  },
  {
    label: "Resources",
    items: [
      ...navItems("Resources"),
      {
        href: site.links.discord,
        label: "Discord",
        external: true,
        icon: "discord",
      },
    ],
  },
  { label: "Company", items: [navLink("About")] },
] as const;

// footer legal links (the desktop app links /legal/third-party)
export const legalNav = [
  { href: "/legal/privacy", label: "Privacy Policy" },
  { href: "/legal/terms", label: "Terms of Service" },
  { href: "/legal/eula", label: "EULA" },
  { href: "/legal/cookies", label: "Cookie Policy" },
  { href: "/legal/refunds", label: "Refund Policy" },
  { href: "/legal/subscription-terms", label: "Subscription Terms" },
  { href: "/legal/third-party", label: "Third-Party Notices" },
  // security.txt points here
  { href: "/security", label: "Report a security issue" },
] as const;
