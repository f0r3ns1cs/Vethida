import { lifetimeScope } from "@/lib/pricing";
import { activationLimit, codeSigned } from "@/lib/site";

export type Faq = { q: string; a: string };

/** Stable key for a section. Other pages select by this, never by `title`. */
export type FaqSectionId = "safety" | "plans" | "account";

type FaqSection = { id: FaqSectionId; title: string; faqs: Faq[] };

export const faqSections: FaqSection[] = [
  {
    id: "safety",
    title: "Safety and scope",
    faqs: [
      {
        q: "What if something goes wrong?",
        a: "Open the Restore page. It lists the changes Vethida made, and most can be undone from there. It can also create a Windows restore point: create one before you apply a large set of changes, and you can take Windows back to it if something disagrees with your hardware.",
      },
      {
        q: "Can it disable Defender, SmartScreen, UAC or Windows Update?",
        a: "Yes, all four, from the Privacy & security section. The recommended set leaves every one of them on, so you only turn one off by choosing it yourself.",
      },
      {
        q: "Do I need administrator rights?",
        a: "Yes. Vethida changes system settings, so Windows asks for administrator rights when you open it.",
      },
      {
        q: "Will my antivirus flag it?",
        a: `It can happen. Antivirus products flag optimization tools generically, because the settings one changes are the settings some malware changes. ${codeSigned ? "Vethida is code-signed, and the download page publishes the SHA-256 of the build we signed." : "The download page publishes the SHA-256 of every installer, so you can check that the file you have is the one we released."}`,
      },
      {
        q: "Do I have to redo this after a Windows update?",
        a: "Sometimes. A Windows feature update can put some settings back to their defaults. Vethida reads the live state of your settings each time it opens, so the Health score and the settings list show what Windows changed back, and you can apply them again.",
      },
      {
        q: "How do I uninstall it?",
        a: "Remove it from Windows Settings like any other application. Everything you applied stays applied afterwards, so turn off anything you do not want to keep before you uninstall.",
      },
    ],
  },
  {
    id: "plans",
    title: "Plans and licensing",
    faqs: [
      {
        q: "Is Free actually free?",
        a: "Yes. No account, no card and no expiry. Pro and Ultimate use a license key.",
      },
      {
        q: "What is the difference between Free and Pro?",
        a: "Free covers the low-risk settings and the system tools. Pro adds the processor, memory, graphics, network, privacy and debloating settings.",
      },
      {
        q: "Which plan do I need, Pro or Ultimate?",
        a: "Ultimate adds Genesis, deep power management, NVIDIA clock and power control, NVMe driver settings and the low-latency power plan, and some of those raise power draw and heat. If you are not sure, Pro is the one to start on.",
      },
      {
        q: "What happens when my plan renews?",
        a: "Monthly and yearly plans are charged again at the start of each term until you cancel. We email you at least 30 days before any price change.",
      },
      {
        q: "What does a lifetime license include?",
        a: `One payment, and it never expires. ${lifetimeScope}`,
      },
      {
        q: "What if I change my mind?",
        a: "You can get a refund within 14 days of your first purchase, on any plan. Email support@vethida.com from the address you bought with.",
      },
      {
        q: "What happens when my subscription ends?",
        a: "Everything you already applied stays on your PC and keeps working, and Vethida drops to Free. Anything Pro or Ultimate adds needs a live license again.",
      },
      {
        q: "Can I move from Pro to Ultimate later?",
        a: "Yes. Buy Ultimate and enter its key in Vethida's Account page. Then email support@vethida.com and we cancel your Pro plan and refund the part of it you had not used yet.",
      },
    ],
  },
  {
    id: "account",
    title: "License and machines",
    faqs: [
      {
        q: "Where is my license key?",
        a: "On the page you see right after checkout, and in the email we send you. Lost it? We can resend it to the email you bought with at api.vethida.com/licenses/lost. Paste it into Vethida's Account page to activate.",
      },
      {
        q: "How many PCs does a license cover?",
        a: `Each license has ${activationLimit} activations, so reinstalling Windows or moving to a new PC never locks you out. Deactivate a PC from Vethida's Account page to free its activation.`,
      },
      {
        q: "Does it work offline?",
        a: "Free works offline. Pro and Ultimate need a connection: Vethida checks your license about once an hour, and without one it runs as Free until it can check again. Changes already applied stay in place, and your plan returns as soon as the connection does.",
      },
    ],
  },
];

/** Flat list, for structured data and for any surface that ignores grouping. */
export const allFaqs: Faq[] = faqSections.flatMap((section) => section.faqs);

/** The section with this id. Throws at module scope, so a bad id fails the build. */
export function faqSection(id: FaqSectionId): Faq[] {
  const section = faqSections.find((s) => s.id === id);
  if (!section) throw new Error(`lib/faqs.ts has no section with id "${id}"`);
  return section.faqs;
}
