import http from "node:http";

import { openDatabase } from "./db.js";
import { sendKeysReminder } from "./email.js";
import {
  activate,
  deactivate,
  effectiveStatus,
  findByEmail,
  findByKey,
  revealKey,
  validate,
} from "./licenses.js";
import { errorPage, lostKeyPage, pendingPage, successPage } from "./pages.js";
import { allow } from "./ratelimit.js";
import { constructEvent, fulfillSession, handleEvent, refreshIfLapsed } from "./stripe.js";

// api.vethida.com. Routes:
//   POST /v1/licenses/activate|validate|deactivate     desktop app
//   POST /webhooks/stripe                              Stripe events
//   GET  /checkout/success?session_id=...              key after checkout
//   GET  /licenses/lost, POST /licenses/lost           resend lost keys
//   GET  /health                                       uptime checks

const PORT = Number(process.env.PORT ?? 8080);
const MAX_BODY = 1024 * 1024;

const PAGE_HEADERS = {
  "Content-Type": "text/html; charset=utf-8",
  "Cache-Control": "no-store",
  "Content-Security-Policy":
    "default-src 'none'; style-src 'unsafe-inline'; img-src 'self'; form-action 'self'; frame-ancestors 'none'; base-uri 'none'",
  "Referrer-Policy": "no-referrer",
  "X-Content-Type-Options": "nosniff",
};

function clientIp(req) {
  const forwarded = String(req.headers["x-forwarded-for"] ?? "").split(",")[0].trim();
  return req.headers["cf-connecting-ip"] || forwarded || req.socket.remoteAddress || "unknown";
}

function readBody(req) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    let size = 0;
    req.on("data", (c) => {
      size += c.length;
      if (size > MAX_BODY) {
        reject(Object.assign(new Error("body too large"), { status: 413 }));
        req.destroy();
      } else chunks.push(c);
    });
    req.on("end", () => resolve(Buffer.concat(chunks)));
    req.on("error", reject);
  });
}

function parseFields(req, raw) {
  const type = String(req.headers["content-type"] ?? "");
  if (type.includes("application/json")) {
    try {
      return JSON.parse(raw.toString("utf8") || "{}");
    } catch {
      return {};
    }
  }
  return Object.fromEntries(new URLSearchParams(raw.toString("utf8")));
}

function json(res, status, body) {
  res.writeHead(status, { "Content-Type": "application/json", "Cache-Control": "no-store" });
  res.end(JSON.stringify(body));
}

function html(res, status, body) {
  res.writeHead(status, PAGE_HEADERS);
  res.end(body);
}

async function licenseRoute(req, res, action) {
  const ip = clientIp(req);
  const limit = action === "validate" ? 120 : 30;
  if (!allow(`${action}:${ip}`, limit, 60 * 1000)) {
    return json(res, 429, { error: "Too many requests. Try again in a minute." });
  }
  const fields = parseFields(req, await readBody(req));
  const key = fields.license_key;

  const existing = findByKey(key);
  if (existing) await refreshIfLapsed(existing);

  let result;
  if (action === "activate") result = activate(key, fields.instance_name);
  else if (action === "validate") result = validate(key, fields.instance_id);
  else result = deactivate(key, fields.instance_id);
  json(res, result.status, result.body);
}

async function stripeWebhook(req, res) {
  const raw = await readBody(req);
  let event;
  try {
    event = constructEvent(raw, req.headers["stripe-signature"]);
  } catch (err) {
    console.warn("rejected webhook:", err.message);
    return json(res, 400, { error: "invalid signature" });
  }
  try {
    await handleEvent(event);
    json(res, 200, { received: true });
  } catch (err) {
    console.error("webhook failed", event.type, event.id, err);
    json(res, 500, { error: "processing failed" });
  }
}

async function checkoutSuccess(req, res, url) {
  const sessionId = url.searchParams.get("session_id") ?? "";
  if (!/^cs_[A-Za-z0-9_]+$/.test(sessionId)) {
    return html(res, 400, errorPage("That checkout link isn't valid."));
  }
  try {
    const result = await fulfillSession(sessionId);
    if (result.pending) return html(res, 200, pendingPage());
    const key = result.key ?? revealKey(result.license);
    html(res, 200, successPage({ key, tier: result.license.tier, email: result.license.email }));
  } catch (err) {
    console.error("success page failed", sessionId, err);
    html(res, 500, errorPage("We couldn't load your license key."));
  }
}

async function lostKey(req, res) {
  if (req.method === "GET") return html(res, 200, lostKeyPage());

  const ip = clientIp(req);
  const fields = parseFields(req, await readBody(req));
  const email = String(fields.email ?? "").trim().toLowerCase();
  const done = () => html(res, 200, lostKeyPage({ sent: true }));
  if (!email.includes("@") || email.length > 254) return html(res, 400, lostKeyPage());
  if (!allow(`lost-ip:${ip}`, 5, 60 * 60 * 1000) || !allow(`lost-email:${email}`, 2, 60 * 60 * 1000)) {
    return done();
  }
  const licenses = findByEmail(email);
  if (licenses.length) {
    await sendKeysReminder({
      to: email,
      keys: licenses.map((l) => ({ key: revealKey(l), tier: l.tier, plan: l.plan, status: effectiveStatus(l) })),
    });
  }
  done();
}

const server = http.createServer(async (req, res) => {
  const url = new URL(req.url ?? "/", "http://localhost");
  const path = url.pathname.replace(/\/+$/, "") || "/";
  try {
    if (req.method === "GET" && path === "/health") {
      openDatabase().prepare("SELECT 1").get();
      res.writeHead(200, { "Content-Type": "text/plain" });
      return res.end("ok");
    }
    const m = path.match(/^\/v1\/licenses\/(activate|validate|deactivate)$/);
    if (m && req.method === "POST") return await licenseRoute(req, res, m[1]);
    if (path === "/webhooks/stripe" && req.method === "POST") return await stripeWebhook(req, res);
    if (path === "/checkout/success" && req.method === "GET") return await checkoutSuccess(req, res, url);
    if (path === "/licenses/lost" && (req.method === "GET" || req.method === "POST")) return await lostKey(req, res);
    if (path === "/" && req.method === "GET") {
      res.writeHead(302, { Location: "https://vethida.com" });
      return res.end();
    }
    json(res, 404, { error: "Not found." });
  } catch (err) {
    console.error(req.method, path, err);
    if (!res.headersSent) json(res, err.status ?? 500, { error: "Server error." });
  }
});

openDatabase();
server.listen(PORT, () => console.log(`api.vethida.com listening on ${PORT}`));
