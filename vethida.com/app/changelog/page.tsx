import type { Metadata } from "next";

import { PageCta } from "@/components/page-cta";
import { pageMetadata } from "@/lib/metadata";
import { CHANGE_KINDS, releases } from "@/lib/releases";

export const metadata: Metadata = pageMetadata({
  title: "Release notes",
  description: "What changed in each release of Vethida, newest first.",
  path: "/changelog",
});

function anchorFor(version: string): string {
  return `v${version.replace(/\./g, "-")}`;
}

export default function Page() {
  return (
    <main>
      <section className="hero-wash masthead">
        <div className="band__inner band__inner--text">
          <h1 className="masthead__title">Release notes</h1>
          <p className="masthead__deck">
            What changed in each release, newest first.
          </p>
        </div>
      </section>

      <section className="band band--tinted">
        <div className="band__inner band__inner--text">
          <ol className="release-list">
            {releases.map((release) => {
              const anchor = anchorFor(release.version);
              return (
                <li key={release.version} id={anchor} className="spec-panel">
                  <div className="release__head">
                    <h2 className="release__version">
                      <a href={`#${anchor}`}>Vethida {release.version}</a>
                    </h2>
                    {release.date ? (
                      <time className="release__date" dateTime={release.date}>
                        {release.date}
                      </time>
                    ) : null}
                  </div>

                  {CHANGE_KINDS.map((kind) => {
                    const notes = release.changes.filter(
                      (c) => c.kind === kind,
                    );
                    if (notes.length === 0) return null;
                    return (
                      <div key={kind} className="release__group">
                        <h3 className="release__kind">{kind}</h3>
                        <ul className="release__notes">
                          {notes.map((change) => (
                            <li key={change.note}>{change.note}</li>
                          ))}
                        </ul>
                      </div>
                    );
                  })}
                </li>
              );
            })}
          </ol>
        </div>
      </section>

      <PageCta variant="buy" />
    </main>
  );
}
