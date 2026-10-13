# Two Thrones Chain Fix

## Download and install

1. [Download the latest release ZIP](https://github.com/Darwin1337/pop-t2t-chain-fix/releases/latest/download/POP3ChainFix.zip).
2. Close the game and extract the ZIP into the game folder containing `POP3_mousefix.EXE`. It adds `scripts/POP3ChainFix.asi` and `scripts/POP3ChainFix.ini`.
3. Launch the game normally. No debugger or mod manager is needed.

Requires a 32-bit ASI loader supporting `InitializeASI`, such as [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader). If your mod pack already has one, keep it. Back up any older `POP3ChainFix` files before replacing them. Remove the ASI to uninstall.

Tested with `POP3_mousefix.EXE` on the chain at **The Lower City Rooftops**: both directions recorded at **60 and 144 FPS**, with successful **30 FPS** play also reported. Other executable builds and chain layouts are not validated. Unsupported executable layouts or conflicting hooks leave the fix inactive; check `scripts/POP3ChainFix.log`.

## Build from source

With [Zig 0.14.1](https://ziglang.org/download/) on Windows:

```powershell
.\scripts\Build.ps1 -ZigExe 'C:\path\to\zig.exe'
```

Output: `build/POP3ChainFix.asi`. The default settings are in `config/POP3ChainFix.ini`.

## What was wrong, and how this fixes it

The game's chain guides contain small gaps between segments. At high FPS, Prince's smaller movement steps can enter a gap and get projected back to the same endpoint every frame, causing sticking or jitter.

The fix projects through a short gap only when there is one uniquely matching continuation. It retains the original projection elsewhere and does not change frame timing or animation speed. The correction uses world-space geometry, so it does not depend on FPS or screen resolution.
