import {
  BoltIcon,
  CpuIcon,
  EraserIcon,
  GpuIcon,
  NetworkIcon,
  PrivacyIcon,
} from "@/components/icons";

// No card promises a number: nothing in this project measures frame rate or input lag.
const AREAS = [
  {
    icon: GpuIcon,
    title: "Graphics",
    body: "GPU scheduling, driver settings and an NVIDIA low-latency profile.",
  },
  {
    icon: CpuIcon,
    title: "Performance",
    body: "How Windows schedules the processor and manages memory.",
  },
  {
    icon: BoltIcon,
    title: "Power",
    body: "Performance power plans, plus hibernation and throttling settings.",
  },
  {
    icon: NetworkIcon,
    title: "Network",
    body: "Adapter, TCP/IP and DNS settings, and per-app traffic priority.",
  },
  {
    icon: PrivacyIcon,
    title: "Privacy",
    body: "Telemetry, the advertising ID and background app access, switched off.",
  },
  {
    icon: EraserIcon,
    title: "Debloat",
    body: "Preinstalled apps, Copilot, OneDrive and startup clutter, removed.",
  },
] as const;

export function Capabilities() {
  return (
    <section className="band band--tinted">
      <div className="band__inner">
        <div className="section-head section-head--center">
          <h2 className="section-head__title">A faster, cleaner Windows</h2>
          <p className="section-head__deck">
            Vethida adjusts the settings Windows ships with to suit your
            hardware.
          </p>
        </div>

        <div className="card-grid card-grid--six">
          {AREAS.map(({ icon: Icon, title, body }) => (
            <div key={title} className="feature-card gap-3">
              <span className="icon-plate">
                <Icon className="size-5" />
              </span>
              <h3 className="text-subhead font-semibold text-foreground">
                {title}
              </h3>
              <p className="text-meta text-foreground-muted">{body}</p>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
}
