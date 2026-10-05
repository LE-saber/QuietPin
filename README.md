# QuietPin

[中文](README.zh.md) · **English**

A lightweight always-on-top tool for Windows 11 x64. Native C++20 / Win32, with no additional runtime required. It runs in the background with no main window, taskbar button or tray icon by default.

[Download v0.2.0](https://github.com/LE-saber/QuietPin/releases/tag/v0.2.0) · [Changelog](CHANGELOG.md)

| Package | Use |
| --- | --- |
| [Windows installer](https://github.com/LE-saber/QuietPin/releases/download/v0.2.0/QuietPinSetup-v0.2.0-win-x64.exe) | Installs for the current user; no administrator permission needed |
| [Portable ZIP](https://github.com/LE-saber/QuietPin/releases/download/v0.2.0/QuietPin-v0.2.0-win-x64.zip) | Extract to a folder and run `QuietPin.exe` |
| [SHA256 checksums](https://github.com/LE-saber/QuietPin/releases/download/v0.2.0/QuietPin-v0.2.0-SHA256SUMS.txt) | Verify downloaded packages |

## Quick start

| Action | Default shortcut |
| --- | --- |
| Toggle always-on-top for the active window | **Ctrl + Alt + T** |
| Open Settings | **Ctrl + Alt + Shift + T** |
| Exit QuietPin | **Ctrl + Alt + Shift + Q** |

Press the toggle shortcut again on the same window to unpin it. Multiple windows can be pinned.

Running `QuietPin.exe` again opens the existing instance's Settings. The packages also include `open-settings.cmd` and `exit.cmd`. Closing Settings returns to the background; **Exit QuietPin** fully exits the application.

## Settings

- Customize the toggle shortcut; conflicting shortcuts keep the previous configuration.
- Enable or disable brief status messages, sign-in startup, the tray icon and the Pin button.
- Select **English** or **中文**, then **Save**. New profiles default to English; existing language settings are preserved.
- Exclude programs by EXE name or full path, one per line. Exclusions prevent both pinning and the Pin button.
- Adjust the Pin button's X/Y offset or reset its position.
- Manually **Check for updates** or open the **Download page**. There is no periodic background check or automatic installation. When a release uses the same version number, the published EXE checksum identifies a different build.

Pin and the tray icon are off by default. The Pin button follows the active window near its title bar without taking focus. It hides when no safe position is available; the keyboard shortcut remains usable.

The installer offers English and Simplified Chinese, defaulting to English. The application's language is selected separately in Settings.

## Installation and removal

The installer defaults to `%LOCALAPPDATA%\Programs\QuietPin`. The Start menu provides Run, Settings, Exit and Uninstall entries; a desktop shortcut is optional. You can also uninstall from Windows **Settings → Apps → Installed apps**.

Upgrade and uninstall request a graceful exit only from the installed executable's path. Uninstall removes installed files and shortcuts, and removes the sign-in startup entry only when it points to that installation. Your settings are kept for reinstalling.

The installer and portable application both store settings in `%LOCALAPPDATA%\QuietPin\settings.ini`. To remove a portable copy, disable sign-in startup in Settings, exit QuietPin and delete its folder. To erase personal settings as well, delete `%LOCALAPPDATA%\QuietPin` after exiting.

## Notes

QuietPin runs with ordinary user permissions. Restricted administrator windows show a friendly message. Desktop, taskbar and special Shell windows are excluded. A topmost window is above ordinary windows; exclusive fullscreen, other topmost windows and the secure desktop may still cover it.

Normal exit attempts to restore windows pinned by this instance. Force termination or another tool changing the same window can prevent restoration. Toggle that window again to remove its topmost state.

## Build from source

Requires Windows, CMake 3.20+ and a C++20 compiler. Packaging uses Inno Setup 6.7+.

```powershell
.\scripts\build.ps1 -Compiler MinGW -SkipTests
# Or use a VS 2022 Developer PowerShell and a fresh build folder:
.\scripts\build.ps1 -Compiler MSVC -SkipTests
.\scripts\package.ps1 -InnoCompiler "C:\Path\ISCC.exe"
```

Command-line options: `--settings`, `--exit`, `--startup`, and `--config-dir "C:\Profile"` for a separate configuration. `--exit-if-owned` is used internally by the installer to stop only its installed copy.
