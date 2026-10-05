# QuietPin 0.2.0 verification record

[中文](https://github.com/LE-saber/QuietPin/blob/main/docs/verification-v0.2.0.zh-CN.md) · **English**

Recorded on 2026-10-05 on the development Windows 11 x64 machine. This is a native C++20/Win32, single-UI-thread, event-driven application with no target-process injection. Inno Setup builds the installer and is not a runtime dependency.

## Evidence collected

MinGW GCC 14.2 / Ninja / Release. All three CTest suites passed in a complete run: configuration/policy, MVP integration and Pin integration. Real SendInput mouse checks exercised Pin/unpin, hit detection, foreground and keyboard focus retention, and cancellation when the target changed between press/release. An MSAA query returned the English action name matching the unpinned state.

Fixtures exercised move/resize, minimize/restore, maximize/restore, borderless fullscreen hiding, saved/reset offsets, immediate exclusions, Pin/Tray lifecycle, target destruction and no-tray exit. Geometric tests covered 100/125/150/200% scaling, negative monitor coordinates, caption-button protection, narrow/unknown title bars and invalid offsets. These tests do not establish real mixed-DPI multi-monitor compatibility.

Chinese and English Settings screenshots were inspected for clipping and overlap. With themed controls warmed and the settings fixture hidden, the final 100-cycle sample showed process handles 224→224, GDI objects 26→24 and USER objects 51→50; no linear growth was observed in that sample. GUI measurements used repeated samples to avoid counting temporary painting objects as retained resources.

| Mode | CPU time added over 60 seconds | Private bytes | Working set |
| --- | --- | --- | --- |
| Pin and Tray disabled | 0 within sampling resolution | 1,925,120 | 10,821,632 |
| Pin displayed, stationary | 0.109375 seconds | 3,104,768 | 20,926,464 |

Default mode had one thread. The Pin sample was slightly above the initial 0.1-second CPU budget, about 0.18% of one logical core, while added private bytes stayed below the 5 MiB budget. Longer sampling and performance refinement remain in Phase 5. This is not a claim of permanently zero CPU usage. The EXE is 1,276,416 bytes and imports Windows system DLLs without third-party runtime DLLs.

The installer was compiled with official Inno Setup 6.7.3. Partial isolated installation checks confirmed Start menu entries and matching installed/source EXE hashes. Final upgrade/uninstall and portable-path acceptance were not completed. The user requested that final testing be done manually, so further automated final acceptance was stopped. Updating documentation and publishing this release does not add new runtime test coverage.

## Installation behavior

The installer targets Windows 11 x64, installs for the current user without elevation, and defaults to `%LOCALAPPDATA%\Programs\QuietPin`. Run, Settings, Exit and Uninstall shortcuts are supplied; the desktop shortcut is optional. Pin, Tray and sign-in startup remain off by default.

Upgrade/uninstall request graceful exit only from the installed executable path. A portable copy at another path is preserved. Uninstall keeps user settings and removes the QuietPin Run entry only when it exactly belongs to the installation path.

The portable ZIP needs no installation but uses `%LOCALAPPDATA%\QuietPin` for settings by default. `--config-dir` supports a custom profile; global shortcut conflicts still apply across profiles.

## Manual feedback and remaining coverage

The user confirmed manual testing passed and approved stable publication on 2026-10-05. The tested files are retained without rebuilding or rerunning final tests. This remains an unsigned MinGW build. The earlier record did not itemize coverage for final installation/upgrade/uninstall, portable extraction into Chinese/space paths, real mixed-DPI multi-monitor movement and hot-plug, lock/unlock, virtual desktops, elevated windows, browser/Electron custom title bars, Explorer restart, actual sign-in startup, MSVC and two-hour stability. No universal Windows 11 application compatibility is claimed.

Pin probes an internal candidate at five points with bounded WM_NCHITTEST calls and requires HTCAPTION. It rechecks occupied geometry at mouse release. If no safe position is available, Pin hides and shortcuts remain usable. Static operation has no default polling loop; movement compensation runs at 33ms for at most five seconds per activation, and disabling Pin releases its windows, hooks, notification registration and timer.
