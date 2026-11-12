"use client";

import { useActionState } from "react";
import {
  approveDesktopAction,
  denyDesktopAction,
  type AuthorizeState,
} from "./actions";

const initialState: AuthorizeState = { error: null };

export function AuthorizeForm({
  userCode,
  licenses,
}: {
  userCode: string;
  licenses: { id: string; key: string; tier: string }[];
}) {
  const [state, formAction, pending] = useActionState(
    approveDesktopAction,
    initialState,
  );

  return (
    <>
      <form action={formAction} className="space-y-4">
        <input type="hidden" name="userCode" value={userCode} />

        {licenses.length === 1 ? (
          <>
            <input type="hidden" name="licenseId" value={licenses[0].id} />
            <p className="text-sm text-foreground-muted">
              Activating{" "}
              <span className="font-medium text-foreground">
                {licenses[0].tier}
              </span>{" "}
              <span className="font-mono text-xs">{licenses[0].key}</span>
            </p>
          </>
        ) : (
          <div>
            <label
              htmlFor="licenseId"
              className="mb-1.5 block text-sm text-foreground-muted"
            >
              License to activate
            </label>
            <select id="licenseId" name="licenseId" className="input">
              {licenses.map((l) => (
                <option key={l.id} value={l.id}>
                  {l.tier}, {l.key}
                </option>
              ))}
            </select>
          </div>
        )}

        {state.error ? (
          <p className="text-sm text-danger" role="alert">
            {state.error}
          </p>
        ) : null}

        <button
          type="submit"
          disabled={pending}
          className="btn btn-primary w-full"
        >
          {pending ? "Approving..." : "Approve"}
        </button>
      </form>

      {/* separate form so Enter doesn't decline */}
      <form action={denyDesktopAction} className="mt-2">
        <input type="hidden" name="userCode" value={userCode} />
        <button type="submit" disabled={pending} className="btn w-full">
          Decline
        </button>
      </form>
    </>
  );
}
