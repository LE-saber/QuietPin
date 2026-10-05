# Phase 4 — Plan Check

日期：2026-10-05。结果：当前会话计划审查通过，运行验收尚未开始。

本项目明确顺序执行、未请求子代理；依技能 Codex adapter 的 inline fallback，研究/规划/审查在当前会话完成。这不等价独立 plan-checker 或外部同行评审。

## 工具证据
- `frontmatter validate <plan> --schema plan`：3/3 valid，无 missing/invalidValue。
- `verify plan-structure <plan>`：3/3 valid；每份 3 tasks，共 9；errors/warnings 均为空。
- `query check.decision-coverage-plan .planning/phases/04-pin .planning/phases/04-pin/04-CONTEXT.md`：passed=true，total=10，covered=10，uncovered=[]。
- 需求 frontmatter 覆盖：CFG-08、PIN-01、PIN-02、PIN-03、PIN-04，5/5。
- `query state.planned-phase --phase 4 --name Pin --plans 3` 已记录当前阶段；旧 STATE 的状态文本经同步纠正。
- `query roadmap.annotate-dependencies 4`：3 waves，2 cross-cutting constraints。
- 验证命令已检查现有 scripts/build.ps1、scripts/package.ps1 与 CTest 配置。未来 Pin 用例随功能落地，当前不能运行或宣称通过。

## 范围与顺序
04-01 无依赖，交付普通窗口完整 tracer；04-02 依赖 04-01，交付窗口/屏幕跟随与偏移；04-03 依赖 04-02，补全体验、验证和候选包。源码小模块围绕现有 toggle/confirm 与配置事务接线，不扩大为重写框架。

| 需求 | 计划 | 验收核心 |
| --- | --- | --- |
| CFG-08 | 01/02/03 | 默认关闭、旧配置兼容、设置开关/偏移、关闭资源 |
| PIN-01 | 01/03 | 真实鼠标、前台/焦点、身份与代次、真实确认 |
| PIN-02 | 02/03 | 移动/缩放/最大化/销毁/失效、陈旧事件不复活 |
| PIN-03 | 02/03 | 物理坐标、自身 DPI、负坐标、真实双屏证据 |
| PIN-04 | 02/03 | 外侧优先、标题区安全、自绘/全屏降级 |

## 审查修订与剩余风险
已明确系统/对象事件过滤区别、不能跳过本工具前台事件、补偿计时最长安全期限、WTS 注册/注销、DWM caption-button 原点、目标 DPI awareness、真实 HWND 寿命竞争与配置回退边界。安全标题区不能由 WS_CAPTION 一项判定；无安全候选隐藏。预算为未来实测目标，无零 CPU 或所有应用兼容保证。

独立审查未执行；真实混合 DPI/硬件、辅助功能、目标应用事件完整性需实施阶段验证。PIN 条目保持未完成，VALIDATION 为 draft/nyquist_compliant=false。本轮没有新增 Pin 代码、构建或发布 v0.2。
