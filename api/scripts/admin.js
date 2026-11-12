import { openDatabase } from "../src/db.js";
import { sendLicenseEmail } from "../src/email.js";
import {
  clearActivations,
  createLicense,
  effectiveStatus,
  findByEmail,
  findByKey,
  revealKey,
  setStatus,
} from "../src/licenses.js";

const [command, arg, extra] = process.argv.slice(2);
openDatabase();

function show(license) {
  const activations = openDatabase()
    .prepare("SELECT id, created_at, last_seen FROM activations WHERE license_id = ?")
    .all(license.id);
  console.log({
    key: revealKey(license),
    tier: license.tier,
    plan: license.plan,
    status: effectiveStatus(license),
    email: license.email,
    expires_at: license.expires_at,
    stripe_subscription: license.stripe_subscription,
    created_at: license.created_at,
    activations,
  });
}

function requireKey(raw) {
  const license = findByKey(raw);
  if (!license) {
    console.error("No license with that key.");
    process.exit(1);
  }
  return license;
}

switch (command) {
  case "find": {
    const found = arg?.includes("@") ? findByEmail(arg) : [findByKey(arg)].filter(Boolean);
    if (!found.length) console.log("Nothing found.");
    found.forEach(show);
    break;
  }
  case "revoke":
    setStatus(requireKey(arg).id, "disabled");
    console.log("Disabled.");
    break;
  case "restore":
    setStatus(requireKey(arg).id, "active");
    console.log("Active again.");
    break;
  case "reset":
    clearActivations(requireKey(arg).id);
    console.log("All activations cleared.");
    break;
  case "emails":
    // Everyone who ever bought, one address per line: who the Privacy Policy
    // promises to email before a material change takes effect.
    for (const { email } of openDatabase()
      .prepare("SELECT DISTINCT email FROM licenses ORDER BY email")
      .all()) {
      console.log(email);
    }
    break;
  case "grant": {
    if (!arg?.includes("@") || !["pro", "ultimate"].includes(extra)) {
      console.error("Usage: grant <email> <pro|ultimate>");
      process.exit(1);
    }
    const { key } = createLicense({ tier: extra, plan: "lifetime", email: arg });
    await sendLicenseEmail({ to: arg, key, tier: extra, plan: "lifetime" });
    console.log(`Created ${extra} lifetime key for ${arg}: ${key}`);
    break;
  }
  default:
    console.log("Commands: find <email|key>, revoke <key>, restore <key>, reset <key>, emails, grant <email> <pro|ultimate>");
}
