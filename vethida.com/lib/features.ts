// Capabilities for /features, grouped like the app, with the lowest tier for each.

export type TierKey = "free" | "pro" | "ultimate";

export type Capability = {
  name: string;
  detail: string;
  /** Lowest tier that includes this. */
  tier: TierKey;
  /** Where in the desktop application this lives: `<path>::<literal in that file>`. */
  evidence: string;
};

export type CapabilityGroup = {
  id: string;
  /** Icon key, resolved to a component by `components/feature-icon.tsx`. */
  icon:
    | "detect"
    | "gauge"
    | "gpu"
    | "network"
    | "eraser"
    | "privacy"
    | "quickfix"
    | "peripheral"
    | "genesis";
  title: string;
  body: string;
  /** A required warning about this group, rendered under its rows. */
  warning?: { title: string; body: string };
  capabilities: Capability[];
};

export const capabilityGroups: CapabilityGroup[] = [
  {
    id: "performance",
    icon: "gauge",
    title: "Processor, memory and power",
    body: "From the scheduler and memory down to the power plan and device power states.",
    capabilities: [
      {
        name: "Processor settings",
        detail:
          "How Windows shares the processor between the app you are using and everything else, and how often it reschedules.",
        tier: "pro",
        evidence:
          "Vethida/vth_core/Tweaks/TweakCatalogActive.cpp::kernel.scheduler-quantum-tuning",
      },
      {
        name: "Memory settings",
        detail:
          "The page file, memory compression, prefetch, and how the memory manager treats the kernel and the file cache.",
        tier: "pro",
        evidence:
          "Vethida/vth_core/Tweaks/TweakCatalogOperatorBatch.cpp::op.performance.memory-management",
      },
      {
        name: "File system settings",
        detail: "Long file paths and NTFS file-name tunneling.",
        tier: "free",
        evidence: "Vethida/vth_core/Tweaks/TweakCatalogActive.cpp::storage.ntfs-long-paths",
      },
      {
        name: "NVMe driver settings",
        detail: "The NVMe driver's performance throttling turned off.",
        tier: "ultimate",
        evidence:
          "Vethida/vth_core/Tweaks/TweakCatalogActive.cpp::storage.nvme.classpnp_disable_perf_throttling",
      },
      {
        name: "Power settings",
        detail:
          "Hibernation, fast startup, timer coalescing, power throttling and Windows' energy tracking.",
        tier: "pro",
        evidence: "Vethida/vth_core/Tweaks/TweakCatalogOperatorGpuPower.cpp::power.hibernate-off",
      },
      {
        name: "Deep power management",
        // The thermal disclosure is part of the row, not a caveat.
        detail:
          "Processor idle states off, and USB devices and PCI Express links kept awake. Raises idle power draw and temperature.",
        tier: "ultimate",
        evidence: "Vethida/vth_core/Tweaks/TweakCatalogOperatorGpuPower.cpp::power.pcie-aspm-off",
      },
      {
        name: "Performance power plans",
        detail: "Vethida's own power plans for desktops and laptops, added to Windows.",
        tier: "pro",
        evidence: "Vethida/gui/Pages/SystemTools.cpp::/powerplans/pro-laptop.pow",
      },
      {
        name: "Low-latency power plan",
        detail:
          "Idle promotion, core parking and interrupt moderation off, trading power draw and heat for response time.",
        tier: "ultimate",
        evidence: "Vethida/gui/Pages/SystemTools.cpp::/powerplans/ultimate-desktop.pow",
      },
    ],
  },
  {
    id: "graphics",
    icon: "gpu",
    title: "Graphics",
    body: "The driver's own settings, and the Windows graphics settings underneath them.",
    capabilities: [
      {
        name: "GPU settings",
        detail:
          "Hardware-accelerated GPU scheduling, message-signaled interrupts, scheduler preemption, and the desktop compositor and DirectX overrides.",
        tier: "pro",
        evidence: "Vethida/vth_core/Tweaks/TweakCatalogActive.cpp::gpu.enable-hags",
      },
      {
        name: "NVIDIA driver profile",
        detail:
          "Writes Vethida's settings to your global NVIDIA profile: low-latency mode, vertical sync, texture filtering, shader cache and more.",
        tier: "pro",
        evidence: "Vethida/vth_core/Windows/NvApi.cpp::optimalProfile",
      },
      {
        name: "NVIDIA driver settings",
        detail:
          "HDCP, Ansel, NVIDIA telemetry, CUDA preemption, write combining and the older image-sharpening control.",
        tier: "pro",
        evidence: "Vethida/vth_core/Tweaks/TweakCatalogOperatorGpuPower.cpp::gpu.nvidia.disable_hdcp",
      },
      {
        name: "NVIDIA clocks and power",
        detail:
          "Keeps the card at its performance clocks: clock gating and dynamic performance states off. Raises idle power draw and GPU temperature.",
        tier: "ultimate",
        evidence:
          "Vethida/vth_core/Tweaks/TweakCatalogOperatorGpuPower.cpp::gpu.nvidia.dynamic_pstate_off",
      },
    ],
  },
  {
    id: "network",
    icon: "network",
    title: "Network and input",
    body: "Vethida measures your connection, then changes the adapter and protocol settings behind it.",
    capabilities: [
      {
        name: "Connection test",
        detail:
          "Measures throughput and idle latency, and grades how much latency your connection adds under load.",
        tier: "free",
        evidence: "Vethida/vth_core/Speedtest.cpp::gradeForBufferbloat",
      },
      {
        name: "Custom DNS",
        detail: "Picks the DNS server your PC uses to look up websites.",
        tier: "pro",
        evidence: "Vethida/vth_core/Windows/Network.h::setStaticDns",
      },
      {
        name: "Network adapter settings",
        detail:
          "Adapter power saving, offloads, receive-side scaling and per-adapter TCP latency.",
        tier: "pro",
        evidence:
          "Vethida/vth_core/Tweaks/TweakCatalogOperatorBatch.cpp::op.net.adapter-power-offload",
      },
      {
        name: "Traffic priority",
        detail: "Give an app's traffic priority, or cap how much bandwidth an app can use.",
        tier: "pro",
        evidence: "Vethida/gui/Views/QosView.cpp::QosView",
      },
      {
        name: "TCP/IP settings",
        detail:
          "Congestion control, the receive window, network throttling and the connectivity check.",
        tier: "pro",
        evidence: "Vethida/vth_core/Tweaks/TweakCatalogActive.cpp::net.ctcp-congestion",
      },
      {
        name: "Mouse and keyboard",
        detail: "Pointer acceleration off, and a faster keyboard and mouse response.",
        tier: "free",
        evidence:
          "Vethida/vth_core/Tweaks/TweakCatalogOperatorBatch.cpp::op.input.mouse-acceleration-off",
      },
    ],
  },
  {
    id: "debloat",
    icon: "eraser",
    title: "Debloat",
    body: "The preinstalled software and the background work a Windows install arrives with, removed or switched off.",
    capabilities: [
      {
        name: "App and provisioning removal",
        detail:
          "Removes the app and the package that would reinstall it, so a feature update does not bring it back.",
        tier: "pro",
        evidence: "Vethida/gui/Views/BloatwareView.cpp::BloatwareView",
      },
      {
        name: "App uninstall",
        detail: "Everything installed on the PC, including what the manufacturer added, with uninstall.",
        tier: "pro",
        evidence: "Vethida/gui/Views/AppsView.cpp::AppsView",
      },
      {
        name: "Startup control",
        detail: "See what runs when you sign in and turn it off.",
        tier: "pro",
        evidence: "Vethida/gui/Views/StartupAppsView.cpp::StartupAppsView",
      },
      {
        name: "Windows extras",
        detail:
          "Copilot, OneDrive, news and interests, Game DVR, suggested apps and web results in search.",
        tier: "pro",
        evidence:
          "Vethida/vth_core/Tweaks/TweakCatalogOperatorBatch.cpp::op.debloat.copilot-cortana",
      },
      {
        name: "Microsoft Edge",
        detail: "Edge's background activity, preloading and updater, or Edge removed entirely.",
        tier: "pro",
        evidence: "Vethida/vth_core/Tweaks/TweakCatalogActive.cpp::browser.edge-harden",
      },
      {
        name: "Background services",
        detail: "Xbox, printing, fax, sync, Bluetooth and other services you may not use.",
        tier: "pro",
        evidence: "Vethida/vth_core/Tweaks/TweakCatalogOperatorBatch.cpp::op.svc.xbox",
      },
    ],
  },
  {
    id: "privacy",
    icon: "privacy",
    title: "Privacy & security",
    body: "What leaves the machine, and what can reach it from outside. The Windows protection settings are here too.",
    warning: {
      title: "Some of these turn a Windows protection off",
      body: "Defender, SmartScreen, User Account Control and Windows Update can all be switched off from this section, and none of them is in the recommended set.",
    },
    capabilities: [
      {
        name: "Telemetry and diagnostics",
        detail:
          "Windows telemetry, feedback and error reporting, the advertising ID, and typing, inking and speech collection.",
        tier: "pro",
        evidence:
          "Vethida/vth_core/Tweaks/TweakCatalogOperatorBatch.cpp::op.privacy.telemetry-feedback",
      },
      {
        name: "App permissions",
        detail: "Location, sensors, and what apps may reach in the background.",
        tier: "pro",
        evidence:
          "Vethida/vth_core/Tweaks/TweakCatalogOperatorBatch.cpp::op.privacy.location-sensors",
      },
      {
        name: "Security settings",
        detail:
          "SmartScreen, User Account Control, Remote Desktop, BitLocker, and the exploit and processor mitigations.",
        tier: "pro",
        evidence:
          "Vethida/vth_core/Tweaks/TweakCatalogOperatorBatch.cpp::op.security.smartscreen-off",
      },
    ],
  },
  {
    id: "repair",
    icon: "quickfix",
    title: "Repair and cleanup",
    body: "Fixes for common Windows faults, a cleaner, restore points, and a short list of trusted apps.",
    capabilities: [
      {
        name: "Fixes",
        detail:
          "One-click repairs for sound, network, Windows Update, the Store, printing, and settings other tools turned off.",
        tier: "free",
        evidence: "Vethida/vth_core/Windows/Fixes.cpp::repairNetworkStack",
      },
      {
        name: "System cleaner",
        detail:
          "Temporary files, application caches, update leftovers, logs and crash dumps, sized by category before you clear them.",
        tier: "free",
        evidence: "Vethida/vth_core/Windows/Cleanup.h::cleaners",
      },
      {
        name: "Restore points and change history",
        detail:
          "Create Windows restore points, and see the changes Vethida made, most with a way to undo them.",
        tier: "free",
        evidence: "Vethida/gui/Views/TweakLedgerView.cpp::TweakLedgerView",
      },
      {
        name: "Get apps",
        detail:
          "Known-good applications, installed through Windows Package Manager or straight from the publisher.",
        tier: "free",
        evidence: "Vethida/vth_core/Windows/AppInstaller.h::installedAmong",
      },
    ],
  },
  {
    id: "genesis",
    icon: "genesis",
    title: "Genesis",
    body: "One switch clears the machine for a play session.",
    capabilities: [
      {
        name: "Genesis",
        detail:
          "Background apps, services and notifications, from a list you set. Apps you close can also have their program file renamed for the length of the session, so Windows has nothing to start.",
        tier: "ultimate",
        evidence: "Vethida/vth_core/Windows/Genesis.h::suppressExecutableNames",
      },
      {
        name: "Genesis session settings",
        detail:
          "A high-resolution timer, freed standby memory and the app in front kept at full speed, for the length of the session and no longer.",
        tier: "ultimate",
        evidence: "Vethida/vth_core/Windows/Genesis.h::timerResolutionRequested",
      },
    ],
  },
  {
    id: "health",
    icon: "detect",
    title: "Health and recommendations",
    body: "A score for this machine, and the set of changes suggested for it.",
    capabilities: [
      {
        name: "Health score",
        detail:
          "Shows how much of what Vethida can do for this PC is already in place.",
        tier: "free",
        evidence: "Vethida/gui/Widgets/HealthScoreGauge.h::HealthScoreGauge",
      },
      {
        name: "Recommended changes",
        detail:
          "A set picked for the hardware and settings Vethida found on this PC, rather than one list handed to every machine.",
        tier: "free",
        evidence: "Vethida/vth_core/Tweaks/Recommend.h::recommended",
      },
    ],
  },
];

/** The tier a whole group carries, or null when its rows differ. */
export function groupTier(group: CapabilityGroup): TierKey | null {
  const tiers = new Set(group.capabilities.map((c) => c.tier));
  return tiers.size === 1 ? [...tiers][0] : null;
}
