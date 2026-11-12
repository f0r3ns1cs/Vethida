"use client";

import { useEffect, useRef } from "react";

const LENGTH = 6;

// 6 boxes for the sign-in code. The parent owns the value.
export function OtpInput({
  value,
  onChange,
  onComplete,
  disabled,
  invalid,
  takeFocus,
}: {
  value: string;
  onChange: (next: string) => void;
  onComplete?: (code: string) => void; // after the 6th digit
  disabled?: boolean;
  invalid?: boolean;
  takeFocus?: boolean; // focus box 1 on mount, the email input that had focus is gone
}) {
  const refs = useRef<(HTMLInputElement | null)[]>([]);

  useEffect(() => {
    if (takeFocus) refs.current[0]?.focus();
  }, [takeFocus]);

  function write(next: string, focusIndex: number) {
    const clean = next.replace(/\D/g, "").slice(0, LENGTH);
    onChange(clean);
    const target = Math.min(focusIndex, LENGTH - 1);
    refs.current[target]?.focus();
    if (clean.length === LENGTH) onComplete?.(clean);
  }

  return (
    <div
      className="flex gap-2"
      role="group"
      aria-label="Six-digit sign-in code"
    >
      {Array.from({ length: LENGTH }, (_, i) => (
        <input
          key={i}
          ref={(el) => {
            refs.current[i] = el;
          }}
          value={value[i] ?? ""}
          disabled={disabled}
          inputMode="numeric"
          autoComplete={i === 0 ? "one-time-code" : "off"}
          maxLength={1}
          aria-label={`Digit ${i + 1}`}
          aria-invalid={invalid || undefined}
          className={`h-14 w-full min-w-0 rounded-lg border bg-background text-center font-mono text-xl text-foreground transition-colors focus:outline-none focus:ring-3 focus:ring-accent/20 disabled:opacity-60 ${
            invalid
              ? "border-danger focus:border-danger"
              : "border-border-strong focus:border-accent"
          }`}
          onChange={(e) => {
            const typed = e.target.value.replace(/\D/g, "");
            if (!typed) return;
            const chars = value.split("");
            chars[i] = typed[typed.length - 1];
            write(chars.join("").slice(0, LENGTH), i + 1);
          }}
          onKeyDown={(e) => {
            if (e.key === "Backspace") {
              e.preventDefault();
              if (value[i]) {
                const chars = value.split("");
                chars[i] = "";
                write(chars.join(""), i);
              } else if (i > 0) {
                const chars = value.split("");
                chars[i - 1] = "";
                write(chars.join(""), i - 1);
              }
            } else if (e.key === "ArrowLeft" && i > 0) {
              refs.current[i - 1]?.focus();
            } else if (e.key === "ArrowRight" && i < LENGTH - 1) {
              refs.current[i + 1]?.focus();
            }
          }}
          onPaste={(e) => {
            // pasting the whole code
            e.preventDefault();
            const pasted = e.clipboardData.getData("text");
            write(pasted, pasted.replace(/\D/g, "").length);
          }}
        />
      ))}
    </div>
  );
}
