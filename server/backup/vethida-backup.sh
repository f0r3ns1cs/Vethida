#!/usr/bin/env bash

set -euo pipefail

ENV_FILE=${VETHIDA_BACKUP_ENV:-/etc/vethida/backup.env}
REPO=${VETHIDA_REPO:-/srv/vethida}
BACKUP_DIR=${VETHIDA_BACKUP_DIR:-/var/backups/vethida}
KEEP=${VETHIDA_BACKUP_KEEP:-14}
MAX_AGE_HOURS=${VETHIDA_BACKUP_MAX_AGE_HOURS:-48}

die() { echo "vethida-backup: $*" >&2; exit 1; }

[ -r "$ENV_FILE" ] || die "$ENV_FILE is not readable"
# shellcheck disable=SC1090
set -a; . "$ENV_FILE"; set +a
[ -n "${DATABASE_URL:-}" ] || die "DATABASE_URL is not set in $ENV_FILE"

command -v node >/dev/null || die "node is not on PATH"
[ -d "$REPO" ] || die "$REPO does not exist; set VETHIDA_REPO"

mode=${1:-}
case "$mode" in
  dump)
    mkdir -p "$BACKUP_DIR"
    node "$REPO/scripts/db/backup.mjs" --out "$BACKUP_DIR" --keep "$KEEP"

    if [ -n "${R2_BUCKET:-}" ]; then
      command -v rclone >/dev/null || die "R2_BUCKET is set but rclone is not installed"
      rclone copy "$BACKUP_DIR" "r2:${R2_BUCKET}/postgres/" --max-age 25h
      echo "vethida-backup: synced to r2:${R2_BUCKET}/postgres/"
    fi
    ;;

  verify)
    node "$REPO/scripts/db/verify-restore.mjs"
    ;;

  age-check)
    newest=$(find "$BACKUP_DIR" -maxdepth 1 -type f -name '*.dump' -printf '%T@\n' 2>/dev/null \
             | sort -n | tail -1 || true)
    [ -n "$newest" ] || die "no dump in $BACKUP_DIR at all"
    age_hours=$(( ( $(date +%s) - ${newest%.*} ) / 3600 ))
    [ "$age_hours" -le "$MAX_AGE_HOURS" ] \
      || die "newest dump is ${age_hours}h old, over the ${MAX_AGE_HOURS}h limit"
    echo "vethida-backup: newest dump is ${age_hours}h old"
    ;;

  *)
    die "usage: vethida-backup.sh {dump|verify|age-check}"
    ;;
esac
