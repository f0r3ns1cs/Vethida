import * as Sentry from "@sentry/nextjs";

export async function register() {
  if (process.env.NEXT_RUNTIME === "nodejs") {
    const { validateEnv } = await import("./lib/env");
    try {
      validateEnv();
    } catch (err) {
      // exit instead of throwing, otherwise next keeps serving 500s
      console.error(err instanceof Error ? err.message : err);
      process.exit(1);
    }

    await import("./sentry.server.config");
  }

  if (process.env.NEXT_RUNTIME === "edge") {
    await import("./sentry.edge.config");
  }
}

export const onRequestError = Sentry.captureRequestError;
