import fs from "node:fs";
import path from "node:path";

import Image from "next/image";

// Homepage screenshot. Size is read from the file header, a broken file fails the build.

const PUBLIC_DIR = path.join(process.cwd(), "public");

const CANDIDATES = ["app-dashboard.png", "app-dashboard.webp"] as const;

type Size = { width: number; height: number };

// PNG: IHDR width/height at 16 and 20, big-endian
function pngSize(head: Buffer): Size | null {
  if (head.toString("ascii", 12, 16) !== "IHDR") return null;
  return { width: head.readUInt32BE(16), height: head.readUInt32BE(20) };
}

// WebP: depends on the chunk type (VP8X / VP8 / VP8L)
function webpSize(head: Buffer): Size | null {
  if (head.toString("ascii", 8, 12) !== "WEBP") return null;
  const chunk = head.toString("ascii", 12, 16);

  if (chunk === "VP8X") {
    return {
      width: (head.readUIntLE(24, 3) & 0xffffff) + 1,
      height: (head.readUIntLE(27, 3) & 0xffffff) + 1,
    };
  }
  if (chunk === "VP8 ") {
    return {
      width: head.readUInt16LE(26) & 0x3fff,
      height: head.readUInt16LE(28) & 0x3fff,
    };
  }
  if (chunk === "VP8L") {
    const bits = head.readUInt32LE(21);
    return {
      width: (bits & 0x3fff) + 1,
      height: ((bits >> 14) & 0x3fff) + 1,
    };
  }
  return null;
}

function readShot(): { file: string; size: Size } | null {
  for (const file of CANDIDATES) {
    const full = path.join(PUBLIC_DIR, file);
    let fd: number;
    try {
      fd = fs.openSync(full, "r");
    } catch {
      continue; // not there, that's fine
    }

    let head: Buffer;
    try {
      head = Buffer.alloc(32);
      if (fs.readSync(fd, head, 0, 32, 0) < 32) head = Buffer.alloc(0);
    } finally {
      fs.closeSync(fd);
    }

    const size = head.length ? (pngSize(head) ?? webpSize(head)) : null;
    if (size && size.width > 0 && size.height > 0) return { file, size };

    const magic = head.length ? head.subarray(0, 4).toString("hex") : "(empty)";
    throw new Error(
      `public/${file} exists but is not a readable PNG or WebP ` +
        `(first bytes ${magic}). Re-export it as PNG or WebP, or remove it.`,
    );
  }
  return null;
}

const shot = readShot();

export const hasProductShot = shot !== null;

export function ProductShot() {
  if (!shot) return null;

  return (
    <div className="masthead__media">
      <div className="hero-shot">
        <Image
          src={`/${shot.file}`}
          alt="The Vethida window, showing the dashboard."
          width={shot.size.width}
          height={shot.size.height}
          sizes="(min-width: 1024px) 34rem, 100vw"
          priority
        />
      </div>
    </div>
  );
}
