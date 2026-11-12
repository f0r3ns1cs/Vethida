// Email renderer used by the api and the portal. Templates live in each app's
// own emails/ folder (relative to cwd).

import { readFileSync } from "node:fs";
import { join } from "node:path";

const TEMPLATE_DIR = join(process.cwd(), "emails");
const HEADER_FIELDS = ["From", "Reply-To", "Subject", "Preview"] as const;

export interface RenderedEmail {
  from: string;
  replyTo: string;
  subject: string;
  preview: string;
  text: string;
  html: string;
}

const cache = new Map<string, string>();

function loadTemplate(name: string): string {
  if (process.env.NODE_ENV !== "production") {
    return readFileSync(join(TEMPLATE_DIR, `${name}.md`), "utf8");
  }
  let cached = cache.get(name);
  if (!cached) {
    cached = readFileSync(join(TEMPLATE_DIR, `${name}.md`), "utf8");
    cache.set(name, cached);
  }
  return cached;
}

function substitute(source: string, vars: Record<string, string>): string {
  return source.replace(/\$\{(\w+)\}/g, (_, key) => {
    const v = vars[key];
    if (v === undefined) {
      throw new Error(`email template referenced unknown variable: ${key}`);
    }
    return v;
  });
}

// strip newlines so a variable can't inject another header
function sanitizeHeader(value: string, field: string): string {
  const flattened = value.replace(/[\r\n]+/g, " ").trim();
  if (flattened.length > 512) {
    throw new Error(`email header "${field}" is implausibly long`);
  }
  return flattened;
}

function escapeHtml(s: string): string {
  return s
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#39;");
}

const EMAIL = {
  bg: "#f4f6fa",
  card: "#ffffff",
  border: "#e8ebee",
  text: "#212936",
  muted: "#667382",
  accent: "#066fd1",
  font: "'Inter',-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif",
};

function linkify(escaped: string): string {
  return escaped.replace(
    /https?:\/\/[^\s<]+[^\s<.,)]/g,
    (url) =>
      `<a href="${url}" style="color:${EMAIL.accent};text-decoration:underline;">${url}</a>`,
  );
}

function button(label: string, url: string): string {
  return (
    `<table role="presentation" cellpadding="0" cellspacing="0" style="margin:8px 0 20px;">` +
    `<tr><td style="border-radius:8px;background:${EMAIL.accent};">` +
    `<a href="${url}" style="display:inline-block;padding:12px 24px;font-family:${EMAIL.font};font-size:15px;font-weight:600;color:#ffffff;text-decoration:none;">${escapeHtml(label)}</a>` +
    `</td></tr></table>`
  );
}

// Supports: "## " headings, "- " lists, paragraphs, [Label](url) or a bare url
// on its own line as a button, and inline links.
const BUTTON_LINE = /^\[([^\]]+)\]\((https?:\/\/[^\s)]+)\)$/;

function renderBody(text: string): string {
  const out: string[] = [];
  let para: string[] = [];
  let list: string[] = [];
  const flushPara = () => {
    if (para.length) {
      out.push(
        `<p style="margin:0 0 16px;font-size:15px;line-height:1.6;color:${EMAIL.text};">${linkify(escapeHtml(para.join(" ")))}</p>`,
      );
      para = [];
    }
  };
  const flushList = () => {
    if (list.length) {
      const items = list
        .map(
          (li) => `<li style="margin:0 0 6px;">${linkify(escapeHtml(li))}</li>`,
        )
        .join("");
      out.push(
        `<ul style="margin:0 0 16px;padding-left:20px;font-size:15px;line-height:1.6;color:${EMAIL.text};">${items}</ul>`,
      );
      list = [];
    }
  };
  for (const raw of text.split("\n")) {
    const line = raw.trim();
    if (line === "") {
      flushPara();
      flushList();
    } else if (line.startsWith("## ")) {
      flushPara();
      flushList();
      out.push(
        `<h2 style="margin:24px 0 12px;font-size:17px;font-weight:600;color:${EMAIL.text};">${escapeHtml(line.slice(3))}</h2>`,
      );
    } else if (line.startsWith("- ")) {
      flushPara();
      list.push(line.slice(2));
    } else if (BUTTON_LINE.test(line)) {
      flushPara();
      flushList();
      const [, label, url] = line.match(BUTTON_LINE)!;
      out.push(button(label, url));
    } else if (/^https?:\/\/\S+$/.test(line)) {
      flushPara();
      flushList();
      out.push(button("Open Vethida", line));
    } else {
      flushList();
      para.push(line);
    }
  }
  flushPara();
  flushList();
  return out.join("");
}

