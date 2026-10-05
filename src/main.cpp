#include "Config.h"
#include "WindowOps.h"
#include "PinController.h"
#include "UpdateChecker.h"
#include <wtsapi32.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <algorithm>
#include <memory>
#include <sstream>
#include <optional>

using namespace qp;
namespace {
constexpr wchar_t HostClass[]=L"QuietPin.Host.v1";
constexpr wchar_t SettingsClass[]=L"QuietPin.Settings.v1";
constexpr wchar_t StatusClass[]=L"QuietPin.Status.v1";
constexpr UINT ManageMessage=WM_APP+1, TrayMessage=WM_APP+2, ForegroundMessage=WM_APP+3, PinMessage=WM_APP+4, UpdateMessage=WM_APP+5;
constexpr UINT_PTR ConfirmTimer=1, HideTimer=2, QuitTimer=3;
constexpr int SettingsKeyId=2, ExitKeyId=3;
constexpr Hotkey SettingsHotkey{MOD_CONTROL|MOD_ALT|MOD_SHIFT,'T'};
constexpr Hotkey ExitHotkey{MOD_CONTROL|MOD_ALT|MOD_SHIFT,'Q'};
enum Control { Ctrl=100,Alt,Shift,Win,Key,ShowStatus,ShowTray,Startup,Language,Exclusions,Save,Close,Quit,Browse,Result,ShowPin,PinX,PinY,PinReset,CheckUpdates,Download };

// Stable per-config identity; default remains one instance per user session.
std::wstring profileTag(const std::filesystem::path& dir) {
    unsigned long long hash=14695981039346656037ull;
    for(wchar_t c:lower(std::filesystem::absolute(dir).lexically_normal().wstring())) { hash^=static_cast<unsigned short>(c); hash*=1099511628211ull; }
    return std::to_wstring(hash);
}
std::wstring userSid() {
    HANDLE token=nullptr; DWORD size=0;
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token)) throw std::runtime_error("Cannot query user token");
    GetTokenInformation(token,TokenUser,nullptr,0,&size); std::vector<BYTE> data(size);
    if(!GetTokenInformation(token,TokenUser,data.data(),size,&size)) { CloseHandle(token); throw std::runtime_error("Cannot query user SID"); }
    const auto sid=reinterpret_cast<TOKEN_USER*>(data.data())->User.Sid;
    std::wstring result=std::to_wstring(*GetSidSubAuthorityCount(sid));
    for(BYTE i=0;i<*GetSidSubAuthorityCount(sid);++i) result+=L"."+std::to_wstring(*GetSidSubAuthority(sid,i));
    CloseHandle(token); return result;
}
std::wstring textOf(HWND control) {
    int n=GetWindowTextLengthW(control); std::wstring text(static_cast<size_t>(n)+1,L'\0');
    GetWindowTextW(control,text.data(),n+1); text.resize(n); return text;
}
class App {
public:
    Settings settings;
    std::filesystem::path directory,file;
    std::wstring title,startupValue,marker,lastResult;
    HWND host=nullptr,ui=nullptr,statusWindow=nullptr;
    HFONT uiFont=nullptr,statusFont=nullptr;
    HICON icon=nullptr;
    HANDLE mutex=nullptr;
    WindowEventMonitor monitor;
    UpdateChecker updater;
    std::unique_ptr<PinController> pin;
    std::optional<Identity> recent;
    std::vector<Identity> managed;
    struct Pending { Identity window; bool wanted; ULONGLONG started; };
    std::optional<Pending> pending;
    bool trayAdded=false,exiting=false,settingsRegistered=false,exitRegistered=false;
    int toggleId=10;
    bool toggleRegistered=false;
    bool allowConfigSave=true;
    UINT taskbarCreated=0;
    UINT uiDpi=96;
    ULONGLONG quitDeadline=0;

