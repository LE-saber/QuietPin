# Requirements: QuietPin

Defined: 2026-10-05
Core Value: 无托盘后台下仍能稳定置顶与管理。

## v1 Requirements
- [ ] **CORE-01**: 活动窗口置顶与再按取消
- [ ] **CORE-02**: 默认 Ctrl+Alt+T 且长按不重复
- [ ] **CORE-03**: 后台启动无主窗口/控制台/任务栏/托盘
- [ ] **CORE-04**: 单实例
- [ ] **SAFE-01**: Shell/桌面/任务栏保护
- [ ] **SAFE-02**: 管理员及受限目标不崩溃
- [ ] **SAFE-03**: 目标销毁/无响应/句柄复用安全
- [ ] **CFG-01**: 修改置顶快捷键并持久化
- [ ] **CFG-02**: 设置快捷键与管理恢复入口
- [ ] **CFG-03**: 设置关闭回后台；设置里可彻底退出
- [ ] **CFG-04**: 全局退出快捷键
- [ ] **CFG-05**: 状态提示开关
- [ ] **CFG-06**: 开机启动开关
- [ ] **CFG-07**: 托盘图标开关
- [ ] **CFG-08**: Pin 按钮开关
- [ ] **CFG-09**: 中文 / English
- [ ] **CFG-10**: 配置损坏/写入失败可恢复
- [ ] **UX-01**: 短暂提示不抢焦点且不阻挡点击
- [ ] **TRAY-01**: 托盘置顶/设置/退出
- [ ] **TRAY-02**: Explorer 重启恢复托盘
- [ ] **EXCL-01**: 支持 EXE 路径/文件名排除
- [ ] **EXCL-02**: 排除规则变更即时生效
- [ ] **PIN-01**: 当前活动窗口附近按钮可切换置顶
- [ ] **PIN-02**: 移动/缩放/最大化/还原/关闭跟随
- [ ] **PIN-03**: 多显示器与 DPI
- [ ] **PIN-04**: 自绘标题栏与全屏降级
- [ ] **COMPAT-01**: Win32/浏览器/Explorer/Electron
- [ ] **PERF-01**: 低资源且无大型运行时依赖
- [ ] **EXIT-01**: 退出清理与尽力撤销新增置顶

## MVP scope
本次实现阶段 1–3；Pin 为后续阶段 4，完整平台矩阵/性能为阶段 5。MVP 验证记录在 docs/verification.zh-CN.md。

## Out of Scope
账号、联网更新、遥测、驱动、注入、透明度、画中画。

## Traceability
| Requirement | Phase | Status |
| --- | --- | --- |
| CORE-01 | Phase 1 | Pending |
| CORE-02 | Phase 1 | Pending |
| CORE-03 | Phase 1 | Pending |
| CORE-04 | Phase 1 | Pending |
| SAFE-01 | Phase 1 | Pending |
| SAFE-02 | Phase 1 | Pending |
| SAFE-03 | Phase 1 | Pending |
| CFG-01 | Phase 2 | Pending |
| CFG-02 | Phase 2 | Pending |
| CFG-03 | Phase 2 | Pending |
| CFG-04 | Phase 2 | Pending |
| CFG-05 | Phase 2 | Pending |
| CFG-06 | Phase 2 | Pending |
| CFG-07 | Phase 3 | Pending |
| CFG-08 | Phase 4 | Pending |
| CFG-09 | Phase 2 | Pending |
| CFG-10 | Phase 2 | Pending |
| UX-01 | Phase 3 | Pending |
| TRAY-01 | Phase 3 | Pending |
| TRAY-02 | Phase 3 | Pending |
| EXCL-01 | Phase 2 | Pending |
| EXCL-02 | Phase 2 | Pending |
| PIN-01 | Phase 4 | Pending |
| PIN-02 | Phase 4 | Pending |
| PIN-03 | Phase 4 | Pending |
| PIN-04 | Phase 4 | Pending |
| COMPAT-01 | Phase 5 | Pending |
| PERF-01 | Phase 5 | Pending |
| EXIT-01 | Phase 3 | Pending |

Coverage: 
29
 requirements, 
29
 mapped, 0 unmapped.
