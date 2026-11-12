import {
  DetectIcon,
  EraserIcon,
  GaugeIcon,
  GenesisIcon,
  GpuIcon,
  NetworkIcon,
  PeripheralIcon,
  PrivacyIcon,
  QuickFixIcon,
} from "@/components/icons";
import type { CapabilityGroup } from "@/lib/features";

// The one place a capability group's icon key becomes a drawing. Both the
// homepage grid and /features render from it.
const ICONS: Record<
  CapabilityGroup["icon"],
  (props: { className?: string }) => React.ReactElement
> = {
  detect: DetectIcon,
  gauge: GaugeIcon,
  gpu: GpuIcon,
  network: NetworkIcon,
  eraser: EraserIcon,
  privacy: PrivacyIcon,
  quickfix: QuickFixIcon,
  peripheral: PeripheralIcon,
  genesis: GenesisIcon,
};

export function FeatureIcon({
  icon,
  className,
}: {
  icon: CapabilityGroup["icon"];
  className?: string;
}) {
  const Icon = ICONS[icon];
  return <Icon className={className} />;
}
