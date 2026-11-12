// Releases, newest first (/changelog and the banner). Add new ones, don't rewrite old ones.
export type ChangeKind = "Added" | "Changed" | "Fixed";

export type Release = {
  version: string;
  date?: string; // YYYY-MM-DD, unset until it ships
  changes: ReadonlyArray<{ kind: ChangeKind; note: string }>;
};

export const CHANGE_KINDS: readonly ChangeKind[] = ["Added", "Changed", "Fixed"];

export const releases: Release[] = [
  {
    version: "1.0.0",
    changes: [
      {
        kind: "Added",
        note: "The tweak catalog, from Windows settings through to the graphics driver.",
      },
      { kind: "Added", note: "Hardware and settings detection on launch." },
      {
        kind: "Added",
        note: "Debloating, startup control and the privacy settings.",
      },
      { kind: "Added", note: "Quick fixes and the system cleaner." },
      {
        kind: "Added",
        note: "Performance power plans, including the low-latency plan on Ultimate.",
      },
      {
        kind: "Added",
        note: "Custom DNS, traffic priority and the connection test.",
      },
      {
        kind: "Added",
        note: "Genesis and the NVIDIA driver profile.",
      },
      {
        kind: "Added",
        note: "Restore points, and a list of the changes Vethida made.",
      },
      {
        kind: "Added",
        note: "Get apps, which installs free software through Windows Package Manager or from its publisher.",
      },
    ],
  },
];

// not "latest", lib/download.ts already has latestRelease for the installer
export const currentRelease: Release = releases[0];
