# Runbooks

Procedures for things that happen to a running system, written to be followed
under pressure by somebody who is tired.

| Runbook | When you need it |
|---|---|
| [deploy.md](deploy.md) | Shipping a web change to the VPS |
| [nginx.md](nginx.md) | Editing the reverse proxy, rotating the origin certificate, refreshing the Cloudflare range list |
| [database.md](database.md) | Backups, restores, migrations against production |
| [restore.md](restore.md) | Proving a backup restores, and restoring production |
| [release.md](release.md) | Building, signing and publishing the desktop app |
| [av-false-positive.md](av-false-positive.md) | An antivirus product flags the installer |
| [incident-response.md](incident-response.md) | Something is broken or a key leaked |
| [key-rotation.md](key-rotation.md) | Rotating any of the four signing or session secrets |

## The rules that apply to all of them

**Read the whole runbook before starting.** These have ordering constraints that
are not obvious from any single step.

**A step that fails stops the runbook.** Do not skip ahead to "the important
part". Every one of these has at least one step whose failure means the next step
would make things worse.

**Nothing here is automated on purpose.** There is one VPS and one person. A
deploy pipeline that can fire without a human is a pipeline that can take the
site down at 3am without one.
