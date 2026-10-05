#include "WindowOps.h"
#include <dwmapi.h>
#include <array>
#include <algorithm>

namespace qp {
static bool integrity(HANDLE process, DWORD& level) {
    HANDLE token=nullptr;
    if(!OpenProcessToken(process,TOKEN_QUERY,&token)) return false;
    DWORD size=0; GetTokenInformation(token,TokenIntegrityLevel,nullptr,0,&size);
    std::vector<BYTE> data(size);
    bool ok=size && GetTokenInformation(token,TokenIntegrityLevel,data.data(),size,&size);
    if(ok) {
        auto* info=reinterpret_cast<TOKEN_MANDATORY_LABEL*>(data.data());
        auto count=*GetSidSubAuthorityCount(info->Label.Sid);
        if(!count) ok=false;
        else level=*GetSidSubAuthority(info->Label.Sid,count-1);
    }
    CloseHandle(token); return ok;
}
CheckedWindow inspectWindow(HWND hwnd,const Settings& s) {
    if(!hwnd || !IsWindow(hwnd)) return {};
    if(hwnd==GetDesktopWindow() || hwnd==GetShellWindow()) return {{},WindowError::System};
    hwnd=GetAncestor(hwnd,GA_ROOT);
    if(!hwnd) return {};
    if(hwnd==GetDesktopWindow() || hwnd==GetShellWindow()) return {{},WindowError::System};
    wchar_t cls[256]{}; GetClassNameW(hwnd,cls,256);
    constexpr std::array<const wchar_t*,10> blocked{L"Progman",L"WorkerW",L"Shell_TrayWnd",L"Shell_SecondaryTrayWnd",
        L"NotifyIconOverflowWindow",L"MultitaskingViewFrame",L"#32768",L"tooltips_class32",L"DV2ControlHost",L"XamlExplorerHostIslandWindow"};
    for(auto name:blocked) if(_wcsicmp(cls,name)==0) return {{},WindowError::System};
    if(!IsWindowVisible(hwnd) || IsIconic(hwnd)) return {};
    if(GetWindowLongPtrW(hwnd,GWL_STYLE)&WS_CHILD) return {{},WindowError::System};
    DWORD cloaked=0;
    if(SUCCEEDED(DwmGetWindowAttribute(hwnd,DWMWA_CLOAKED,&cloaked,sizeof(cloaked))) && cloaked) return {};
    Identity id; id.hwnd=hwnd; id.tid=GetWindowThreadProcessId(hwnd,&id.pid);
    if(!id.tid) return {};
    if(id.pid==GetCurrentProcessId()) return {{},WindowError::Own};
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,id.pid);
    if(!process) return {{},WindowError::Restricted};
    DWORD ours=0,theirs=0; FILETIME exit{},kernel{},user{};
    DWORD length=32768; std::wstring path(length,L'\0');
    bool ok=QueryFullProcessImageNameW(process,0,path.data(),&length) &&
            GetProcessTimes(process,&id.created,&exit,&kernel,&user) &&
            integrity(GetCurrentProcess(),ours) && integrity(process,theirs);
    CloseHandle(process);
    if(!ok || theirs>ours) return {{},WindowError::Restricted};
    path.resize(length); id.path=std::move(path);
    if(isExcluded(id.path,s.excluded)) return {{},WindowError::Excluded};
    if(!sameWindow(id)) return {};
    return {id,WindowError::None};
}
bool sameWindow(const Identity& id) {
    DWORD pid=0;
    if(!IsWindow(id.hwnd) || GetWindowThreadProcessId(id.hwnd,&pid)!=id.tid || pid!=id.pid) return false;
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
    if(!process) return false;
    FILETIME created{},exit{},kernel{},user{};
    bool ok=GetProcessTimes(process,&created,&exit,&kernel,&user) && CompareFileTime(&created,&id.created)==0;
    CloseHandle(process); return ok;
}
bool isTopmost(HWND hwnd) { return (GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TOPMOST)!=0; }
bool requestTopmost(const Identity& id,bool topmost,DWORD& error) {
    if(!sameWindow(id)) { error=ERROR_INVALID_WINDOW_HANDLE; return false; }
    if(!SetWindowPos(id.hwnd,topmost?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,
                     SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_ASYNCWINDOWPOS)) {
        error=GetLastError(); return false;
    }
    error=0; return true;
}
std::wstring windowErrorText(WindowError e,bool zh) {
    switch(e) {
    case WindowError::System: return zh?L"不能操作桌面、任务栏或系统窗口。":L"Desktop, taskbar and system windows are protected.";
    case WindowError::Own: return zh?L"请选择其他应用的窗口。":L"Select a window from another application.";
    case WindowError::Excluded: return zh?L"此程序已被排除。":L"This application is excluded.";
    case WindowError::Restricted: return zh?L"此窗口权限较高或受保护，当前无法操作。":L"This window has higher privileges or is protected.";
    default: return zh?L"没有可操作的活动窗口，或窗口已关闭。":L"No available active window, or the window has closed.";
    }
}
}
