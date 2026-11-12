import { createAuthClient } from "better-auth/react";
import {
  emailOTPClient,
  inferAdditionalFields,
} from "better-auth/client/plugins";
import type { auth } from "@/lib/auth";

// type-only import so the extra user fields are typed client-side
export const authClient = createAuthClient({
  plugins: [inferAdditionalFields<typeof auth>(), emailOTPClient()],
});

export const {
  signIn,
  signOut,
  useSession,
  emailOtp,
  linkSocial,
  unlinkAccount,
} = authClient;
