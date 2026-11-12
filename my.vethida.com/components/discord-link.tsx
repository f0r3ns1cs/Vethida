"use client";

import { useEffect, useRef, useState, useTransition } from "react";
import { useRouter } from "next/navigation";
import { linkSocial } from "@/lib/auth-client";
import { syncDiscordAction } from "@/app/dashboard/settings/discord-actions";

export function DiscordLink({
  linked,
  justLinked = false,
}: {
  linked: boolean;
  justLinked?: boolean;
}) {
  const [pending, setPending] = useState(false);
  const [, startTransition] = useTransition();
  const [syncFailed, setSyncFailed] = useState(false);
  const [syncing, setSyncing] = useState(false);
  const router = useRouter();
  const synced = useRef(false); // strict mode runs effects twice

  // sync once after linking (as a POST, not on the GET), then drop ?discord=linked
  useEffect(() => {
    if (!justLinked || !linked || synced.current) return;
    synced.current = true;
    startTransition(async () => {
      const { ok } = await syncDiscordAction();
      setSyncFailed(!ok);
      router.replace("/dashboard/settings");
    });
  }, [justLinked, linked, router]);

  if (!linked) {
    return (
      <button
        type="button"
        disabled={pending}
        className="btn btn-secondary"
        onClick={async () => {
          setPending(true);
          await linkSocial({
            provider: "discord",
            callbackURL: "/dashboard/settings?discord=linked",
          });
        }}
      >
        {pending ? "Redirecting..." : "Link Discord"}
      </button>
    );
  }

  return (
    <div className="flex flex-col gap-2">
      <div className="flex items-center gap-3">
        <span className="text-sm text-foreground-muted">Discord linked.</span>
        <button
          type="button"
          disabled={syncing}
          className="btn btn-secondary h-8 px-3"
          onClick={() => {
            setSyncing(true);
            startTransition(async () => {
              const { ok } = await syncDiscordAction();
              setSyncFailed(!ok);
              setSyncing(false);
            });
          }}
        >
          {syncing ? "Syncing..." : "Sync roles"}
        </button>
      </div>

      {syncFailed ? (
        <p role="alert" className="text-sm text-danger">
          We couldn&apos;t update your Discord roles. If you have left the
          server, rejoin and try again; otherwise contact support and we will
          sort it out.
        </p>
      ) : null}
    </div>
  );
}
