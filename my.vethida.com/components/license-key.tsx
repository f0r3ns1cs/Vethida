"use client";

import { useEffect, useRef, useState } from "react";

// license key with a copy button
export function LicenseKey({ licenseKey }: { licenseKey: string }) {
  const [state, setState] = useState<"idle" | "copied" | "failed">("idle");
  const timer = useRef<ReturnType<typeof setTimeout> | null>(null);

  useEffect(
    () => () => {
      if (timer.current) clearTimeout(timer.current);
    },
    [],
  );

  function flash(next: "copied" | "failed") {
    setState(next);
    if (timer.current) clearTimeout(timer.current);
    timer.current = setTimeout(() => setState("idle"), 2000);
  }

  return (
    <div className="flex flex-wrap items-center gap-2">
      {/* select-all for when the clipboard API isn't available */}
      <code className="code select-all rounded-md border border-border bg-background px-2.5 py-1.5 text-foreground">
        {licenseKey}
      </code>
      <button
        type="button"
        className="btn btn-secondary btn-sm"
        onClick={async () => {
          try {
            await navigator.clipboard.writeText(licenseKey);
            flash("copied");
          } catch {
            flash("failed");
          }
        }}
      >
        {state === "copied"
          ? "Copied"
          : state === "failed"
            ? "Select it"
            : "Copy"}
      </button>
      <span aria-live="polite" className="sr-only">
        {state === "copied"
          ? "License key copied to clipboard"
          : state === "failed"
            ? "Copying is unavailable. Select the key and copy it manually."
            : ""}
      </span>
    </div>
  );
}
