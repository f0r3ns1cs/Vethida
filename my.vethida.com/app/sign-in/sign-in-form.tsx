"use client";

import { useEffect, useRef, useState } from "react";
import { useRouter } from "next/navigation";
import { emailOtp, signIn } from "@/lib/auth-client";
import { OtpInput } from "@/components/otp-input";
import { Turnstile, isTurnstileConfigured } from "@/components/turnstile";
import {
  readNextFromLocation,
  rememberPostAuthRedirect,
  takePostAuthRedirect,
} from "@/lib/post-auth-redirect";

type Step = "email" | "code";

const RESEND_AFTER = 30; // seconds, UI only (server allows 3 sends / 15 min)

// our own wording, anything else gets the generic message
const MESSAGES: Record<string, string> = {
  INVALID_OTP: "That code is wrong. Check it and try again.",
  OTP_EXPIRED: "That code has expired. Request a new one.",
  TOO_MANY_ATTEMPTS: "Too many tries on that code. Request a new one.",
  INVALID_EMAIL: "That address does not look right. Check it and try again.",
};

const GENERIC_SEND = "We couldn't send the code. Try again in a moment.";
const GENERIC_VERIFY = "That code is wrong or has expired. Request a new one.";

function messageFor(
  error: { code?: string } | null | undefined,
  fallback: string,
): string {
  const code = error?.code;
  return (code && MESSAGES[code]) || fallback;
}

export function SignInForm({
  onStepChange,
}: {
  onStepChange?: (s: Step) => void;
}) {
  const router = useRouter();
  const [step, setStep] = useState<Step>("email");
  const [email, setEmail] = useState("");
  const [code, setCode] = useState("");
  const [error, setError] = useState<string | null>(null);
  const [pending, setPending] = useState(false);
  const [cooldown, setCooldown] = useState(0);
  const submitted = useRef(false);

  // single-use token, reset after every send
  const [turnstileToken, setTurnstileToken] = useState<string | null>(null);
  const [turnstileNonce, setTurnstileNonce] = useState(0);
  const turnstileRequired = isTurnstileConfigured();

  useEffect(() => {
    rememberPostAuthRedirect(readNextFromLocation());
  }, []);

  useEffect(() => {
    onStepChange?.(step);
  }, [step, onStepChange]);

  useEffect(() => {
    if (cooldown <= 0) return;
    const t = setTimeout(() => setCooldown((c) => c - 1), 1000);
    return () => clearTimeout(t);
  }, [cooldown]);

  async function sendCode(target: string) {
    // fast submits can beat the widget
    if (turnstileRequired && !turnstileToken) {
      setError(
        "Still running a quick security check. Try again in a moment.",
      );
      return false;
    }

    setError(null);
    setPending(true);

    // catch too, otherwise a rejection leaves pending stuck on
    let sendError: { code?: string } | null = null;
    try {
      ({ error: sendError } = await emailOtp.sendVerificationOtp({
        email: target,
        type: "sign-in",
        fetchOptions: turnstileToken
          ? { headers: { "x-turnstile-token": turnstileToken } }
          : undefined,
      }));
    } catch {
      sendError = {};
    }

    setPending(false);

    setTurnstileToken(null);
    setTurnstileNonce((n) => n + 1);

    if (sendError) {
      setError(messageFor(sendError, GENERIC_SEND));
      return false;
    }
    setCooldown(RESEND_AFTER);
    return true;
  }

  async function verify(value: string) {
    // both the last digit and the button call this
    if (submitted.current) return;
    submitted.current = true;
    setError(null);
    setPending(true);

    let signInError: { code?: string } | null = null;
    try {
      ({ error: signInError } = await signIn.emailOtp({ email, otp: value }));
    } catch {
      signInError = {};
    }

    if (signInError) {
      submitted.current = false;
      setPending(false);
      setCode("");
      setError(messageFor(signInError, GENERIC_VERIFY));
      return;
    }
    router.push(takePostAuthRedirect());
    router.refresh();
  }

  if (step === "code") {
    return (
      <div className="space-y-5">
        <div
          role="status"
          className="rounded-lg border border-border bg-surface px-3.5 py-2.5 text-sm text-foreground-muted"
        >
          Sent to <span className="text-foreground">{email}</span>{" "}
          <button
            type="button"
            className="text-accent underline-offset-2 hover:underline"
            onClick={() => {
              setStep("email");
              setCode("");
              setError(null);
              submitted.current = false;
            }}
          >
            change
          </button>
        </div>

        <div>
          <p className="label">Six-digit code</p>
          <OtpInput
            value={code}
            onChange={setCode}
            onComplete={verify}
            disabled={pending}
            invalid={Boolean(error)}
            takeFocus
          />
        </div>

        {error ? (
          <div role="alert" className="alert alert-danger">
            <p>{error}</p>
          </div>
        ) : null}

        <button
          type="button"
          disabled={pending || code.length < 6}
          className="btn btn-primary w-full"
          onClick={() => verify(code)}
        >
          {pending ? "Checking..." : "Continue"}
        </button>

        <p className="text-xs leading-relaxed text-foreground-faint">
          Didn&apos;t arrive? Check spam, then{" "}
          <button
            type="button"
            disabled={cooldown > 0 || pending}
            className="text-accent underline-offset-2 hover:underline disabled:text-foreground-faint disabled:no-underline"
            onClick={async () => {
              setCode("");
              submitted.current = false;
              await sendCode(email);
            }}
          >
            {cooldown > 0
              ? `send a new code in ${cooldown}s`
              : "send a new code"}
          </button>
          . Three wrong attempts and the code stops working.
        </p>

        {/* resend needs a token too */}
        <Turnstile onVerify={setTurnstileToken} resetSignal={turnstileNonce} />
      </div>
    );
  }

  return (
    <form
      className="space-y-4"
      onSubmit={async (e) => {
        e.preventDefault();
        const target = email.trim();
        if (!target) return;
        if (await sendCode(target)) {
          setEmail(target);
          setStep("code");
        }
      }}
    >
      <div>
        <label htmlFor="email" className="label">
          Email address
        </label>
        <input
          id="email"
          type="email"
          inputMode="email"
          autoComplete="email"
          required
          placeholder="you@example.com"
          className="input"
          value={email}
          onChange={(e) => setEmail(e.target.value)}
        />
      </div>

      {error ? (
        <div role="alert" className="alert alert-danger">
          <p>{error}</p>
        </div>
      ) : null}

      <button
        type="submit"
        disabled={pending}
        className="btn btn-primary w-full"
      >
        {pending ? "Sending..." : "Send code"}
      </button>

      <Turnstile onVerify={setTurnstileToken} resetSignal={turnstileNonce} />
    </form>
  );
}
