interface LegalPageProps {
  title: string;
  /** The date the document takes effect, formatted for reading. */
  updated?: string;
  children: React.ReactNode;
}

export function LegalPage({ title, updated, children }: LegalPageProps) {
  return (
    <main>
      <section className="hero-wash masthead">
        <div className="band__inner band__inner--prose">
          <h1 className="masthead__title">{title}</h1>
          {/* "Effective", because nothing records when a document was last revised. */}
          {updated ? (
            <p className="masthead__deck">Effective {updated}</p>
          ) : null}
        </div>
      </section>

      <section className="band band--ruled">
        <div className="band__inner band__inner--prose">
          <article className="prose">{children}</article>
        </div>
      </section>
    </main>
  );
}
