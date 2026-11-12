// Loading placeholders. Hidden from screen readers, SkeletonPage announces "Loading" once.
export function Skeleton({ className = "" }: { className?: string }) {
  return <div aria-hidden className={`skeleton ${className}`} />;
}

export function SkeletonPage({
  children,
}: Readonly<{ children: React.ReactNode }>) {
  return (
    <div role="status" aria-busy aria-live="polite">
      <span className="sr-only">Loading</span>
      {children}
    </div>
  );
}

export function SkeletonHeader() {
  return (
    <div className="page-header">
      <Skeleton className="h-7 w-48" />
      <Skeleton className="mt-3 h-4 w-full max-w-md" />
    </div>
  );
}

export function SkeletonCard({ rows = 3 }: { rows?: number }) {
  return (
    <section className="card">
      <div className="card-header">
        <Skeleton className="h-4 w-32" />
      </div>
      <div className="card-body space-y-3">
        {Array.from({ length: rows }, (_, i) => (
          <Skeleton
            key={i}
            className="h-4"
            {...{ style: { width: `${[92, 74, 84, 60][i % 4]}%` } }}
          />
        ))}
      </div>
    </section>
  );
}
