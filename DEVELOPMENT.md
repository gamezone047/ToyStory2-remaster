# Development setup

This repository contains the custom launcher, updater and native mod/plugin sources, not the original game's engine source. Co-op is experimental; complete campaign synchronization is unfinished.

## Requirements

- Windows, Git and GitHub access for pushing changes.
- Visual Studio 2017 Community with the x86 C++ toolchain. Native build scripts currently reference its standard installation path; adjust that path if using another installation.
- .NET Framework compiler at `C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe`.
- Python 3.9 for binary analysis, with `pefile` and `capstone` installed (`py -3.9 -m pip install pefile capstone`).
- A separate local game installation or verified release ZIP. The verified 2026.11.7.5 runtime is now included via Git LFS. Install Git LFS before cloning and run `git lfs pull` to fetch the actual binary content. Binary inspection and some tests require these files at the checkout root. Never run build/deployment scripts against your installed game directory.

Run all commands from the repository root:

```bat
controls-src\test.cmd
controls-src\coop\build-native.cmd
controls-src\coop\build-test.cmd
controls-src\build.cmd
controls-src\build-launcher.cmd
controls-src\build-updater.cmd
controls-src\coop\build-launcher.cmd
py -3.9 controls-src\coop\verify-native.py
```

`build-native.cmd` runs native tests and produces the frame fixture needed by `build-test.cmd`. No command above launches the game. The native plugins use addresses for the supported Toy Story 2 executable; static verification must pass before testing a different baseline.

Outputs are under `controls-src/build-output/` and `controls-src/coop/`, not installed automatically. Follow AGENTS.md: extract the latest release ZIP into a temporary staging folder, copy new runtime builds into staging, update its Coop-build.sha256 manifest, advance the four-part version and release notes, then run:

```powershell
controls-src/package-release.ps1 -Version <new-version> -BuildRoot <staging-folder>
```

Packaging creates a local archive and verifies it with the updater. Publishing is a separate, explicit action through the owner's Desktop BAT. Do not run publishing/network setup scripts for routine source development. Never commit saves, credentials or generated development binaries. Runtime updates should come from a verified staged build, with updated manifests.

## Collaboration

Clone this repository and create a feature branch for each change. Your friend can fork the public repository and submit pull requests, or the repository owner can grant collaborator access in GitHub settings. No collaborator invitation is sent by this source upload.

Level 3-15 encounters and script variables are described per level in `controls-src/coop/ScriptSync.h`; their wire whitelist lives in `CoopScriptBounds`/`CoopActorRules` (`CoopFrame.h`) and must be mirrored in `GameFrame.cs` (the managed tests compare both through `native-rules.bin`).

See `controls-src/coop/IMPLEMENTATION-STATUS.txt`, `CAMPAIGN-COVERAGE.txt` and `ReleaseNotes.txt` for current limitations and completed work. Native tests are offline approximations; both players must still validate behavior in-game.

## Clone the complete game and source

```bat
git lfs install
git clone https://github.com/gamezone047/ToyStory2-remaster.git
cd ToyStory2-remaster
git lfs pull
git lfs fsck
```

Use Git clone with Git LFS, rather than GitHub's source ZIP, to obtain a complete working copy. The committed runtime baseline is 2026.11.7.5; saves are deliberately excluded. Git LFS storage and bandwidth usage applies to binary uploads/downloads.

For changes, create a branch, edit sources, build into staging, test, then commit and push. A collaborator with write access can push branches; otherwise use a fork and pull request. Git pushes do not publish launcher updates: the existing publishing BAT remains the release mechanism.