    App(std::filesystem::path dir):directory(std::move(dir)),file(directory/L"settings.ini") {
        const auto tag=profileTag(directory);
        title=L"QuietPin."+userSid()+L"."+tag;
        startupValue=directory==defaultConfigDirectory()?L"QuietPin":L"QuietPin."+tag;
        marker=L"QuietPin.Managed."+std::to_wstring(GetCurrentProcessId());
    }
    ~App() {
        updater.stop();
        pin.reset(); monitor.stop();
        if(trayAdded) removeTray();
        if(statusWindow && IsWindow(statusWindow)) DestroyWindow(statusWindow);
        if(ui && IsWindow(ui)) DestroyWindow(ui);
        if(host && IsWindow(host)) DestroyWindow(host);
        if(uiFont) DeleteObject(uiFont);
        if(statusFont) DeleteObject(statusFont);
        if(icon) DestroyIcon(icon);
        if(mutex) CloseHandle(mutex);
    }
    const wchar_t* tr(const wchar_t* zh,const wchar_t* en) const { return settings.chinese?zh:en; }
    void result(std::wstring message, bool failure=false, HWND target=nullptr) {
        lastResult=std::move(message);
        if(ui) SetWindowTextW(GetDlgItem(ui,Result),lastResult.c_str());
        if(settings.status && !exiting) showStatus(failure,target);
        if(pin && !exiting) monitor.refresh();
    }
    bool initialize() {
        std::wstring error;
        const bool configOk=loadSettings(file,settings,error);
        if(!configOk) {
            const auto backup=std::filesystem::path(file.wstring()+L".corrupt."+std::to_wstring(GetTickCount64())+L".bak");
            std::error_code ec; const auto length=std::filesystem::file_size(file,ec);
            allowConfigSave=!ec && length<=65536 && CopyFileW(file.c_str(),backup.c_str(),TRUE);
            lastResult=(allowConfigSave?
                tr(L"配置无法读取；已保存 .bak 备份。保存将恢复当前设置。 ",L"Cannot read configuration; a .bak copy was saved. Save restores current settings. "):
                tr(L"配置无法读取且备份失败，当前禁止覆盖；请手动备份或移走配置文件。 ",L"Cannot read or back up configuration. Move or back up the file before saving. "))+error;
        }
        else {
            StartupSnapshot snapshot;
            if(readStartup(startupValue,snapshot,error) && settings.startup!=startupMatches(snapshot))
                lastResult=tr(L"开机启动配置与系统状态不同，请在设置中核对。",L"Startup setting differs from Windows. Review it in Settings.");
        }
        host=CreateWindowExW(WS_EX_TOOLWINDOW,HostClass,title.c_str(),WS_POPUP,0,0,0,0,nullptr,nullptr,GetModuleHandleW(nullptr),this);
        if(!host) return false;
        taskbarCreated=RegisterWindowMessageW(L"TaskbarCreated");
        icon=CopyIcon(LoadIconW(nullptr,IDI_APPLICATION));
        toggleRegistered=RegisterHotKey(host,toggleId,settings.toggle.modifiers|MOD_NOREPEAT,settings.toggle.key)!=0;
        settingsRegistered=RegisterHotKey(host,SettingsKeyId,SettingsHotkey.modifiers|MOD_NOREPEAT,SettingsHotkey.key)!=0;
        exitRegistered=RegisterHotKey(host,ExitKeyId,ExitHotkey.modifiers|MOD_NOREPEAT,ExitHotkey.key)!=0;
        if(!toggleRegistered || !settingsRegistered || !exitRegistered) {
            if(!lastResult.empty()) lastResult+=L"\r\n";
            lastResult+=tr(L"快捷键被占用；请修改置顶快捷键。可再次运行 EXE 或使用 --settings / --exit 管理。",
                           L"A shortcut is unavailable. Change the toggle shortcut; rerun the EXE or use --settings / --exit.");
        }
        applyTray();
        pin=std::make_unique<PinController>(host,PinMessage,monitor,settings,[this](HWND target){toggle(target);},[this]{return pending.has_value() || exiting;});
        if(!pin->apply()) result(pin->error(),true);
        if(!configOk || !toggleRegistered || !settingsRegistered || !exitRegistered) openSettings();
        return true;
    }
    void rememberForeground(HWND window) {
        if(exiting || !settings.tray) return;
        auto checked=inspectWindow(window,settings);
        if(checked.window) recent=std::move(checked.window);
        else {
            // Shell/menu transitions retain the last real external target; real disallowed apps clear it.
            if(checked.error==WindowError::Excluded || checked.error==WindowError::Restricted ||
               (checked.error==WindowError::Own && window!=host) || window==GetShellWindow() || window==GetDesktopWindow()) recent.reset();
        }
    }
    void toggle(HWND target) {
        if(exiting) return;
        if(pending) { result(tr(L"上一次窗口操作尚未完成。",L"The previous window operation is still pending."),true,target); return; }
        auto checked=inspectWindow(target,settings);
        if(!checked.window) { result(windowErrorText(checked.error,settings.chinese),true,target); return; }
        Identity window=*checked.window;
        bool wanted=!isTopmost(window.hwnd); DWORD error=0;
        if(!requestTopmost(window,wanted,error)) { result(tr(L"无法操作窗口：",L"Cannot change window: ")+winError(error),true,window.hwnd); return; }
        if(wanted && SetPropW(window.hwnd,marker.c_str(),reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(GetCurrentProcessId())))) {
            managed.erase(std::remove_if(managed.begin(),managed.end(),[&](const auto& entry){ return entry.hwnd==window.hwnd; }),managed.end());
            managed.push_back(window);
        }
        pending=Pending{window,wanted,GetTickCount64()};
        SetTimer(host,ConfirmTimer,30,nullptr);
        if(pin) monitor.refresh();
    }
    void confirm() {
        if(!pending) { KillTimer(host,ConfirmTimer); return; }
        auto p=*pending;
        if(!sameWindow(p.window)) {
            pending.reset(); KillTimer(host,ConfirmTimer);
            result(windowErrorText(WindowError::Unavailable,settings.chinese),true); return;
        }
        if(isTopmost(p.window.hwnd)==p.wanted) {
            pending.reset(); KillTimer(host,ConfirmTimer);
            managed.erase(std::remove_if(managed.begin(),managed.end(),[&](const auto& w){ return !sameWindow(w) || w.hwnd==p.window.hwnd; }),managed.end());
            if(p.wanted) {
                if(SetPropW(p.window.hwnd,marker.c_str(),reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(GetCurrentProcessId()))))
                    managed.push_back(p.window);
            } else RemovePropW(p.window.hwnd,marker.c_str());
            result(p.wanted?tr(L"已置顶",L"Window pinned"):tr(L"已取消置顶",L"Window unpinned"),false,p.window.hwnd);
        } else if(GetTickCount64()-p.started>=500) {
            pending.reset(); KillTimer(host,ConfirmTimer);
            result(tr(L"窗口未响应或不支持此次操作。",L"Window did not respond or does not support this operation."),true,p.window.hwnd);
        }
    }
    void removeTray() {
        NOTIFYICONDATAW data{}; data.cbSize=sizeof(data); data.hWnd=host; data.uID=1;
        Shell_NotifyIconW(NIM_DELETE,&data); trayAdded=false;
    }
    void applyTray() {
        monitor.observeForeground(host,ForegroundMessage,!exiting && (settings.tray || settings.pin));
        if(settings.tray) {
            if(monitor.foregroundReady()) {
                rememberForeground(GetForegroundWindow());
            }
            if(!trayAdded) {
                NOTIFYICONDATAW data{}; data.cbSize=sizeof(data); data.hWnd=host; data.uID=1;
                data.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP; data.hIcon=icon; data.uCallbackMessage=TrayMessage;
                wcscpy_s(data.szTip,L"QuietPin");
                trayAdded=Shell_NotifyIconW(NIM_ADD,&data)!=0;
                if(trayAdded) { data.uVersion=NOTIFYICON_VERSION_4; Shell_NotifyIconW(NIM_SETVERSION,&data); }
                else result(tr(L"托盘图标无法显示，可使用快捷键或再次运行 EXE 打开设置。",L"Tray icon unavailable. Use the shortcut or rerun the EXE for Settings."),true);
            }
            if(!monitor.foregroundReady()) result(tr(L"无法跟踪托盘目标；托盘置顶项不可用。",L"Cannot track tray target; tray pin command is unavailable."),true);
        } else {
            if(trayAdded) removeTray();
            recent.reset();
        }
    }
    void trayMenu(POINT point) {
        auto target=recent;
        if(target && (!monitor.foregroundReady() || !sameWindow(*target) || !inspectWindow(target->hwnd,settings).window)) target.reset();
        if(pin) pin->suspend(true);
        HMENU menu=CreatePopupMenu();
        AppendMenuW(menu,MF_STRING|(target?0:MF_GRAYED),1,target && isTopmost(target->hwnd)?tr(L"取消当前窗口置顶",L"Unpin current window"):tr(L"置顶当前窗口",L"Pin current window"));
        AppendMenuW(menu,MF_STRING,2,tr(L"设置",L"Settings"));
        AppendMenuW(menu,MF_SEPARATOR,0,nullptr);
        AppendMenuW(menu,MF_STRING,3,tr(L"退出",L"Exit"));
        SetForegroundWindow(host);
        UINT command=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY|TPM_RIGHTBUTTON,point.x,point.y,0,host,nullptr);
        DestroyMenu(menu); PostMessageW(host,WM_NULL,0,0);
        if(pin) pin->suspend(false);
        if(command==1 && target) toggle(target->hwnd);
        else if(command==2) openSettings();
        else if(command==3) beginExit();
    }
    void showStatus(bool failure,HWND target) {
        if(!statusWindow) statusWindow=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_LAYERED|WS_EX_TRANSPARENT,
            StatusClass,L"QuietPin status",WS_POPUP,0,0,0,0,host,nullptr,GetModuleHandleW(nullptr),this);
        if(!statusWindow) return;
        SetLayeredWindowAttributes(statusWindow,0,245,LWA_ALPHA);
        HMONITOR monitor=MonitorFromWindow(target?target:GetForegroundWindow(),MONITOR_DEFAULTTONEAREST);
        MONITORINFO info{}; info.cbSize=sizeof(info); GetMonitorInfoW(monitor,&info);
        SetWindowPos(statusWindow,nullptr,info.rcWork.left+20,info.rcWork.top+20,1,1,SWP_NOZORDER|SWP_NOACTIVATE);
        UINT dpi=GetDpiForWindow(statusWindow); if(!dpi) dpi=96;
        int width=MulDiv(440,dpi,96),height=MulDiv(76,dpi,96);
        width=std::min(width,static_cast<int>(info.rcWork.right-info.rcWork.left-24));
        if(statusFont) DeleteObject(statusFont);
        statusFont=CreateFontW(-MulDiv(14,dpi,96),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        SetWindowPos(statusWindow,HWND_TOPMOST,info.rcWork.right-width-12,info.rcWork.bottom-height-12,width,height,SWP_NOACTIVATE);
        ShowWindow(statusWindow,SW_SHOWNOACTIVATE); InvalidateRect(statusWindow,nullptr,TRUE);
        SetTimer(host,HideTimer,failure?2500:1200,nullptr);
    }
    void hideStatus() { KillTimer(host,HideTimer); if(statusWindow) ShowWindow(statusWindow,SW_HIDE); }
    HWND control(const wchar_t* cls,const std::wstring& label,DWORD style,int id,int x,int y,int w,int h,UINT dpi) {
        HWND c=CreateWindowExW(cls==std::wstring(L"EDIT")?WS_EX_CLIENTEDGE:0,cls,label.c_str(),WS_CHILD|WS_VISIBLE|style,
            MulDiv(x,dpi,96),MulDiv(y,dpi,96),MulDiv(w,dpi,96),MulDiv(h,dpi,96),ui,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
        SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(uiFont),TRUE); return c;
    }
    void buildControls() {
        UINT dpi=GetDpiForWindow(ui); if(!dpi) dpi=96;
        uiDpi=dpi;
        if(uiFont) DeleteObject(uiFont);
        uiFont=CreateFontW(-MulDiv(14,dpi,96),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        control(L"STATIC",tr(L"置顶快捷键",L"Toggle shortcut"),0,0,20,18,240,24,dpi);
        const int ids[]={Ctrl,Alt,Shift,Win}; const UINT bits[]={MOD_CONTROL,MOD_ALT,MOD_SHIFT,MOD_WIN};
        const wchar_t* names[]={L"Ctrl",L"Alt",L"Shift",L"Win"};
        for(int i=0;i<4;++i) {
            HWND c=control(L"BUTTON",names[i],BS_AUTOCHECKBOX|WS_TABSTOP,ids[i],20+80*i,48,75,26,dpi);
            SendMessageW(c,BM_SETCHECK,(settings.toggle.modifiers&bits[i])?BST_CHECKED:BST_UNCHECKED,0);
        }
        HWND combo=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL,Key,348,48,104,260,dpi);
        for(UINT key='A';key<='Z';++key) addKey(combo,key);
        for(UINT key='0';key<='9';++key) addKey(combo,key);
        for(UINT key=VK_F1;key<=VK_F11;++key) addKey(combo,key);
        control(L"STATIC",tr(L"设置：Ctrl + Alt + Shift + T    退出：Ctrl + Alt + Shift + Q",
                           L"Settings: Ctrl + Alt + Shift + T    Exit: Ctrl + Alt + Shift + Q"),0,0,20,87,600,38,dpi);
        checkbox(ShowStatus,tr(L"显示短暂状态提示（不抢焦点）",L"Show brief status messages (without taking focus)"),settings.status,130,dpi);
        checkbox(ShowTray,tr(L"显示托盘图标",L"Show system tray icon"),settings.tray,162,dpi);
        StartupSnapshot snapshot; std::wstring error;
        bool actual=readStartup(startupValue,snapshot,error)?startupMatches(snapshot):settings.startup;
        checkbox(Startup,tr(L"登录 Windows 时启动",L"Start when signing in to Windows"),actual,194,dpi);
        control(L"STATIC",tr(L"语言",L"Language"),0,0,20,236,100,24,dpi);
        HWND lang=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,Language,125,232,185,100,dpi);
        SendMessageW(lang,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"中文"));
        SendMessageW(lang,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"English"));
        SendMessageW(lang,CB_SETCURSEL,settings.chinese?0:1,0);
        control(L"BUTTON",tr(L"检查更新",L"Check for updates"),BS_PUSHBUTTON|WS_TABSTOP,CheckUpdates,330,232,142,29,dpi);
        control(L"BUTTON",tr(L"下载页面",L"Download page"),BS_PUSHBUTTON|WS_TABSTOP,Download,486,232,130,29,dpi);
        control(L"STATIC",tr(L"排除程序：每行一个完整 EXE 路径或文件名（例如 chrome.exe）",
                            L"Excluded apps: one full EXE path or file name per line (e.g. chrome.exe)"),0,0,20,272,600,36,dpi);
        std::wstring rules; for(const auto& rule:settings.excluded) rules+=rule+L"\r\n";
        HWND edit=control(L"EDIT",rules,ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL|WS_TABSTOP,Exclusions,20,313,596,115,dpi);
        SendMessageW(edit,EM_SETLIMITTEXT,32700,0);
        control(L"BUTTON",tr(L"选择 EXE…",L"Choose EXE…"),BS_PUSHBUTTON|WS_TABSTOP,Browse,20,440,140,30,dpi);
        auto pinCheck=control(L"BUTTON",tr(L"显示活动窗口 Pin 按钮",L"Show Pin near the active window"),BS_AUTOCHECKBOX|WS_TABSTOP,ShowPin,180,440,430,30,dpi);
        SendMessageW(pinCheck,BM_SETCHECK,settings.pin?BST_CHECKED:BST_UNCHECKED,0);
        control(L"STATIC",tr(L"Pin 位置偏移（DIP）",L"Pin position offset (DIP)"),0,0,20,485,210,24,dpi);
        control(L"STATIC",tr(L"水平",L"X"),0,0,245,485,55,24,dpi);
        auto x=control(L"EDIT",std::to_wstring(settings.pinOffsetX),ES_AUTOHSCROLL|WS_TABSTOP,PinX,300,480,70,28,dpi);
        control(L"STATIC",tr(L"垂直",L"Y"),0,0,390,485,55,24,dpi);
        auto y=control(L"EDIT",std::to_wstring(settings.pinOffsetY),ES_AUTOHSCROLL|WS_TABSTOP,PinY,445,480,70,28,dpi);
        SendMessageW(x,EM_SETLIMITTEXT,6,0); SendMessageW(y,EM_SETLIMITTEXT,6,0);
        control(L"BUTTON",tr(L"恢复默认位置",L"Reset position"),BS_PUSHBUTTON|WS_TABSTOP,PinReset,20,522,165,29,dpi);
        control(L"STATIC",tr(L"无安全位置时隐藏，可继续使用快捷键。",L"Hidden when no safe position; shortcuts remain available."),0,0,200,526,414,24,dpi);
        EnableWindow(x,settings.pin); EnableWindow(y,settings.pin); EnableWindow(GetDlgItem(ui,PinReset),settings.pin);
        control(L"STATIC",lastResult,SS_LEFT,Result,20,566,596,60,dpi);
        control(L"BUTTON",tr(L"保存",L"Save"),BS_DEFPUSHBUTTON|WS_TABSTOP,Save,20,640,105,34,dpi);
        control(L"BUTTON",tr(L"关闭设置",L"Close settings"),BS_PUSHBUTTON|WS_TABSTOP,Close,140,640,155,34,dpi);
        control(L"BUTTON",tr(L"退出 QuietPin",L"Exit QuietPin"),BS_PUSHBUTTON|WS_TABSTOP,Quit,455,640,161,34,dpi);
        updateFeedback();
        SetFocus(GetDlgItem(ui,Ctrl));
    }
    void updateFeedback() {
        const auto update=updater.result();
        std::wstring message;
        switch(update.status) {
        case UpdateStatus::Idle: break;
        case UpdateStatus::Checking: message=tr(L"正在连接 GitHub 检查更新…",L"Connecting to GitHub to check for updates…"); break;
        case UpdateStatus::Current: message=tr(L"已是最新发布文件：v",L"You have the current release build: v")+update.version; break;
        case UpdateStatus::NewVersion: message=tr(L"发现新版本：v",L"New version available: v")+update.version+tr(L"。点击“下载页面”获取。",L". Open Download page to get it."); break;
        case UpdateStatus::DifferentBuild: message=tr(L"v",L"v")+update.version+tr(L" 发布文件与当前程序不同。请到下载页面查看。",L" release files differ from this copy. Review the Download page."); break;
        case UpdateStatus::Unverified: message=tr(L"GitHub 最新版本：v",L"Latest GitHub version: v")+update.version+tr(L"；无法核对当前构建，可到下载页面查看。",L"; build comparison unavailable. Review the Download page."); break;
        case UpdateStatus::Failed: message=tr(L"无法检查更新。请检查网络连接，稍后重试，或打开下载页面。",L"Unable to check for updates. Check your connection and retry, or open the Download page."); break;
        }
        if(!message.empty()) lastResult=std::move(message);
        if(ui) {
            EnableWindow(GetDlgItem(ui,CheckUpdates),update.status!=UpdateStatus::Checking);
            SetWindowTextW(GetDlgItem(ui,Result),lastResult.c_str());
        }
    }
    void openDownloadPage() {
        if(reinterpret_cast<INT_PTR>(ShellExecuteW(ui,L"open",DownloadPage,nullptr,nullptr,SW_SHOWNORMAL))<=32)
            result(tr(L"无法打开浏览器，请访问 github.com/LE-saber/QuietPin/releases。",L"Unable to open your browser. Visit github.com/LE-saber/QuietPin/releases."),true);
    }
    void checkbox(int id,const wchar_t* label,bool checked,int y,UINT dpi) {
        auto c=control(L"BUTTON",label,BS_AUTOCHECKBOX|WS_TABSTOP,id,20,y,596,28,dpi);
        SendMessageW(c,BM_SETCHECK,checked?BST_CHECKED:BST_UNCHECKED,0);
    }
    void addKey(HWND combo,UINT key) {
        auto label=keyName(key);
        LRESULT index=SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));
        SendMessageW(combo,CB_SETITEMDATA,index,key);
        if(key==settings.toggle.key) SendMessageW(combo,CB_SETCURSEL,index,0);
    }
    void resizeSettings(UINT dpi,RECT* suggested=nullptr) {
        MONITORINFO info{}; info.cbSize=sizeof(info); GetMonitorInfoW(MonitorFromWindow(ui,MONITOR_DEFAULTTONEAREST),&info);
        RECT bounds{0,0,MulDiv(636,dpi,96),MulDiv(700,dpi,96)};
        AdjustWindowRectExForDpi(&bounds,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE,WS_EX_TOOLWINDOW,dpi);
        const int height=bounds.bottom-bounds.top,width=bounds.right-bounds.left;
        int x=suggested?suggested->left:info.rcWork.left+(info.rcWork.right-info.rcWork.left-width)/2;
        int y=suggested?suggested->top:info.rcWork.top+std::max(0,static_cast<int>((info.rcWork.bottom-info.rcWork.top-height)/2));
        SetWindowPos(ui,nullptr,x,y,width,std::min(height,static_cast<int>(info.rcWork.bottom-info.rcWork.top)),SWP_NOZORDER|SWP_NOACTIVATE);
        // A scrollable client preserves all controls on small screens / high scaling.
        RECT client{}; GetClientRect(ui,&client);
        SCROLLINFO scroll{}; scroll.cbSize=sizeof(scroll); scroll.fMask=SIF_RANGE|SIF_PAGE|SIF_POS; scroll.nMax=MulDiv(700,dpi,96)-1;
        scroll.nPage=client.bottom; scroll.nPos=0; SetScrollInfo(ui,SB_VERT,&scroll,TRUE);
    }
    void openSettings() {
        if(exiting) return;
        if(pin) pin->suspend(true);
        if(!ui) {
            ui=CreateWindowExW(WS_EX_TOOLWINDOW,SettingsClass,tr(L"QuietPin 设置 · 0.2.0",L"QuietPin Settings · 0.2.0"),
                WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_VSCROLL,0,0,640,640,host,nullptr,GetModuleHandleW(nullptr),this);
            if(!ui) { if(pin) pin->suspend(false); return; }
            resizeSettings(GetDpiForWindow(ui)); buildControls();
        }
        ShowWindow(ui,SW_RESTORE); SetForegroundWindow(ui);
    }
    bool checked(int id) { return SendMessageW(GetDlgItem(ui,id),BM_GETCHECK,0,0)==BST_CHECKED; }
    void saveUI() {
        if(!allowConfigSave) { result(tr(L"请先备份或移走配置文件，再重新启动 QuietPin。",L"Back up or move the configuration file, then restart QuietPin."),true); return; }
        Settings candidate=settings;
        candidate.toggle.modifiers=(checked(Ctrl)?MOD_CONTROL:0)|(checked(Alt)?MOD_ALT:0)|(checked(Shift)?MOD_SHIFT:0)|(checked(Win)?MOD_WIN:0);
        HWND combo=GetDlgItem(ui,Key); LRESULT index=SendMessageW(combo,CB_GETCURSEL,0,0);
        candidate.toggle.key=static_cast<UINT>(SendMessageW(combo,CB_GETITEMDATA,index,0));
        if(!validHotkey(candidate.toggle) || candidate.toggle==SettingsHotkey || candidate.toggle==ExitHotkey) {
            result(tr(L"快捷键无效，或与设置/退出快捷键重复。",L"Invalid shortcut, or it duplicates Settings / Exit."),true); return;
        }
        candidate.status=checked(ShowStatus); candidate.tray=checked(ShowTray); candidate.startup=checked(Startup);
        candidate.pin=checked(ShowPin);
        if(!pinOffset(textOf(GetDlgItem(ui,PinX)),-512,512,candidate.pinOffsetX) ||
           !pinOffset(textOf(GetDlgItem(ui,PinY)),-256,256,candidate.pinOffsetY)) {
            result(tr(L"Pin 偏移无效：水平 -512～512，垂直 -256～256（DIP）。",L"Invalid Pin offset: X -512 to 512, Y -256 to 256 DIP."),true); return;
        }
        candidate.chinese=SendMessageW(GetDlgItem(ui,Language),CB_GETCURSEL,0,0)==0;
        candidate.excluded.clear(); std::wistringstream lines(textOf(GetDlgItem(ui,Exclusions))); std::wstring rule;
        while(std::getline(lines,rule)) {
            rule=trim(rule); if(rule.empty()) continue;
            std::filesystem::path p(rule);
            if(lower(p.extension().wstring())!=L".exe" || (rule.find_first_of(L"\\/:")!=rule.npos && !p.is_absolute()) || rule.size()>2048) {
                result(tr(L"排除项必须是 EXE 文件名或完整绝对路径。",L"Exclusions must be EXE file names or full absolute paths."),true); return;
            }
            rule=p.lexically_normal().wstring();
            if(std::none_of(candidate.excluded.begin(),candidate.excluded.end(),[&](const auto& s){return lower(s)==lower(rule);})) candidate.excluded.push_back(rule);
        }
        if(candidate.excluded.size()>128) { result(tr(L"最多支持 128 条排除规则。",L"At most 128 exclusion rules are supported."),true); return; }
        int nextId=toggleId;
        bool changed=candidate.toggle!=settings.toggle || !toggleRegistered;
        if(changed) {
            nextId=toggleId==10?11:10;
            if(!RegisterHotKey(host,nextId,candidate.toggle.modifiers|MOD_NOREPEAT,candidate.toggle.key)) {
                result(tr(L"快捷键已被占用，旧快捷键和配置仍然有效：",L"Shortcut unavailable; existing shortcut and settings were kept: ")+winError(GetLastError()),true); return;
            }
        }
        auto undoHotkey=[&] { if(changed) UnregisterHotKey(host,nextId); };
        std::wstring error; StartupSnapshot old;
        if(!readStartup(startupValue,old,error)) { undoHotkey(); result(error,true); return; }
        bool startupChanged=candidate.startup!=startupMatches(old);
        if(startupChanged && !writeStartup(startupValue,candidate.startup,error)) { undoHotkey(); result(tr(L"开机启动保存失败：",L"Startup update failed: ")+error,true); return; }
        if(!saveSettings(file,candidate,error)) {
            std::wstring rollback;
            if(startupChanged && !restoreStartup(startupValue,old,rollback)) error+=L"; startup rollback: "+rollback;
            undoHotkey(); result(tr(L"配置保存失败，原配置保留：",L"Could not save; previous configuration kept: ")+error,true); return;
        }
        if(changed) {
            if(toggleRegistered) UnregisterHotKey(host,toggleId);
            toggleId=nextId; toggleRegistered=true;
        }
        bool languageChanged=candidate.chinese!=settings.chinese;
        settings=std::move(candidate);
        if(pending && isExcluded(pending->window.path,settings.excluded)) { pending.reset(); KillTimer(host,ConfirmTimer); }
        if(!settings.status) hideStatus();
        // Excluding an app also releases only pinning introduced by this instance.
        for(auto it=managed.begin();it!=managed.end();) {
            if(!sameWindow(*it)) { it=managed.erase(it); continue; }
            if(isExcluded(it->path,settings.excluded)) {
                DWORD ignored=0;
                if(GetPropW(it->hwnd,marker.c_str()) && isTopmost(it->hwnd)) requestTopmost(*it,false,ignored);
                RemovePropW(it->hwnd,marker.c_str()); it=managed.erase(it);
            } else ++it;
        }
        applyTray();
        const bool pinOk=!pin || pin->apply();
        result(!pinOk?pin->error():settings.tray && (!trayAdded || !monitor.foregroundReady())?
            tr(L"设置已保存，但托盘未完全启用；请关闭再开启托盘重试。",L"Settings saved, but tray setup failed. Toggle the tray off and on to retry."):
            tr(L"设置已保存。",L"Settings saved."));
        if(languageChanged) { DestroyWindow(ui); openSettings(); }
    }
    void browse() {
        wchar_t path[32768]{}; OPENFILENAMEW picker{}; picker.lStructSize=sizeof(picker);
        picker.hwndOwner=ui; picker.lpstrFile=path; picker.nMaxFile=32768;
        picker.lpstrFilter=L"Windows applications (*.exe)\0*.exe\0\0";
        picker.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
        if(GetOpenFileNameW(&picker)) {
            auto rules=textOf(GetDlgItem(ui,Exclusions));
            if(!rules.empty() && rules.back()!=L'\n') rules+=L"\r\n";
            rules+=path; SetWindowTextW(GetDlgItem(ui,Exclusions),rules.c_str());
        }
    }
    void beginExit() {
        if(exiting) return;
        exiting=true; hideStatus();
        updater.stop();
        if(ui) ShowWindow(ui,SW_HIDE);
        if(toggleRegistered) UnregisterHotKey(host,toggleId);
        if(settingsRegistered) UnregisterHotKey(host,SettingsKeyId);
        if(exitRegistered) UnregisterHotKey(host,ExitKeyId);
        if(pin) pin->stop();
        monitor.stop();
        if(trayAdded) removeTray();
        // A requested pin may finish just after exit is requested; retain it until cleanup settles.
        pending.reset(); KillTimer(host,ConfirmTimer);
        quitDeadline=GetTickCount64()+700;
        for(const auto& window:managed) if(sameWindow(window) && GetPropW(window.hwnd,marker.c_str())) {
            DWORD ignored=0; requestTopmost(window,false,ignored);
        }
        SetTimer(host,QuitTimer,30,nullptr);
    }
    void cleanupPins() {
        bool waiting=false;
        for(auto it=managed.begin();it!=managed.end();) {
            if(!sameWindow(*it)) { it=managed.erase(it); continue; }
            // Pending operations have no marker yet; do not touch unrelated windows.
            HANDLE own=GetPropW(it->hwnd,marker.c_str());
            if(own && isTopmost(it->hwnd)) { DWORD ignored=0; requestTopmost(*it,false,ignored); waiting=true; ++it; }
            else { if(own) RemovePropW(it->hwnd,marker.c_str()); it=managed.erase(it); }
        }
        if(!waiting || GetTickCount64()>=quitDeadline) {
            for(const auto& window:managed) if(sameWindow(window)) RemovePropW(window.hwnd,marker.c_str());
            managed.clear(); KillTimer(host,QuitTimer); PostQuitMessage(0);
        }
    }
    LRESULT message(UINT msg,WPARAM w,LPARAM l) {
        if(taskbarCreated && msg==taskbarCreated) { trayAdded=false; applyTray(); return 0; }
        switch(msg) {
        case ManageMessage:
            if(w==1) openSettings(); else if(w==2) beginExit(); return 1;
        case WM_HOTKEY:
            if(exiting) return 0;
            if(static_cast<int>(w)==toggleId && toggleRegistered) toggle(GetForegroundWindow());
            else if(w==SettingsKeyId) openSettings(); else if(w==ExitKeyId) beginExit();
            return 0;
        case WM_TIMER:
            if(w==ConfirmTimer) confirm(); else if(w==HideTimer) hideStatus(); else if(w==QuitTimer) cleanupPins(); else if(w==40 && pin) pin->timer(); return 0;
        case ForegroundMessage: {
            auto events=monitor.take();
            if(exiting) return 0;
            if(events.foreground) rememberForeground(events.latestForeground);
            if(pin) pin->refresh(events.invalidated,events.moving);
            return 0;
        }
        case PinMessage: if(!exiting) monitor.refresh(); return 0;
        case UpdateMessage: if(!exiting) updateFeedback(); return 0;
        case WM_WTSSESSION_CHANGE:
            if(pin) {
                if(w==WTS_SESSION_LOCK || w==WTS_CONSOLE_DISCONNECT || w==WTS_REMOTE_DISCONNECT || w==WTS_SESSION_LOGOFF) pin->session(false);
                else if(w==WTS_SESSION_UNLOCK || w==WTS_CONSOLE_CONNECT || w==WTS_REMOTE_CONNECT) pin->session(true);
            } return 0;
        case WM_DISPLAYCHANGE: if(pin) pin->refresh(true); return 0;
        case WM_SETTINGCHANGE: if(pin) monitor.refresh(); return 0;
        case TrayMessage: {
            UINT event=LOWORD(l);
            if(event==WM_CONTEXTMENU) { POINT p{}; GetCursorPos(&p); trayMenu(p); }
            else if(event==NIN_SELECT || event==NIN_KEYSELECT) openSettings();
            return 0;
        }
        case WM_QUERYENDSESSION: beginExit(); return TRUE;
        case WM_ENDSESSION: if(w) { cleanupPins(); PostQuitMessage(0); } return 0;
        case WM_CLOSE: beginExit(); return 0;
        }
        return DefWindowProcW(host,msg,w,l);
    }
};

