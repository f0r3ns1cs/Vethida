import fs from "node:fs";
import path from "node:path";
import { DatabaseSync } from "node:sqlite";

let db;

export function openDatabase(file = process.env.DB_PATH ?? "data/vethida.db") {
  if (db) return db;
  if (file !== ":memory:") fs.mkdirSync(path.dirname(file), { recursive: true });
  db = new DatabaseSync(file);
  db.exec(`
    PRAGMA journal_mode = WAL;
    PRAGMA foreign_keys = ON;
    PRAGMA busy_timeout = 5000;

    CREATE TABLE IF NOT EXISTS licenses (
      id                   INTEGER PRIMARY KEY,
      key_hash             TEXT NOT NULL UNIQUE,
      key_encrypted        TEXT NOT NULL,
      tier                 TEXT NOT NULL CHECK (tier IN ('pro', 'ultimate')),
      plan                 TEXT NOT NULL CHECK (plan IN ('month', 'year', 'lifetime')),
      status               TEXT NOT NULL DEFAULT 'active'
                           CHECK (status IN ('active', 'expired', 'disabled')),
      email                TEXT NOT NULL,
      activation_limit     INTEGER NOT NULL DEFAULT 3,
      expires_at           TEXT,
      stripe_customer      TEXT,
      stripe_subscription  TEXT UNIQUE,
      stripe_session       TEXT UNIQUE,
      stripe_payment       TEXT,
      created_at           TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),
      updated_at           TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now'))
    );
    CREATE INDEX IF NOT EXISTS licenses_email ON licenses (email);
    CREATE INDEX IF NOT EXISTS licenses_payment ON licenses (stripe_payment);

    CREATE TABLE IF NOT EXISTS activations (
      id          TEXT PRIMARY KEY,
      license_id  INTEGER NOT NULL REFERENCES licenses (id) ON DELETE CASCADE,
      name        TEXT NOT NULL,
      created_at  TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),
      last_seen   TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now'))
    );
    CREATE INDEX IF NOT EXISTS activations_license ON activations (license_id);

    CREATE TABLE IF NOT EXISTS stripe_events (
      id          TEXT PRIMARY KEY,
      type        TEXT NOT NULL,
      received_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now'))
    );
  `);
  return db;
}

export function transaction(fn) {
  const d = openDatabase();
  d.exec("BEGIN IMMEDIATE");
  try {
    const result = fn(d);
    d.exec("COMMIT");
    return result;
  } catch (err) {
    d.exec("ROLLBACK");
    throw err;
  }
}
