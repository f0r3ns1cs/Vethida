import fs from "node:fs/promises";
import path from "node:path";

export interface EmailMessage {
  to: string;
  subject: string;
  html: string;
  text: string;
  from?: string;
  replyTo?: string;
}

// EMAIL_TRANSPORT=file writes mail to .mail/ for local dev (otherwise order.paid
// keeps failing and Polar retries forever). Ignored in production.
function fileTransportDir(): string | null {
  if (process.env.EMAIL_TRANSPORT !== "file") return null;
  if (process.env.NODE_ENV === "production") {
    console.error("[email] EMAIL_TRANSPORT=file is refused in production");
    return null;
  }
  return path.join(process.cwd(), ".mail");
}

async function writeToDisk(dir: string, msg: EmailMessage): Promise<boolean> {
  try {
    await fs.mkdir(dir, { recursive: true });
    const stamp = new Date().toISOString().replace(/[:.]/g, "-");
    const slug = msg.to.replace(/[^a-zA-Z0-9]+/g, "_").slice(0, 40);
    const base = path.join(dir, `${stamp}_${slug}`);
    await fs.writeFile(
      `${base}.html`,
      `<!-- to: ${msg.to}\n     subject: ${msg.subject} -->\n${msg.html}`,
      "utf8",
    );
    await fs.writeFile(
      `${base}.txt`,
      `To: ${msg.to}\nSubject: ${msg.subject}\n\n${msg.text}`,
      "utf8",
    );
    console.log(`[email] wrote ${base}.html`);
    return true;
  } catch (err) {
    console.error("[email] file transport failed", err);
    return false;
  }
}

export async function sendEmail(msg: EmailMessage): Promise<boolean> {
  const dir = fileTransportDir();
  if (dir) return writeToDisk(dir, msg);

  const apiKey = process.env.RESEND_API_KEY;
  if (!apiKey) {
    console.error("[email] RESEND_API_KEY is not set; cannot deliver");
    return false;
  }

  try {
    const res = await fetch("https://api.resend.com/emails", {
      method: "POST",
      headers: {
        Authorization: `Bearer ${apiKey}`,
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        from: msg.from ?? process.env.RESEND_FROM ?? "Vethida <license@vethida.com>",
        to: [msg.to],
        reply_to: msg.replyTo,
        subject: msg.subject,
        html: msg.html,
        text: msg.text,
      }),
    });

    if (!res.ok) {
      const detail = await res.text().catch(() => "");
      console.error("[email] Resend error", res.status, detail);
      return false;
    }
    return true;
  } catch (err) {
    console.error("[email] Resend fetch failed", err);
    return false;
  }
}
