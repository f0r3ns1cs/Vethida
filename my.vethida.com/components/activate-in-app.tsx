"use client";

import { useState } from "react";

// Opens vethida://activate-<key> (UrlProtocol.cpp), the app still asks to confirm.
// A button instead of a link so we can show a hint if nothing handles it.
export function ActivateInApp({ licenseKey }: { licenseKey: string }) {
  const [tried, setTried] = useState(false);

  return (
    <div>
      <button
        type="button"
        className="btn btn-secondary h-8 px-3"
        onClick={() => {
          setTried(true);
          window.location.href = `vethida://activate-${encodeURIComponent(licenseKey)}`;
        }}
      >
        Activate in Vethida
      </button>
      {tried ? (
        <p className="mt-2 text-xs text-foreground-faint">
          Nothing happened? Vethida may not be installed on this PC. Copy the
          key above and enter it in the app instead.
        </p>
      ) : null}
    </div>
  );
}
