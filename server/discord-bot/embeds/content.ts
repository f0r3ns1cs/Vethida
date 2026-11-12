import { tierIncludes } from "@vethida/db";

import {
  WEBSITE_URL,
  PORTAL_URL,
  API_BASE_URL,
  TIERS,
  PURCHASABLE_TIERS,
  REFUND_WINDOW_DAYS,
} from "../lib/config";

export const RULES_CONTENT = {
  title: "Server Rules",
  description: "By participating in this server, you agree to the following:",
  fields: [
    { name: "1. Be Respectful", value: "Treat everyone with basic decency. Harassment, discrimination, hate speech, or personal attacks will result in an immediate ban.", inline: false },
    { name: "2. No Piracy or Key Sharing", value: "Do not share, request, or distribute license keys, cracked software, or pirated content. This applies to Vethida and all third-party software. No exceptions.", inline: false },
    { name: "3. No Spam or Self-Promotion", value: "Unsolicited links, repeated messages, and promotional content are not permitted without staff approval.", inline: false },
    { name: "4. Use Channels Appropriately", value: "Post in the channel that fits the topic. Off-topic conversation belongs in #off-topic.", inline: false },
    { name: "5. No NSFW Content", value: "Explicit, graphic, or otherwise inappropriate material is prohibited across all channels.", inline: false },
    { name: "6. Use Tickets for Support", value: "If you need help, use `/ticket` to open a support ticket. Don't DM or ping staff directly.", inline: false },
    { name: "7. No Reverse Engineering", value: "Decompiling, tampering with, or reverse engineering any Vethida product is strictly prohibited.", inline: false },
    { name: "8. Follow Discord's Terms of Service", value: "All members must comply with Discord's [Terms of Service](https://discord.com/terms) and [Community Guidelines](https://discord.com/guidelines).", inline: false },
    { name: "9. English Only", value: "All conversations must be in English so staff can moderate effectively.", inline: false },
    { name: "10. Staff Decisions Are Final", value: "Moderators may act on behavior that harms the community, even if it isn't explicitly covered above.", inline: false },
  ],
  footer: "Violations may result in a mute, kick, or permanent ban at staff discretion.",
};

export const ABOUT_CONTENT = {
  title: "About Vethida",
  description: [
    "Vethida is a Windows tweaking utility built by Rens Engineering.",
    "",
    "Every Windows installation ships with background telemetry, unnecessary services, and settings most people never see - let alone know how to change.",
  ].join("\n"),
  fields: [
    { name: "System Optimization", value: "Remove bloat, apply optimizations, and improve performance.", inline: true },
    { name: "Privacy Hardening", value: "Disable telemetry, tracking, and background data collection.", inline: true },
  ],
};

export const VERIFY_CONTENT = {
  title: "Link Your Vethida Account",
  description: [
    "To access the full server, link your Discord account to your Vethida account.",
    "",
    "**How it works:**",
    "1. Click the button below to open your Vethida dashboard",
    "2. Log in (or create an account) at my.vethida.com",
    "3. Click \"Link Discord\" in your dashboard",
    "4. Authorize the connection - your role will be assigned automatically",
    "",
    "Once linked, you'll gain access to all community channels and any tier-exclusive areas based on your license.",
  ].join("\n"),
  buttonLabel: "Link Account",
  buttonUrl: `${PORTAL_URL}/dashboard/settings`,
};

export const ROLES_CONTENT = {
  title: "Notification Roles",
  description: [
    "Choose which notifications you'd like to receive. Click the buttons below to toggle roles on or off.",
    "",
    "**Announcements** - Product releases, major updates, and company news",
    "**Changelog** - Version-by-version Vethida update notifications",
  ].join("\n"),
};

