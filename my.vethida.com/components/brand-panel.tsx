export function BrandPanel({ headline }: { headline: string }) {
  return (
    <aside
      className="hidden bg-foreground p-12 lg:flex lg:flex-col lg:justify-end"
      aria-hidden
    >
      <p className="max-w-[16ch] text-balance text-[2.125rem] font-semibold leading-[1.12] tracking-[-0.025em] text-background">
        {headline}
      </p>
    </aside>
  );
}
