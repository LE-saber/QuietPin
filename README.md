# QuietPin MVP 0.1

Windows 11 x64 轻量窗口置顶工具。原生 C++/Win32，不需要安装 .NET、Electron 或其他运行时。默认无主窗口、无控制台、无任务栏图标、无托盘图标。

[下载 v0.1.0 便携包](https://github.com/LE-saber/QuietPin/releases/tag/v0.1.0) · [Pin 下一阶段技术计划](docs/pin-implementation-plan.zh-CN.md)。当前发布版不含 Pin；计划已完成，实施待后续指令。

## 开始使用

解压完整便携包到固定目录，双击 **QuietPin.exe**。正常启动没有弹窗，直接后台运行。

| 操作 | 默认快捷键 |
| --- | --- |
| 置顶 / 取消置顶当前活动窗口 | **Ctrl + Alt + T** |
| 打开设置 | **Ctrl + Alt + Shift + T** |
| 完全退出程序 | **Ctrl + Alt + Shift + Q** |

置顶某个窗口后，再次激活该窗口并按置顶快捷键即可取消。可同时置顶多个窗口；置顶只保证高于普通窗口，无法保证覆盖其他置顶窗口、独占全屏或安全桌面。

无托盘时，**再次双击 QuietPin.exe 会打开已有实例的设置**。也可双击包内 `open-settings.cmd`。`exit.cmd` 请求已有实例退出。关闭设置窗口仅返回后台，设置内的“退出 QuietPin”按钮才会退出。

## 本版功能

- 默认全局热键置顶/取消，长按不连续切换。
- 自定义置顶快捷键（Ctrl/Alt/Shift/Win + 字母、数字或 F1–F11），冲突时保留旧组合与配置。
- 中文 / English 设置、状态提示开关、登录启动开关。
- 可选托盘，菜单提供置顶/取消、设置和退出；默认关闭。
- 排除 EXE 文件名或完整路径；不区分大小写，精确匹配，每行一条。
- 状态提示不抢焦点；关闭后操作结果可在设置中查看。
- 保护桌面、任务栏及 Shell 特殊窗口；普通权限下拒绝受限窗口，不自动提权。
- 单实例运行；正常退出时尽力撤销本实例添加的置顶。

**Pin 按钮尚未包含在 MVP 中。** 设置显示说明，不提供无效开关。设置和退出快捷键在本版固定；只有置顶快捷键可修改。

## 设置与启动

设置保存在 `%LOCALAPPDATA%\QuietPin\settings.ini`，按“保存”生效。损坏配置会尝试保留 `.bak` 备份后打开设置；备份失败时禁止覆盖。

排除 `chrome.exe` 会排除所有同名 EXE；排除 `C:\某目录\chrome.exe` 只匹配该路径。浏览器和资源管理器正常窗口默认允许操作，系统桌面和任务栏始终受保护。

“登录 Windows 时启动”默认关闭。开启后设置写入当前用户的 Run 启动项，路径带双引号。程序移动到另一目录后，需要在设置中重新核对启动项；遇到已有启动项指向其他路径时保留它，先手动处理原路径或关闭原版本启动项。退出程序不等于关闭下一次登录启动。

如果热键被其他应用占用，程序会打开设置提示。设置/退出入口冲突时可用再次运行 EXE、`open-settings.cmd` 或 `exit.cmd` 管理。

## 命令行

```text
QuietPin.exe
QuietPin.exe --settings
QuietPin.exe --exit
QuietPin.exe --startup
```

`--startup` 在已有实例时安静返回。`--exit` 没有实例时直接返回。退出码：0 成功或无实例；1 启动/运行失败；2 参数错误；3 已有实例无响应。成功的退出请求表示命令已送达，进程清理仍可能花费约 0.7 秒。

高级用途：`--config-dir "C:\某目录"` 使用独立配置与实例身份，主要用于隔离测试。不同配置实例仍不能占用相同全局快捷键，日常使用不需要此参数。

## MVP 验证与边界

已在本机 Windows 11 x64 构建并执行配置/策略测试及 Win32 真实窗口集成测试，包含真实 SendInput 热键、焦点保持、单实例、冲突回滚、语言/排除持久化、退出快捷键和正常退出清理。详见 `verification.zh-CN.md`。

这是 **MinGW 构建的未签名 MVP**。尚未完成管理员窗口实机矩阵、Explorer 重启、所有常见应用、多屏/混合 DPI、登录后启动及两小时稳定性验收。MSVC 发布构建也未在此机器上验证。不自动修改 UAC 或要求管理员权限。

强制结束/崩溃可能留下目标窗口的置顶状态；重启工具后对该窗口按置顶快捷键即可取消。与其他置顶工具同时运行时，Windows 不提供可靠的状态所有权，退出恢复只能尽力执行。

## 源码构建

需要 Windows、CMake 3.20+ 和 C++20 编译器。本次已验证 MinGW GCC 14.2 + Ninja。MSVC 路径提供但未实测。

```powershell
# 在源码目录执行；测试会短暂创建自己的窗口并切换焦点
.\scripts\build.ps1 -Compiler MinGW
# 在 VS 2022 Developer PowerShell 中使用（更换编译器时用新的构建目录）
.\scripts\build.ps1 -Compiler MSVC
```

运行测试：`ctest --test-dir build --output-on-failure`。测量默认空闲模式：`.\scripts\measure-idle.ps1 -Seconds 60`。

便携包内附 `technical-design.zh-CN.md` 与 `verification.zh-CN.md`。源码目录中的完整方案和后续路线分别位于 `docs/technical-design.zh-CN.md` 与 `.planning/ROADMAP.md`。
