# ToyStory2-remaster

Toy Story 2 remastered project.

[Download the latest build](https://github.com/gamezone047/ToyStory2-remaster/releases/latest)

Download the `ToyStory2-remaster-<version>.zip` release asset, extract the whole ZIP into a writable folder, and open **Toy Story 2 Launcher.exe**. Do not run it inside the ZIP. The launcher checks GitHub Releases automatically and installs newer versions before opening, preserving local saves and settings. Release assets include a SHA-256 checksum file.

Choose **Multiplayer** to Host or Join. The host can **Copy IP**, and the joining player can **Paste IP**. Hosting requires inbound TCP **27922**. The launcher configures a program-specific Windows Firewall rule and attempts router UPnP; administrator approval is required. If UPnP is unavailable, forward that port manually to the host PC's LAN address. Carrier-grade NAT may prevent direct hosting; a shared VPN can be used instead. A successful connection from another network is required to verify Internet reachability.

**Current co-op status:** experimental two-player first-level prototype. Remote player movement, shared coins and ordinary enemy deaths are implemented. Enemy positions, quests, tokens and campaign transitions are not synchronized. Full campaign co-op and four-player support are not ready. See `Multiplayer.txt` inside the ZIP for test instructions and limitations.

Builds are distributed through this repository's **Releases**, not Git file storage. Personal saves and development artifacts are excluded.
