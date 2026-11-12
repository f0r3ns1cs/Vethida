# vethida.com container. Build from the repo root (it needs /legal):
#   docker build -f vethida.com/Dockerfile -t vethida-site .
#   docker run --rm -p 8080:8080 vethida-site

FROM node:24-bookworm-slim AS deps
WORKDIR /repo/vethida.com
RUN corepack enable
COPY vethida.com/package.json vethida.com/pnpm-lock.yaml vethida.com/pnpm-workspace.yaml ./
RUN pnpm install --frozen-lockfile

FROM node:24-bookworm-slim AS build
WORKDIR /repo/vethida.com
RUN corepack enable
ENV NEXT_TELEMETRY_DISABLED=1
# NEXT_PUBLIC_ is baked in at build time
ARG NEXT_PUBLIC_SENTRY_DSN=""
ENV NEXT_PUBLIC_SENTRY_DSN=$NEXT_PUBLIC_SENTRY_DSN
COPY --from=deps /repo/vethida.com/node_modules ./node_modules
COPY vethida.com/ ./
COPY legal /repo/legal
RUN pnpm build

FROM node:24-bookworm-slim AS run
WORKDIR /app
ENV NODE_ENV=production \
    NEXT_TELEMETRY_DISABLED=1 \
    PORT=8080 \
    HOSTNAME=0.0.0.0 \
    LEGAL_DIR=/app/legal
RUN groupadd --system app && useradd --system --gid app --no-create-home app
COPY --from=build --chown=app:app /repo/vethida.com/.next/standalone ./
COPY --from=build --chown=app:app /repo/vethida.com/.next/static ./.next/static
COPY --from=build --chown=app:app /repo/vethida.com/public ./public
COPY --from=build --chown=app:app /repo/legal ./legal
USER app
EXPOSE 8080
HEALTHCHECK --interval=30s --timeout=5s --start-period=15s \
  CMD node -e "fetch('http://127.0.0.1:8080/robots.txt').then(r=>process.exit(r.ok?0:1)).catch(()=>process.exit(1))"
CMD ["node", "server.js"]