export const PURCHASE_CONTENT = {
  title: "Purchase Vethida",
  description: [
    "Current pricing is on the marketing site; every tier runs on one",
    "machine per license.",
    "",
    ...PURCHASABLE_TIERS.flatMap((slug) => [
      `**${TIERS[slug].label}** - ${tierIncludes(slug)}`,
      "",
    ]),
  ].join("\n"),
  fields: [
    { name: "What's included", value: "Access to paid Vethida features and community Discord access.", inline: false },
    { name: "How to purchase", value: `Visit [vethida.com/pricing](${WEBSITE_URL}/pricing) to see pricing and buy. Your license key arrives by email.`, inline: false },
    { name: "Please note", value: "License keys are non-transferable and bound to the original purchaser. Only purchase from official Vethida channels; third-party resellers are not supported.", inline: false },
  ],
};

export const VETHIDA_INFO_CONTENT = {
  title: "Vethida",
  description: [
    "Windows optimization, privacy and debloating.",
    "",
    "**Categories:**", // same order as vethida.com/lib/features.ts
    "Processor, memory and power - Graphics - Network and input - Debloat - Privacy and security - Repair and cleanup - Genesis - Health and recommendations",
  ].join("\n"),
  fields: [
    { name: "System requirements", value: "Windows 10 version 1903 or later, or Windows 11 - 64-bit (x64) processor.\nVethida runs with administrator rights; Windows prompts for them at launch.", inline: false },
  ],
};

export const GUIDES_GETTING_STARTED = {
  title: "Getting Started with Vethida",
  description: [
    "**1 - Download Vethida**",
    `Take the installer from [vethida.com/download](${WEBSITE_URL}/download). Download Vethida only from vethida.com.`,
    "",
    "**2 - Run the Installer**",
    "Windows asks for administrator rights before setup starts.",
    "",
    "**3 - Activate a License**",
    "Free needs no key. For Pro or Ultimate, open Vethida's Account page and enter the key from your purchase email. Activation binds the license to this machine.",
    "",
    "**4 - Start Using It**",
    "Read what each optimization does and apply what fits your setup.",
  ].join("\n"),
  fields: [
    { name: "Helpful Links", value: `[Download](${WEBSITE_URL}/download) - The installer\n[Lost key](${API_BASE_URL}/licenses/lost) - Resent to your purchase email\n#known-issues - Check here before reporting a bug\nUse \`/ticket\` to reach our support team directly`, inline: false },
  ],
};

export const FAQ_CONTENT = {
  title: "Frequently Asked Questions",
  fields: [
    { name: "How many PCs does a license cover?", value: "A license runs on one machine at a time. It has a limited number of activations, so a Windows reinstall or a new PC does not lock you out.", inline: false },
    { name: "Can I move my license to another PC?", value: "Deactivate the old PC from Vethida's Account page to free its activation, then enter your key on the new one.", inline: false },
    { name: "Is there a subscription?", value: "Vethida can be bought monthly, yearly, or as a one-time lifetime license. Subscriptions can be canceled any time from your dashboard; a lifetime license does not renew and has nothing to cancel. New accounts get a free trial. See vethida.com/pricing.", inline: false },
    { name: "Do you offer refunds?", value: `Refundable within ${REFUND_WINDOW_DAYS} days of the initial purchase of a paid license. It does not reset on renewal. After that, purchases are non-refundable except where the law requires otherwise. Run \`/refund\` for the full policy.`, inline: false },
    { name: "Can I upgrade my tier?", value: "Yes, you can move up to a higher tier at any time. See [vethida.com/pricing](https://vethida.com/pricing) for the current tiers and prices.", inline: false },
    { name: "What Windows versions are supported?", value: "Windows 10 version 1903 or later, or Windows 11, on a 64-bit (x64) processor.", inline: false },
    { name: "How do I get support?", value: "Use `/ticket` to open a support ticket. Check #known-issues first.", inline: false },
  ],
};

export const WELCOME_MESSAGE = (username: string) => ({
  title: `Welcome to Vethida, ${username}`,
  description: [
    "The Vethida community server.",
    "",
    "**Get started:**",
    "-> #rules - read before posting",
    "-> #verify - link your account to unlock the server",
    "-> #purchase - grab a license",
    "-> Use `/ticket` for direct support",
  ].join("\n"),
});
