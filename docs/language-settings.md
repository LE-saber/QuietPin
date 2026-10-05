# Language settings / 语言设置 — v0.2.1

## 中文

- 交互安装打开语言选择框，可选 **English / 简体中文**，默认 English；升级也默认 English，用户仍可重新选择。
- 安装向导、安装选项、结束页面、快捷方式名称和退出失败提示随选择切换。Windows 自身消息框按钮可能采用系统语言。
- 程序没有旧配置时默认英语。安装向导选择中文只影响安装流程，不自动改写应用配置。
- 安装后按 **Ctrl+Alt+Shift+T** 打开设置，在 **Language** 选择 **中文 / English**，点击 **Save / 保存**，立即更新设置界面并持久化。
- 升级保留已有 `settings.ini` 中的语言；已经保存为中文的用户不会被强制改回英语。

本次以 v0.2.1 新包交付，不替换用户已验收的 v0.2.0。仅构建与打包，按此前要求由用户执行最终人工测试。

## English

- Interactive installation offers **English / 简体中文**, with English selected by default, including upgrades.
- Wizard pages, installer tasks, completion action, shortcut labels and graceful-exit errors follow that choice. Windows-native message-box buttons may follow the OS language.
- New application profiles default to English. Selecting Chinese for the installer does not rewrite the application's settings.
- After installation press **Ctrl+Alt+Shift+T**, choose **中文 / English** under **Language**, then **Save**. The Settings window updates immediately and the choice is saved.
- Upgrades preserve the language already saved in `settings.ini`; existing Chinese configurations are not reset.

This change is delivered as a new v0.2.1 package without replacing the accepted v0.2.0 files. Only compilation and packaging are performed; final manual testing remains with the user.

## Implementation references

Installer defaults follow the official [LanguageDetectionMethod](https://jrsoftware.org/ishelp/topic_setup_languagedetectionmethod.htm), [ShowLanguageDialog](https://jrsoftware.org/ishelp/topic_setup_showlanguagedialog.htm) and [UsePreviousLanguage](https://jrsoftware.org/ishelp/topic_setup_usepreviouslanguage.htm) documentation. Runtime language defaults are in `src/Config.h`; existing configuration loading and the Settings language selector preserve user choices.