LRESULT CALLBACK hostProc(HWND hwnd,UINT msg,WPARAM w,LPARAM l) noexcept {
    auto* app=reinterpret_cast<App*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE) { app=static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams); app->host=hwnd; SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(app)); }
    try { if(app) return app->message(msg,w,l); }
    catch(...) { PostQuitMessage(1); return 0; }
    return DefWindowProcW(hwnd,msg,w,l);
}
LRESULT CALLBACK settingsProc(HWND hwnd,UINT msg,WPARAM w,LPARAM l) noexcept {
    auto* app=reinterpret_cast<App*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE) { app=static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams); app->ui=hwnd; SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(app)); }
    try {
        if(app) switch(msg) {
        case WM_CTLCOLORSTATIC: case WM_CTLCOLORBTN:
            SetBkColor(reinterpret_cast<HDC>(w),GetSysColor(COLOR_WINDOW));
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
        case WM_COMMAND:
            if(LOWORD(w)==Save) app->saveUI();
            else if(LOWORD(w)==Close || LOWORD(w)==IDCANCEL) DestroyWindow(hwnd);
            else if(LOWORD(w)==Quit) app->beginExit();
            else if(LOWORD(w)==Browse) app->browse();
            else if(LOWORD(w)==CheckUpdates) { app->updater.start(app->host,UpdateMessage); app->updateFeedback(); }
            else if(LOWORD(w)==Download) app->openDownloadPage();
            else if(LOWORD(w)==ShowPin) {
                bool enabled=app->checked(ShowPin); EnableWindow(GetDlgItem(hwnd,PinX),enabled); EnableWindow(GetDlgItem(hwnd,PinY),enabled); EnableWindow(GetDlgItem(hwnd,PinReset),enabled);
            } else if(LOWORD(w)==PinReset) { SetWindowTextW(GetDlgItem(hwnd,PinX),L"0"); SetWindowTextW(GetDlgItem(hwnd,PinY),L"0"); }
            return 0;
        case WM_CLOSE: DestroyWindow(hwnd); return 0;
        case WM_NCDESTROY: app->ui=nullptr; if(app->pin && !app->exiting) app->pin->suspend(false); return DefWindowProcW(hwnd,msg,w,l);
        case WM_DPICHANGED: {
            // Preserve unsaved input; scale existing child rectangles instead of rebuilding settings.
            auto* rect=reinterpret_cast<RECT*>(l);
            UINT newDpi=HIWORD(w);
            SCROLLINFO scroll{}; scroll.cbSize=sizeof(scroll); scroll.fMask=SIF_POS; GetScrollInfo(hwnd,SB_VERT,&scroll);
            double ratio=static_cast<double>(newDpi)/app->uiDpi;
            app->resizeSettings(newDpi,rect);
            app->uiDpi=newDpi;
            if(app->uiFont) DeleteObject(app->uiFont);
            app->uiFont=CreateFontW(-MulDiv(14,newDpi,96),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
            for(HWND child=GetWindow(hwnd,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)) {
                RECT r{}; GetWindowRect(child,&r); MapWindowPoints(nullptr,hwnd,reinterpret_cast<POINT*>(&r),2);
                SetWindowPos(child,nullptr,static_cast<int>(r.left*ratio),static_cast<int>((r.top+scroll.nPos)*ratio),static_cast<int>((r.right-r.left)*ratio),static_cast<int>((r.bottom-r.top)*ratio),SWP_NOZORDER|SWP_NOACTIVATE);
                SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(app->uiFont),TRUE);
            }
            return 0;
        }
        case WM_VSCROLL: case WM_MOUSEWHEEL: {
            SCROLLINFO s{}; s.cbSize=sizeof(s); s.fMask=SIF_ALL; GetScrollInfo(hwnd,SB_VERT,&s);
            int next=s.nPos;
            if(msg==WM_MOUSEWHEEL) next-=GET_WHEEL_DELTA_WPARAM(w)/WHEEL_DELTA*48;
            else switch(LOWORD(w)) {
            case SB_LINEUP: next-=30; break; case SB_LINEDOWN: next+=30; break;
            case SB_PAGEUP: next-=s.nPage; break; case SB_PAGEDOWN: next+=s.nPage; break;
            case SB_THUMBTRACK: next=s.nTrackPos; break;
            }
            next=std::clamp(next,0,std::max(0,s.nMax-static_cast<int>(s.nPage)+1));
            if(next!=s.nPos) { ScrollWindowEx(hwnd,0,s.nPos-next,nullptr,nullptr,nullptr,nullptr,SW_SCROLLCHILDREN|SW_INVALIDATE|SW_ERASE); s.fMask=SIF_POS; s.nPos=next; SetScrollInfo(hwnd,SB_VERT,&s,TRUE); }
            return 0;
        }
        }
    } catch(...) { if(app) SetWindowTextW(GetDlgItem(hwnd,Result),app->tr(L"操作失败，请关闭设置后重试。",L"Operation failed. Close Settings and retry.")); return 0; }
    return DefWindowProcW(hwnd,msg,w,l);
}
LRESULT CALLBACK statusProc(HWND hwnd,UINT msg,WPARAM w,LPARAM l) noexcept {
    auto* app=reinterpret_cast<App*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE) { app=static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams); SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(app)); }
    if(msg==WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    if(msg==WM_NCHITTEST) return HTTRANSPARENT;
    if(msg==WM_PAINT && app) {
        PAINTSTRUCT p{}; HDC dc=BeginPaint(hwnd,&p); RECT r{}; GetClientRect(hwnd,&r);
        HBRUSH brush=CreateSolidBrush(RGB(32,36,44)); FillRect(dc,&r,brush); DeleteObject(brush);
        SetBkMode(dc,TRANSPARENT); SetTextColor(dc,RGB(245,245,245));
        HGDIOBJ old=SelectObject(dc,app->statusFont); InflateRect(&r,-14,-10);
        DrawTextW(dc,app->lastResult.c_str(),-1,&r,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX|DT_END_ELLIPSIS);
        SelectObject(dc,old); EndPaint(hwnd,&p); return 0;
    }
    return DefWindowProcW(hwnd,msg,w,l);
}
bool registerClass(const wchar_t* name,WNDPROC proc) {
    WNDCLASSEXW c{}; c.cbSize=sizeof(c); c.lpfnWndProc=proc; c.hInstance=GetModuleHandleW(nullptr); c.lpszClassName=name;
    c.hCursor=LoadCursorW(nullptr,IDC_ARROW); c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
    return RegisterClassExW(&c)!=0;
}
}

