# Project Research Summary

## Key Findings
C++20 + Win32 适合默认无 UI 的轻量后台工具。PinWindow 为 .NET 8/WinForms；PowerToys 的窗口筛选、排除和 WinEvent 可作原则参考。全部源码独立编写。

## Implications for Roadmap
先核心热键，再管理设置和退出，再提示/托盘，最后 Pin 与多屏验证。托盘操作不能以菜单打开后的 Shell 前台作为目标；异步 SetWindowPos 需要核对实际状态。默认态不应有周期计时器。

## Sources
完整证据和源码快照见 [技术文档](../../docs/technical-design.zh-CN.md#19-调研来源与文档核对记录)。Context7 Windows API 库 `/websites/learn_microsoft_en-us_windows_win32_api`；CMake `/kitware/cmake`，2026-10-05 查询。研究在当前会话内完成，未派生代理。
