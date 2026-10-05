# 更新日志 / Changelog

## v0.2.0 — 2026-10-05 · 预发布 / Prerelease

### 中文

- 新增默认关闭的 Pin 按钮，点击置顶/取消，不抢前台或键盘焦点。
- 通过 WinEvent 跟随窗口，移动时使用有界补偿；停用后释放 Pin 专属资源。
- 外侧优先定位、标题区安全探测、无安全位置/无标题全屏隐藏。
- 新增 DIP 位置偏移与重置；排除程序立即隐藏 Pin，旧配置默认保持 Pin 关闭。
- 提供中英文 Pin 提示与 MSAA 名称。
- 新增 Windows 11 x64 当前用户安装版，保留便携 ZIP；安装提供设置、退出和卸载入口。
- 升级/卸载仅请求本安装路径实例退出，保留配置与其他路径启动项。
- 补齐中英文 README、验证记录和双语发布说明。

最终人工验收待完成，真实混合 DPI/多屏与完整应用矩阵不标记通过。完整证据见 [验证记录](https://github.com/LE-saber/QuietPin/blob/main/docs/verification-v0.2.0.zh-CN.md)。

### English

- Added an optional, default-off Pin button that toggles topmost state without taking foreground or keyboard focus.
- Added WinEvent tracking with bounded movement compensation and cleanup when Pin is disabled.
- Added outside-first placement, title-bar hit testing and safe hiding for unsupported positions or borderless fullscreen.
- Added DIP offsets/reset, immediate exclusion handling and v0.1 configuration compatibility with Pin off.
- Added Chinese/English Pin feedback and MSAA action names.
- Added a Windows 11 x64 per-user installer alongside the portable ZIP, with Settings, Exit and Uninstall entry points.
- Upgrade/uninstall only request exit from the installed path, preserving configuration and other paths' startup entries.
- Added Chinese/English READMEs, verification records and bilingual release notes.

Final manual acceptance is pending. Real mixed-DPI/multi-monitor and the complete application matrix are not marked verified. See the [verification record](https://github.com/LE-saber/QuietPin/blob/main/docs/verification-v0.2.0.en.md).

## v0.1.0 — 2026-10-05

**中文：** 发布原生 Win32 MVP：无托盘后台运行、全局置顶热键、自定义热键、中英文设置、非激活提示、可选托盘、排除、登录启动、单实例及无托盘退出。用户确认 MVP 测试成功；完整平台矩阵仍保留待验。

**English:** Released the native Win32 MVP: background operation without a tray icon, global/custom topmost shortcuts, Chinese/English Settings, nonactivating feedback, optional tray, exclusions, sign-in startup, single-instance management and no-tray exit. The user confirmed the MVP worked; broader platform acceptance remained pending.
