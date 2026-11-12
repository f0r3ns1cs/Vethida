import { NextResponse, type NextRequest } from "next/server";
import { tierLabel, type License } from "@vethida/db";
import { auth } from "@/lib/auth";
import {
  findActiveSubscriptionForTier,
  findPerpetualLicenseForTier,
  findUpgradeableProLicense,
} from "@/lib/licenses";
import {
  createUpgradeCredit,
  fetchProductAmount,
  getPolar,
  getPolarProductId,
  isCadence,
  upgradeCreditMinor,
  type Tier,
} from "@/lib/polar";
import {
  clientIp,
  contentLengthExceeds,
  createRateLimiter,
} from "@/lib/rate-limit";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

const MAX_BODY_BYTES = 1024;
const isLimited = createRateLimiter({ windowMs: 60 * 1000, max: 10 });

export async function POST(req: NextRequest) {
  try {
    if (contentLengthExceeds(req, MAX_BODY_BYTES)) {
      return NextResponse.json({ error: "Payload too large." }, { status: 413 });
    }
    if (isLimited(clientIp(req))) {
      return NextResponse.json({ error: "Too many requests." }, { status: 429 });
    }

    const session = await auth.api.getSession({ headers: req.headers });
    if (!session) {
      return NextResponse.json(
        { error: "Sign in to purchase a license." },
        { status: 401 },
      );
    }

    let body: { tier?: unknown; cadence?: unknown };
    try {
      body = await req.json();
    } catch {
      return NextResponse.json(
        { error: "Invalid request body." },
        { status: 400 },
      );
    }
    const tier = body.tier;
    if (tier !== "pro" && tier !== "ultimate") {
      return NextResponse.json(
        { error: "tier must be 'pro' or 'ultimate'." },
        { status: 400 },
      );
    }

    // yearly if not given
    const requestedCadence =
      body.cadence === undefined ? "yearly" : body.cadence;
    if (!isCadence(requestedCadence)) {
      return NextResponse.json(
        { error: "That billing interval isn't one we sell. Reload and try again." },
        { status: 400 },
      );
    }
    const cadence = requestedCadence;
    const plan = tierLabel(tier);

    // already has lifetime on this tier
    const perpetual = await findPerpetualLicenseForTier(session.user.id, tier);
    if (perpetual) {
      return NextResponse.json(
        {
          error: `You already own ${plan} for life. There's nothing further to buy on this tier.`,
        },
        { status: 409 },
      );
    }

    // No second subscription to the same tier (double clicks, stale tabs).
    // Buying lifetime on top of a subscription is fine, it replaces it below.
    const duplicate = await findActiveSubscriptionForTier(session.user.id, tier);
    if (duplicate && cadence !== "lifetime") {
      return NextResponse.json(
        {
          error: `You already have an active ${plan} subscription. Manage it from the billing page.`,
        },
        { status: 409 },
      );
    }

    // Ultimate already includes Pro
    if (tier === "pro") {
      const ultimate = await findActiveSubscriptionForTier(
        session.user.id,
        "ultimate",
      );
      if (ultimate) {
        return NextResponse.json(
          {
            error:
              "You're on Ultimate, which already includes everything in Pro. To move to Pro, cancel Ultimate on the billing page, then subscribe to Pro once the term you have paid for ends.",
          },
          { status: 409 },
        );
      }
    }

    const productId = getPolarProductId(tier, cadence);

    // the license this purchase replaces, if any
    let supersedes: { license: License; tier: Tier } | null = null;

    // subscription -> lifetime on the same tier, the webhook cancels the subscription
    if (duplicate && cadence === "lifetime") {
      supersedes = { license: duplicate, tier };
    }

    // Pro -> Ultimate, credited with what's left of the Pro license
    if (tier === "ultimate" && !supersedes) {
      const upgradeFrom = await findUpgradeableProLicense(session.user.id);

      // lifetime Pro can only move to lifetime Ultimate
      if (upgradeFrom && !upgradeFrom.expiresAt && cadence !== "lifetime") {
        return NextResponse.json(
          {
            error:
              "You own Pro for life. Moving up to Ultimate on a subscription would retire that, so the upgrade from a lifetime license is Ultimate lifetime. Contact support if you want something else.",
          },
          { status: 409 },
        );
      }
      if (upgradeFrom) supersedes = { license: upgradeFrom, tier: "pro" };
    }

    if (!productId) {
      console.error(
        "[checkout] no product id configured",
        `tier=${tier} cadence=${cadence}`,
      );
      const unavailable: Record<typeof cadence, string> = {
        monthly:
          "Monthly billing isn't available for this plan yet. Choose yearly, or contact support.",
        lifetime:
          "A lifetime license isn't available for this plan yet. Choose monthly or yearly, or contact support.",
        yearly: "Checkout is not available right now.",
      };
      return NextResponse.json({ error: unavailable[cadence] }, { status: 503 });
    }

    // Credit is created before the checkout, and any failure here fails the
    // request (502) rather than charging full price. Trials get no credit.
    let discountId: string | null = null;
    if (supersedes && !supersedes.license.trial) {
      const now = new Date();
      const paidCadence =
        supersedes.license.billingInterval === "month"
          ? "monthly"
          : supersedes.license.billingInterval === "year"
            ? "yearly"
            : "lifetime";
      const [paid, target] = await Promise.all([
        fetchProductAmount(supersedes.tier, paidCadence),
        fetchProductAmount(tier, cadence),
      ]);
      const amountMinor = upgradeCreditMinor({
        paidMinor: paid.minor,
        expiresAt: supersedes.license.expiresAt,
        billingInterval: supersedes.license.billingInterval,
        targetMinor: target.minor,
        now,
      });
      if (amountMinor > 0) {
        discountId = await createUpgradeCredit({
          amountMinor,
          currency: target.currency,
          productId,
          licenseId: supersedes.license.id,
          userId: session.user.id,
          now,
        });
      }
    }

    const checkout = await getPolar().checkouts.create({
      products: [productId],
      ...(discountId ? { discountId } : {}),
      allowDiscountCodes: false,
      customerEmail: session.user.email,
      // so the portal session works before the webhook stores the customer id
      externalCustomerId: session.user.id,
      // read by the api.vethida.com webhook. Values must be scalars, so
      // supersedesLicenseId is left out when there's nothing to replace.
      metadata: {
        userId: session.user.id,
        tier,
        cadence,
        ...(supersedes ? { supersedesLicenseId: supersedes.license.id } : {}),
      },
      successUrl: `${process.env.BETTER_AUTH_URL ?? "https://my.vethida.com"}/dashboard/licenses?status=success`,
    });

    return NextResponse.json({ url: checkout.url });
  } catch (err) {
    console.error("[checkout] failed to create checkout session", err);
    return NextResponse.json(
      { error: "Couldn't start checkout. Please try again shortly." },
      { status: 502 },
    );
  }
}
