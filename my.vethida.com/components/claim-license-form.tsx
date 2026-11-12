"use client";

import { useActionState } from "react";
import {
  claimLicenseAction,
  type ClaimState,
} from "@/app/dashboard/licenses/actions";

const initialState: ClaimState = { error: null };

export function ClaimLicenseForm() {
  const [state, formAction, pending] = useActionState(
    claimLicenseAction,
    initialState,
  );

  return (
    <form action={formAction} className="card p-5">
      <label htmlFor="licenseKey" className="label">
        Add a license key to this account
      </label>
      <div className="flex gap-2">
        <input
          id="licenseKey"
          name="licenseKey"
          type="text"
          required
          placeholder="ABCDE-FGHJK-MNPQR-STVWX-YZ012"
          autoComplete="off"
          spellCheck={false}
          maxLength={64}
          className="input font-mono"
        />
        <button
          type="submit"
          disabled={pending}
          className="btn btn-primary shrink-0"
        >
          {pending ? "Adding..." : "Add"}
        </button>
      </div>
      {state.error ? (
        <p className="mt-2 text-sm text-danger">{state.error}</p>
      ) : null}
    </form>
  );
}
