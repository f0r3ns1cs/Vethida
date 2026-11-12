import crypto from "node:crypto";

import { openDatabase, transaction } from "./db.js";
import {
  decryptLicenseKey,
  encryptLicenseKey,
  generateLicenseKey,
  hashLicenseKey,
  parseLicenseKey,
} from "./keys.js";

export const DEFAULT_ACTIVATION_LIMIT = 3;

const now = () => new Date().toISOString();

export function createLicense({
  tier,
  plan,
  email,
  expiresAt = null,
  customer = null,
  subscription = null,
  session = null,
  payment = null,
}) {
  const d = openDatabase();
  // A collision on 100 random bits will not happen, but a unique index makes
  // it an error rather than two customers sharing a key; try again if it does.
  for (let attempt = 0; attempt < 5; attempt++) {
    const key = generateLicenseKey();
    try {
      const row = d
        .prepare(
          `INSERT INTO licenses (key_hash, key_encrypted, tier, plan, email,
             activation_limit, expires_at, stripe_customer, stripe_subscription,
             stripe_session, stripe_payment)
           VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
           RETURNING *`,
        )
        .get(
          hashLicenseKey(key),
          encryptLicenseKey(key),
          tier,
          plan,
          email.toLowerCase(),
          DEFAULT_ACTIVATION_LIMIT,
          expiresAt,
          customer,
          subscription,
          session,
          payment,
        );
      return { key, license: row };
    } catch (err) {
      if (!String(err.message).includes("key_hash")) throw err;
    }
  }
  throw new Error("could not generate a unique license key");
}

export function findByKey(rawKey) {
  const key = parseLicenseKey(rawKey);
  if (!key) return null;
  return openDatabase()
    .prepare("SELECT * FROM licenses WHERE key_hash = ?")
    .get(hashLicenseKey(key)) ?? null;
}

export function findBySession(sessionId) {
  return openDatabase()
    .prepare("SELECT * FROM licenses WHERE stripe_session = ?")
    .get(sessionId) ?? null;
}

export function findBySubscription(subscriptionId) {
  return openDatabase()
    .prepare("SELECT * FROM licenses WHERE stripe_subscription = ?")
    .get(subscriptionId) ?? null;
}

export function findByPayment(paymentId) {
  return openDatabase()
    .prepare("SELECT * FROM licenses WHERE stripe_payment = ?")
    .get(paymentId) ?? null;
}

export function findByEmail(email) {
  return openDatabase()
    .prepare("SELECT * FROM licenses WHERE email = ? ORDER BY id")
    .all(String(email).trim().toLowerCase());
}

export function revealKey(license) {
  return decryptLicenseKey(license.key_encrypted);
}

export function setStatus(licenseId, status, expiresAt) {
  const d = openDatabase();
  if (expiresAt === undefined) {
    d.prepare("UPDATE licenses SET status = ?, updated_at = ? WHERE id = ?")
      .run(status, now(), licenseId);
  } else {
    d.prepare("UPDATE licenses SET status = ?, expires_at = ?, updated_at = ? WHERE id = ?")
      .run(status, expiresAt, now(), licenseId);
  }
}

export function clearActivations(licenseId) {
  openDatabase().prepare("DELETE FROM activations WHERE license_id = ?").run(licenseId);
}

function activationsOf(licenseId) {
  return openDatabase()
    .prepare("SELECT * FROM activations WHERE license_id = ? ORDER BY created_at")
    .all(licenseId);
}

// What the key grants right now. A subscription whose paid period has run out reads
// as expired even before Stripe's webhook says so.
export function effectiveStatus(license, at = Date.now()) {
  if (license.status !== "active") return license.status;
  if (license.expires_at && Date.parse(license.expires_at) <= at) return "expired";
  return "active";
}

function describe(license, instance) {
  const usage = activationsOf(license.id).length;
  const status = effectiveStatus(license);
  return {
    license_key: {
      id: license.id,
      status: status === "active" && usage === 0 ? "inactive" : status,
      activation_limit: license.activation_limit,
      activation_usage: usage,
      created_at: license.created_at,
      expires_at: license.expires_at,
    },
    instance: instance
      ? { id: instance.id, name: instance.name, created_at: instance.created_at }
      : null,
    meta: {
      product: "vethida",
      tier: license.tier,
      plan: license.plan,
      customer_email: license.email,
    },
  };
}

const NOT_FOUND = { status: 404, error: "license_key not found." };

function refusal(license) {
  const status = effectiveStatus(license);
  if (status === "expired") return "This license key is expired.";
  if (status === "disabled") return "This license key is disabled.";
  return null;
}

export function activate(rawKey, instanceName) {
  return transaction(() => {
    const license = findByKey(rawKey);
    if (!license) return { status: NOT_FOUND.status, body: { activated: false, error: NOT_FOUND.error } };

    const refused = refusal(license);
    if (refused) {
      return { status: 400, body: { activated: false, error: refused, ...describe(license, null) } };
    }
    if (activationsOf(license.id).length >= license.activation_limit) {
      return {
        status: 400,
        body: {
          activated: false,
          error: "This license key has reached the activation limit.",
          ...describe(license, null),
        },
      };
    }

    const instance = openDatabase()
      .prepare("INSERT INTO activations (id, license_id, name) VALUES (?, ?, ?) RETURNING *")
      .get(crypto.randomUUID(), license.id, String(instanceName || "Windows PC").slice(0, 100));
    return { status: 200, body: { activated: true, error: null, ...describe(license, instance) } };
  });
}

export function validate(rawKey, instanceId) {
  const license = findByKey(rawKey);
  if (!license) return { status: NOT_FOUND.status, body: { valid: false, error: NOT_FOUND.error } };

  const instance = instanceId
    ? openDatabase()
        .prepare("SELECT * FROM activations WHERE id = ? AND license_id = ?")
        .get(String(instanceId), license.id)
    : null;
  if (instanceId && !instance) {
    return {
      status: 404,
      body: { valid: false, error: "license_key instance not found.", ...describe(license, null) },
    };
  }
  if (instance) {
    openDatabase().prepare("UPDATE activations SET last_seen = ? WHERE id = ?").run(now(), instance.id);
  }

  const refused = refusal(license);
  return {
    status: 200,
    body: { valid: !refused, error: refused, ...describe(license, instance ?? null) },
  };
}

export function deactivate(rawKey, instanceId) {
  const license = findByKey(rawKey);
  if (!license) return { status: NOT_FOUND.status, body: { deactivated: false, error: NOT_FOUND.error } };

  const result = openDatabase()
    .prepare("DELETE FROM activations WHERE id = ? AND license_id = ?")
    .run(String(instanceId ?? ""), license.id);
  if (result.changes === 0) {
    return {
      status: 404,
      body: { deactivated: false, error: "license_key instance not found.", ...describe(license, null) },
    };
  }
  return { status: 200, body: { deactivated: true, error: null, ...describe(license, null) } };
}
