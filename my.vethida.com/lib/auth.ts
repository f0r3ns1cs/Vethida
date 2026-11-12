import { betterAuth, type BetterAuthOptions } from "better-auth";
import { drizzleAdapter } from "better-auth/adapters/drizzle";
import { APIError, createAuthMiddleware } from "better-auth/api";
import { emailOTP } from "better-auth/plugins";
import { db } from "@vethida/db";
import { sendEmail } from "./email";
import { legalEntityAddress } from "./email/identity";
import { renderTemplate } from "./email/render";
import { verifyTurnstile } from "./turnstile";

const isProduction = process.env.NODE_ENV === "production";

const socialProviders: BetterAuthOptions["socialProviders"] =
  process.env.DISCORD_CLIENT_ID && process.env.DISCORD_CLIENT_SECRET
    ? {
        discord: {
          clientId: process.env.DISCORD_CLIENT_ID,
          clientSecret: process.env.DISCORD_CLIENT_SECRET,
          // guilds.join lets us add the linked user to the server.
          scope: ["guilds.join"],
        },
      }
    : undefined;

// Email OTP only. Sign-up happens on first sign-in and the plugin marks the
// email verified, so there's no reset or verification flow.
export const auth = betterAuth({
  database: drizzleAdapter(db, { provider: "pg" }),
  socialProviders,
  account: {
    accountLinking: {
      enabled: true,
      // discord only links to an existing signed-in account
      trustedProviders: ["discord"],
    },
  },
  hooks: {
    // Turnstile on the send-code endpoint (token comes in a header so the
    // body schema doesn't reject it).
    before: createAuthMiddleware(async (ctx) => {
      // unused, and it answers differently for unknown vs known emails
      // (USER_NOT_FOUND vs INVALID_OTP in better-auth 1.6.23)
      if (ctx.path === "/email-otp/check-verification-otp") {
        throw new APIError("NOT_FOUND", { message: "Not found." });
      }

      if (ctx.path !== "/email-otp/send-verification-otp") return;
      const passed = await verifyTurnstile(
        ctx.headers?.get("x-turnstile-token"),
        ctx.headers?.get("cf-connecting-ip") ?? undefined,
      );
      if (!passed) {
        throw new APIError("BAD_REQUEST", {
          message: "Could not verify that request. Please try again.",
        });
      }
    }),
  },
  session: {
    // 8h absolute. updateAge has to equal expiresIn or the session slides on
    // every request and never actually expires.
    expiresIn: 60 * 60 * 8,
    updateAge: 60 * 60 * 8,
  },
  rateLimit: {
    enabled: true,
    // in the db so restarts don't reset it
    storage: "database",
    window: 60,
    max: 60,
    customRules: {
      // sending is strict (mailbombing), verifying is already capped by allowedAttempts
      "/email-otp/send-verification-otp": { window: 900, max: 3 },
      "/sign-in/email-otp": { window: 900, max: 10 },
    },
  },
  advanced: {
    ipAddress: {
      // behind Cloudflare, don't trust X-Forwarded-For
      ipAddressHeaders: ["cf-connecting-ip", "x-real-ip"],
    },
    // We want __Host- instead of Better Auth's __Secure-, so turn its prefix
    // off, put __Host- in cookiePrefix and set Secure ourselves.
    useSecureCookies: false,
    defaultCookieAttributes: {
      sameSite: "strict",
      ...(isProduction ? { secure: true } : {}),
    },
    cookies: {
      // Lax for these two: the return from checkout and the Discord OAuth
      // callback are cross-site GETs and Strict would drop them
      session_token: { attributes: { sameSite: "lax" } },
      state: { attributes: { sameSite: "lax" } }, // library default, 300s single-use
    },
    cookiePrefix: isProduction ? "__Host-vethida" : "vethida",
  },
  plugins: [
    emailOTP({
      otpLength: 6,
      expiresIn: 300, // 5 min
      allowedAttempts: 3,
      storeOTP: "hashed",
      sendVerificationOTP: async ({ email, otp, type }) => {
        // only sign-in is used
        if (type !== "sign-in") {
          console.error(`[auth] unexpected OTP type "${type}"`);
          return;
        }
        const rendered = renderTemplate("sign-in-code", {
          code: otp,
          expiryMinutes: "5",
          legalEntityAddress: legalEntityAddress(),
        });
        const delivered = await sendEmail({ to: email, ...rendered });
        // throw so the form shows an error instead of the code box
        if (!delivered) {
          throw new Error("sign-in code delivery failed");
        }
      },
    }),
  ],
  user: {
    additionalFields: {
      // input: false or a signup request could set role: admin
      role: { type: "string", defaultValue: "user", required: true, input: false },
      completedOnboarding: { type: "boolean", defaultValue: false, required: true },
    },
  },
});
