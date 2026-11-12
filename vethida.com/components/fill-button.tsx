import type { ComponentType, SVGProps } from "react";
import Link from "next/link";

// hero button, the animation is .btn-fx in globals.css

type FillButtonProps = {
  href: string;
  label: string; // rendered twice, the second copy is aria-hidden
  icon: ComponentType<SVGProps<SVGSVGElement>>;
};

export function FillButton({ href, label, icon: Icon }: FillButtonProps) {
  return (
    <Link href={href} className="btn btn-lg btn-fx">
      <span className="btn-fx__dot" aria-hidden>
        <span className="btn-fx__fill" />
      </span>
      <span className="btn-fx__label">{label}</span>
      <span className="btn-fx__swap" aria-hidden>
        {label}
        <Icon />
      </span>
    </Link>
  );
}
