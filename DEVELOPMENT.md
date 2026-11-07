# Development setup

This repository contains the custom launcher, updater and native mod/plugin sources, not the original game's engine source. Co-op is experimental; complete campaign synchronization is unfinished.

## Requirements

- Windows, Git and GitHub access for pushing changes.
- Visual Studio 2017 Community with the x86 C++ toolchain. Native build scripts currently reference its standard installation path; adjust that path if using another installation.
- .NET Framework compiler at `C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe`.
- Python 3.9 for binary analysis, with `pefile` and `capstone` installed (`py -3.9 -m pip install pefile capstone`).
- A separate local game installation or verified release ZIP. Original executable/data are not stored in Git. Binary inspection and some tests require `toy2.exe` and `data/` at the checkout root. Copy these from your own local baseline; they are ignored by Git. Never run build/deployment scripts against your installed game directory.

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

Packaging creates a local archive and verifies it with the updater. Publishing is a separate, explicit action through the owner's Desktop BAT. Do not run publishing/network setup scripts for routine source development. Never commit saves, credentials, runtime files or generated binaries.

## Collaboration

Clone this repository and create a feature branch for each change. Your friend can fork the public repository and submit pull requests, or the repository owner can grant collaborator access in GitHub settings. No collaborator invitation is sent by this source upload.

See `controls-src/coop/IMPLEMENTATION-STATUS.txt`, `CAMPAIGN-COVERAGE.txt` and `ReleaseNotes.txt` for current limitations and completed work. Native tests are offline approximations; both players must still validate behavior in-game.
