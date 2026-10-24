# QuietPin

[English](README.md) · [简体中文](README.zh.md) · [繁體中文](README.zh-TW.md) · [日本語](README.ja.md)

Windows 11 x64 輕量視窗置頂工具，以原生 C++20 / Win32 實作，不需額外執行階段。預設在背景執行，沒有主視窗、工作列按鈕或系統匣圖示。

[下載 v0.2.0](https://github.com/LE-saber/QuietPin/releases/tag/v0.2.0) · [更新記錄](CHANGELOG.md)

| 套件 | 用途 |
| --- | --- |
| [Windows 安裝程式](https://github.com/LE-saber/QuietPin/releases/download/v0.2.0/QuietPinSetup-v0.2.0-win-x64.exe) | 安裝至目前使用者，不需要系統管理員權限 |
| [可攜式 ZIP](https://github.com/LE-saber/QuietPin/releases/download/v0.2.0/QuietPin-v0.2.0-win-x64.zip) | 解壓縮完整資料夾後執行 `QuietPin.exe` |
| [SHA256 校驗值](https://github.com/LE-saber/QuietPin/releases/download/v0.2.0/QuietPin-v0.2.0-SHA256SUMS.txt) | 驗證下載檔案 |

## 快速開始

| 操作 | 預設快速鍵 |
| --- | --- |
| 切換目前視窗的置頂狀態 | **Ctrl + Alt + T** |
| 開啟設定 | **Ctrl + Alt + Shift + T** |
| 結束 QuietPin | **Ctrl + Alt + Shift + Q** |

對相同視窗再按一次即可取消置頂，也可以同時置頂多個視窗。

再次執行 `QuietPin.exe` 會開啟既有執行個體的設定。套件亦包含 `open-settings.cmd` 與 `exit.cmd`。關閉設定視窗會回到背景，選擇 **Exit QuietPin** 才會完全結束程式。

## 設定

- 自訂置頂快速鍵；若發生衝突則保留原設定。
- 啟用或停用短暫狀態提示、登入啟動、系統匣圖示與 Pin 按鈕。
- 選擇 **English** 或 **中文**，再按 **Save**。新設定預設為英語，保留既有語言選擇。
- 依 EXE 名稱或完整路徑排除程式，每行一個；被排除程式無法置頂，也不顯示 Pin。
- 調整 Pin 的 X/Y 位移或重設位置。
- 手動 **Check for updates** 或開啟 **Download page**。沒有定期背景檢查或自動安裝；相同版本號可藉發布的 EXE 雜湊辨識不同組建。

Pin 與系統匣預設關閉。Pin 會跟隨作用中視窗，靠近標題列顯示且不搶走焦點；沒有安全位置時隱藏，鍵盤快速鍵仍可使用。

安裝程式提供英語與簡體中文，預設英語。應用程式的語言須另於設定中選擇。

## 安裝與移除

預設安裝至 `%LOCALAPPDATA%\Programs\QuietPin`。開始功能表包含執行、設定、結束與解除安裝捷徑；桌面捷徑為選用，也可由 Windows「設定 → 應用程式 → 已安裝的應用程式」移除。

升級與解除安裝只會要求此安裝路徑的執行個體正常結束。移除已安裝檔案與捷徑時，僅在登入啟動項指向此次安裝時移除該項；個人設定會保留，方便重新安裝。

安裝版與可攜版皆將設定儲存於 `%LOCALAPPDATA%\QuietPin\settings.ini`。移除可攜版前，先在設定關閉登入啟動、結束程式，再刪除資料夾。若也要清除個人設定，結束後刪除 `%LOCALAPPDATA%\QuietPin`。

## 使用範圍

QuietPin 以一般使用者權限執行，遇到權限較高或受保護的視窗會顯示提示。桌面、工作列與特殊 Shell 視窗會被排除。置頂只代表高於普通視窗，獨佔全螢幕、其他置頂視窗與安全桌面仍可能覆蓋它。

正常結束會嘗試恢復此執行個體置頂的視窗。強制終止或其他工具修改相同視窗可能使恢復失敗，可再次切換該視窗的置頂狀態。

## 從原始碼建置

需要 Windows、CMake 3.20+ 與 C++20 編譯器；封裝使用 Inno Setup 6.7+。

```powershell
.\scripts\build.ps1 -Compiler MinGW -SkipTests
```

或在 VS 2022 Developer PowerShell 與全新建置目錄執行：

```powershell
.\scripts\build.ps1 -Compiler MSVC -SkipTests
.\scripts\package.ps1 -InnoCompiler "C:\Path\ISCC.exe"
```

命令列選項為 `--settings`、`--exit`、`--startup`，以及用於獨立設定的 `--config-dir "C:\Profile"`。`--exit-if-owned` 由安裝程式內部使用，只停止它所安裝的副本。

## 文件與授權

四語 README 不會增加程式介面語言；目前程式與安裝器支援英語及簡體中文。儲存庫未提供根目錄專案授權，安裝器語言檔案的個別授權見[說明](installer/languages/README.md)。
