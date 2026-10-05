# QuietPin

**中文** · [English](README.md)

Windows 11 x64 轻量窗口置顶工具。原生 C++20 / Win32，无需额外运行时。默认后台运行，无主窗口、无任务栏图标、无托盘图标。

[下载 v0.2.0](https://github.com/LE-saber/QuietPin/releases/tag/v0.2.0) · [更新日志](CHANGELOG.md)

| 下载 | 用途 |
| --- | --- |
| [Windows 安装版](https://github.com/LE-saber/QuietPin/releases/download/v0.2.0/QuietPinSetup-v0.2.0-win-x64.exe) | 当前用户安装，无需管理员权限 |
| [便携版 ZIP](https://github.com/LE-saber/QuietPin/releases/download/v0.2.0/QuietPin-v0.2.0-win-x64.zip) | 解压到固定目录，运行 `QuietPin.exe` |
| [SHA256 校验文件](https://github.com/LE-saber/QuietPin/releases/download/v0.2.0/QuietPin-v0.2.0-SHA256SUMS.txt) | 核对下载文件 |

## 开始使用

| 操作 | 默认快捷键 |
| --- | --- |
| 置顶 / 取消置顶当前活动窗口 | **Ctrl + Alt + T** |
| 打开设置 | **Ctrl + Alt + Shift + T** |
| 完全退出程序 | **Ctrl + Alt + Shift + Q** |

对同一窗口再次按置顶快捷键即可取消，支持同时置顶多个窗口。

再次运行 `QuietPin.exe` 会打开已有实例的设置。包内也提供 `open-settings.cmd` 和 `exit.cmd`。关闭设置会返回后台，点击 **退出 QuietPin** 才会完全退出。

## 设置

- 修改置顶快捷键；快捷键冲突时保留原有配置。
- 开启或关闭短暂状态提示、登录启动、托盘图标和 Pin 按钮。
- 选择 **English** 或 **中文** 后保存。首次运行默认英语，已有语言选择会保留。
- 排除程序：每行输入一个 EXE 文件名或完整路径；被排除的程序不执行置顶，也不显示 Pin 按钮。
- 调整 Pin 按钮水平 / 垂直偏移，或恢复默认位置。
- 手动 **检查更新** 或打开 **下载页面**。无后台定时检查，不自动安装；同版本发布文件有变化时，通过 EXE 校验值识别不同构建。

Pin 按钮和托盘图标默认关闭。Pin 按钮显示在活动窗口标题栏附近，跟随窗口，不抢焦点；没有安全位置时隐藏，仍可使用快捷键。

安装向导可选 English / 简体中文，默认 English。程序界面语言在设置中单独选择。

## 安装与卸载

默认安装目录为 `%LOCALAPPDATA%\Programs\QuietPin`。开始菜单提供启动、设置、退出和卸载入口，桌面快捷方式可选。也可从 Windows **设置 → 应用 → 已安装的应用** 卸载。

升级和卸载仅请求本安装路径的实例正常退出。卸载删除安装文件、快捷方式，并仅删除指向本安装路径的登录启动项。个人设置保留，便于重新安装。

安装版与便携版的配置均保存在 `%LOCALAPPDATA%\QuietPin\settings.ini`。移除便携版时，在设置中关闭登录启动，退出程序后删除解压目录。如需一并清除个人设置，退出后删除 `%LOCALAPPDATA%\QuietPin`。

## 使用说明

程序以普通权限运行，受限管理员窗口会给出友好提示。桌面、任务栏和特殊 Shell 窗口不允许操作。置顶使窗口高于普通窗口，独占全屏、其他置顶窗口和安全桌面仍可能遮挡它。

正常退出会尽力撤销本实例添加的置顶。强制结束程序或其他工具修改同一窗口可能影响恢复，此时对该窗口再次切换置顶即可。

## 源码构建

需要 Windows、CMake 3.20+ 和 C++20 编译器，安装包使用 Inno Setup 6.7+。

```powershell
.\scripts\build.ps1 -Compiler MinGW -SkipTests
# 或在 VS 2022 Developer PowerShell 中使用新的构建目录：
.\scripts\build.ps1 -Compiler MSVC -SkipTests
.\scripts\package.ps1 -InnoCompiler "C:\路径\ISCC.exe"
```

命令行参数：`--settings`、`--exit`、`--startup`。`--config-dir "C:\配置目录"` 可使用独立配置；`--exit-if-owned` 供安装程序退出本安装路径的实例。
