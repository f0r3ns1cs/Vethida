# api.vethida.com

The licensing API. Stripe sells the plans through Managed Payments, as merchant of
record; this service issues one license key per completed checkout and answers the
desktop app's activate, validate and deactivate calls. The routes are listed at the
top of `src/server.js`.

```
npm ci
cp .env.example .env      # then fill it in
npm start
```

`DB_PATH` (default `data/vethida.db`) is the only state: every license, activation and
processed Stripe event. `KEY_SECRET` decrypts the stored keys. Back up both, and keep
them apart: the database without the secret still validates keys but can no longer
show or resend them.

## Stripe setup

Everything below lives in the Stripe dashboard, so nothing in the code can check it.

1. Turn on Managed Payments for the account.
2. One product per tier, with metadata `tier` set to `pro` or `ultimate` on the product
   or on each price. A checkout whose price carries neither is refused, loudly, rather
   than issued as the wrong plan.
3. Three prices per product: monthly, yearly, and one-time. A one-time price is a
   lifetime license; a recurring one follows its subscription.
4. One Payment Link per price. Under After payment, redirect to
   `https://api.vethida.com/checkout/success?session_id={CHECKOUT_SESSION_ID}`, which
   shows the key. Copy each link into `vethida.com/lib/pricing.ts`.
5. A webhook endpoint at `https://api.vethida.com/webhooks/stripe` sending exactly the
   events `handleEvent` in `src/stripe.js` switches on. Put its signing secret in
   `STRIPE_WEBHOOK_SECRET`.
6. The customer portal (Settings > Billing > Customer portal): allow card updates, plan
   switches and cancellation, and copy its login link into the `/billing` redirect in
   `vethida.com/next.config.ts`.

`node scripts/release-ready.mjs` from the repository root fails until steps 4 and 6
are done.

## Revoking a key

A full refund disables the key, and so does a dispute while it is open. A dispute
decided in our favor gives the key back; a lost one leaves it disabled. Anything else
is by hand:

```
npm run admin -- find <email|key>
npm run admin -- revoke <key>        # fraud, resale, a key posted publicly
npm run admin -- restore <key>       # a revocation made in error
npm run admin -- reset <key>         # frees every activation slot
```

Never revoke because a customer went quiet or because the card behind a lifetime key
expired. A revoked key drops the app to Free on its next check; applied tweaks stay
where they are and roll back as before.

## Telling customers about a change

The Privacy Policy and the Terms promise notice before a material change takes effect.
`npm run admin -- emails` prints every address that has bought a license. Send the
notice to that list through Resend before the new effective date, and keep the message
and the date it went out with the business records. The changed document itself is
dated by its commit.

## Rotating secrets

`STRIPE_WEBHOOK_SECRET`: roll it in Stripe and deploy the new value before the old one
stops being accepted. `RESEND_API_KEY`: create the new key, deploy it, then revoke the
old one. `KEY_SECRET` is not rotated; every stored key is encrypted under it.
