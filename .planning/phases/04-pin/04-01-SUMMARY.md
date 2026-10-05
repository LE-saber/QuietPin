---
phase: 04-pin
plan: 01
subsystem: native-ui
tags: [win32, pin, foreground, configuration]
requires: [01-mvp]
provides: [默认关闭的 Pin, 非激活真实点击, Tray/Pin 共享前台订阅]
affects: [04-02, 04-03]
tech-stack:
  added: [Windows system oleacc, wtsapi32]
  patterns: [单 UI 线程事件合并, 手势身份及代次校验]
key-files:
  created: [src/PinOverlay.cpp, src/PinController.cpp, src/WindowEventMonitor.cpp, tests/pin_integration.cpp]
  modified: [src/main.cpp, src/Config.cpp, CMakeLists.txt]
requirements-completed: [CFG-08, PIN-01]
completed: "2026-10-05"
---

# 04-01 — 非激活 Pin 最小完整路径

配置可选字段/带符号偏移及旧版本兼容已完成；设置增加默认关闭的 Pin 开关。独立无 owner 的 TOOLWINDOW/NOACTIVATE/TOPMOST popup 使用 GDI；Tray/Pin 前台订阅独立启停，点击复用 App toggle/confirm。

按下和释放复核前台、Identity、绑定代次，目标切换取消手势。设置打开隐藏、关闭恢复；Pin 停用销毁与注销 WTS/目标 hook；原无托盘退出保留。

## 验证
本机 MinGW Release 构建及 CTest 3/3 通过（约 2.38 秒），含原 MVP 回归和真实 SendInput 鼠标 Pin/Unpin、实际命中、前台/键盘焦点保持、按下后切换目标取消、Pin/Tray 开关与目标销毁。配置测试包含旧 schema 1 缺字段、负偏移、溢出/越界、不合法保存保持原文件。

真实点击最初发现“IsWindowVisible 为真但不在最前层”；通过在创建时明确 WS_EX_TOPMOST、独立 popup，并增加鼠标命中断言修复。本地临时诊断代码已移除。MinGW oleacc/uuid GUID 冲突使用本地标准 IAccessible IID 避免重复导入符号。

## 未完成范围
目前仅保守顶边外侧布局；完整安全内侧/偏移界面、多屏/混合 DPI、会话/资源与辅助功能验证由 04-02/03 承接。已有目标事件过滤作为后续接线基础，但本 SUMMARY 不宣称完整跟随/DPI 验收。
