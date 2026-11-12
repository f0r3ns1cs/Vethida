import { DEFAULT_ACTIVATION_LIMIT } from "./licenses.js";

const FROM = process.env.EMAIL_FROM ?? "Vethida <licenses@vethida.com>";
const REPLY_TO = process.env.EMAIL_REPLY_TO ?? "support@vethida.com";

const PLAN_NAMES = { month: "monthly", year: "yearly", lifetime: "lifetime" };

function tierName(tier) {
  return tier === "ultimate" ? "Ultimate" : "Pro";
}

async function send({ to, subject, text }) {
  const apiKey = process.env.RESEND_API_KEY;
  if (!apiKey) {
    console.warn(`email not sent (RESEND_API_KEY unset): "${subject}"`);
    return false;
  }
  const res = await fetch("https://api.resend.com/emails", {
    method: "POST",
    headers: { Authorization: `Bearer ${apiKey}`, "Content-Type": "application/json" },
    body: JSON.stringify({ from: FROM, to: [to], reply_to: REPLY_TO, subject, text }),
  });
  if (!res.ok) {
    console.error("email failed", res.status, await res.text());
    return false;
  }
  return true;
}

export async function sendLicenseEmail({ to, key, tier, plan }) {
  return send({
    to,
    subject: `Your Vethida ${tierName(tier)} license key`,
    text: [
      `Thanks for buying Vethida ${tierName(tier)} (${PLAN_NAMES[plan] ?? plan}).`,
      "",
      "Your license key:",
      "",
      `    ${key}`,
      "",
      "To activate it, open Vethida, go to Account, and paste the key.",
      // The EULA licenses one machine at a time.
      `It runs on one PC at a time and has ${DEFAULT_ACTIVATION_LIMIT} activations. Deactivate a PC from the same page to free one.`,
      "",
      "Download Vethida: https://vethida.com/download",
      // A lifetime license has no subscription to manage.
      ...(plan === "lifetime" ? [] : ["Manage your subscription: https://vethida.com/billing"]),
      "",
      "Questions? Reply to this email.",
    ].join("\n"),
  });
}

export async function sendKeysReminder({ to, keys }) {
  return send({
    to,
    subject: "Your Vethida license keys",
    text: [
      "Here are the Vethida license keys for this email address:",
      "",
      ...keys.map((k) => `    ${k.key}   (${tierName(k.tier)}, ${PLAN_NAMES[k.plan] ?? k.plan}, ${k.status})`),
      "",
      "Paste a key into Vethida's Account page to activate it.",
      "",
      "If you didn't ask for this, you can ignore this email.",
    ].join("\n"),
  });
}
