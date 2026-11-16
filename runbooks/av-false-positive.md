# Antivirus false positive

Vethida edits the registry, services and scheduled tasks, so some engines flag it on heuristics. What matters is whether a detection blocks installing or running it.

## Check

For a signed release, compare the packed and unpacked installers:

```bash
node scripts/release/virustotal-submit.mjs --packed dist/VethidaSetup.exe --unpacked dist/VethidaSetup-unpacked.exe
```

The result is saved to `release-history/<version>/av-submissions.json`.

For a GitHub build, look up the zip's `Vethida.exe` hash on [VirusTotal](https://www.virustotal.com).

UPX packing isn't worth it: it made the installer about 1 MB larger (the installer already compresses with LZMA), and it doesn't work with Control Flow Guard on. Keep the Qt DLLs unpacked either way, as the LGPL requires.

## Report it

For each engine that flags it:

1. Submit the file and its SHA-256 through the vendor's false-positive form. For Microsoft Defender: <https://www.microsoft.com/en-us/wdsi/filesubmission>.
2. Say what it is: Vethida by Rens Engineering, a Windows tuning tool that writes to HKLM, installs a service and creates scheduled tasks. Link the source.
3. Record the case id in `av-submissions.json`. Follow up after a week if it's still flagged.

Signed builds clear faster; unsigned ones may be ignored.

## While it's flagged

- Don't tell people to turn their antivirus off.
- Point them to the VirusTotal page for the exact hash they downloaded.
- If it blocks installation, hold the release.
