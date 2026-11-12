#!/usr/bin/env node

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const read = (rel) => fs.readFileSync(path.join(root, rel), "utf8");

const items = [
  {
    file: "vethida.com/lib/pricing.ts",
    what: "the six checkout links (Stripe Payment Links)",
    from: "Stripe dashboard > Payment Links > each link > Copy link",
    missing: (src) => src.includes("REPLACE-"),
  },
  {
    file: "vethida.com/next.config.ts",
    what: "the Stripe customer portal link behind vethida.com/billing",
    from: "Stripe dashboard > Settings > Billing > Customer portal > Login link",
    missing: (src) => src.includes("REPLACE-PORTAL-LINK"),
  },
  {
    file: "Vethida/vth_core/Runtime/AutoUpdate.h",
    what: "update signing public key (kEdDsaPublicKey)",
    from: "node vethida.com/scripts/appcast.mjs keys, on your own PC; keep the private key out of the repository",
    missing: (src) => /kEdDsaPublicKey\s*=\s*""/.test(src),
  },
  {
    file: "vethida.com/lib/site.ts",
    what: "legal effective date (release day, on or after the company's formation date)",
    from: "vethida.com/lib/site.ts effectiveDate, scripts/eula.txt and legal/THIRD_PARTY_NOTICES.txt, all the same date",
    missing: (src) => {
      const date = src.match(/effectiveDate:\s*"([^"]+)"/)?.[1] ?? "";
      const others = ["scripts/eula.txt", "legal/THIRD_PARTY_NOTICES.txt"]
        .filter((rel) => fs.existsSync(path.join(root, rel)))
        .map(read);
      return date === "September 30, 2026" || others.some((t) => !t.includes(date));
    },
  },
  {
    file: "Vethida/vth_core/Services/SecurityConfig.h",
    what: "Sentry DSN for opt-in crash reports",
    from: "sentry.io > your project > Settings > Client Keys (DSN)",
    missing: (src) => src.includes("SENTRY-DSN-PLACEHOLDER"),
  },
];

let open = 0;
for (const item of items) {
  const missing = item.missing(read(item.file));
  if (missing) open++;
  console.log(`${missing ? "  todo" : "  ok  "}  ${item.what}`);
  if (missing) console.log(`        in ${item.file}\n        from ${item.from}`);
}

if (open) {
  console.log(`\nrelease-ready: ${open} of ${items.length} still to fill in`);
  process.exit(1);
}
console.log("\nrelease-ready: ok");
