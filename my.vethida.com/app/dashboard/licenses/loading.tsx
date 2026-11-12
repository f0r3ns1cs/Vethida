import {
  SkeletonCard,
  SkeletonHeader,
  SkeletonPage,
} from "@/components/skeleton";

export default function Loading() {
  return (
    <SkeletonPage>
      <SkeletonHeader />
      <div className="space-y-4">
        <SkeletonCard rows={4} />
        <SkeletonCard rows={3} />
      </div>
    </SkeletonPage>
  );
}
