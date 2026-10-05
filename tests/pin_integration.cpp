#include "Config.h"
#include "WindowOps.h"
#include <oleacc.h>
#include <dwmapi.h>
#include <psapi.h>
#include <iostream>
#include <functional>
#include <stdexcept>
#include <algorithm>

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
    check(waitFor([&]{RECT r{}; GetWindowRect(h,&r); POINT point{(r.left+r.right)/2,(r.top+r.bottom)/2};
        SetCursorPos(point.x,point.y);return IsWindowVisible(h) && WindowFromPoint(point)==h;}),"mouse actually hits Pin");
    INPUT i{}; i.type=INPUT_MOUSE; i.mi.dwFlags=flags;
    check(SendInput(1,&i,sizeof(i))==1,"real mouse input");
    messages();
}
static void click(HWND h) {
    // Wait out DWM's activation/restore animation before a stationary gesture.
    waitFor([]{return false;},300);
    mouse(h,MOUSEEVENTF_LEFTDOWN); waitFor([]{return false;},50); mouse(h,MOUSEEVENTF_LEFTUP);
}
static LRESULT CALLBACK targetProc(HWND h,UINT msg,WPARAM w,LPARAM l) { return DefWindowProcW(h,msg,w,l); }
int wmain(int count,wchar_t** args) {
    HANDLE process=nullptr; HWND host=nullptr,target=nullptr,other=nullptr; DWORD pid=0;
    HWND previous=GetForegroundWindow(); POINT cursor{}; GetCursorPos(&cursor);
    auto dir=std::filesystem::temp_directory_path()/(L"QuietPin-pin-test-"+std::to_wstring(GetCurrentProcessId()));
    try {
        check(count==2 || count==3,"exe argument");
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
        SendMessageW(GetDlgItem(ui,115),BM_SETCHECK,BST_CHECKED,0); SendMessageW(GetDlgItem(ui,110),BM_CLICK,0,0);
        Settings saved;
        check(loadSettings(dir/L"settings.ini",saved,error) && saved.pin,"Pin setting persists");
        HWND pin=nullptr;
        check(waitFor([&]{pin=find(pid,L"QuietPin.Pin.v1");return pin!=nullptr;}),"Pin created");
        check(!IsWindowVisible(pin),"Pin hidden during Settings");
        SendMessageW(ui,WM_CLOSE,0,0); activate(target);
        check(waitFor([&]{return IsWindowVisible(pin);}),"Pin visible for active target");
        waitFor([]{return false;},250);
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
        check(waitFor([&]{wchar_t label[128]{}; GetWindowTextW(pin,label,128); return wcscmp(label,L"Pin this window")==0;}),"Pin confirmation completes before accessibility query");
        constexpr IID accessibleId{0x618736e0,0x3c3d,0x11cf,{0x81,0x0c,0x00,0xaa,0x00,0x38,0x9b,0x71}};
        IAccessible* accessible=nullptr;
        check(SUCCEEDED(AccessibleObjectFromWindow(pin,OBJID_CLIENT,accessibleId,reinterpret_cast<void**>(&accessible))),"MSAA object available");
        VARIANT self{}; self.vt=VT_I4; self.lVal=CHILDID_SELF; BSTR name=nullptr;
        auto named=accessible->get_accName(self,&name);
        bool correct=SUCCEEDED(named) && name && wcscmp(name,L"Pin this window")==0;
        if(!correct) std::wcerr<<L"MSAA name="<<(name?name:L"<null>")<<L" HRESULT="<<named<<L"\n";
        SysFreeString(name); accessible->Release(); check(correct,"MSAA action name matches real unpinned state");
        RECT before{}; GetWindowRect(pin,&before);
        SetWindowPos(target,nullptr,220,210,720,340,SWP_NOZORDER|SWP_NOACTIVATE);
        check(waitFor([&]{RECT r{};GetWindowRect(pin,&r);return IsWindowVisible(pin) && r.left!=before.left && r.top!=before.top;}),"Pin follows move and resize events");
        ShowWindow(target,SW_MINIMIZE);
        check(waitFor([&]{return !IsWindowVisible(pin) || GetForegroundWindow()!=target;}),"minimized target no stale Pin");
        ShowWindow(target,SW_RESTORE); activate(target);
        check(waitFor([&]{return IsWindowVisible(pin);}),"restored target rebinds");
        ShowWindow(target,SW_MAXIMIZE);
        check(waitFor([&]{RECT r{};GetWindowRect(pin,&r);return !IsWindowVisible(pin) || r.top!=before.top;}),"maximized geometry updates or safely hides");
        ShowWindow(target,SW_RESTORE); activate(target);
        check(waitFor([&]{return IsWindowVisible(pin);}),"restore after maximize");
        auto style=GetWindowLongPtrW(target,GWL_STYLE);
        MONITORINFO monitor{sizeof(monitor),{},{},0}; GetMonitorInfoW(MonitorFromWindow(target,MONITOR_DEFAULTTONEAREST),&monitor);
        SetWindowLongPtrW(target,GWL_STYLE,WS_POPUP|WS_VISIBLE);
        SetWindowPos(target,nullptr,monitor.rcMonitor.left,monitor.rcMonitor.top,monitor.rcMonitor.right-monitor.rcMonitor.left,
            monitor.rcMonitor.bottom-monitor.rcMonitor.top,SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
        check(waitFor([&]{return !IsWindowVisible(pin);}),"borderless fullscreen hides Pin");
        SetWindowLongPtrW(target,GWL_STYLE,style); SetWindowPos(target,nullptr,220,210,720,340,SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
        activate(target); check(waitFor([&]{return IsWindowVisible(pin);}),"return from fullscreen");
        RECT clientRect{}; GetWindowRect(target,&clientRect);
        SetCursorPos(clientRect.left+100,clientRect.top+120);
        waitFor([]{return false;},2000);
        FILETIME created{},exited{},kernel{},user{}; GetProcessTimes(process,&created,&exited,&kernel,&user);
        auto ticks=[](FILETIME t){return (static_cast<ULONGLONG>(t.dwHighDateTime)<<32)|t.dwLowDateTime;};
        auto cpu=ticks(kernel)+ticks(user); DWORD handles=0; GetProcessHandleCount(process,&handles);
        PROCESS_MEMORY_COUNTERS_EX memory{}; memory.cb=sizeof(memory); GetProcessMemoryInfo(process,reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),sizeof(memory));
        int sample=count==3?60000:1000;
        waitFor([]{return false;},sample);
        GetProcessTimes(process,&created,&exited,&kernel,&user);
        std::cout<<"PIN_IDLE: seconds="<<sample/1000<<" CPU_seconds="<<(ticks(kernel)+ticks(user)-cpu)/10000000.0
            <<" private_bytes="<<memory.PrivateUsage<<" working_set="<<memory.WorkingSetSize<<" handles="<<handles
            <<" GDI="<<GetGuiResources(process,GR_GDIOBJECTS)<<" USER="<<GetGuiResources(process,GR_USEROBJECTS)<<"\n";
        mouse(pin,MOUSEEVENTF_LEFTDOWN); waitFor([]{return false;},40); activate(other);
        waitFor([]{return false;},80);
        INPUT release{}; release.type=INPUT_MOUSE; release.mi.dwFlags=MOUSEEVENTF_LEFTUP;
        check(SendInput(1,&release,sizeof(release))==1,"release cancelled gesture after target switch");
        waitFor([]{return false;},80); check(!isTopmost(target) && !isTopmost(other),"target switch cancels down/up gesture");
        activate(target); check(waitFor([&]{return IsWindowVisible(pin);}),"target rebinds");
        // Disabling Pin while Tray remains enabled must retain foreground subscription.
        auto reopen=launch(exe,options+L" --settings"); WaitForSingleObject(reopen.hProcess,3000); CloseHandle(reopen.hProcess);
        check(waitFor([&]{ui=find(pid,L"QuietPin.Settings.v1");return ui && IsWindowVisible(ui);}),"reopen settings");
        SendMessageW(GetDlgItem(ui,116),WM_SETTEXT,0,reinterpret_cast<LPARAM>(L"-20")); SendMessageW(GetDlgItem(ui,117),WM_SETTEXT,0,reinterpret_cast<LPARAM>(L"-8"));
        SendMessageW(GetDlgItem(ui,110),BM_CLICK,0,0);
        if(!waitFor([&]{return loadSettings(dir/L"settings.ini",saved,error) && saved.pinOffsetX==-20 && saved.pinOffsetY==-8;})) {
            wchar_t result[512]{},x[32]{},y[32]{}; GetWindowTextW(GetDlgItem(ui,114),result,512);
            GetWindowTextW(GetDlgItem(ui,116),x,32); GetWindowTextW(GetDlgItem(ui,117),y,32);
            std::wcerr<<L"Offset save: "<<result<<L" x="<<x<<L" y="<<y<<L"\n";
            throw std::runtime_error("UI offsets persist");
        }
        SendMessageW(ui,WM_COMMAND,118,0); SendMessageW(GetDlgItem(ui,110),BM_CLICK,0,0);
        check(loadSettings(dir/L"settings.ini",saved,error) && saved.pinOffsetX==0 && saved.pinOffsetY==0,"UI reset offsets");
        SendMessageW(GetDlgItem(ui,109),WM_SETTEXT,0,reinterpret_cast<LPARAM>(L"quietpin_pin_integration.exe")); SendMessageW(GetDlgItem(ui,110),BM_CLICK,0,0);
        SendMessageW(ui,WM_CLOSE,0,0); activate(target);
        check(waitFor([&]{return !IsWindowVisible(pin);}),"excluded executable hides Pin immediately");
        auto exclusions=launch(exe,options+L" --settings"); WaitForSingleObject(exclusions.hProcess,3000); CloseHandle(exclusions.hProcess);
        check(waitFor([&]{ui=find(pid,L"QuietPin.Settings.v1");return ui && IsWindowVisible(ui);}),"exclusions settings");
        SendMessageW(GetDlgItem(ui,109),WM_SETTEXT,0,reinterpret_cast<LPARAM>(L"")); SendMessageW(GetDlgItem(ui,110),BM_CLICK,0,0);
        // Keep the configuration fixture alive but remove its repaint/hover churn
        // from the measurement of Pin's repeated allocation and teardown.
        ShowWindow(ui,SW_HIDE);
        waitFor([]{return false;},150);
        DWORD initialGdi=0,initialUser=0,initialHandles=0;
        auto resourceFloor=[&](DWORD kind) {
            DWORD minimum=MAXDWORD;
            waitFor([&]{minimum=std::min(minimum,GetGuiResources(process,kind));return false;},800);
            return minimum;
        };
        // Warm themed controls before comparing the following 100 full cycles.
        for(int i=0;i<110;++i) {
            if(i==10) {
                waitFor([]{return false;},500);
                initialGdi=resourceFloor(GR_GDIOBJECTS); initialUser=resourceFloor(GR_USEROBJECTS);
                GetProcessHandleCount(process,&initialHandles);
            }
            SendMessageW(GetDlgItem(ui,115),BM_SETCHECK,BST_UNCHECKED,0); SendMessageW(GetDlgItem(ui,110),BM_CLICK,0,0);
            check(find(pid,L"QuietPin.Pin.v1")==nullptr,"100-cycle disable destroys overlay");
            SendMessageW(GetDlgItem(ui,115),BM_SETCHECK,BST_CHECKED,0); SendMessageW(GetDlgItem(ui,110),BM_CLICK,0,0);
            check(find(pid,L"QuietPin.Pin.v1")!=nullptr,"100-cycle enable creates overlay");
        }
        waitFor([]{return false;},500);
        DWORD finalHandles=0; GetProcessHandleCount(process,&finalHandles);
        auto finalGdi=resourceFloor(GR_GDIOBJECTS),finalUser=resourceFloor(GR_USEROBJECTS);
        std::cout<<"PIN_100_CYCLES: handles="<<initialHandles<<" -> "<<finalHandles<<" GDI="<<initialGdi<<" -> "<<finalGdi
            <<" USER="<<initialUser<<" -> "<<finalUser<<"\n";
        check(finalHandles<=initialHandles+3 && finalGdi<=initialGdi+2 && finalUser<=initialUser+2,"no linear GUI or handle leak across 100 cycles");
        ShowWindow(ui,SW_SHOWNOACTIVATE);
        SendMessageW(GetDlgItem(ui,106),BM_SETCHECK,BST_CHECKED,0); SendMessageW(GetDlgItem(ui,110),BM_CLICK,0,0);
        SendMessageW(GetDlgItem(ui,115),BM_SETCHECK,BST_UNCHECKED,0); SendMessageW(GetDlgItem(ui,110),BM_CLICK,0,0);
        check(waitFor([&]{return find(pid,L"QuietPin.Pin.v1")==nullptr;}),"Pin disabled destroys window");
        SendMessageW(GetDlgItem(ui,115),BM_SETCHECK,BST_CHECKED,0); SendMessageW(GetDlgItem(ui,106),BM_SETCHECK,BST_UNCHECKED,0);
        SendMessageW(GetDlgItem(ui,110),BM_CLICK,0,0); SendMessageW(ui,WM_CLOSE,0,0); activate(target);
        check(waitFor([&]{pin=find(pid,L"QuietPin.Pin.v1");return pin && IsWindowVisible(pin);}),"Pin works after Tray disabled");
        click(pin);
        if(!waitFor([&]{return isTopmost(target);})) {
            wchar_t label[128]{}; GetWindowTextW(pin,label,128);
            std::wcerr<<L"Feature-switch Pin="<<label<<L" foreground="<<GetForegroundWindow()<<L" target="<<target<<L"\n";
            throw std::runtime_error("Pin after feature switches");
        }
        DestroyWindow(target); target=nullptr;
        activate(other); check(waitFor([&]{return IsWindowVisible(pin);}),"target close safely switches");
        auto copy=dir/L"another-path.exe"; std::filesystem::copy_file(exe,copy,std::filesystem::copy_options::overwrite_existing);
        auto foreignExit=launch(copy.wstring(),options+L" --exit-if-owned");
        check(WaitForSingleObject(foreignExit.hProcess,3000)==WAIT_OBJECT_0 && WaitForSingleObject(process,0)==WAIT_TIMEOUT,"owned exit preserves instance at another path");
        CloseHandle(foreignExit.hProcess);
        auto stop=launch(exe,options+L" --exit-if-owned"); check(WaitForSingleObject(stop.hProcess,6000)==WAIT_OBJECT_0,"owned exit waits for cleanup"); CloseHandle(stop.hProcess);
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



