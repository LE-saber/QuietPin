---
phase: 04-pin
plan: 02
subsystem: geometry-events
requires: [04-01]
provides: [安全位置候选, 事件跟随, DIP 偏移与重置, 会话隔离]
key-files:
  created: [src/PinPlacement.h, src/PinPlacement.cpp]
  modified: [src/PinController.cpp, src/main.cpp, tests/core_tests.cpp, tests/pin_integration.cpp]
requirements-completed: [PIN-02]
completed: "2026-10-05"
---

外侧优先，内侧需要五点 HTCAPTION 验证；探测总预算 50ms，无安全位置隐藏。屏幕工作区、系统按钮、窄窗和无标题全屏过滤；overlay 自身 DPI 仅缩放一次，支持负坐标。设置可保存 DIP 偏移并重置。会话可用状态独立于设置/托盘暂停，锁屏后不会由关闭设置重新启用。

绑定窗口的精确 WinEvent 范围驱动位置刷新；移动期间 33ms 补偿最多 5 秒，正常静止不轮询。位置改变取消已按下手势。Pin 停用销毁 popup、tooltip、目标 hooks、WTS 注册与 timer。

MinGW Release 和 CTest 3/3 通过。真实窗口测试覆盖移动/缩放、最小化恢复、最大化后恢复、偏移保存/重置、排除立即隐藏。几何测试覆盖 100/125/150/200% 和负坐标、全屏/窄窗/未知标题安全隐藏。MSAA 动作名称读取通过。测试跨进程修改 Edit 必须发送 WM_SETTEXT；修正夹具后保存验证通过。

真实双屏混合 DPI、热插拔、锁屏及各应用自绘标题栏尚未手工验收，PIN-03/PIN-04 不据此宣称全部通过。
