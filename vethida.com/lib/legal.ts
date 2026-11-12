import fs from "node:fs";
import path from "node:path";
import { Marked } from "marked";

import { legal } from "@/lib/site";

// adds heading ids (the docs link to their own sections). Own instance so the
// global marked isn't changed.
const renderer = new Marked({ gfm: true });
renderer.use({
  renderer: {
    heading({ tokens, depth }) {
      const text = this.parser.parseInline(tokens);
      const id = text
        .toLowerCase()
        .replace(/<[^>]+>/g, "")
        .replace(/[^a-z0-9]+/g, "-")
        .replace(/^-+|-+$/g, "");
      return `<h${depth} id="${id}">${text}</h${depth}>\n`;
    },
  },
});

// Reads the repo-root legal/ at build time.

export interface LegalDoc {
  slug: string;
  title: string;
  updated: string; // effective date
  description: string;
  path: string; // defaults to /legal/<slug>
  html: string;
}

// LEGAL_DIR is set in the container
const LEGAL_ROOT = process.env.LEGAL_DIR ?? path.join(process.cwd(), "..", "legal");

// simple `key: value` frontmatter, not real YAML
function parseFrontmatter(raw: string): {
  meta: Record<string, unknown>;
  body: string;
} {
  if (!raw.startsWith("---\n")) return { meta: {}, body: raw };
  const end = raw.indexOf("\n---\n", 4);
  if (end === -1) return { meta: {}, body: raw };

  const meta: Record<string, unknown> = {};
  for (const line of raw.slice(4, end).split("\n")) {
    const sep = line.indexOf(":");
    if (sep === -1) continue;
    const key = line.slice(0, sep).trim();
    const value = line.slice(sep + 1).trim();
    if (!key || !value) continue;
    try {
      meta[key] = JSON.parse(value);
    } catch {
      meta[key] = value;
    }
  }
  return { meta, body: raw.slice(end + 5) };
}

function str(v: unknown): string {
  return typeof v === "string" ? v : "";
}

// {{tokens}} in the docs. Unknown ones are left as-is so typos are visible.
const TOKENS: Record<string, string> = {
  entity: legal.entity,
  jurisdiction: legal.jurisdiction,
  effectiveDate: legal.effectiveDate,
  "email.support": legal.email.support,
  "email.privacy": legal.email.privacy,
  "email.security": legal.email.security,
  "email.legal": legal.email.legal,
};

function resolveTokens(body: string): string {
  return body.replace(/\{\{([a-zA-Z.]+)\}\}/g, (whole, key: string) =>
    key in TOKENS ? TOKENS[key] : whole,
  );
}

function loadDoc(file: string): LegalDoc {
  const raw = fs
    .readFileSync(path.join(LEGAL_ROOT, file), "utf-8")
    .replace(/\r\n/g, "\n");
  const { meta, body } = parseFrontmatter(raw);
  const slug = file.replace(/\.md$/, "");
  return {
    slug,
    path: str(meta.path) || `/legal/${slug}`,
    title: str(meta.title) || file,
    updated: str(meta.effective) || legal.effectiveDate,
    description: str(meta.description),
    html: renderer.parse(resolveTokens(body), { async: false }),
  };
}

// everything except docs with their own `path`
export function listLegalRouteDocs(): LegalDoc[] {
  return listLegalDocs().filter((doc) => doc.path === `/legal/${doc.slug}`);
}

export function listLegalDocs(): LegalDoc[] {
  if (!fs.existsSync(LEGAL_ROOT)) return [];
  return fs
    .readdirSync(LEGAL_ROOT)
    .filter((f) => f.endsWith(".md"))
    .map(loadDoc)
    .sort((a, b) => a.slug.localeCompare(b.slug));
}

export function getLegalDoc(slug: string): LegalDoc | null {
  // slug is from the URL, no path traversal
  if (!/^[a-z0-9-]+$/.test(slug)) return null;
  const file = path.join(LEGAL_ROOT, `${slug}.md`);
  if (!fs.existsSync(file)) return null;
  return loadDoc(`${slug}.md`);
}
