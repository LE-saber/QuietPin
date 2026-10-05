---
phase: "4"
slug: "pin"
status: draft
nyquist_compliant: false
wave_0_complete: false
created: "2026-10-05"
---

# Phase 4 — Validation Strategy

## Test Infrastructure
现有 C++ console tests + Win32 集成夹具，CTest/CMake，无新框架。快速命令 `ctest --test-dir build -R core --output-on-failure`；完整命令 `ctest --test-dir build --output-on-failure`。编译后运行 `powershell -NoProfile -File scripts/build.ps1 -Compiler MinGW`（现有脚本包含配置、构建与测试）。现有 MVP 两项测试约 1.3 秒，Pin 扩展预期 <35 秒，超时需按实测有界调整。

## Sampling Rate
- 配置/几何任务：每次提交前 core 测试；涉及输入/宿主任务每次完整测试。
- 每个 wave：完整测试和对应人工观察。
- 打包前：全套通过；打包后中文/空格路径解压复验。
- 反馈目标 <60 秒；无 watch 模式。测试使用自建目标和独立配置，禁止修改用户窗口和真实登录项。

## Per-Task Verification Map
| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | Status |
| --- | --- | --- | --- | --- | --- | --- |
| 04-01-01 | 01 | 1 | CFG-08 | config/core | build.ps1 -Compiler MinGW | pending |
| 04-01-02 | 01 | 1 | PIN-01 | real input/integration | build.ps1 -Compiler MinGW | pending |
| 04-01-03 | 01 | 1 | CFG-08, PIN-01 | lifecycle/integration | ctest --test-dir build --output-on-failure | pending |
| 04-02-01 | 02 | 2 | PIN-03, PIN-04, CFG-08 | geometry/config | build.ps1 -Compiler MinGW | pending |
| 04-02-02 | 02 | 2 | PIN-02, PIN-04 | lifecycle/integration | build.ps1 -Compiler MinGW | pending |
| 04-02-03 | 02 | 2 | PIN-02, PIN-03, PIN-04 | integration + manual | ctest --test-dir build --output-on-failure | pending |
| 04-03-01 | 03 | 3 | PIN-01, CFG-08 | focus/race/integration | build.ps1 -Compiler MinGW | pending |
| 04-03-02 | 03 | 3 | PIN-01–04 | regression + perf/manual | ctest --test-dir build --output-on-failure | pending |
| 04-03-03 | 03 | 3 | CFG-08, PIN-01–04 | package + extracted integration | package.ps1 | pending |

## Wave 0 Requirements
无需安装基础设施。新增用例须与被测功能同 task 落地：core_tests 的 Pin 配置/几何用例、integration 的真实鼠标/焦点/竞争用例；没有预写空壳。运行时证据全部待开发后采集，不能把当前 green MVP 推断为 Pin 测试 green。

## Manual-Only Verifications
| Behavior | Requirement | Reason and steps |
| --- | --- | --- |
| 混合 DPI / 负坐标 / 跨屏 / 热插拔 | PIN-03 | 真实双屏 100/150或200%，跨屏20次、主屏改变；记录设备和截图/采样，不以纯数学替代 |
| 自绘标签区、全屏安全隐藏 | PIN-04 | Chrome/Edge/Electron 实际版本，最大化和全屏；确认安全探测，记录隐藏原因 |
| 会话/虚拟桌面/管理员目标 | PIN-02, PIN-04 | 锁屏恢复/桌面切换，实际提升权限窗口；无残留无误操作 |
| Tooltip、可访问名称、主题 | PIN-01 | 真实鼠标/系统辅助功能检查，不激活按钮，非颜色区分 |
| 资源 | CFG-08, PIN-01–04 | 四模式采样及100次启停，核对无高频扫描、计数不线性增长 |

## Validation Sign-Off
- [x] 9/9 tasks 有自动命令，未来测试随功能同 task 补齐。
- [x] 无 3 个连续 task 无自动反馈，无 watch 模式。
- [ ] 所有 Pin 测试已实现且通过。
- [ ] 实机多屏和必测人工项通过。
- [ ] 运行时验证完成后再设置 nyquist_compliant。

Approval: 计划结构审查完成；运行验证 pending。独立代理评审未执行。
