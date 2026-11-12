"use client";

// Accept and reject are both one click. No answer = no optional cookies.
// Only renders client side since it reads document.cookie.

import * as React from "react";
import Link from "next/link";

import {
  ALL_OFF,
  ALL_ON,
  CONSENT_COOKIE,
  type ConsentChoices,
  readConsent,
  writeConsent,
} from "@/lib/consent";

const CATEGORIES = [
  {
    key: "essential" as const,
    name: "Essential",
    body: "These cookies are necessary for the website to function and cannot be switched off in our systems. They are usually only set in response to actions made by you which amount to a request for services, such as setting your privacy preferences, signing in or filling in forms. You can set your browser to block or alert you about these cookies, but some parts of the site will not then work.",
  },
  {
    key: "functional" as const,
    name: "Functional",
    body: "These cookies enable the website to provide enhanced functionality and personalization. They may be set by us or by third party providers whose services we have added to our pages. If you do not allow these cookies then some or all of these services may not function properly.",
  },
  {
    key: "performance" as const,
    name: "Performance",
    body: "These cookies allow us to count visits and traffic sources so we can measure and improve the performance of our site. They help us to know which pages are the most and least popular and see how visitors move around the site. If you do not allow these cookies we will not know when you have visited our site, and will not be able to monitor its performance.",
  },
];

export function CookieConsentBanner() {
  const [visible, setVisible] = React.useState(false);
  const [customizing, setCustomizing] = React.useState(false);
  const [choices, setChoices] = React.useState<ConsentChoices>(ALL_OFF);
  const headingId = React.useId();

  React.useEffect(() => {
    // eslint-disable-next-line react-hooks/set-state-in-effect
    if (readConsent() === null) setVisible(true);
  }, []);

  // show again if the cookie got cleared (pageshow also fires on bfcache restore)
  React.useEffect(() => {
    const onPageShow = () => {
      if (!document.cookie.includes(`${CONSENT_COOKIE}=`)) setVisible(true);
    };
    window.addEventListener("pageshow", onPageShow);
    return () => window.removeEventListener("pageshow", onPageShow);
  }, []);

  if (!visible) return null;

  const answer = (next: ConsentChoices) => {
    writeConsent(next);
    setVisible(false);
  };

  return (
    <div
      role="dialog"
      aria-modal="false"
      aria-labelledby={headingId}
      className="cookie-banner"
    >
      <h2 id={headingId} className="cookie-banner__title">
        Cookies on vethida.com
      </h2>

      <p className="cookie-banner__text">
        We use cookies and similar technologies to operate this website, keep
        you signed in, remember your preferences and understand how the site is
        used. Some are set by providers who run parts of the service for us. You
        can accept all of them, keep only the ones the site cannot work without,
        or choose for yourself below. Our{" "}
        <Link href="/legal/cookies" className="cookie-banner__link">
          cookie policy
        </Link>{" "}
        lists every cookie we set, and our{" "}
        <Link href="/legal/privacy" className="cookie-banner__link">
          privacy policy
        </Link>{" "}
        explains your rights over the data behind them.
      </p>

      {customizing ? (
        <div className="cookie-banner__categories">
          {CATEGORIES.map((category) => {
            const essential = category.key === "essential";
            const checked = essential || choices[category.key];
            return (
              <div key={category.key} className="cookie-category">
                <div className="cookie-category__head">
                  <label
                    className="cookie-category__name"
                    htmlFor={`${headingId}-${category.key}`}
                  >
                    {category.name}
                  </label>
                  <input
                    id={`${headingId}-${category.key}`}
                    type="checkbox"
                    className="cookie-category__switch"
                    checked={checked}
                    disabled={essential}
                    aria-describedby={`${headingId}-${category.key}-desc`}
                    onChange={(event) => {
                      if (essential) return;
                      setChoices((prev) => ({
                        ...prev,
                        [category.key]: event.target.checked,
                      }));
                    }}
                  />
                </div>
                <p
                  id={`${headingId}-${category.key}-desc`}
                  className="cookie-category__body"
                >
                  {category.body}
                  {essential ? " Always on." : null}
                </p>
              </div>
            );
          })}
        </div>
      ) : null}

      {/* accept/reject look the same on purpose */}
      <div className="cookie-banner__actions">
        <button
          type="button"
          onClick={() => answer(ALL_ON)}
          className="btn btn-sm btn-secondary"
        >
          Continue with all
        </button>
        <button
          type="button"
          onClick={() => answer(ALL_OFF)}
          className="btn btn-sm btn-secondary"
        >
          Accept essentials only
        </button>
        {customizing ? (
          <button
            type="button"
            onClick={() => answer(choices)}
            className="btn btn-sm btn-secondary"
          >
            Save my choices
          </button>
        ) : (
          <button
            type="button"
            onClick={() => setCustomizing(true)}
            className="btn btn-sm cookie-banner__quiet"
          >
            Customize my choices
          </button>
        )}
      </div>
    </div>
  );
}
