"use client";

// no will-change here, it makes the rotated icon blurry

import { motion, useReducedMotion } from "motion/react";

// also used by the FAQ panel
export const CHEVRON_SPRING = {
  type: "spring",
  visualDuration: 0.4,
  bounce: 0,
} as const;

export function Chevron({
  open,
  size = 16,
  className,
}: {
  open: boolean;
  size?: number;
  className?: string;
}) {
  const reduced = useReducedMotion();

  return (
    <motion.svg
      xmlns="http://www.w3.org/2000/svg"
      width={size}
      height={size}
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="2"
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden
      animate={open ? "open" : "closed"}
      variants={{ open: { rotate: 180 }, closed: { rotate: 0 } }}
      transition={reduced ? { duration: 0 } : CHEVRON_SPRING}
      className={className}
    >
      <path d="m6 9 6 6 6-6" />
    </motion.svg>
  );
}
