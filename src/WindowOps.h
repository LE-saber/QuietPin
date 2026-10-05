#pragma once
#include "Config.h"
#include <optional>

namespace qp {
enum class WindowError { None, Unavailable, System, Own, Excluded, Restricted };
struct Identity {
    HWND hwnd=nullptr;
    DWORD pid=0, tid=0;
    FILETIME created{};
    std::wstring path;
};
struct CheckedWindow { std::optional<Identity> window; WindowError error=WindowError::Unavailable; };
CheckedWindow inspectWindow(HWND hwnd, const Settings& settings);
bool sameWindow(const Identity& identity);
bool isTopmost(HWND hwnd);
bool requestTopmost(const Identity& window, bool topmost, DWORD& error);
std::wstring windowErrorText(WindowError error, bool chinese);
}
