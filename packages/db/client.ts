import { drizzle } from "drizzle-orm/node-postgres";
import { Pool, type PoolConfig } from "pg";
import * as schema from "./schema";

const isProduction = process.env.NODE_ENV === "production";

// TLS is set here instead of trusting sslmode in the URL (its meaning depends on
// other params). Production needs the CA cert or it won't start.
function tls(): PoolConfig["ssl"] {
  if (!isProduction) return undefined; // local db

  // next build runs with NODE_ENV=production but never connects
  if (process.env.NEXT_PHASE === "phase-production-build") {
    return undefined;
  }

  const ca = process.env.DATABASE_CA_CERT;
  if (!ca) {
    throw new Error(
      "DATABASE_CA_CERT is required in production. Download the CA certificate " +
        "from the DigitalOcean database dashboard and set it as an environment " +
        "variable. Refusing to connect without verified TLS.",
    );
  }

  return { ca, rejectUnauthorized: true };
}

// reuse the pool across dev hot reloads
const globalForDb = globalThis as unknown as { vethidaPgPool?: Pool };

const pool =
  globalForDb.vethidaPgPool ??
  new Pool({
    connectionString: process.env.DATABASE_URL,
    ssl: tls(),
    max: Number(process.env.DATABASE_POOL_MAX ?? 10), // managed pg caps connections, shared with the bot
    connectionTimeoutMillis: 10_000,
    idleTimeoutMillis: 30_000,
  });

if (!isProduction) globalForDb.vethidaPgPool = pool;

export const db = drizzle(pool, { schema });
