# Requirements: QuietPin

Defined: 2026-10-05
Core Value: 无托盘后台下仍能稳定置顶与管理。

## v1 Requirements
- [x] **CORE-01**: 活动窗口置顶与再按取消
- [x] **CORE-02**: 默认 Ctrl+Alt+T 且长按不重复
- [x] **CORE-03**: 后台启动无主窗口/控制台/任务栏/托盘
- [x] **CORE-04**: 单实例
- [ ] **SAFE-01**: Shell/桌面/任务栏保护
- [ ] **SAFE-02**: 管理员及受限目标不崩溃
- [ ] **SAFE-03**: 目标销毁/无响应/句柄复用安全
- [x] **CFG-01**: 修改置顶快捷键并持久化
- [x] **CFG-02**: 设置快捷键与管理恢复入口
- [x] **CFG-03**: 设置关闭回后台；设置里可彻底退出
- [x] **CFG-04**: 全局退出快捷键
- [x] **CFG-05**: 状态提示开关
- [ ] **CFG-06**: 开机启动开关
- [ ] **CFG-07**: 托盘图标开关
- [ ] **CFG-08**: Pin 按钮开关
- [x] **CFG-09**: 中文 / English
- [x] **CFG-10**: 配置损坏/写入失败可恢复
- [ ] **UX-01**: 短暂提示不抢焦点且不阻挡点击
- [ ] **TRAY-01**: 托盘置顶/设置/退出
- [ ] **TRAY-02**: Explorer 重启恢复托盘
- [x] **EXCL-01**: 支持 EXE 路径/文件名排除
- [x] **EXCL-02**: 排除规则变更即时生效
- [ ] **PIN-01**: 当前活动窗口附近按钮可切换置顶
- [ ] **PIN-02**: 移动/缩放/最大化/还原/关闭跟随
- [ ] **PIN-03**: 多显示器与 DPI
- [ ] **PIN-04**: 自绘标题栏与全屏降级
- [ ] **COMPAT-01**: Win32/浏览器/Explorer/Electron
- [ ] **PERF-01**: 低资源且无大型运行时依赖
- [x] **EXIT-01**: 退出清理与尽力撤销新增置顶

## MVP scope
本次实现阶段 1–3；Pin 为后续阶段 4，完整平台矩阵/性能为阶段 5。MVP 验证记录在 docs/verification.zh-CN.md。

## Out of Scope
账号、联网更新、遥测、驱动、注入、透明度、画中画。

## Traceability
| Requirement | Phase | Status |
| --- | --- | --- |
| CORE-01 | Phase 1 | MVP verified |
| CORE-02 | Phase 1 | MVP verified |
| CORE-03 | Phase 1 | MVP verified |
| CORE-04 | Phase 1 | MVP verified |
| SAFE-01 | Phase 1 | Implemented; manual coverage pending |
| SAFE-02 | Phase 1 | Implemented; manual coverage pending |
| SAFE-03 | Phase 1 | Implemented; manual coverage pending |
| CFG-01 | Phase 2 | MVP verified |
| CFG-02 | Phase 2 | MVP verified |
| CFG-03 | Phase 2 | MVP verified |
| CFG-04 | Phase 2 | MVP verified |
| CFG-05 | Phase 2 | MVP verified |
| CFG-06 | Phase 2 | Implemented; manual coverage pending |
| CFG-07 | Phase 3 | Implemented; manual coverage pending |
| CFG-08 | Phase 4 | Pending |
| CFG-09 | Phase 2 | MVP verified |
| CFG-10 | Phase 2 | MVP verified |
| UX-01 | Phase 3 | Implemented; manual coverage pending |
| TRAY-01 | Phase 3 | Implemented; manual coverage pending |
| TRAY-02 | Phase 3 | Implemented; manual coverage pending |
| EXCL-01 | Phase 2 | MVP verified |
| EXCL-02 | Phase 2 | MVP verified |
| PIN-01 | Phase 4 | Pending |
| PIN-02 | Phase 4 | Pending |
| PIN-03 | Phase 4 | Pending |
| PIN-04 | Phase 4 | Pending |
| COMPAT-01 | Phase 5 | Pending |
| PERF-01 | Phase 5 | Pending |
| EXIT-01 | Phase 3 | MVP verified |

Coverage: 
29
 requirements, 
29
 mapped, 0 unmapped.
