#!/usr/bin/env node
// Generates components/icons.tsx from the Tabler Icons SVGs.
//   node scripts/gen-icons.mjs            regenerate
//   node scripts/gen-icons.mjs --check    fail if out of date
// Tabler isn't in the repo, point TABLER_SVG_DIR at an extracted release (only
// needed to regenerate, the output is committed).

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const APP = path.resolve(HERE, "..");
const OUT = path.join(APP, "components", "icons.tsx");

const TABLER_SVG_DIR =
  process.env.TABLER_SVG_DIR ??
  "../tabler-icons/svg";

// "arrow-right" -> ArrowRightIcon, or [tablerName, ExportName]
const ICONS = [
  "alert-triangle",
  "arrow-right",
  "bolt",
  "check",
  "cpu",
  "eraser",
  "gauge",
  "list-details",
  // menu-2, the normal one looks broken at 20px
  ["menu-2", "Menu"],
  "x",
  ["brand-discord", "Discord"],

  // feature areas (components/feature-icon.tsx)
  ["radar", "Detect"],
  ["first-aid-kit", "QuickFix"],
  ["lock", "Privacy"],
  ["device-desktop-analytics", "Gpu"],
  ["usb", "Peripheral"],
  ["network", "Network"],
  ["rocket", "Genesis"],

  // How it works
  "download",
  ["adjustments-horizontal", "Adjust"],
  "history",
];

const pascal = (name) =>
  name
    .split("-")
    .map((p) => p.charAt(0).toUpperCase() + p.slice(1))
    .join("");

function readIcon(tablerName) {
  for (const variant of ["outline", "filled"]) {
    const file = path.join(TABLER_SVG_DIR, variant, `${tablerName}.svg`);
    if (fs.existsSync(file)) {
      return { svg: fs.readFileSync(file, "utf8"), variant };
    }
  }
  throw new Error(
    `no Tabler icon named "${tablerName}" under ${TABLER_SVG_DIR}/{outline,filled}`,
  );
}

// Inner elements only, <Icon> sets viewBox/stroke/size. Skips Tabler's empty
// 24x24 bounding box path.
function extractChildren(svg) {
  const inner = svg.replace(/^[\s\S]*?<svg[^>]*>/, "").replace(/<\/svg>[\s\S]*$/, "");
  const nodes = inner.match(/<(path|circle|rect|line|polyline|polygon|ellipse)\b[^>]*\/>/g) ?? [];
  return nodes
    .filter((n) => !/d="M0 0h24v24H0z"/.test(n))
    .map((n) =>
      // kebab-case -> JSX props
      n
        .replace(/\bstroke-width=/g, "strokeWidth=")
        .replace(/\bstroke-linecap=/g, "strokeLinecap=")
        .replace(/\bstroke-linejoin=/g, "strokeLinejoin=")
        .replace(/\bfill-rule=/g, "fillRule=")
        .replace(/\bclip-rule=/g, "clipRule=")
        .replace(/\s*\/>$/, " />"),
    );
}

const entries = ICONS.map((entry) => {
  const [tablerName, alias] = Array.isArray(entry) ? entry : [entry, null];
  const { svg, variant } = readIcon(tablerName);
  return {
    tablerName,
    variant,
    exportName: `${alias ?? pascal(tablerName)}Icon`,
    children: extractChildren(svg),
  };
});

for (const e of entries) {
  if (e.children.length === 0) {
    throw new Error(`extracted no drawable nodes from "${e.tablerName}"`);
  }
}

const header = `// GENERATED FILE - do not edit by hand.
//
// Produced by scripts/gen-icons.mjs from the Tabler Icons SVG set. To add or
// change an icon, edit the ICONS list in that script and rerun it:
//
//   node scripts/gen-icons.mjs
//
// Icons are Tabler Icons, MIT licensed, (c) Tabler. The notice lives in
// legal/THIRD_PARTY_NOTICES.txt. Only the path data is Tabler's; the wrapper
// below is ours and owns sizing, stroke and accessibility so that every icon
// on the site answers to one set of decisions.

import type { SVGProps } from "react";

type IconProps = SVGProps<SVGSVGElement>;

function Icon({ children, ...props }: IconProps) {
  return (
    <svg
      xmlns="http://www.w3.org/2000/svg"
      viewBox="0 0 24 24"
      width={24}
      height={24}
      fill="none"
      stroke="currentColor"
      strokeWidth={2}
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden
      {...props}
    >
      {children}
    </svg>
  );
}
`;

const body = entries
  .map(
    (e) => `
/** Tabler \`${e.variant}/${e.tablerName}\`. */
export function ${e.exportName}(props: IconProps) {
  return (
    <Icon {...props}>
${e.children.map((c) => `      ${c}`).join("\n")}
    </Icon>
  );
}
`,
  )
  .join("");

const output = `${header}${body}`;

if (process.argv.includes("--check")) {
  const current = fs.existsSync(OUT) ? fs.readFileSync(OUT, "utf8") : "";
  if (current !== output) {
    console.error(
      "components/icons.tsx is out of date. Run: node scripts/gen-icons.mjs",
    );
    process.exit(1);
  }
  console.log(`icons: up to date (${entries.length} icons)`);
} else {
  fs.writeFileSync(OUT, output, "utf8");
  console.log(`icons: wrote ${entries.length} icons to components/icons.tsx`);
  for (const e of entries) {
    console.log(`  ${e.exportName.padEnd(20)} <- ${e.variant}/${e.tablerName}.svg`);
  }
}
