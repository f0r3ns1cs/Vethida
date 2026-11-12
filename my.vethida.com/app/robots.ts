import type { MetadataRoute } from "next";

// nothing on the portal gets indexed
export default function robots(): MetadataRoute.Robots {
  return {
    rules: { userAgent: "*", disallow: "/" },
  };
}
