#include "UpdateChecker.h"
#include <winhttp.h>
#include <bcrypt.h>
#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace qp {
namespace {
struct InternetHandle {
    HINTERNET value;
    ~InternetHandle() { if(value) WinHttpCloseHandle(value); }
};
void require(bool ok) { if(!ok) throw std::runtime_error("Update check failed"); }
struct Response { DWORD status=0; std::wstring url; std::string body; };
Response get(const std::wstring& path,bool body,const std::atomic<bool>& stopped) {
    require(!stopped.load());
    InternetHandle session{WinHttpOpen(L"QuietPin/0.2.0",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};
    require(session.value);
    require(WinHttpSetTimeouts(session.value,5000,5000,5000,5000));
    DWORD retries=1;
    require(WinHttpSetOption(session.value,WINHTTP_OPTION_CONNECT_RETRIES,&retries,sizeof(retries)));
    InternetHandle connection{WinHttpConnect(session.value,L"github.com",INTERNET_DEFAULT_HTTPS_PORT,0)};
    require(connection.value);
    InternetHandle request{WinHttpOpenRequest(connection.value,L"GET",path.c_str(),nullptr,
        WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE)};
    require(request.value);
    DWORD policy=WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    require(WinHttpSetOption(request.value,WINHTTP_OPTION_REDIRECT_POLICY,&policy,sizeof(policy)));
    DWORD features=WINHTTP_DISABLE_COOKIES|WINHTTP_DISABLE_AUTHENTICATION;
    require(WinHttpSetOption(request.value,WINHTTP_OPTION_DISABLE_FEATURE,&features,sizeof(features)));
    require(WinHttpSendRequest(request.value,L"Cache-Control: no-cache\r\n",static_cast<DWORD>(-1),
        WINHTTP_NO_REQUEST_DATA,0,0,0));
    require(WinHttpReceiveResponse(request.value,nullptr));
    require(!stopped.load());
    Response response;
    DWORD length=sizeof(response.status);
    require(WinHttpQueryHeaders(request.value,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,&response.status,&length,WINHTTP_NO_HEADER_INDEX));
    length=0;
    WinHttpQueryOption(request.value,WINHTTP_OPTION_URL,nullptr,&length);
    require(length>0 && length<=32768);
    std::vector<wchar_t> url(length/sizeof(wchar_t)+1,0);
    require(WinHttpQueryOption(request.value,WINHTTP_OPTION_URL,url.data(),&length));
    response.url=url.data();
    if(body && response.status==200) {
        const auto deadline=GetTickCount64()+15000;
        char buffer[4096]; DWORD read=0;
        do {
            require(!stopped.load() && GetTickCount64()<deadline);
            require(WinHttpReadData(request.value,buffer,sizeof(buffer),&read));
            require(response.body.size()+read<=65536);
            response.body.append(buffer,read);
        } while(read);
    }
    return response;
}
std::array<unsigned,3> versionParts(const std::wstring& version) {
    std::array<unsigned,3> parts{}; size_t position=0;
    for(size_t i=0;i<parts.size();++i) {
        const auto start=position;
        while(position<version.size() && version[position]>=L'0' && version[position]<=L'9') {
            require(parts[i]<100000);
            parts[i]=parts[i]*10+static_cast<unsigned>(version[position++]-L'0');
        }
        require(position>start);
        if(i+1<parts.size()) require(position<version.size() && version[position++]==L'.');
    }
    require(position==version.size());
    return parts;
}
std::string executableHash() {
    std::vector<wchar_t> path(32768,0);
    DWORD length=GetModuleFileNameW(nullptr,path.data(),static_cast<DWORD>(path.size()));
    require(length>0 && length<path.size());
    std::ifstream file(std::filesystem::path(path.data()),std::ios::binary);
    require(file.is_open());
    struct HashHandles {
        BCRYPT_ALG_HANDLE algorithm=nullptr;
        BCRYPT_HASH_HANDLE hash=nullptr;
        ~HashHandles() { if(hash) BCryptDestroyHash(hash); if(algorithm) BCryptCloseAlgorithmProvider(algorithm,0); }
    } handles;
    require(BCryptOpenAlgorithmProvider(&handles.algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0);
    require(BCryptCreateHash(handles.algorithm,&handles.hash,nullptr,0,nullptr,0,0)>=0);
    std::array<unsigned char,8192> buffer{};
    while(file.read(reinterpret_cast<char*>(buffer.data()),buffer.size()) || file.gcount()>0)
        require(BCryptHashData(handles.hash,buffer.data(),static_cast<ULONG>(file.gcount()),0)>=0);
    require(file.eof() && !file.bad());
    std::array<unsigned char,32> digest{};
    require(BCryptFinishHash(handles.hash,digest.data(),static_cast<ULONG>(digest.size()),0)>=0);
    std::string result; result.reserve(64);
    for(auto byte:digest) { result+= "0123456789abcdef"[byte>>4]; result+= "0123456789abcdef"[byte&15]; }
    return result;
}
UpdateResult check(const std::atomic<bool>& stopped) {
    const auto release=get(L"/LE-saber/QuietPin/releases/latest",false,stopped);
    require(release.status==200);
    const std::wstring prefix=L"https://github.com/LE-saber/QuietPin/releases/tag/v";
    require(release.url.starts_with(prefix));
    const auto version=release.url.substr(prefix.size());
    const auto remote=versionParts(version),local=versionParts(CurrentVersion);
    if(remote>local) return {UpdateStatus::NewVersion,version};
    if(remote<local) return {UpdateStatus::Unverified,version};
    // Same-version replacements are detected by the published EXE checksum.
    // A difference is deliberately not described as newer: it could be an older build.
    const auto sums=get(L"/LE-saber/QuietPin/releases/download/v"+version+L"/QuietPin-v"+version+L"-SHA256SUMS.txt",true,stopped);
    require(sums.status==200);
    std::istringstream lines(sums.body); std::string line;
    while(std::getline(lines,line)) {
        if(line.ends_with('\r')) line.pop_back();
        if(line.size()!=78 || line.substr(64)!="  QuietPin.exe") continue;
        const auto hash=line.substr(0,64);
        require(hash.find_first_not_of("0123456789abcdef")==std::string::npos);
        return {hash==executableHash()?UpdateStatus::Current:UpdateStatus::DifferentBuild,version};
    }
    return {UpdateStatus::Unverified,version};
}
}
struct UpdateChecker::State {
    std::mutex mutex;
    UpdateResult result;
    HWND window=nullptr;
    std::atomic<bool> stopped=false;
};
UpdateChecker::UpdateChecker():state(std::make_shared<State>()) {}
UpdateChecker::~UpdateChecker() { stop(); }
UpdateResult UpdateChecker::result() const { std::lock_guard lock(state->mutex); return state->result; }
void UpdateChecker::stop() {
    std::lock_guard lock(state->mutex);
    state->window=nullptr; state->stopped=true;
}
void UpdateChecker::start(HWND window,UINT message) {
    std::lock_guard lock(state->mutex);
    if(state->stopped || state->result.status==UpdateStatus::Checking) return;
    state->window=window; state->result={UpdateStatus::Checking,{}};
    try {
        // The worker owns all synchronous WinHTTP handles. Never close them from
        // the UI thread. Shared state outlives Settings/App; exit does not wait on DNS.
        std::thread([shared=state,message] {
            UpdateResult result{UpdateStatus::Failed,{}};
            try { result=check(shared->stopped); } catch(...) {}
            std::lock_guard guard(shared->mutex);
            shared->result=std::move(result);
            if(shared->window && !shared->stopped) PostMessageW(shared->window,message,0,0);
        }).detach();
    } catch(...) { state->result={UpdateStatus::Failed,{}}; }
}
}
