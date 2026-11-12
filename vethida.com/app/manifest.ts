import type { MetadataRoute } from "next";

import { siteDescription, siteTitle } from "@/lib/site";

export default function manifest(): MetadataRoute.Manifest {
  return {
    name: siteTitle,
    short_name: "Vethida",
    description: siteDescription,
    start_url: "/",
    display: "standalone",
    background_color: "#ffffff",
    theme_color: "#ffffff",
    icons: [
      {
        src: "/icon-128.png",
        sizes: "128x128",
        type: "image/png",
      },
      {
        src: "/icon-256.png",
        sizes: "256x256",
        type: "image/png",
      },
    ],
  };
}
