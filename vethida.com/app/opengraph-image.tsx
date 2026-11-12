import { ImageResponse } from "next/og";

// OG card, no external fonts
export const alt = "Vethida - optimize, debloat, and speed up your PC";
export const size = { width: 1200, height: 630 };
export const contentType = "image/png";

// copied from globals.css, ImageResponse can't read CSS variables
const FOREGROUND = "#0c1424";
const MUTED = "#5b6472";
const ACCENT = "#003cce";
const BACKGROUND = "#ffffff";
const BORDER = "#e9edf2";

export default function OpengraphImage() {
  return new ImageResponse(
    (
      <div
        style={{
          width: "100%",
          height: "100%",
          display: "flex",
          flexDirection: "column",
          justifyContent: "space-between",
          background: BACKGROUND,
          padding: "72px 80px",
          borderTop: `16px solid ${ACCENT}`,
        }}
      >
        <div style={{ display: "flex", flexDirection: "column" }}>
          <div
            style={{
              fontSize: 34,
              letterSpacing: "0.24em",
              textTransform: "uppercase",
              color: ACCENT,
              fontWeight: 600,
            }}
          >
            Vethida
          </div>
          <div
            style={{
              marginTop: 36,
              fontSize: 82,
              lineHeight: 1.08,
              letterSpacing: "-0.03em",
              color: FOREGROUND,
              fontWeight: 700,
              maxWidth: 940,
            }}
          >
            Optimize, debloat, and speed up your PC.
          </div>
        </div>

        <div
          style={{
            display: "flex",
            justifyContent: "space-between",
            alignItems: "flex-end",
            borderTop: `2px solid ${BORDER}`,
            paddingTop: 32,
            fontSize: 30,
            color: MUTED,
          }}
        >
          <div style={{ display: "flex" }}>
            Windows optimization for Windows 10 and 11
          </div>
          <div style={{ display: "flex", color: FOREGROUND, fontWeight: 600 }}>
            vethida.com
          </div>
        </div>
      </div>
    ),
    size,
  );
}
