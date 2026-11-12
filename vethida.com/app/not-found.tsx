import Link from "next/link";

export default function NotFound() {
  return (
    <main className="masthead masthead--center">
      <div className="band__inner band__inner--prose">
        <p className="err-code" aria-hidden>
          404
        </p>
        <h1 className="masthead__title">Page not found</h1>
        <p className="masthead__deck">
          The page you were looking for has moved or never existed. If a link
          on this site sent you here, <Link href="/support">let us know</Link>.
        </p>
        <div className="masthead__actions">
          <Link href="/" className="btn btn-lg btn-primary">
            Back to home
          </Link>
        </div>
      </div>
    </main>
  );
}