int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
    try {
        bool settings=false,exit=false,startup=false,exitIfOwned=false; auto directory=defaultConfigDirectory();
        int count=0; LPWSTR* args=CommandLineToArgvW(GetCommandLineW(),&count);
        if(!args) return 1;
        for(int i=1;i<count;++i) {
            std::wstring arg=args[i];
            if(arg==L"--settings") settings=true;
            else if(arg==L"--exit") exit=true;
            else if(arg==L"--exit-if-owned") { exit=true; exitIfOwned=true; }
            else if(arg==L"--startup") startup=true;
            else if(arg==L"--config-dir" && i+1<count) directory=std::filesystem::absolute(args[++i]).lexically_normal();
            else { LocalFree(args); MessageBoxW(nullptr,L"QuietPin [--settings | --exit | --startup] [--config-dir <directory>]",L"QuietPin",MB_OK|MB_ICONINFORMATION); return 2; }
        }
        LocalFree(args);
        App app(directory);
        std::wstring mutexName=L"Local\\"+app.title;
        app.mutex=CreateMutexW(nullptr,FALSE,mutexName.c_str());
        if(!app.mutex) return 1;
        bool exists=GetLastError()==ERROR_ALREADY_EXISTS;
        if(exists) {
            if(startup && !settings && !exit) return 0;
            for(int i=0;i<30;++i) {
                HWND running=FindWindowW(HostClass,app.title.c_str());
                if(running) {
                    DWORD pid=0; GetWindowThreadProcessId(running,&pid); AllowSetForegroundWindow(pid);
                    HANDLE owned=nullptr;
                    if(exitIfOwned) {
                        owned=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,pid);
                        wchar_t path[32768]{}; DWORD length=32768;
                        bool matches=owned && QueryFullProcessImageNameW(owned,0,path,&length) && lower(path)==lower(executablePath().wstring());
                        if(!matches) { if(owned) CloseHandle(owned); return 0; }
                    }
                    DWORD_PTR reply=0;
                    bool sent=SendMessageTimeoutW(running,ManageMessage,exit?2:1,0,SMTO_ABORTIFHUNG|SMTO_BLOCK,1500,&reply)!=0;
                    if(owned) { auto waited=sent?WaitForSingleObject(owned,5000):WAIT_FAILED; CloseHandle(owned); return waited==WAIT_OBJECT_0?0:3; }
                    if(sent) return 0;
                    break;
                }
                Sleep(50);
            }
            if(!exit) MessageBoxW(nullptr,L"The running QuietPin instance did not respond. / QuietPin 当前实例未响应。",L"QuietPin",MB_OK|MB_ICONWARNING);
            return 3;
        }
        if(exit) return 0;
        INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_STANDARD_CLASSES}; InitCommonControlsEx(&controls);
        if(!registerClass(HostClass,hostProc) || !registerClass(SettingsClass,settingsProc) || !registerClass(StatusClass,statusProc)) return 1;
        if(!app.initialize()) return 1;
        if(settings) app.openSettings();
        MSG msg{}; int status=0;
        while((status=GetMessageW(&msg,nullptr,0,0))>0) {
            if(app.ui && IsDialogMessageW(app.ui,&msg)) continue;
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        return status==-1?1:static_cast<int>(msg.wParam);
    } catch(...) {
        MessageBoxW(nullptr,L"QuietPin could not start. Check folder permissions. / 无法启动 QuietPin，请检查目录权限。",L"QuietPin",MB_OK|MB_ICONERROR);
        return 1;
    }
}



