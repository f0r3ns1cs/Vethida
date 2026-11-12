const esc = (s) =>
  String(s).replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[c]);

function page(title, body) {
  return `<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="robots" content="noindex">
<title>${esc(title)}: Vethida</title>
<style>
  :root { color-scheme: light; }
  body { margin: 0; min-height: 100vh; display: grid; place-items: center;
         background: #f6f8fa; color: #0c1424;
         font: 16px/1.6 "IBM Plex Sans", ui-sans-serif, system-ui, sans-serif; }
  main { width: min(32rem, calc(100% - 2.5rem)); padding: 2.5rem 2rem;
         background: #fff; border: 1px solid #c6ccd6; border-radius: 1.125rem;
         box-shadow: 0 1px 2px rgb(12 20 36 / .05), 0 4px 10px -6px rgb(12 20 36 / .1); }
  h1 { margin: 0 0 .5rem; font-size: 1.5rem; letter-spacing: -.01em; }
  p { margin: 0 0 1rem; color: #5b6472; }
  .key { margin: 1.25rem 0; padding: .875rem 1rem; border: 1px solid #c6ccd6;
         border-radius: .625rem; background: #f6f8fa; text-align: center;
         font: 600 1.125rem/1.4 ui-monospace, Consolas, monospace; letter-spacing: .04em;
         color: #0c1424; user-select: all; overflow-wrap: anywhere; }
  label { display: block; margin-bottom: .375rem; font-size: .875rem; font-weight: 600; }
  input { box-sizing: border-box; width: 100%; height: 2.75rem; padding: 0 .875rem;
          border: 1px solid #787a7e; border-radius: .5rem; font: inherit; }
  button, .btn { display: inline-flex; align-items: center; justify-content: center;
          height: 2.75rem; margin-top: 1rem; padding: 0 1.5rem; border: 0; border-radius: 999px;
          background: #0c1424; color: #fff; font: 600 1rem/1 inherit; text-decoration: none;
          cursor: pointer; }
  a { color: #003cce; }
  small { color: #6b7280; }
</style>
</head>
<body><main>${body}</main></body>
</html>`;
}

export function successPage({ key, tier, email }) {
  const name = tier === "ultimate" ? "Ultimate" : "Pro";
  return page(
    "Your license key",
    `<h1>Thanks for buying Vethida ${name}</h1>
     <p>Here is your license key. We also emailed it to ${esc(email)}.</p>
     <div class="key">${esc(key)}</div>
     <p>Open Vethida, go to <strong>Account</strong>, and paste the key.</p>
     <a class="btn" href="https://vethida.com/download">Download Vethida</a>`,
  );
}

export function pendingPage() {
  return page(
    "Payment processing",
    `<h1>Your payment is processing</h1>
     <p>Some payment methods take a little longer to confirm. We'll email your license key as soon as it does.</p>
     <p><small>Nothing arrived after an hour? Write to <a href="mailto:support@vethida.com">support@vethida.com</a>.</small></p>`,
  );
}

export function errorPage(message) {
  return page(
    "Something went wrong",
    `<h1>Something went wrong</h1>
     <p>${esc(message)}</p>
     <p><small>Write to <a href="mailto:support@vethida.com">support@vethida.com</a> and we'll sort it out.</small></p>`,
  );
}

export function lostKeyPage({ sent = false } = {}) {
  if (sent) {
    return page(
      "Check your email",
      `<h1>Check your email</h1>
       <p>If that address bought Vethida, its license keys are on their way. It can take a few minutes.</p>
       <p><small>Nothing arrived? Write to <a href="mailto:support@vethida.com">support@vethida.com</a>.</small></p>`,
    );
  }
  return page(
    "Find your license key",
    `<h1>Find your license key</h1>
     <p>Enter the email you bought with, and we'll send your keys to it.</p>
     <form method="post" action="/licenses/lost">
       <label for="email">Email</label>
       <input id="email" name="email" type="email" autocomplete="email" required>
       <button type="submit">Send my keys</button>
     </form>`,
  );
}