// hidden preview text. The &zwnj; padding stops clients from appending the body after it
function preheader(text: string): string {
  return (
    `<div style="display:none;max-height:0;overflow:hidden;mso-hide:all;font-size:1px;line-height:1px;color:transparent;opacity:0;">` +
    escapeHtml(text) +
    "&zwnj;&nbsp;".repeat(60) +
    `</div>`
  );
}

// charset is needed or Outlook mangles apostrophes. color-scheme keeps dark mode clients
// from recoloring everything
const HEAD =
  `<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />` +
  `<meta name="viewport" content="width=device-width, initial-scale=1.0" />` +
  `<meta name="x-apple-disable-message-reformatting" />` +
  `<meta content="telephone=no" name="format-detection" />` +
  `<meta name="color-scheme" content="light dark" />` +
  `<meta name="supported-color-schemes" content="light dark" />` +
  `<style>:root{color-scheme:light dark;supported-color-schemes:light dark;}` +
  `a[x-apple-data-detectors]{color:inherit!important;text-decoration:none!important;}</style>`;

function toHtml(text: string, previewText: string): string {
  const [main, foot] = text.split(/^----\s*$/m);
  const body = renderBody(main.trim());
  const footer = foot
    ? `<p style="margin:0;font-size:12px;line-height:1.6;color:${EMAIL.muted};">${linkify(escapeHtml(foot.trim())).replace(/\n/g, "<br />")}</p>`
    : "";
  return (
    `<!DOCTYPE html><html lang="en" style="color-scheme:light dark;supported-color-schemes:light dark;">` +
    `<head>${HEAD}<title>Vethida</title></head>` +
    `<body style="margin:0;padding:0;background:${EMAIL.bg};">` +
    preheader(previewText) +
    `<table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background:${EMAIL.bg};"><tr><td align="center" style="padding:32px 16px;">` +
    `<table role="presentation" width="560" cellpadding="0" cellspacing="0" style="max-width:560px;width:100%;background:${EMAIL.card};border:1px solid ${EMAIL.border};border-radius:14px;overflow:hidden;">` +
    `<tr><td style="padding:22px 32px;border-bottom:1px solid ${EMAIL.border};"><span style="font-family:${EMAIL.font};font-size:18px;font-weight:700;letter-spacing:-0.02em;color:${EMAIL.text};">Vethida</span></td></tr>` +
    `<tr><td style="padding:28px 32px;font-family:${EMAIL.font};">${body}</td></tr>` +
    (footer
      ? `<tr><td style="padding:20px 32px;border-top:1px solid ${EMAIL.border};background:#fafbfc;font-family:${EMAIL.font};">${footer}</td></tr>`
      : "") +
    `</table></td></tr></table></body></html>`
  );
}

// footer is "Rens Engineering | address | url", drop the empty bits when address isn't set
function tidyFooterSeparators(text: string): string {
  return text
    .split("\n")
    .map((line) => {
      if (!line.includes("|")) return line;
      const parts = line.split("|").map((p) => p.trim());
      const kept = parts.filter((p) => p.length > 0);
      if (kept.length === parts.length) return line;
      return kept.join(" | ");
    })
    .join("\n");
}

export function renderTemplate(
  name: string,
  vars: Record<string, string>,
): RenderedEmail {
  const source = loadTemplate(name);
  const lines = source.split(/\r?\n/);

  const headers: Record<string, string> = {};
  let bodyStart = 0;
  for (let i = 0; i < lines.length; i++) {
    if (lines[i] === "") {
      bodyStart = i + 1;
      break;
    }
    const m = lines[i].match(/^([\w-]+):\s*(.*)$/);
    if (!m) {
      bodyStart = i;
      break;
    }
    headers[m[1]] = m[2];
  }

  for (const field of HEADER_FIELDS) {
    if (!headers[field]) {
      throw new Error(`template ${name} is missing header "${field}"`);
    }
  }

  const text = tidyFooterSeparators(
    substitute(lines.slice(bodyStart).join("\n").trim(), vars),
  );
  const preview = sanitizeHeader(substitute(headers.Preview, vars), "Preview");

  return {
    from: sanitizeHeader(substitute(headers.From, vars), "From"),
    replyTo: sanitizeHeader(substitute(headers["Reply-To"], vars), "Reply-To"),
    subject: sanitizeHeader(substitute(headers.Subject, vars), "Subject"),
    preview,
    text,
    html: toHtml(text, preview),
  };
}
