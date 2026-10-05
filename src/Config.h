#pragma once
#include <windows.h>
#include <filesystem>
#include <string>
#include <vector>

namespace qp {
struct Hotkey {
    UINT modifiers = MOD_CONTROL | MOD_ALT;
    UINT key = 'T';
    bool operator==(const Hotkey&) const = default;
};
struct Settings {
    Hotkey toggle;
    bool status = true;
    bool tray = false;
    bool startup = false;
    bool pin = false;
    UINT pinSize = 24;
    int pinOffsetX = 0, pinOffsetY = 0;
    bool chinese = false;
    std::vector<std::wstring> excluded;
};
std::wstring trim(std::wstring text);
std::wstring lower(std::wstring text);
bool validHotkey(Hotkey hotkey);
bool pinOffset(const std::wstring& text, int minimum, int maximum, int& value);
bool validPinSettings(const Settings& settings);
std::wstring keyName(UINT key);
std::wstring hotkeyText(Hotkey hotkey);
bool isExcluded(const std::wstring& path, const std::vector<std::wstring>& rules);
std::wstring winError(DWORD code);
bool loadSettings(const std::filesystem::path& file, Settings& settings, std::wstring& error);
bool saveSettings(const std::filesystem::path& file, const Settings& settings, std::wstring& error);
std::filesystem::path executablePath();
std::filesystem::path defaultConfigDirectory();
struct StartupSnapshot {
    bool exists = false;
    DWORD type = REG_SZ;
    std::vector<BYTE> bytes;
};
bool readStartup(const std::wstring& value, StartupSnapshot& old, std::wstring& error);
bool writeStartup(const std::wstring& value, bool enabled, std::wstring& error);
bool restoreStartup(const std::wstring& value, const StartupSnapshot& old, std::wstring& error);
bool startupMatches(const StartupSnapshot& snapshot);
}
