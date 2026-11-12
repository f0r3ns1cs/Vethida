"use client";

import { useState } from "react";

import { formatDate } from "@/lib/format";

const PHRASE = "DELETE MY ACCOUNT";
type Status = "idle" | "submitting" | "done" | "error";

export function DeleteAccount() {
  const [text, setText] = useState("");
  const [status, setStatus] = useState<Status>("idle");
  const [message, setMessage] = useState("");

  async function submit() {
    setStatus("submitting");
    try {
      const res = await fetch("/api/account", { method: "DELETE" });
      const data: { hardDeleteScheduledFor?: string; error?: string } =
        await res.json().catch(() => ({}));
      if (res.ok || res.status === 409) {
        const when = data.hardDeleteScheduledFor
          ? formatDate(data.hardDeleteScheduledFor)
          : "in 30 days";
        setStatus("done");
        setMessage(
          `Your account is scheduled for deletion on ${when}. You have been signed out. Sign back in within 30 days to cancel.`,
        );
      } else {
        setStatus("error");
        setMessage(data.error ?? "Couldn't process the request.");
      }
    } catch {
      setStatus("error");
      setMessage("Network error. Please try again.");
    }
  }

  if (status === "done") {
    return (
      <p role="status" className="text-sm text-foreground-muted">
        {message}
      </p>
    );
  }

  return (
    <div className="flex flex-col gap-3">
      <label htmlFor="confirm-delete" className="text-xs text-foreground-muted">
        Type <span className="font-mono text-foreground">{PHRASE}</span> to
        confirm
      </label>
      <input
        id="confirm-delete"
        value={text}
        onChange={(event) => setText(event.target.value)}
        autoComplete="off"
        className="h-9 rounded-md border border-border bg-background px-3 font-mono text-sm text-foreground outline-none focus:border-border-strong"
      />
      <button
        type="button"
        disabled={text !== PHRASE || status === "submitting"}
        onClick={submit}
        className="btn btn-secondary h-9 self-start px-4 text-danger disabled:opacity-50"
      >
        {status === "submitting" ? "Working..." : "Delete my account"}
      </button>
      {status === "error" ? (
        <p role="alert" className="text-sm text-danger">
          {message}
        </p>
      ) : null}
    </div>
  );
}
