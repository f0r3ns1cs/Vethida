// RFC 9116 - security.txt requires Contact and Expires fields.
// Expires is regenerated on each build/deploy, one year forward.

export const dynamic = "force-static";

const expiresAt = new Date(
  Date.now() + 365 * 24 * 60 * 60 * 1000,
).toISOString();

const body = `Contact: mailto:security@vethida.com
Expires: ${expiresAt}
Preferred-Languages: en
Canonical: https://vethida.com/.well-known/security.txt
Policy: https://vethida.com/security
`;

export function GET() {
  return new Response(body, {
    headers: {
      "Content-Type": "text/plain; charset=utf-8",
      "Cache-Control": "public, max-age=86400",
      "X-Robots-Tag": "noindex",
    },
  });
}
