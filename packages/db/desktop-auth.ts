import crypto from "node:crypto";
import { and, eq, isNull, lt, or, sql } from "drizzle-orm";
import { db } from "./client";
import { desktopAuthSession, license, type DesktopAuthSession } from "./licensing";

// Desktop browser sign-in (device flow). The api starts/polls these, the portal approves them.

export const DESKTOP_AUTH_TTL_MS = 10 * 60 * 1000;
export const DESKTOP_AUTH_POLL_INTERVAL_MS = 2000; // sent to the client, so it can change server side

const CODE_CHARS = "0123456789ABCDEFGHJKMNPQRSTVWXYZ"; // same alphabet as license keys

// user code is only shown for comparison, the device code is the actual secret
export function generateUserCode(): string {
  let out = "";
  for (let i = 0; i < 8; i++) out += CODE_CHARS[crypto.randomInt(32)];
  return `${out.slice(0, 4)}-${out.slice(4)}`;
}

export function generateDeviceCode(): string {
  return crypto.randomBytes(32).toString("base64url");
}

export function hashDeviceCode(deviceCode: string): string {
  return crypto.createHash("sha256").update(deviceCode).digest("hex");
}

export function normalizeUserCode(raw: string): string | null {
  if (typeof raw !== "string") return null;
  const compact = raw
    .toUpperCase()
    .replace(/[\s-]/g, "")
    .replace(/[IL]/g, "1")
    .replace(/O/g, "0")
    .replace(/U/g, "V");
  if (!/^[0-9A-HJ-NP-TV-Z]{8}$/.test(compact)) return null;
  return `${compact.slice(0, 4)}-${compact.slice(4)}`;
}

export function isLive(session: DesktopAuthSession): boolean {
  return session.expiresAt.getTime() > Date.now();
}

export async function findLiveSessionByUserCode(
  userCode: string,
): Promise<DesktopAuthSession | null> {
  const [row] = await db
    .select()
    .from(desktopAuthSession)
    .where(
      and(
        eq(desktopAuthSession.userCode, userCode),
        eq(desktopAuthSession.status, "pending"),
        sql`${desktopAuthSession.expiresAt} > now()`,
      ),
    )
    .limit(1);
  return row ?? null;
}

export async function findSessionByDeviceCode(
  deviceCode: string,
): Promise<DesktopAuthSession | null> {
  const [row] = await db
    .select()
    .from(desktopAuthSession)
    .where(eq(desktopAuthSession.deviceCodeHash, hashDeviceCode(deviceCode)))
    .limit(1);
  return row ?? null;
}

// false if it was already handled or expired
export async function approveSession(args: {
  sessionId: string;
  userId: string;
  licenseId: string;
}): Promise<boolean> {
  const updated = await db
    .update(desktopAuthSession)
    .set({
      status: "approved",
      userId: args.userId,
      licenseId: args.licenseId,
      approvedAt: new Date(),
    })
    .where(
      and(
        eq(desktopAuthSession.id, args.sessionId),
        eq(desktopAuthSession.status, "pending"),
        sql`${desktopAuthSession.expiresAt} > now()`,
      ),
    )
    .returning({ id: desktopAuthSession.id });
  return updated.length > 0;
}

export async function denySession(sessionId: string): Promise<boolean> {
  const updated = await db
    .update(desktopAuthSession)
    .set({ status: "denied" })
    .where(
      and(
        eq(desktopAuthSession.id, sessionId),
        eq(desktopAuthSession.status, "pending"),
      ),
    )
    .returning({ id: desktopAuthSession.id });
  return updated.length > 0;
}

// one-shot, two polls racing can't both get it
export async function consumeSession(sessionId: string): Promise<boolean> {
  const updated = await db
    .update(desktopAuthSession)
    .set({ status: "consumed", consumedAt: new Date() })
    .where(
      and(
        eq(desktopAuthSession.id, sessionId),
        eq(desktopAuthSession.status, "approved"),
        sql`${desktopAuthSession.expiresAt} > now()`,
      ),
    )
    .returning({ id: desktopAuthSession.id });
  return updated.length > 0;
}

// best tier first, then newest. revoked/expired are left out
export async function selectableLicensesFor(userId: string) {
  return db
    .select()
    .from(license)
    .where(
      and(
        eq(license.userId, userId),
        isNull(license.revokedAt),
        or(
          isNull(license.expiresAt),
          sql`${license.expiresAt} > now()`,
        ),
      ),
    )
    .orderBy(sql`
      CASE ${license.tier} WHEN 'ultimate' THEN 0 WHEN 'pro' THEN 1 ELSE 2 END,
      ${license.createdAt} DESC
    `);
}

export async function purgeExpiredDesktopAuthSessions(): Promise<number> {
  const deleted = await db
    .delete(desktopAuthSession)
    .where(lt(desktopAuthSession.expiresAt, new Date()))
    .returning({ id: desktopAuthSession.id });
  return deleted.length;
}
