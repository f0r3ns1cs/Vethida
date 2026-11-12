"use client";

import { useState } from "react";
import { AuthCard } from "@/components/auth-card";
import { SignInForm } from "./sign-in-form";

const PANEL_HEADLINE = "Take control of how Windows runs.";

export function SignInScreen() {
  const [step, setStep] = useState<"email" | "code">("email");

  return (
    <AuthCard
      title={step === "email" ? "Sign in to your account" : "Enter your code"}
      subtitle={
        step === "email"
          ? "Please enter your email address. If you have bought Vethida before, use the same address to reach your licenses and purchase history."
          : "A six-digit code has been sent to your email address. It expires in five minutes."
      }
      panelHeadline={PANEL_HEADLINE}
    >
      <SignInForm onStepChange={setStep} />
    </AuthCard>
  );
}
