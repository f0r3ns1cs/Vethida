import { BrandPanel } from "@/components/brand-panel";

// layout for signed-out pages: form left, brand panel right
export function AuthCard({
  title,
  subtitle,
  children,
  panelHeadline,
}: {
  title: string;
  subtitle?: string;
  children: React.ReactNode;
  panelHeadline: string;
}) {
  return (
    <div className="grid min-h-screen lg:grid-cols-2">
      <main className="flex flex-col px-6 py-10 sm:px-10 lg:px-14">
        <div className="enter mx-auto flex w-full max-w-sm flex-1 flex-col justify-center">
          <VethidaMark className="mb-9 h-6 w-32 text-foreground" />
          <h1 className="text-xl font-semibold tracking-tight sm:text-[1.3125rem]">
            {title}
          </h1>
          {subtitle ? (
            <p className="mt-2 max-w-[44ch] text-sm leading-relaxed text-foreground-muted">
              {subtitle}
            </p>
          ) : null}
          <div className="mt-6">{children}</div>
        </div>

        <footer className="mx-auto flex w-full max-w-sm flex-wrap items-center gap-x-4 gap-y-1 pt-8 text-xs text-foreground-faint">
          <span>&copy; {new Date().getFullYear()} Rens Engineering</span>
          <a
            href="https://vethida.com/legal/privacy"
            className="underline underline-offset-2 hover:text-foreground-muted"
          >
            Privacy
          </a>
          <a
            href="https://vethida.com/legal/terms"
            className="underline underline-offset-2 hover:text-foreground-muted"
          >
            Terms
          </a>
          <a
            href="mailto:support@vethida.com"
            className="underline underline-offset-2 hover:text-foreground-muted"
          >
            Support
          </a>
        </footer>
      </main>

      <BrandPanel headline={panelHeadline} />
    </div>
  );
}

// css mask so the wordmark uses currentColor
function VethidaMark({ className }: { className?: string }) {
  return (
    <span
      role="img"
      aria-label="Vethida"
      className={className}
      style={{
        display: "block",
        backgroundColor: "currentColor",
        WebkitMaskImage: "url(/vethida-wordmark.svg)",
        maskImage: "url(/vethida-wordmark.svg)",
        WebkitMaskRepeat: "no-repeat",
        maskRepeat: "no-repeat",
        WebkitMaskPosition: "left center",
        maskPosition: "left center",
        WebkitMaskSize: "contain",
        maskSize: "contain",
      }}
    />
  );
}
