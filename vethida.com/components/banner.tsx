import Link from "next/link";

import { ArrowRightIcon } from "@/components/icons";
import { isPublished } from "@/lib/download";
import { currentRelease } from "@/lib/releases";

// A release claim, so it renders only once the installer feed carries a build.
export function Banner() {
  if (!isPublished) return null;

  return (
    <div className="site-banner">
      <div className="band__inner band__inner--full site-banner__inner">
        <span className="tier-pill tier-pill--strong site-banner__tag">
          {currentRelease.version}
        </span>

        <span>Vethida {currentRelease.version} is available.</span>

        <Link href="/changelog" className="site-banner__link">
          See what changed
          <ArrowRightIcon className="btn-arrow size-4" />
        </Link>
      </div>
    </div>
  );
}
