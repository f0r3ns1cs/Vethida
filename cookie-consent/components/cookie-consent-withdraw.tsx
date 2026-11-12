"use client";

// one click to withdraw, same as giving consent (GDPR Art. 7(3))

import { useEffect, useState } from "react";

import {
  clearConsent,
  CONSENT_CATEGORIES,
  readConsent,
  type ConsentState,
} from "@/lib/consent";

export function CookieConsentWithdraw() {
  const [state, setState] = useState<ConsentState | null>(null);
  const [hydrated, setHydrated] = useState(false);

  useEffect(() => {
    // eslint-disable-next-line react-hooks/set-state-in-effect
    setState(readConsent());
    setHydrated(true);
  }, []);

  if (!hydrated) {
    return (
      <p className="text-meta text-foreground-faint">
        Checking your current preference...
      </p>
    );
  }

  if (!state) {
    return (
      <p className="text-meta text-foreground-muted">
        You have not set a preference yet. The banner will ask on your next
        page load.
      </p>
    );
  }

  return (
    <div className="flex flex-col items-start gap-3">
      <dl className="flex flex-col gap-1 text-meta text-foreground-muted">
        {CONSENT_CATEGORIES.map((category) => (
          <div key={category} className="flex gap-2">
            <dt className="capitalize">{category}:</dt>
            <dd className="font-semibold text-foreground">
              {state[category] ? "Allowed" : "Not allowed"}
            </dd>
          </div>
        ))}
      </dl>
      <button
        type="button"
        onClick={() => {
          clearConsent();
          window.location.reload(); // banner lives in the root layout
        }}
        className="btn btn-sm btn-secondary"
      >
        Change my preference
      </button>
    </div>
  );
}
