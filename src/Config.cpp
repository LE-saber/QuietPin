#include "Config.h"
#include <shlobj.h>
#include <algorithm>
#include <cwctype>
#include <map>
#include <sstream>
#include <limits>

namespace qp {
std::wstring trim(std::wstring s) {
    auto first = s.find_first_not_of(L" \t\r\n");
    return first == s.npos ? L"" : s.substr(first, s.find_last_not_of(L" \t\r\n") - first + 1);
}
std::wstring lower(std::wstring s) {
    if (!s.empty()) CharLowerBuffW(s.data(), static_cast<DWORD>(s.size()));
    return s;
}
bool validHotkey(Hotkey h) {
    const bool key = (h.key >= 'A' && h.key <= 'Z') || (h.key >= '0' && h.key <= '9') ||
                     (h.key >= VK_F1 && h.key <= VK_F11);
    return key && h.modifiers > 0 && (h.modifiers & ~15u) == 0;
}
std::wstring keyName(UINT key) {
    if (key >= VK_F1 && key <= VK_F11) return L"F" + std::to_wstring(key - VK_F1 + 1);
    return std::wstring(1, static_cast<wchar_t>(key));
}
std::wstring hotkeyText(Hotkey h) {
    std::wstring s;
    if (h.modifiers & MOD_CONTROL) s += L"Ctrl + ";
    if (h.modifiers & MOD_ALT) s += L"Alt + ";
    if (h.modifiers & MOD_SHIFT) s += L"Shift + ";
    if (h.modifiers & MOD_WIN) s += L"Win + ";
    return s + keyName(h.key);
}
bool isExcluded(const std::wstring& path, const std::vector<std::wstring>& rules) {
    const auto p = lower(path);
    const auto name = lower(std::filesystem::path(path).filename().wstring());
    for (const auto& rule : rules) {
        const auto r = lower(trim(rule));
        if (r.find_first_of(L"\\/:") != r.npos ? p == r : name == r) return true;
    }
    return false;
}
std::wstring winError(DWORD code) {
    wchar_t* message = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, code, 0, reinterpret_cast<LPWSTR>(&message), 0, nullptr);
    std::wstring result = message ? trim(message) : L"Windows error " + std::to_wstring(code);
    if (message) LocalFree(message);
    return result + L" (" + std::to_wstring(code) + L")";
}
static bool number(const std::wstring& value, UINT& result) {
    if (value.empty() || value.find_first_not_of(L"0123456789") != value.npos) return false;
    unsigned long long n = 0;
    for (wchar_t c : value) {
        n = n * 10 + (c - L'0');
        if (n > std::numeric_limits<UINT>::max()) return false;
    }
    result = static_cast<UINT>(n);
    return true;
}
bool loadSettings(const std::filesystem::path& file, Settings& settings, std::wstring& error) {
    HANDLE handle = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND) return true;
        error = winError(GetLastError()); return false;
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(handle, &size) || size.QuadPart < 2 || size.QuadPart > 65536 || size.QuadPart % 2) {
        CloseHandle(handle); error = L"Invalid configuration size"; return false;
    }
    std::wstring data(static_cast<size_t>(size.QuadPart / 2), L'\0');
    DWORD read = 0;
    const bool ok = ReadFile(handle, data.data(), static_cast<DWORD>(size.QuadPart), &read, nullptr) && read == size.QuadPart;
    CloseHandle(handle);
    if (!ok || data[0] != 0xfeff || data.find(L'\0') != data.npos) {
        error = L"Expected UTF-16LE INI configuration"; return false;
    }
    std::wistringstream lines(data.substr(1));
    std::map<std::wstring, std::wstring> fields;
    std::wstring line, section;
    while (std::getline(lines, line)) {
        line = trim(line);
        if (line.empty() || line[0] == L';') continue;
        if (line.front() == L'[' && line.back() == L']') { section = line.substr(1, line.size()-2); continue; }
        auto equals = line.find(L'=');
        if (equals == line.npos || section.empty()) { error = L"Invalid INI entry"; return false; }
        const auto field = section + L"." + trim(line.substr(0, equals));
        if (fields.contains(field)) { error = L"Duplicate configuration entry"; return false; }
        fields[field] = trim(line.substr(equals + 1));
    }
    Settings candidate = settings;
    UINT schema = 0, count = 0;
    auto integer = [&](const wchar_t* key, UINT& value) {
        auto it = fields.find(key); return it != fields.end() && number(it->second, value);
    };
    auto boolean = [&](const wchar_t* key, bool& value) {
        UINT n=0; if (!integer(key,n) || n > 1) return false; value=n!=0; return true;
    };
    if (!integer(L"General.SchemaVersion",schema) || schema != 1 ||
        !boolean(L"General.ShowStatus",candidate.status) || !boolean(L"General.ShowTray",candidate.tray) ||
        !boolean(L"General.StartWithWindows",candidate.startup) ||
        !integer(L"Hotkeys.ToggleModifiers",candidate.toggle.modifiers) ||
        !integer(L"Hotkeys.ToggleKey",candidate.toggle.key) || !validHotkey(candidate.toggle) ||
        (candidate.toggle.modifiers==7 && (candidate.toggle.key=='T' || candidate.toggle.key=='Q')) ||
        !integer(L"Exclusions.Count",count) || count > 128) {
        error=L"Invalid or unsupported configuration values"; return false;
    }
    const auto language = fields[L"General.Language"];
    if (language != L"zh-CN" && language != L"en") { error=L"Unsupported language"; return false; }
    candidate.chinese=language==L"zh-CN";
    candidate.excluded.clear();
    for(UINT i=0;i<count;++i) {
        const auto rule=fields[L"Exclusions.Value" + std::to_wstring(i)];
        if (rule.empty() || rule.size()>2048) { error=L"Invalid exclusion rule"; return false; }
        candidate.excluded.push_back(rule);
    }
    settings=std::move(candidate); return true;
}
bool saveSettings(const std::filesystem::path& file, const Settings& s, std::wstring& error) {
    if (!validHotkey(s.toggle) || s.excluded.size()>128) { error=L"Invalid settings"; return false; }
    std::wstring data=L"\xfeff[General]\r\nSchemaVersion=1\r\nLanguage=";
    data += s.chinese ? L"zh-CN" : L"en";
    data += L"\r\nShowStatus="+std::to_wstring(s.status)+L"\r\nShowTray="+std::to_wstring(s.tray)+
        L"\r\nStartWithWindows="+std::to_wstring(s.startup)+L"\r\n[Hotkeys]\r\nToggleModifiers="+
        std::to_wstring(s.toggle.modifiers)+L"\r\nToggleKey="+std::to_wstring(s.toggle.key)+
        L"\r\n[Exclusions]\r\nCount="+std::to_wstring(s.excluded.size())+L"\r\n";
    for(size_t i=0;i<s.excluded.size();++i) {
        if (s.excluded[i].empty() || s.excluded[i].size()>2048 || s.excluded[i].find_first_of(L"\r\n\0",0,3)!=std::wstring::npos) {
            error=L"Invalid exclusion rule"; return false;
        }
        data += L"Value"+std::to_wstring(i)+L"="+s.excluded[i]+L"\r\n";
    }
    if (data.size()*2>65536) { error=L"Configuration too large"; return false; }
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(),ec);
    if(ec) { error=L"Cannot create configuration directory: " + std::to_wstring(ec.value()); return false; }
    const auto temp=std::filesystem::path(file.wstring()+L".tmp."+std::to_wstring(GetCurrentProcessId()));
    HANDLE h=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE) { error=winError(GetLastError()); return false; }
    DWORD written=0; DWORD length=static_cast<DWORD>(data.size()*sizeof(wchar_t));
    bool ok=WriteFile(h,data.data(),length,&written,nullptr) && written==length && FlushFileBuffers(h);
    DWORD failure=ok?0:GetLastError(); CloseHandle(h);
    if(ok && !MoveFileExW(temp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) { ok=false; failure=GetLastError(); }
    if(!ok) { DeleteFileW(temp.c_str()); error=winError(failure); }
    return ok;
}
std::filesystem::path executablePath() {
    std::wstring path(32768,L'\0');
    DWORD n=GetModuleFileNameW(nullptr,path.data(),static_cast<DWORD>(path.size()));
    if(!n || n>=path.size()) throw std::runtime_error("Cannot resolve executable path");
    path.resize(n); return path;
}
std::filesystem::path defaultConfigDirectory() {
    PWSTR path=nullptr;
    if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&path))) throw std::runtime_error("Cannot resolve LocalAppData");
    auto result=std::filesystem::path(path)/L"QuietPin"; CoTaskMemFree(path); return result;
}
static constexpr auto RunKey=L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static std::wstring startupCommand() { return L"\""+executablePath().wstring()+L"\" --startup"; }
bool readStartup(const std::wstring& value, StartupSnapshot& old, std::wstring& error) {
    old={}; HKEY key=nullptr;
    LSTATUS result=RegOpenKeyExW(HKEY_CURRENT_USER,RunKey,0,KEY_QUERY_VALUE,&key);
    if(result==ERROR_FILE_NOT_FOUND) return true;
    if(result!=ERROR_SUCCESS) { error=winError(result); return false; }
    DWORD length=0;
    result=RegQueryValueExW(key,value.c_str(),nullptr,&old.type,nullptr,&length);
    if(result==ERROR_FILE_NOT_FOUND) { RegCloseKey(key); return true; }
    if(result!=ERROR_SUCCESS || length>65536) { RegCloseKey(key); error=L"Cannot read startup entry"; return false; }
    old.bytes.resize(length);
    result=RegQueryValueExW(key,value.c_str(),nullptr,&old.type,old.bytes.data(),&length); RegCloseKey(key);
    if(result!=ERROR_SUCCESS) { error=winError(result); return false; }
    old.bytes.resize(length); old.exists=true; return true;
}
bool startupMatches(const StartupSnapshot& old) {
    if(!old.exists || old.type!=REG_SZ || old.bytes.size()<2 || old.bytes.size()%2) return false;
    const auto* text=reinterpret_cast<const wchar_t*>(old.bytes.data());
    if(text[old.bytes.size()/2-1]!=0) return false;
    return lower(std::wstring(text,old.bytes.size()/2-1))==lower(startupCommand());
}
bool restoreStartup(const std::wstring& value, const StartupSnapshot& old, std::wstring& error) {
    HKEY key=nullptr;
    LSTATUS result=RegCreateKeyExW(HKEY_CURRENT_USER,RunKey,0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr);
    if(result!=ERROR_SUCCESS) { error=winError(result); return false; }
    result=old.exists?RegSetValueExW(key,value.c_str(),0,old.type,old.bytes.data(),static_cast<DWORD>(old.bytes.size())):
                      RegDeleteValueW(key,value.c_str());
    RegCloseKey(key);
    if(result==ERROR_FILE_NOT_FOUND) return true;
    if(result!=ERROR_SUCCESS) { error=winError(result); return false; } return true;
}
bool writeStartup(const std::wstring& value, bool enabled, std::wstring& error) {
    StartupSnapshot old;
    if(!readStartup(value,old,error)) return false;
    if(old.exists && !startupMatches(old)) { error=L"Startup entry belongs to another path; it was preserved"; return false; }
    StartupSnapshot next;
    if(enabled) {
        auto command=startupCommand();
        if(command.size()>=260) { error=L"Startup command exceeds Windows Run limit"; return false; }
        next.exists=true; next.type=REG_SZ;
        const auto* p=reinterpret_cast<const BYTE*>(command.c_str());
        next.bytes.assign(p,p+(command.size()+1)*sizeof(wchar_t));
    }
    return restoreStartup(value,next,error);
}
}
