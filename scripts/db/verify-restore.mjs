#!/usr/bin/env node
// Dumps the database, restores it into a temp database and compares tables,
// row counts, indexes and constraints. Drops the temp db afterwards.
//   node scripts/db/verify-restore.mjs

import { execFileSync } from "node:child_process";
import { mkdtempSync, statSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import path from "node:path";

const SOURCE_URL =
  process.env.DATABASE_URL ??
  "postgresql://vethida:vethida_dev@127.0.0.1:5432/vethida";

// PG_VIA_DOCKER=true runs pg_dump/psql inside a compose container instead.
// PG_SERVICE is the compose service name, not the container name.
const VIA_DOCKER = process.env.PG_VIA_DOCKER === "true";
const SERVICE = process.env.PG_SERVICE ?? "postgres";

function parse(url) {
  const u = new URL(url);
  return {
    user: decodeURIComponent(u.username),
    password: decodeURIComponent(u.password),
    host: u.hostname,
    port: u.port || "5432",
    database: u.pathname.replace(/^\//, ""),
  };
}

const src = parse(SOURCE_URL);
const verifyDb = `${src.database}_restore_check`;

function psqlArgs(database, sqlText) {
  return ["-U", src.user, "-d", database, "-t", "-A", "-c", sqlText];
}

function run(cmd, args, opts = {}) {
  return execFileSync(cmd, args, {
    encoding: "utf8",
    stdio: ["ignore", "pipe", "pipe"],
    ...opts,
  });
}

function inContainer(bin, args, opts = {}) {
  if (!VIA_DOCKER) {
    return run(bin, args, {
      ...opts,
      env: { ...process.env, PGPASSWORD: src.password },
    });
  }
  return run(
    "docker",
    ["compose", "exec", "-T", "-e", `PGPASSWORD=${src.password}`, SERVICE, bin, ...args],
    opts,
  );
}

function query(database, sqlText) {
  return inContainer("psql", psqlArgs(database, sqlText)).trim();
}

function tableCounts(database) {
  // all tables in one query
  const sqlText = `
    SELECT string_agg(format('%s=%s', tablename, cnt), ',' ORDER BY tablename)
    FROM (
      SELECT c.relname AS tablename,
             (SELECT count(*) FROM pg_catalog.pg_class x WHERE x.oid = c.oid) * 0
               + (xpath('/row/c/text()',
                        query_to_xml(format('SELECT count(*) AS c FROM %I.%I', n.nspname, c.relname),
                                     false, true, '')))[1]::text::bigint AS cnt
      FROM pg_catalog.pg_class c
      JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace
      WHERE n.nspname = 'public' AND c.relkind = 'r'
    ) t`;
  const out = query(database, sqlText);
  const counts = new Map();
  if (!out) return counts;
  for (const pair of out.split(",")) {
    const [name, n] = pair.split("=");
    counts.set(name, Number(n));
  }
  return counts;
}

function objectCount(database, kind) {
  const sqlText =
    kind === "index"
      ? "SELECT count(*) FROM pg_indexes WHERE schemaname='public'"
      : "SELECT count(*) FROM pg_constraint c JOIN pg_namespace n ON n.oid=c.connamespace WHERE n.nspname='public'";
  return Number(query(database, sqlText));
}

const problems = [];
const workDir = mkdtempSync(path.join(tmpdir(), "vethida-restore-"));
const dumpHostPath = path.join(workDir, "dump.sql");
const dumpContainerPath = "/tmp/vethida-restore-check.sql";

try {
  console.log(`source     ${src.host}:${src.port}/${src.database}`);
  console.log(`verify db  ${verifyDb}`);
  console.log("");

  // dump
  process.stdout.write("dump ... ");
  if (VIA_DOCKER) {
    run("docker", [
      "compose",
      "exec",
      "-T",
      "-e",
      `PGPASSWORD=${src.password}`,
      SERVICE,
      "sh",
      "-c",
      `pg_dump -U ${src.user} -d ${src.database} --no-owner --no-privileges -f ${dumpContainerPath}`,
    ]);
    const size = Number(
      run("docker", [
        "compose",
        "exec",
        "-T",
        SERVICE,
        "sh",
        "-c",
        `wc -c < ${dumpContainerPath}`,
      ]).trim(),
    );
    if (!size) problems.push("dump is empty");
    console.log(`ok (${(size / 1024).toFixed(0)} KB)`);
  } else {
    run("pg_dump", [
      "-U",
      src.user,
      "-h",
      src.host,
      "-p",
      src.port,
      "-d",
      src.database,
      "--no-owner",
      "--no-privileges",
      "-f",
      dumpHostPath,
    ], { env: { ...process.env, PGPASSWORD: src.password } });
    const size = statSync(dumpHostPath).size;
    if (!size) problems.push("dump is empty");
    console.log(`ok (${(size / 1024).toFixed(0)} KB)`);
  }

  // restore
  process.stdout.write("restore ... ");
  query("postgres", `DROP DATABASE IF EXISTS ${verifyDb}`);
  query("postgres", `CREATE DATABASE ${verifyDb}`);
  const restoreOut = VIA_DOCKER
    ? run("docker", [
        "compose",
        "exec",
        "-T",
        "-e",
        `PGPASSWORD=${src.password}`,
        SERVICE,
        "sh",
        "-c",
        `psql -U ${src.user} -d ${verifyDb} -v ON_ERROR_STOP=1 -f ${dumpContainerPath} 2>&1`,
      ])
    : run("psql", [
        "-U",
        src.user,
        "-h",
        src.host,
        "-p",
        src.port,
        "-d",
        verifyDb,
        "-v",
        "ON_ERROR_STOP=1",
        "-f",
        dumpHostPath,
      ], { env: { ...process.env, PGPASSWORD: src.password } });
  if (/^ERROR:/m.test(restoreOut)) {
    problems.push(`restore reported errors:\n${restoreOut}`);
    console.log("FAILED");
  } else {
    console.log("ok");
  }

  // tables + row counts
  process.stdout.write("compare ... ");
  const before = tableCounts(src.database);
  const after = tableCounts(verifyDb);

  for (const [table, n] of before) {
    if (!after.has(table)) {
      problems.push(`table missing after restore: ${table}`);
      continue;
    }
    if (after.get(table) !== n) {
      problems.push(
        `row count differs for ${table}: source ${n}, restored ${after.get(table)}`,
      );
    }
  }
  for (const table of after.keys()) {
    if (!before.has(table)) problems.push(`unexpected extra table: ${table}`);
  }
  console.log(`${before.size} tables`);

  // indexes + constraints
  process.stdout.write("integrity ... ");
  for (const kind of ["index", "constraint"]) {
    const a = objectCount(src.database, kind);
    const b = objectCount(verifyDb, kind);
    if (a !== b) problems.push(`${kind} count differs: source ${a}, restored ${b}`);
  }
  console.log("ok");
} finally {
  try {
    query("postgres", `DROP DATABASE IF EXISTS ${verifyDb}`);
  } catch {
    console.error(`\nCould not drop ${verifyDb}. Remove it by hand.`);
  }
  if (VIA_DOCKER) {
    try {
      run("docker", ["compose", "exec", "-T", SERVICE, "rm", "-f", dumpContainerPath]);
    } catch {} // in the container's /tmp anyway
  }
  rmSync(workDir, { recursive: true, force: true });
}

console.log("");
if (problems.length === 0) {
  console.log("restore verified: dump, restore, table set, row counts, constraints, indexes");
  process.exit(0);
}
for (const p of problems) console.error(`  ${p}`);
console.error(`\n${problems.length} problem(s). The backup is not proven restorable.`);
process.exit(1);
