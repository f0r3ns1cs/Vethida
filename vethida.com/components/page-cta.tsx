import Link from "next/link";

import { ArrowRightIcon } from "@/components/icons";

type Action = { href: string; label: string; external?: boolean };

type PageCtaProps = {
  variant?: "support" | "buy";
  title?: string;
  body?: string;
  primary?: Action;
  secondary?: Action | null;
};

// defaults for "buy"
const BUY = {
  title: "Try Vethida free",
  body: "Free covers the essentials. Pro and Ultimate unlock the rest.",
  primary: { href: "/download", label: "Download free" } satisfies Action,
  secondary: { href: "/pricing", label: "See plans" } satisfies Action,
};

function ActionLink({
  action,
  variant,
  arrow,
}: {
  action: Action;
  variant: "primary" | "secondary";
  arrow?: boolean;
}) {
  const className = `btn btn-lg btn-${variant}`;
  const content = (
    <>
      {action.label}
      {arrow ? <ArrowRightIcon className="btn-arrow" /> : null}
    </>
  );

  if (action.external) {
    return (
      <a
        href={action.href}
        target="_blank"
        rel="noopener noreferrer"
        className={className}
      >
        {content}
      </a>
    );
  }
  return (
    <Link href={action.href} className={className}>
      {content}
    </Link>
  );
}

export function PageCta({
  variant = "support",
  title,
  body,
  primary,
  secondary,
}: PageCtaProps) {
  const buying = variant === "buy";

  return (
    <section className="band band--inverse">
      <div className="band__inner band__inner--prose">
        <div className="section-head section-head--center">
          <h2 className="section-head__title">{title ?? BUY.title}</h2>
          <p className="section-head__deck">{body ?? BUY.body}</p>
        </div>

        <div className="flex flex-col items-stretch gap-3 sm:flex-row sm:justify-center">
          <ActionLink
            action={primary ?? BUY.primary}
            variant={buying ? "primary" : "secondary"}
            arrow={buying}
          />
          {(secondary === undefined ? (buying ? BUY.secondary : null) : secondary) ? (
            <ActionLink
              action={(secondary ?? BUY.secondary) as Action}
              variant="secondary"
            />
          ) : null}
        </div>
      </div>
    </section>
  );
}
