"use client";

import { useRef } from "react";

type SegmentedProps<T extends string> = {
  label: string; // aria-label
  options: ReadonlyArray<{ value: T; label: string }>;
  value: T;
  onChange: (next: T) => void;
  // for tab panels: buttons get <name>-tab-<value>, panels need <name>-panel-<value>
  name?: string;
};

export function Segmented<T extends string>({
  label,
  options,
  value,
  onChange,
  name,
}: SegmentedProps<T>) {
  const buttons = useRef<Array<HTMLButtonElement | null>>([]);

  // arrow keys wrap, and focus follows the selection
  function select(index: number) {
    const next = (index + options.length) % options.length;
    onChange(options[next].value);
    buttons.current[next]?.focus();
  }

  return (
    <div
      role="tablist"
      aria-label={label}
      className="inline-flex flex-wrap items-center justify-center gap-1 rounded-full border border-[var(--border-card)] bg-surface p-1"
    >
      {options.map((option, index) => {
        const active = option.value === value;
        return (
          <button
            key={option.value}
            ref={(el) => {
              buttons.current[index] = el;
            }}
            type="button"
            role="tab"
            id={name ? `${name}-tab-${option.value}` : undefined}
            aria-controls={name ? `${name}-panel-${option.value}` : undefined}
            aria-selected={active}
            tabIndex={active ? 0 : -1}
            onClick={() => onChange(option.value)}
            onKeyDown={(event) => {
              const current = options.findIndex((o) => o.value === value);
              if (event.key === "ArrowRight") {
                event.preventDefault();
                select(current + 1);
              } else if (event.key === "ArrowLeft") {
                event.preventDefault();
                select(current - 1);
              } else if (event.key === "Home") {
                event.preventDefault();
                select(0);
              } else if (event.key === "End") {
                event.preventDefault();
                select(options.length - 1);
              }
            }}
            className="relative rounded-full px-5 py-2 text-meta font-medium transition-colors"
          >
            {active ? (
              <span
                aria-hidden
                className="absolute inset-0 rounded-full bg-background ring-1 ring-[var(--border-card)] [box-shadow:var(--shadow-card)]"
              />
            ) : null}
            <span
              className={`relative z-10 ${
                active ? "text-accent" : "text-foreground-muted"
              }`}
            >
              {option.label}
            </span>
          </button>
        );
      })}
    </div>
  );
}
