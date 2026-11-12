import {
  Skeleton,
  SkeletonCard,
  SkeletonHeader,
  SkeletonPage,
} from "@/components/skeleton";

// default for /dashboard routes, shaped like the overview page
export default function Loading() {
  return (
    <SkeletonPage>
      <SkeletonHeader />
      <div className="grid gap-4 sm:grid-cols-3">
        {[0, 1, 2].map((i) => (
          <section key={i} className="card">
            <div className="card-body">
              <Skeleton className="h-3 w-20" />
              <Skeleton className="mt-2 h-6 w-28" />
            </div>
          </section>
        ))}
      </div>
      <div className="mt-4 space-y-4">
        <SkeletonCard rows={2} />
        <SkeletonCard rows={3} />
      </div>
    </SkeletonPage>
  );
}
