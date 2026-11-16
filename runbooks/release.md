# Release

Windows, with Qt 6.11.1 and MSVC. Every step from a clean checkout.

## Environment

Nothing here belongs in an app `.env.example`: these are read by the release
tooling, not by anything that serves a request. Set them in the shell running
the release.

| Variable | Read by | What it is |
|---|---|---|
| `QTDIR` | the build, `release:package` | The Qt kit being linked. `windeployqt` is taken from its `bin/`, and every shipped `Qt6*.dll` is compared byte for byte against it. |
| `SIGNING_CERT_THUMBPRINT` | `release:sign` | SHA-1 thumbprint of the OV/EV certificate, passed to signtool as `/sha1`. Required. |
| `SIGNING_TIMESTAMP_URL` | `release:sign` | RFC 3161 timestamp server. Without one the signature dies with the certificate. |
| `SIGNING_CSP` | `release:sign` | Optional. The provider name, for tokens and cloud HSMs that need the container named. |
| `SIGNING_KEY_CONTAINER` | `release:sign` | Optional, and required if `SIGNING_CSP` is set. |
| `SIGNTOOL_PATH` | `release:sign` | Optional. Full path to `signtool.exe` when it is not resolvable. |
| `THEMIDA_PROJECT` | `release:package` | The `.tmd` authored against `dist/payload/Vethida.exe`. `--require-protection` fails without it. |
| `R2_BUCKET` | `release:publish` | Destination bucket for the installer and the update feed. |
| `R2_PUBLIC_BASE` | `release:publish` | Public origin the bucket is served from, used to build the URLs written into the feed. |
| `VT_API_KEY` | `release:virustotal` | VirusTotal API key for the packed/unpacked submission. |

Three more are read by development tooling and never by a release:
`PG_SERVICE` and `PG_VIA_DOCKER` (`db:verify-restore`, naming the Postgres
service and whether it is reached through Docker), `POLAR_DISCOUNT_SAVE`
(`polar:check`), and `QT_ROOT_DIR`, which lets `check-integrity` find `rcc.exe`
when `QTDIR` is not set.

1. `git pull && pnpm install --frozen-lockfile`
2. Edit `VERSION`. One line, `MAJOR.MINOR.PATCH`.
3. `pnpm release:bump`
4. `node scripts/preflight.mjs && pnpm check`
5. Read `vethida.com/lib/features.ts`. For each capability, open the file its
   `evidence` names, find the literal, and confirm the sentence still describes
   it. This is the claims register and it is the one step no script does.
6. `pnpm release:check` must report zero blocking items.
7. `pnpm release:build`

## Packing and signing

Signing is last for each binary, because Themida and UPX both rewrite the image
and would strip a signature applied before them. There are two binaries and so
two signing steps.

**Step 8 does not currently complete.** `release:package` runs UPX unless
`--unpacked` is passed, and UPX refuses a Control Flow Guard image; the script
stops with the GUARD_CF message rather than packing one. So the only build this
runbook can produce today is the `--unpacked` one at step 13, and it is
described there as a comparison build because that is what it was for.

There are two ways out and neither is a code change: drop the UPX step, or turn CFG off in `Vethida/common.props`
deliberately. Until one is chosen, run step 8 as
`pnpm release:package --unpacked --require-protection` and sign that payload,
and read every "packed" below as "the build you have".

8. `pnpm release:package --require-protection`
9. `pnpm release:sign dist/payload/Vethida.exe`
10. `pnpm release:installer --stub Vethida/build/x64_Release/vethida-setup.exe
    --payload-dir dist/payload --uninstall Vethida/build/x64_Release/uninstall.exe
    --out dist/VethidaSetup.exe`
11. `pnpm release:sign dist/VethidaSetup.exe`
12. `pnpm release:sign --verify dist/VethidaSetup.exe` and read the output. The
    signer's CN must be the string in `UpdatePublisherCommonName`
    (`Vethida/vth_core/Services/SecurityConfig.h`), exactly. If they differ the
    updater rejects our own builds.

The unpacked twin, for the antivirus comparison. It skips UPX, which is also why
it is the only variant that builds while the CFG question is open. Never
published:

13. `pnpm release:package --unpacked --require-protection`, then sign
    `dist/payload-unpacked/Vethida.exe`, then `release:installer` again with
    `--payload-dir dist/payload-unpacked --out dist/VethidaSetup-unpacked.exe`,
    then sign that.

## Publishing

14. `pnpm release:source-offer --qt-src <unpacked Qt source tree>`
15. `pnpm release:virustotal --packed dist/VethidaSetup.exe --unpacked dist/VethidaSetup-unpacked.exe`
16. Install the signed installer on a clean Windows 11 and a clean Windows 10
    machine. Activate, apply one tweak, revert it, uninstall.
17. `pnpm release:publish --installer dist/VethidaSetup.exe --source-offer dist/source-offer`
18. Run the `release:stamp` command step 17 prints. Existing installs begin
    downloading within the hour; there is no unstamp.
19. Publish the installer's SHA-256 next to the download. `release:publish`
    prints it and `release-history/<version>/release.json` records it.
20. Write the changelog entry on vethida.com.

## Deploying the web surfaces

1. `ssh` to the VPS.
2. `git pull && pnpm install --frozen-lockfile`
3. `pnpm db:migrate`
4. `pnpm --filter <app> build`
5. `nginx -t` before any reload. `systemd-analyze verify <unit>` before any
   enable.
6. `systemctl restart <unit>` one unit at a time. `systemctl status` after each.

## Rolling back

1. `git checkout <previous tag> && pnpm install --frozen-lockfile && pnpm --filter <app> build`
2. `systemctl restart <unit>`
3. Migrations do not roll back. If the previous build cannot read the current
   schema, restore from backup instead: [restore.md](restore.md).
