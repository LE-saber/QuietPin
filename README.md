# QuietPin 0.2

[中文](https://github.com/LE-saber/QuietPin/blob/main/README.zh.md) · **English**

A lightweight always-on-top tool for Windows 11 x64, built with native C++20 and Win32. It runs in the background with no main window, console, taskbar button or tray icon by default. No .NET, Electron or additional runtime installation is required.

v0.2 adds an optional Pin button and a per-user Windows installer. **The user confirmed manual testing passed and approved v0.2.0 for stable release.** The v0.1 release remains available.

[Release v0.2.0](https://github.com/LE-saber/QuietPin/releases/tag/v0.2.0) · [Changelog](https://github.com/LE-saber/QuietPin/blob/main/CHANGELOG.md) · [Verification and limitations](https://github.com/LE-saber/QuietPin/blob/main/docs/verification-v0.2.0.en.md)

## Download and start

| Download | Use |
| --- | --- |
| [Windows installer EXE](https://github.com/LE-saber/QuietPin/releases/download/v0.2.0/QuietPinSetup-v0.2.0-win-x64.exe) | Windows 11 x64; installs for the current user without administrator rights |
| [Portable ZIP](https://github.com/LE-saber/QuietPin/releases/download/v0.2.0/QuietPin-v0.2.0-win-x64.zip) | Extract the whole folder and run QuietPin.exe |
| [SHA256 checksums](https://github.com/LE-saber/QuietPin/releases/download/v0.2.0/QuietPin-v0.2.0-SHA256SUMS.txt) | Verify the installer and ZIP |

The installer defaults to `%LOCALAPPDATA%\Programs\QuietPin`. Start menu entries provide Run, Settings, Exit and Uninstall; a desktop shortcut is optional. The installer interface is English; the application supports Chinese and English. Installing does not enable sign-in startup, the tray icon or Pin.

Normal startup is quiet. Use these shortcuts immediately:

| Action | Default shortcut |
| --- | --- |
| Toggle always-on-top for the active window | **Ctrl + Alt + T** |
| Open Settings | **Ctrl + Alt + Shift + T** |
| Exit completely | **Ctrl + Alt + Shift + Q** |

Activate the same window and press the toggle shortcut again to unpin it. Multiple windows may be pinned. Always-on-top places windows above ordinary windows; it does not guarantee placement above other topmost windows, exclusive fullscreen applications or the secure desktop.

With no tray icon, running QuietPin.exe again opens the existing instance's Settings. `open-settings.cmd` and `exit.cmd` provide additional entry points. Closing Settings returns to the background; **Exit QuietPin** terminates the program.

## Features

- Global toggle shortcut with key-repeat suppression.
- Custom toggle shortcut: Ctrl/Alt/Shift/Win with a letter, digit or F1–F11. A conflicting shortcut keeps the existing shortcut and configuration.
- Chinese / English Settings and brief status messages that do not take focus.
- Optional tray menu with Toggle, Settings and Exit; disabled by default.
- Optional Pin button with event-driven tracking and position offsets; disabled by default.
- Exact executable exclusions, using a file name or full path, case insensitive.
- Optional startup at Windows sign-in, configured for the current user.
- Desktop, taskbar and special Shell window protection; restricted targets produce an error without automatic elevation.
- One instance per configuration profile. Normal exit attempts to undo pinning introduced by that instance.

## Enable Pin

Open Settings, check **Show Pin near the active window**, then Save. Click the button to toggle the active window without taking its foreground or keyboard focus.

Pin prefers a position above the title bar. A title-bar position is used only when safety checks allow it. If no safe location is available, including narrow windows, borderless fullscreen or unsupported custom title bars, the button hides and the global shortcut remains available.

Settings supports X/Y offsets in device-independent pixels (DIP) and **Reset position**. Excluded applications have no Pin button and cannot be toggled. Settings and Exit shortcuts are fixed in this version; the toggle shortcut is configurable.

## Configuration and sign-in startup

Both installer and portable builds use `%LOCALAPPDATA%\QuietPin\settings.ini` by default. The ZIP is portable in the sense that installation is unnecessary; its default configuration is stored in your user profile. Save applies changes. A corrupt configuration is backed up before replacement; if backup fails, saving is disabled to protect the file.

Enter one exclusion per line. `chrome.exe` excludes every executable with that name; a full path excludes only that path. Old v0.1 configuration files remain supported and upgrade with Pin disabled.

Sign-in startup is off by default. Enabling it writes a quoted path to the current user's Run key. After moving the executable, review the startup setting. An entry belonging to another path is preserved; disable it from the previous copy or remove that stale entry deliberately. Exiting does not disable future sign-in startup.

Uninstall keeps your configuration and deletes the QuietPin Run entry only if it points to that installed executable. Upgrade and uninstall request a graceful exit only from an instance running at the installation path; another portable copy is preserved.

## Command line

```text
QuietPin.exe
QuietPin.exe --settings
QuietPin.exe --exit
QuietPin.exe --startup
QuietPin.exe --config-dir "C:\My QuietPin Profile"
```

`--startup` quietly returns when an instance exists. `--exit` returns when no instance exists. Exit codes: 0 for success/no instance, 1 for startup or runtime failure, 2 for invalid arguments, 3 for an unresponsive instance. A successful regular exit request means delivery succeeded; cleanup may take about 0.7 seconds.

`--config-dir` gives a profile its own settings and instance identity, useful for isolated testing. Profiles must still use different global shortcuts. The internal installer command `--exit-if-owned` checks the executable path and waits for the matching instance to finish cleanup.

## Verification and current limits

Configuration, policy, real Win32 window integration, mouse Pin/unpin, focus retention, movement, lifecycle and MSAA action-name checks have been run on the development machine. See the [verification record](https://github.com/LE-saber/QuietPin/blob/main/docs/verification-v0.2.0.en.md) for evidence and pending items.

This is an **unsigned MinGW build**. On 2026-10-05, the user confirmed manual testing passed and approved stable publication. The tested installer and ZIP are retained without rebuilding. That feedback does not provide an itemized record for real mixed-DPI multi-monitor behavior, display hot-plug, administrator targets, lock/unlock, virtual desktops, Explorer restart, sign-in startup, the browser/Electron application matrix, MSVC or two-hour stability; broader platform validation remains tracked separately.

Force termination or a crash may leave a target topmost. Restart QuietPin and toggle that window to undo it. Windows does not provide reliable ownership of topmost state when several tools change the same window; exit restoration is best effort.

## Build from source

Requires Windows, CMake 3.20+ and a C++20 compiler. MinGW GCC 14.2 with Ninja has been used. The MSVC build path is provided but has not been verified on this machine.

```powershell
# Run in the repository. Tests briefly create windows and change foreground focus.
.\scripts\build.ps1 -Compiler MinGW
# In a VS 2022 Developer PowerShell; use a fresh build directory when changing compilers.
.\scripts\build.ps1 -Compiler MSVC

# Optional regression and idle measurement
ctest --test-dir build --output-on-failure
.\scripts\measure-idle.ps1 -Seconds 60

# Inno Setup 6.7+ is needed to build the installer, not to run QuietPin.
.\scripts\package.ps1 -InnoCompiler "C:\Path\ISCC.exe"
```

Packaging checks the EXE version and produces the installer, ZIP and SHA256 checksum file. Both packages include Chinese/English READMEs, the bilingual changelog and verification records. Native implementation details are available in the [Chinese technical design](https://github.com/LE-saber/QuietPin/blob/main/docs/technical-design.zh-CN.md) and [Pin plan](https://github.com/LE-saber/QuietPin/blob/main/docs/pin-implementation-plan.zh-CN.md).
