#!/usr/bin/env node
// Builds and validates public/desktop/updates.json, the feed the desktop
// auto-updater polls.
//
//   node scripts/update-feed.mjs --check                      validate the current file
//   node scripts/update-feed.mjs --channel stable \           publish an update
//     --version 1.0.1 --url https://... --sha256 <hex> \
//     --size 48210944 --jws <compact-jws>
//   node scripts/update-feed.mjs --channel beta --clear-update   withdraw an update
//
// The client (UpdateFeed.cpp) ignores a bad feed without any error, so check it here.
// Each channel has an `update` object (version, download_url, sha256 required) and
// optionally `disabled_tweaks`. manifest_jws is an ES256 JWS from the release key
// whose payload repeats version/sha256/download_url. This script doesn't sign anything.

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const APP = path.resolve(HERE, "..");
const FEED = path.join(APP, "public", "desktop", "updates.json");

const CHANNELS = ["stable", "beta"];
const SHA256_RE = /^[a-f0-9]{64}$/i;
// shape only, catches a bad paste
const JWS_RE = /^[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+$/;

function fail(msg) {
  console.error(`update-feed: ${msg}`);
  process.exitCode = 1;
}

function readFeed() {
  if (!fs.existsSync(FEED)) {
    throw new Error(`${FEED} does not exist`);
  }
  return JSON.parse(fs.readFileSync(FEED, "utf-8"));
}

function decodeJwsPayload(jws) {
  const parts = jws.split(".");
  if (parts.length !== 3) return null;
  try {
    const json = Buffer.from(parts[1], "base64url").toString("utf-8");
    return JSON.parse(json);
  } catch {
    return null;
  }
}

// returns a list of problems, empty = ok
function validateUpdate(channel, update) {
  const problems = [];
  const where = `${channel}.update`;

  for (const field of ["version", "download_url", "sha256"]) {
    if (typeof update[field] !== "string" || update[field].trim() === "") {
      problems.push(`${where}.${field} is required and must be a non-empty string`);
    }
  }

  if (typeof update.sha256 === "string" && !SHA256_RE.test(update.sha256)) {
    problems.push(`${where}.sha256 must be 64 hex characters`);
  }

  if (typeof update.download_url === "string" && update.download_url !== "") {
    let url;
    try {
      url = new URL(update.download_url);
    } catch {
      problems.push(`${where}.download_url is not a valid URL`);
    }
    if (url && url.protocol !== "https:") {
      problems.push(`${where}.download_url must be https`);
    }
  }

  if (update.size_bytes !== undefined) {
    if (!Number.isInteger(update.size_bytes) || update.size_bytes <= 0) {
      problems.push(`${where}.size_bytes must be a positive integer`);
    }
  } else {
    problems.push(
      `${where}.size_bytes is missing - the client uses it as a download sanity check`,
    );
  }

  // only used for the changelog
  if (typeof update.published_at !== "string" || update.published_at === "") {
    problems.push(`${where}.published_at is required (ISO 8601)`);
  } else if (Number.isNaN(Date.parse(update.published_at))) {
    problems.push(`${where}.published_at is not a parseable date`);
  }

  if (update.channel !== undefined && update.channel !== channel) {
    problems.push(
      `${where}.channel is "${update.channel}" but sits under "${channel}"`,
    );
  }

  if (typeof update.manifest_jws !== "string" || update.manifest_jws === "") {
    problems.push(
      `${where}.manifest_jws is missing - a release build refuses unsigned updates`,
    );
  } else if (!JWS_RE.test(update.manifest_jws)) {
    problems.push(`${where}.manifest_jws is not a compact JWS (three base64url segments)`);
  } else {
    const claims = decodeJwsPayload(update.manifest_jws);
    if (!claims) {
      problems.push(`${where}.manifest_jws payload is not decodable JSON`);
    } else {
      // must match exactly (sha256 ignoring case)
      if (claims.version !== update.version) {
        problems.push(
          `${where}: signed version "${claims.version}" does not match advertised "${update.version}"`,
        );
      }
      if (
        typeof claims.sha256 !== "string" ||
        claims.sha256.toLowerCase() !== String(update.sha256).toLowerCase()
      ) {
        problems.push(`${where}: signed sha256 does not match advertised sha256`);
      }
      if (claims.download_url !== update.download_url) {
        problems.push(`${where}: signed download_url does not match advertised download_url`);
      }
      if (claims.iss !== undefined && claims.iss !== "vethida-release") {
        problems.push(`${where}: signed iss is "${claims.iss}", expected "vethida-release"`);
      }
      if (claims.aud !== undefined && claims.aud !== "vethida-updater") {
        problems.push(`${where}: signed aud is "${claims.aud}", expected "vethida-updater"`);
      }
    }
  }

  return problems;
}

function validateFeed(feed) {
  const problems = [];
  let advertising = 0;

  for (const channel of CHANNELS) {
    const payload = feed[channel];
    if (payload === undefined) {
      problems.push(`channel "${channel}" is missing - the client falls back to the root object`);
      continue;
    }
    if (typeof payload !== "object" || payload === null) {
      problems.push(`channel "${channel}" must be an object`);
      continue;
    }
    if (payload.news !== undefined) {
      problems.push(
        `${channel}.news is not part of the feed contract - nothing reads it`,
      );
    }
    if (payload.update !== undefined) {
      if (typeof payload.update !== "object" || payload.update === null) {
        problems.push(`${channel}.update must be an object`);
      } else {
        problems.push(...validateUpdate(channel, payload.update));
        advertising += 1;
      }
    }
  }

  return { problems, advertising };
}

function parseArgs(argv) {
  const out = {};
  for (let i = 0; i < argv.length; i += 1) {
    const arg = argv[i];
    if (!arg.startsWith("--")) continue;
    const key = arg.slice(2);
    const next = argv[i + 1];
    if (next === undefined || next.startsWith("--")) {
      out[key] = true;
    } else {
      out[key] = next;
      i += 1;
    }
  }
  return out;
}

function main() {
  const args = parseArgs(process.argv.slice(2));
  const feed = readFeed();

  if (args.check || Object.keys(args).length === 0) {
    const { problems, advertising } = validateFeed(feed);
    for (const p of problems) fail(p);
    if (problems.length === 0) {
      console.log(
        advertising === 0
          ? "update-feed: valid. No channel is advertising an update."
          : `update-feed: valid. ${advertising} channel(s) advertising an update.`,
      );
    }
    return;
  }

  const channel = args.channel;
  if (!CHANNELS.includes(channel)) {
    fail(`--channel must be one of: ${CHANNELS.join(", ")}`);
    return;
  }
  feed[channel] ??= {};

  if (args["clear-update"]) {
    delete feed[channel].update;
    fs.writeFileSync(FEED, `${JSON.stringify(feed, null, 2)}\n`);
    console.log(`update-feed: cleared the ${channel} update.`);
    return;
  }

  const update = {
    version: args.version,
    channel,
    download_url: args.url,
    sha256: args.sha256,
    size_bytes: args.size === undefined ? undefined : Number(args.size),
    published_at:
      args["published-at"] === undefined
        ? new Date().toISOString()
        : args["published-at"],
    manifest_jws: args.jws,
    ...(args.notes ? { notes: args.notes } : {}),
  };

  const problems = validateUpdate(channel, update);
  if (problems.length > 0) {
    for (const p of problems) fail(p);
    console.error("update-feed: refusing to write an update the client would reject.");
    return;
  }

  feed[channel].update = update;
  fs.writeFileSync(FEED, `${JSON.stringify(feed, null, 2)}\n`);
  console.log(`update-feed: ${channel} now advertises ${update.version}.`);
}

main();
