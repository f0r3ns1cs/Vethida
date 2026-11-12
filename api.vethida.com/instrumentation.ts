import * as Sentry from "@sentry/nextjs";

export async function register() {
  if (process.env.NEXT_RUNTIME === "nodejs") {
    // check env before serving anything
    const { validateEnv } = await import("./lib/env");
    try {
      validateEnv();
    } catch (err) {
      // throwing isn't enough, next just logs it and keeps running
      console.error(err instanceof Error ? err.message : err);
      process.exit(1);
    }

    await import("./sentry.server.config");
  }
}

export const onRequestError = Sentry.captureRequestError;
