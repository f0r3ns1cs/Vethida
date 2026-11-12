// Loader so `node --test` can run our TypeScript: handles extensionless imports,
// the "@/..." alias and the @vethida/* workspace packages.
//   node --import ./scripts/resolve-ts.mjs --test <file>

import { registerHooks } from "node:module";
import { existsSync } from "node:fs";
import path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const repoRoot = path.resolve(fileURLToPath(import.meta.url), "../..");

// longest first so vethida.com doesn't match my.vethida.com
const ALIAS_ROOTS = ["api.vethida.com", "my.vethida.com", "vethida.com"]
  .map((dir) => path.join(repoRoot, dir))
  .filter((dir) => existsSync(dir));

const WORKSPACE = new Map([
  ["@vethida/db", path.join(repoRoot, "packages/db/index.ts")],
  ["@vethida/http", path.join(repoRoot, "packages/http")],
  ["@vethida/observability", path.join(repoRoot, "packages/observability")],
]);

function firstExisting(candidate) {
  const tries = [
    candidate,
    `${candidate}.ts`,
    `${candidate}.tsx`,
    `${candidate}.mjs`,
    `${candidate}.js`,
    path.join(candidate, "index.ts"),
    path.join(candidate, "index.tsx"),
  ];
  for (const t of tries) {
    if (existsSync(t) && !isDirectory(t)) return t;
  }
  return null;
}

function isDirectory(p) {
  try {
    return existsSync(p) && path.extname(p) === "";
  } catch {
    return false;
  }
}

registerHooks({
  resolve(specifier, context, nextResolve) {
    for (const [name, target] of WORKSPACE) {
      if (specifier === name) {
        return { url: pathToFileURL(target).href, shortCircuit: true };
      }
      if (specifier.startsWith(`${name}/`)) {
        const rest = specifier.slice(name.length + 1);
        const base = path.join(path.dirname(target), rest);
        const hit = firstExisting(base);
        if (hit) return { url: pathToFileURL(hit).href, shortCircuit: true };
      }
    }

    // "@/" depends on which app the importing file is in
    if (specifier.startsWith("@/") && context.parentURL) {
      const parent = fileURLToPath(context.parentURL);
      const root = ALIAS_ROOTS.find((r) => parent.startsWith(r + path.sep));
      if (root) {
        const hit = firstExisting(path.join(root, specifier.slice(2)));
        if (hit) return { url: pathToFileURL(hit).href, shortCircuit: true };
      }
    }

    if (specifier.startsWith(".") && context.parentURL) {
      const base = path.resolve(
        path.dirname(fileURLToPath(context.parentURL)),
        specifier,
      );
      const hit = firstExisting(base);
      if (hit) return { url: pathToFileURL(hit).href, shortCircuit: true };
    }

    return nextResolve(specifier, context);
  },
});
