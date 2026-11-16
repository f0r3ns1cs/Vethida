# Release

Windows, with Qt 6.11.1 and MSVC.

## Environment

Set these in the shell running the release.

| Variable | Used by | What |
|---|---|---|
| `QTDIR` | build, package | Qt kit. `windeployqt` comes from its `bin/`. |
| `SIGNING_CERT_THUMBPRINT` | sign | SHA-1 thumbprint of the code-signing certificate. |
| `SIGNING_TIMESTAMP_URL` | sign | RFC 3161 timestamp server. |
| `SIGNING_CSP`, `SIGNING_KEY_CONTAINER` | sign | Optional, for tokens and cloud HSMs. |
| `SIGNTOOL_PATH` | sign | Optional path to `signtool.exe`. |
| `THEMIDA_PROJECT` | package | The `.tmd` for `dist/payload/Vethida.exe`. |
| `R2_BUCKET`, `R2_PUBLIC_BASE` | publish | Bucket and public URL for the installer and update feed. |
| `VT_API_KEY` | virustotal | VirusTotal API key. |

## Build

1. `git pull && pnpm install --frozen-lockfile`
2. Update `VERSION` (`MAJOR.MINOR.PATCH`), then `pnpm release:bump`.
3. `pnpm check`
4. Check each claim in `vethida.com/lib/features.ts` against the code it cites.
5. `pnpm release:ready` must report nothing blocking.
6. `pnpm release:build`

## Package and sign

Sign each binary last; Themida and UPX rewrite the image.

UPX refuses the binary while Control Flow Guard is on, so package with `--unpacked` until either UPX is dropped or CFG is turned off in `Vethida/common.props`.

7. `node scripts/release/package.mjs --unpacked --require-protection`
8. `node scripts/release/sign.mjs dist/payload-unpacked/Vethida.exe`
9. `node scripts/release/make-installer.mjs --stub Vethida/build/x64_Release/vethida-setup.exe --payload-dir dist/payload-unpacked --uninstall Vethida/build/x64_Release/uninstall.exe --out dist/VethidaSetup.exe`
10. `node scripts/release/sign.mjs dist/VethidaSetup.exe`
11. `node scripts/release/sign.mjs --verify dist/VethidaSetup.exe`. The signer CN must match `UpdatePublisherCommonName` in `vth_core/Services/SecurityConfig.h`, or the updater rejects the build.

## Publish

12. `node scripts/release/generate-source-offer.mjs --qt-src <Qt source>`
13. `node scripts/release/virustotal-submit.mjs` (see [av-false-positive.md](av-false-positive.md)).
14. Install on clean Windows 10 and 11: activate, apply and revert a tweak, uninstall.
15. `node scripts/release/publish.mjs --installer dist/VethidaSetup.exe --source-offer dist/source-offer`
16. Run the stamp command it prints. Installs start updating within the hour, and it can't be undone.
17. Publish the installer's SHA-256 next to the download, and write the changelog entry.

## Roll back

A published update can't be pulled back. Publish a new version with the fix.
