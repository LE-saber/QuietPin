# Inno Setup language data

`ChineseSimplified.isl` is the unmodified Simplified Chinese translation from the official [jrsoftware/issrc repository](https://github.com/jrsoftware/issrc/blob/main/Files/Languages/ChineseSimplified.isl), retrieved on 2026-10-05. Its original maintainer attribution is retained. Upstream Git blob: `d0d44258f51aa3127fc0d1167a7c8be328a9bdcf` (Inno Setup 6.5.0+ format).

The upstream Inno Setup license is included in `INNO-LICENSE.txt`. These are installer resources, not runtime libraries or QuietPin-authored translations.

QuietPin uses English first, `LanguageDetectionMethod=none`, `ShowLanguageDialog=yes` and `UsePreviousLanguage=no`, so every interactive installation offers both languages with English selected by default. Application-specific task names, shortcut names, launch text and exit errors are translated in QuietPin.iss. Application Settings language remains independent from wizard language; new application profiles default to English and existing profiles retain their saved language.

中文：简体中文语言文件来自 Inno Setup 官方仓库，保留原作者署名与许可证。安装时可选 English / 简体中文，默认 English；程序首次运行也默认英语，已有语言配置保持不变。安装向导语言与应用设置语言相互独立。
