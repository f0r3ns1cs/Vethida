"use client";

import * as React from "react";
import { MotionConfig, motion, useReducedMotion } from "motion/react";

import { Chevron, CHEVRON_SPRING } from "@/components/chevron";
import type { Faq } from "@/lib/faqs";

export function FaqAccordion({ items }: { items: Faq[] }) {
  const [openIdx, setOpenIdx] = React.useState<number | null>(null);
  const reduced = useReducedMotion();

  return (
    <MotionConfig transition={reduced ? { duration: 0 } : CHEVRON_SPRING}>
      <div className="faq">
        {items.map((item, i) => (
          <AccordionItem
            key={item.q}
            header={item.q}
            isOpen={openIdx === i}
            isLast={i === items.length - 1}
            onToggle={() => setOpenIdx(openIdx === i ? null : i)}
          >
            <p>{item.a}</p>
          </AccordionItem>
        ))}
      </div>
    </MotionConfig>
  );
}

type ItemProps = {
  header: string;
  isOpen: boolean;
  isLast: boolean;
  onToggle: () => void;
  children: React.ReactNode;
};

function AccordionItem({
  header,
  isOpen,
  isLast,
  onToggle,
  children,
}: ItemProps) {
  const id = React.useId();

  return (
    <motion.section
      initial={false}
      animate={isOpen ? "open" : "closed"}
      className="faq__item"
    >
      <h3 className="faq__heading">
        <button
          id={`${id}-button`}
          type="button"
          aria-expanded={isOpen}
          aria-controls={id}
          onClick={onToggle}
          className="faq__trigger"
        >
          <span className="faq__question">{header}</span>
          <Chevron open={isOpen} size={20} className="faq__chevron" />
        </button>
      </h3>

      <motion.div
        id={id}
        role="region"
        aria-labelledby={`${id}-button`}
        variants={{
          open: {
            height: "auto",
            maskImage:
              "linear-gradient(to bottom, black 100%, transparent 100%)",
          },
          closed: {
            height: 0,
            maskImage: "linear-gradient(to bottom, black 50%, transparent 100%)",
          },
        }}
        className="faq__panel"
      >
        <motion.div
          variants={{
            open: { opacity: 1 },
            closed: { opacity: 0 },
          }}
          className="faq__panel-inner"
        >
          {children}
        </motion.div>
      </motion.div>

      {isLast ? null : <hr className="faq__divider" aria-hidden />}
    </motion.section>
  );
}
