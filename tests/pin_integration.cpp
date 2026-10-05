#include "Config.h"
#include "WindowOps.h"
#include <oleacc.h>
#include <dwmapi.h>
#include <iostream>
#include <functional>
#include <stdexcept>

using namespace qp;
static void check(bool ok,const char* label) { if(!ok) throw std::runtime_error(label); }
static void messages() { MSG m{}; while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)) { TranslateMessage(&m); DispatchMessageW(&m); } }
static bool waitFor(const std::function<bool()>& condition,int timeout=2000) {
    auto end=GetTickCount64()+timeout;
    do { messages(); if(condition()) return true; Sleep(10); } while(GetTickCount64()<end);
    return false;
}
static HWND find(DWORD pid,const wchar_t* name) {
    struct Search { DWORD pid; const wchar_t* name; HWND result=nullptr; } s{pid,name};
    EnumWindows([](HWND h,LPARAM p)->BOOL {
        auto& s=*reinterpret_cast<Search*>(p); DWORD pid=0; GetWindowThreadProcessId(h,&pid);
        wchar_t name[128]{}; GetClassNameW(h,name,128);
        if(pid==s.pid && wcscmp(name,s.name)==0) { s.result=h; return FALSE; } return TRUE;
    },reinterpret_cast<LPARAM>(&s)); return s.result;
}
static PROCESS_INFORMATION launch(const std::wstring& exe,const std::wstring& args) {
    auto command=L"\""+exe+L"\" "+args; STARTUPINFOW si{sizeof(si)}; PROCESS_INFORMATION pi{};
    check(CreateProcessW(nullptr,command.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&si,&pi)!=0,"launch isolated Pin app");
    CloseHandle(pi.hThread); return pi;
}
static void activate(HWND h) {
    auto foreground=GetWindowThreadProcessId(GetForegroundWindow(),nullptr);
    bool attached=foreground && foreground!=GetCurrentThreadId() && AttachThreadInput(GetCurrentThreadId(),foreground,TRUE);
    SetForegroundWindow(h); SetFocus(h);
    if(attached) AttachThreadInput(GetCurrentThreadId(),foreground,FALSE);
    check(waitFor([&]{return GetForegroundWindow()==h;}),"test target activation");
}
static void mouse(HWND h,DWORD flags) {
    RECT r{}; GetWindowRect(h,&r); SetCursorPos((r.left+r.right)/2,(r.top+r.bottom)/2);
    POINT point{(r.left+r.right)/2,(r.top+r.bottom)/2};
    check(waitFor([&]{return WindowFromPoint(point)==h;}),"mouse actually hits Pin");
    INPUT i{}; i.type=INPUT_MOUSE; i.mi.dwFlags=flags;
    check(SendInput(1,&i,sizeof(i))==1,"real mouse input");
    messages();
}
static void click(HWND h) { mouse(h,MOUSEEVENTF_LEFTDOWN); waitFor([]{return false;},50); mouse(h,MOUSEEVENTF_LEFTUP); }
static LRESULT CALLBACK targetProc(HWND h,UINT msg,WPARAM w,LPARAM l) { return DefWindowProcW(h,msg,w,l); }
int wmain(int count,wchar_t** args) {
    HANDLE process=nullptr; HWND host=nullptr,target=nullptr,other=nullptr; DWORD pid=0;
    HWND previous=GetForegroundWindow(); POINT cursor{}; GetCursorPos(&cursor);
    auto dir=std::filesystem::temp_directory_path()/(L"QuietPin-pin-test-"+std::to_wstring(GetCurrentProcessId()));
    try {
        check(count==2,"exe argument");
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        std::wstring exe=std::filesystem::absolute(args[1]).wstring(),error;
        Settings settings; settings.status=false; settings.chinese=false; settings.toggle={MOD_CONTROL|MOD_ALT|MOD_SHIFT,VK_F8};
        check(saveSettings(dir/L"settings.ini",settings,error),"seed Pin profile");
        auto options=L"--config-dir \""+dir.wstring()+L"\"";
        WNDCLASSW wc{}; wc.lpfnWndProc=targetProc; wc.hInstance=GetModuleHandleW(nullptr); wc.lpszClassName=L"QuietPin.PinTest.Target";
        check(RegisterClassW(&wc)!=0,"register test windows");
        target=CreateWindowW(wc.lpszClassName,L"QuietPin Pin test target",WS_OVERLAPPEDWINDOW,160,160,640,300,nullptr,nullptr,wc.hInstance,nullptr);
        other=CreateWindowW(wc.lpszClassName,L"QuietPin other test target",WS_OVERLAPPEDWINDOW,240,260,560,280,nullptr,nullptr,wc.hInstance,nullptr);
        check(target && other,"create test windows"); ShowWindow(other,SW_SHOW); ShowWindow(target,SW_SHOW);
        activate(target);
        auto app=launch(exe,options); process=app.hProcess; pid=app.dwProcessId;
        check(waitFor([&]{host=find(pid,L"QuietPin.Host.v1"); return host!=nullptr;}),"host ready");
        check(find(pid,L"QuietPin.Pin.v1")==nullptr,"Pin disabled by default");
        auto settingsClient=launch(exe,options+L" --settings");
        check(WaitForSingleObject(settingsClient.hProcess,3000)==WAIT_OBJECT_0,"settings client"); CloseHandle(settingsClient.hProcess);
        HWND ui=nullptr;
        check(waitFor([&]{ui=find(pid,L"QuietPin.Settings.v1"); return ui && IsWindowVisible(ui);}),"settings ready");
        SendMessageW(GetDlgItem(ui,115),BM_SETCHECK,BST_CHECKED,0); SendMessageW(ui,WM_COMMAND,110,0);
        Settings saved;
        check(loadSettings(dir/L"settings.ini",saved,error) && saved.pin,"Pin setting persists");
        HWND pin=nullptr;
        check(waitFor([&]{pin=find(pid,L"QuietPin.Pin.v1");return pin!=nullptr;}),"Pin created");
        check(!IsWindowVisible(pin),"Pin hidden during Settings");
        SendMessageW(ui,WM_CLOSE,0,0); activate(target);
        check(waitFor([&]{return IsWindowVisible(pin);}),"Pin visible for active target");
        check((GetWindowLongPtrW(pin,GWL_EXSTYLE)&(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE))==(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE),"Pin nonactivation styles");
        click(pin);
        if(!waitFor([&]{return isTopmost(target);})) {
            wchar_t label[256]{}; GetWindowTextW(pin,label,256); std::wcerr<<L"Pin label: "<<label<<L" foreground="<<GetForegroundWindow()<<L" target="<<target<<L"\n";
            throw std::runtime_error("Pin true mouse pins");
        }
        check(GetForegroundWindow()==target && GetFocus()==target,"Pin keeps foreground and keyboard focus");
        waitFor([]{return false;},120);
        click(pin); check(waitFor([&]{return !isTopmost(target);}),"Pin true mouse unpins");
        check(GetForegroundWindow()==target && GetFocus()==target,"unpin keeps focus");
        mouse(pin,MOUSEEVENTF_LEFTDOWN); waitFor([]{return false;},40); activate(other);
        waitFor([]{return false;},80); mouse(pin,MOUSEEVENTF_LEFTUP);
        waitFor([]{return false;},80); check(!isTopmost(target) && !isTopmost(other),"target switch cancels down/up gesture");
        activate(target); check(waitFor([&]{return IsWindowVisible(pin);}),"target rebinds");
        // Disabling Pin while Tray remains enabled must retain foreground subscription.
        auto reopen=launch(exe,options+L" --settings"); WaitForSingleObject(reopen.hProcess,3000); CloseHandle(reopen.hProcess);
        check(waitFor([&]{ui=find(pid,L"QuietPin.Settings.v1");return ui && IsWindowVisible(ui);}),"reopen settings");
        SendMessageW(GetDlgItem(ui,106),BM_SETCHECK,BST_CHECKED,0); SendMessageW(ui,WM_COMMAND,110,0);
        SendMessageW(GetDlgItem(ui,115),BM_SETCHECK,BST_UNCHECKED,0); SendMessageW(ui,WM_COMMAND,110,0);
        check(waitFor([&]{return find(pid,L"QuietPin.Pin.v1")==nullptr;}),"Pin disabled destroys window");
        SendMessageW(GetDlgItem(ui,115),BM_SETCHECK,BST_CHECKED,0); SendMessageW(GetDlgItem(ui,106),BM_SETCHECK,BST_UNCHECKED,0);
        SendMessageW(ui,WM_COMMAND,110,0); SendMessageW(ui,WM_CLOSE,0,0); activate(target);
        check(waitFor([&]{pin=find(pid,L"QuietPin.Pin.v1");return pin && IsWindowVisible(pin);}),"Pin works after Tray disabled");
        click(pin); check(waitFor([&]{return isTopmost(target);}),"Pin after feature switches");
        DestroyWindow(target); target=nullptr;
        activate(other); check(waitFor([&]{return IsWindowVisible(pin);}),"target close safely switches");
        auto stop=launch(exe,options+L" --exit"); WaitForSingleObject(stop.hProcess,3000); CloseHandle(stop.hProcess);
        check(waitFor([&]{return WaitForSingleObject(process,0)==WAIT_OBJECT_0;},3000),"no-tray exit");
        CloseHandle(process); process=nullptr;
        DestroyWindow(other); other=nullptr;
        std::filesystem::remove_all(dir);
        SetCursorPos(cursor.x,cursor.y); if(previous && IsWindow(previous)) SetForegroundWindow(previous);
        std::cout<<"PASS: default off, settings enable, real mouse pin/unpin, keyboard focus, target-switch gesture cancellation, Pin/Tray lifecycle, target destruction and no-tray exit\n";
        return 0;
    } catch(const std::exception& e) {
        INPUT up{}; up.type=INPUT_MOUSE; up.mi.dwFlags=MOUSEEVENTF_LEFTUP; SendInput(1,&up,sizeof(up));
        if(host) PostMessageW(host,WM_APP+1,2,0);
        if(process) { if(!waitFor([&]{return WaitForSingleObject(process,0)==WAIT_OBJECT_0;},3000)) TerminateProcess(process,1); CloseHandle(process); }
        if(target) DestroyWindow(target); if(other) DestroyWindow(other);
        SetCursorPos(cursor.x,cursor.y); if(previous && IsWindow(previous)) SetForegroundWindow(previous);
        std::cerr<<"FAIL: "<<e.what()<<"\n"; return 1;
    }
}
