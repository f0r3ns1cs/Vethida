import type { Metadata } from "next";

import { DownloadIcon } from "@/components/icons";
import { formatSize, latestRelease } from "@/lib/download";
import { pageMetadata } from "@/lib/metadata";
import { currentRelease } from "@/lib/releases";

export const metadata: Metadata = pageMetadata({
  title: "Download",
  description: "Download Vethida for Windows 10 and 11.",
  path: "/download",
});

export default function Page() {
  const release = latestRelease;
  const version = release?.version ?? currentRelease.version;
  // The installer's published name, from scripts/Vethida.iss
  // (OutputBaseFilename=VethidaSetup-{version}).
  const href = release?.url ?? `/desktop/VethidaSetup-${version}.exe`;
  const size = release ? formatSize(release.sizeBytes) : null;

  return (
    <main>
      <section className="hero-wash masthead masthead--center dl-hero">
        <div className="band__inner">
          <h1 className="masthead__title">Download Vethida</h1>
          <p className="masthead__deck">Free for Windows 10 and 11.</p>
          <div className="masthead__actions">
            <a href={href} className="btn btn-lg btn-primary dl-hero__btn">
              <DownloadIcon />
              Download for Windows
            </a>
          </div>
          <p className="dl-hero__meta">
            Version {version}
            {size ? `, ${size}` : ""}, 64-bit
          </p>
          {release ? (
            <p className="dl-hero__hash">
              SHA-256 <code className="mono-key">{release.sha256}</code>
            </p>
          ) : null}
        </div>
      </section>
    </main>
  );
}
