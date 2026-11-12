"use client";

import { useActionState } from "react";
import {
  startTrialAction,
  type TrialState,
} from "@/app/dashboard/licenses/actions";

const initialState: TrialState = { error: null };

// The caller only renders this if there's no license and no used trial. The
// error still covers two tabs racing.
export function StartTrial({ days }: { days: number }) {
  const [state, formAction, pending] = useActionState(
    startTrialAction,
    initialState,
  );

  return (
    <form action={formAction}>
      <button type="submit" disabled={pending} className="btn btn-primary">
        {pending ? "Starting..." : `Start ${days}-day free trial`}
      </button>
      {state.error ? (
        <p className="mt-2 text-sm text-danger">{state.error}</p>
      ) : null}
    </form>
  );
}
