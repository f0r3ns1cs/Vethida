import type { ReactNode } from "react";
import type { Metadata } from "next";
import Link from "next/link";

import { ListDetailsIcon } from "@/components/icons";
import { PageCta } from "@/components/page-cta";
import { pageMetadata } from "@/lib/metadata";
import { activationLimit, legal, site } from "@/lib/site";

export const metadata: Metadata = pageMetadata({
  title: "Support",
  description:
    "How to get help with Vethida: which inbox to write to, and how a license moves between PCs.",
  path: "/support",
});

type Inbox = {
  address: string;
  label: string;
  detail: string;
  /** A policy governing this address, linked after the detail. */
  policyHref?: string;
  policyLabel?: string;
};

const INBOXES: Inbox[] = [
  {
    address: legal.email.support,
    label: "Support",
    detail:
      "Installation, activation, licensing, billing, and anything else about the product.",
  },
  {
    address: legal.email.security,
    label: "Security",
    detail: "Vulnerability reports, under the disclosure policy.",
    policyHref: "/security",
    policyLabel: "Read the policy",
  },
];

const LICENSE: Array<{ term: string; body: ReactNode }> = [
  {
    term: "Your key",
    body: (
      <>
        In the email we sent after checkout. Lost it?{" "}
        <a href={site.links.licenses} target="_blank" rel="noopener noreferrer">
          Get it resent
        </a>{" "}
        to the email you bought with.
      </>
    ),
  },
  {
    term: "Activations",
    body: `Each license has ${activationLimit}, so a reinstall or a new PC never locks you out.`,
  },
  {
    term: "Moving to another PC",
    body: "Deactivate the old PC from Vethida's Account page, then activate the new one with the same key.",
  },
  {
    term: "Billing",
    body: (
      <>
        Update your card or cancel in the{" "}
        <a href={site.links.billing} target="_blank" rel="noopener noreferrer">
          billing portal
        </a>
        .
      </>
    ),
  },
];

export default function Page() {
  return (
    <main>
      <section className="hero-wash masthead">
        <div className="band__inner band__inner--text">
          <h1 className="masthead__title">Support</h1>
          <p className="masthead__deck">
            Most questions are answered in the <Link href="/faq">FAQ</Link>. For
            anything else, write to one of the addresses below.
          </p>
        </div>
      </section>

      <section className="band band--tinted">
        <div className="band__inner band__inner--text">
          <div className="section-head">
            <h2 className="section-head__title">Where to write</h2>
          </div>

          <dl className="spec-list spec-list--terms">
            {INBOXES.map((inbox) => (
              <div key={inbox.address} className="spec-row">
                <dt className="spec-row__key">{inbox.label}</dt>
                <dd className="spec-row__value">
                  <a
                    href={`mailto:${inbox.address}`}
                    className="mono-key no-underline"
                  >
                    {inbox.address}
                  </a>
                  <p>
                    {inbox.detail}
                    {inbox.policyHref ? (
                      <>
                        {" "}
                        <Link href={inbox.policyHref}>{inbox.policyLabel}</Link>.
                      </>
                    ) : null}
                  </p>
                </dd>
              </div>
            ))}

            <div className="spec-row">
              <dt className="spec-row__key">Discord</dt>
              <dd className="spec-row__value">
                <a
                  href={site.links.discord}
                  target="_blank"
                  rel="noopener noreferrer"
                  className="mono-key no-underline"
                >
                  {site.links.discordDisplay}
                </a>
                <p>
                  Where other customers and the people who build Vethida answer
                  questions in the open.
                </p>
              </dd>
            </div>
          </dl>

          <div className="callout callout--spaced">
            <ListDetailsIcon className="callout__icon" />
            <div>
              <p className="callout__title">What to include</p>
              <p className="callout__body">
                The email you bought with, and your order number if you have
                it.
              </p>
            </div>
          </div>
        </div>
      </section>

      <section className="band">
        <div className="band__inner band__inner--text">
          <div className="section-head">
            <h2 className="section-head__title">Your license and your PC</h2>
          </div>

          <dl className="spec-list spec-list--terms">
            {LICENSE.map((row) => (
              <div key={row.term} className="spec-row">
                <dt className="spec-row__key">{row.term}</dt>
                <dd className="spec-row__value">{row.body}</dd>
              </div>
            ))}
          </dl>
        </div>
      </section>

      <PageCta variant="buy" />
    </main>
  );
}
