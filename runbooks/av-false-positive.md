# Antivirus false positive

Vethida edits the registry, services and scheduled tasks, and the installer
ships packed. All three are heuristic triggers, so detections are expected. What
matters is whether one blocks installation.

## Measure

1. `pnpm release:virustotal --packed dist/VethidaSetup.exe --unpacked dist/VethidaSetup-unpacked.exe`
2. Read the delta it prints. It appends the result to
   `release-history/<version>/av-submissions.json`.
3. Over two extra detections from packing is a decision point, not an automatic
   removal. Weigh it against what packing costs, which is measured rather than
   assumed: see "What UPX costs" below. The Qt DLLs stay unpacked either way, or
   LGPL section 4 breaks.

## What UPX costs

Measured on 1.0.0, 2026-08-08, over the staged payload:

| Installer payload, 7z -mx=7 | Size |
|---|---|
| Vethida.exe as linked | 10 907 145 bytes |
| Vethida.exe UPX-packed | 11 865 174 bytes |

Packing makes the download 958 KB **larger**. The installer already compresses
its payload with LZMA, and a UPX-packed image is incompressible, so the second
pass gives back more than the first saved.

UPX also refuses the binary outright: `CantPackException: GUARD_CF enabled PE
files are not supported`. Control Flow Guard is on in `Vethida/common.props`, and
the only way past it is `--force`, which packs the image with its CFG metadata
left describing a layout that no longer exists.

So packing currently costs a larger download, Control Flow Guard, and whatever
detections the delta above reports. Re-measure before reinstating it.

## Dispute

For each vendor that flagged the build:

1. Find that vendor's false-positive submission form.
2. Submit the **signed** installer and its SHA-256. An unsigned binary will be
   rejected or ignored.
3. State: publisher Rens Engineering LLC, product Vethida, what the installer
   does (writes to HKLM, installs a service, creates scheduled tasks), and that
   the build is reproducible.
4. Record the case id and the submission URL in
   `release-history/<version>/av-submissions.json`.
5. Most vendors clear a signed, reproducible build within days. Chase after a
   week using the case id.

## While a detection is live

1. Do not tell a customer to disable their antivirus.
2. Point them at the VirusTotal page for the exact SHA-256 they downloaded, so
   they can see which engines flagged it and which did not.
3. If the detection blocks installation outright, hold the release.
