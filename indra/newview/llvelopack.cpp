/**
 * @file llvelopack.cpp
 * @brief Velopack installer and update framework integration
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#if LL_VELOPACK

#include "llviewerprecompiledheaders.h"
#include "llvelopack.h"
#include "llstring.h"
#include "llcorehttputil.h"
#include "llversioninfo.h"

#include <boost/json.hpp>
#include <fstream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include "llnotificationsutil.h"
#include "llviewercontrol.h"
#include "llappviewer.h"
#include "llcoros.h"

#include "Velopack.h"

#if LL_WINDOWS
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <objbase.h>
#include <filesystem>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#endif // LL_WINDOWS

struct VelopackDownloadRequest
{
    bool allow_downgrade = false;
    bool show_progress_notification = false;
    bool quit_when_ready = false;
    bool required_update = false;
    std::string version;
};

class VelopackUpdaterController
{
public:
    std::string mUpdateUrl;
    std::function<void(int)> mProgressCallback;
    vpkc_update_manager_t* mUpdateManager = nullptr;
    vpkc_update_info_t* mPendingUpdate = nullptr;
    vpkc_update_source_t* mUpdateSource = nullptr;
    LLNotificationPtr mDownloadingNotification;
    bool mRestartAfterUpdate = false;
    bool mBackgroundStageActive = false;
    bool mRequiredUpdateInProgress = false;
    std::string mRequiredUpdateVersion;
    std::string mRequiredUpdateRelnotes;
    std::unordered_map<std::string, std::string> mAssetUrlMap;
    std::unordered_map<std::string, std::string> mPreDownloadedAssets;
    std::mutex mStateMutex;
    std::thread mBackgroundStageThread;
};

static VelopackUpdaterController& updater()
{
    static VelopackUpdaterController sController;
    return sController;
}

// Forward declarations
static void show_required_update_prompt();
static void show_downloading_notification(const std::string& version);
static void dismiss_downloading_notification();

//
// Custom update source helpers
//

static std::string extract_basename(const std::string& url)
{
    // Strip query params / fragment
    std::string path = url;
    auto qpos = path.find('?');
    if (qpos != std::string::npos) path = path.substr(0, qpos);
    auto fpos = path.find('#');
    if (fpos != std::string::npos) path = path.substr(0, fpos);

    auto spos = path.rfind('/');
    if (spos != std::string::npos && spos + 1 < path.size())
        return path.substr(spos + 1);
    return path;
}

static std::string sanitize_asset_filename(const std::string& filename)
{
    std::string basename = extract_basename(filename);
    if (basename.empty() || basename == "." || basename == "..")
    {
        return {};
    }

    if (basename.find('/') != std::string::npos ||
        basename.find('\\') != std::string::npos ||
        basename.find("..") != std::string::npos)
    {
        return {};
    }

    return basename;
}

static void rewrite_asset_urls(boost::json::value& jv)
{
    if (jv.is_object())
    {
        auto& obj = jv.as_object();
        auto it = obj.find("FileName");
        if (it != obj.end() && it->value().is_string())
        {
            std::string filename(it->value().as_string());
            if (filename.find("://") != std::string::npos)
            {
                std::string basename = extract_basename(filename);
                updater().mAssetUrlMap[basename] = filename;
                it->value() = basename;
                LL_DEBUGS("Velopack") << "Rewrote FileName: " << basename << LL_ENDL;
            }
        }
        for (auto& kv : obj)
        {
            rewrite_asset_urls(kv.value());
        }
    }
    else if (jv.is_array())
    {
        for (auto& elem : jv.as_array())
        {
            rewrite_asset_urls(elem);
        }
    }
}

static std::string rewrite_release_feed(const std::string& json_str)
{
    boost::json::value jv = boost::json::parse(json_str);
    rewrite_asset_urls(jv);
    return boost::json::serialize(jv);
}

static std::string download_url_raw(const std::string& url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    auto httpAdapter = std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("VelopackSource", httpPolicy);
    auto httpRequest = std::make_shared<LLCore::HttpRequest>();
    auto httpOpts = std::make_shared<LLCore::HttpOptions>();
    httpOpts->setFollowRedirects(true);

    LLSD result = httpAdapter->getRawAndSuspend(httpRequest, url, httpOpts);
    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
    if (!status)
    {
        LL_WARNS("Velopack") << "HTTP request failed for " << url << ": " << status.toString() << LL_ENDL;
        return {};
    }

    const LLSD::Binary& rawBody = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_RAW].asBinary();
    return std::string(rawBody.begin(), rawBody.end());
}

static bool download_url_to_file(const std::string& url, const std::string& local_path)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    auto httpAdapter = std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("VelopackDownload", httpPolicy);
    auto httpRequest = std::make_shared<LLCore::HttpRequest>();
    auto httpOpts = std::make_shared<LLCore::HttpOptions>();
    httpOpts->setFollowRedirects(true);
    httpOpts->setTransferTimeout(1200);

    LLSD result = httpAdapter->getRawAndSuspend(httpRequest, url, httpOpts);
    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
    if (!status)
    {
        LL_WARNS("Velopack") << "Download failed for " << url << ": " << status.toString() << LL_ENDL;
        return false;
    }

    const LLSD::Binary& rawBody = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_RAW].asBinary();
    llofstream outFile(local_path, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open())
    {
        LL_WARNS("Velopack") << "Failed to open file for writing: " << local_path << LL_ENDL;
        return false;
    }
    outFile.write(reinterpret_cast<const char*>(rawBody.data()), rawBody.size());
    outFile.close();
    return true;
}

//
// Custom source callbacks
//

static char* custom_get_release_feed(void* user_data, const char* releases_name)
{
    std::string base = updater().mUpdateUrl;
    if (!base.empty() && base.back() == '/')
        base.pop_back();
    std::string url = base + "/" + releases_name;
    LL_INFOS("Velopack") << "Fetching release feed: " << url << LL_ENDL;

    std::string json_str = download_url_raw(url);
    if (json_str.empty())
    {
        return nullptr;
    }

    try
    {
        std::string rewritten = rewrite_release_feed(json_str);
        char* result = static_cast<char*>(malloc(rewritten.size() + 1));
        if (result)
        {
            memcpy(result, rewritten.c_str(), rewritten.size() + 1);
        }
        return result;
    }
    catch (const std::exception& e)
    {
        LL_WARNS("Velopack") << "Failed to parse/rewrite release feed: " << e.what() << LL_ENDL;
        // Return original unmodified feed as fallback
        char* result = static_cast<char*>(malloc(json_str.size() + 1));
        if (result)
        {
            memcpy(result, json_str.c_str(), json_str.size() + 1);
        }
        return result;
    }
}

static void custom_free_release_feed(void* user_data, char* feed)
{
    free(feed);
}

static bool custom_download_asset(void* user_data,
                                   const vpkc_asset_t* asset,
                                   const char* local_path,
                                   size_t progress_callback_id)
{
    // The asset has already been downloaded at the coroutine level (before vpkc_download_updates).
    // This callback just copies the pre-downloaded file to where Velopack expects it.
    // We cannot use getRawAndSuspend here — coroutine context is lost through the Rust FFI boundary.
    std::string filename = asset->FileName ? asset->FileName : "";
    auto& pre_downloaded_assets = updater().mPreDownloadedAssets;
    auto it = pre_downloaded_assets.find(filename);
    if (it == pre_downloaded_assets.end())
    {
        LL_WARNS("Velopack") << "No pre-downloaded asset available for: " << filename << LL_ENDL;
        return false;
    }

    std::string source_path = it->second;
    LL_INFOS("Velopack") << "Download asset callback: filename=" << filename
        << " local_path=" << local_path
        << " size=" << asset->Size << LL_ENDL;
    vpkc_source_report_progress(progress_callback_id, 0);

    std::ifstream src(source_path, std::ios::binary);
    llofstream dst(local_path, std::ios::binary | std::ios::trunc);
    if (!src.is_open() || !dst.is_open())
    {
        LL_WARNS("Velopack") << "Failed to open files for copy" << LL_ENDL;
        return false;
    }

    dst << src.rdbuf();
    dst.close();
    src.close();

    vpkc_source_report_progress(progress_callback_id, 100);
    LL_INFOS("Velopack") << "Asset copy complete" << LL_ENDL;
    return true;
}

//
// Platform-specific helpers and hooks
//

#if LL_WINDOWS

static const wchar_t* PROTOCOL_SECONDLIFE = L"secondlife";
static const wchar_t* PROTOCOL_GRID_INFO = L"x-grid-location-info";
static std::wstring get_viewer_exe_name()
{
    return ll_convert<std::wstring>(gDirUtilp->getExecutableFilename());
}

static std::wstring get_install_dir()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    return path;
}

static std::wstring get_app_name()
{
    // Release installs should use the base brand name; special channels keep their suffix.
    std::wstring channel = LL_TO_WSTRING(LL_VIEWER_CHANNEL);
    if (channel.size() >= 8 && channel.compare(channel.size() - 8, 8, L" Release") == 0)
    {
        return channel.substr(0, channel.size() - 8);
    }
    return channel;
}

static std::wstring get_app_name_oneword()
{
    std::wstring name = get_app_name();
    name.erase(std::remove(name.begin(), name.end(), L' '), name.end());
    return name;
}

static std::wstring get_start_menu_path()
{
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, path)))
    {
        return path;
    }
    return L"";
}

static std::wstring get_desktop_path()
{
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path)))
    {
        return path;
    }
    return L"";
}

static bool create_shortcut(const std::wstring& shortcut_path,
                            const std::wstring& target_path,
                            const std::wstring& arguments,
                            const std::wstring& description,
                            const std::wstring& icon_path)
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) return false;

    IShellLinkW* shell_link = nullptr;
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                          IID_IShellLinkW, (void**)&shell_link);
    if (SUCCEEDED(hr))
    {
        shell_link->SetPath(target_path.c_str());
        shell_link->SetArguments(arguments.c_str());
        shell_link->SetDescription(description.c_str());
        shell_link->SetIconLocation(icon_path.c_str(), 0);

        wchar_t work_dir[MAX_PATH];
        wcscpy_s(work_dir, target_path.c_str());
        PathRemoveFileSpecW(work_dir);
        shell_link->SetWorkingDirectory(work_dir);

        IPersistFile* persist_file = nullptr;
        hr = shell_link->QueryInterface(IID_IPersistFile, (void**)&persist_file);
        if (SUCCEEDED(hr))
        {
            hr = persist_file->Save(shortcut_path.c_str(), TRUE);
            persist_file->Release();
        }
        shell_link->Release();
    }

    CoUninitialize();
    return SUCCEEDED(hr);
}

static void register_protocol_handler(const std::wstring& protocol,
                                       const std::wstring& description,
                                       const std::wstring& exe_path)
{
    std::wstring key_path = L"SOFTWARE\\Classes\\" + protocol;
    HKEY hkey;

    if (RegCreateKeyExW(HKEY_CURRENT_USER, key_path.c_str(), 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueExW(hkey, NULL, 0, REG_SZ,
                      (BYTE*)description.c_str(), (DWORD)((description.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hkey, L"URL Protocol", 0, REG_SZ, (BYTE*)L"", sizeof(wchar_t));
        RegCloseKey(hkey);
    }

    std::wstring icon_key_path = key_path + L"\\DefaultIcon";
    if (RegCreateKeyExW(HKEY_CURRENT_USER, icon_key_path.c_str(), 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL) == ERROR_SUCCESS)
    {
        std::wstring icon_value = L"\"" + exe_path + L"\"";
        RegSetValueExW(hkey, NULL, 0, REG_SZ,
                      (BYTE*)icon_value.c_str(), (DWORD)((icon_value.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hkey);
    }

    std::wstring cmd_key_path = key_path + L"\\shell\\open\\command";
    if (RegCreateKeyExW(HKEY_CURRENT_USER, cmd_key_path.c_str(), 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL) == ERROR_SUCCESS)
    {
        std::wstring cmd_value = L"\"" + exe_path + L"\" -url \"%1\"";
        RegSetValueExW(hkey, NULL, 0, REG_EXPAND_SZ,
                      (BYTE*)cmd_value.c_str(), (DWORD)((cmd_value.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hkey);
    }
}

void clear_nsis_links()
{
    wchar_t path[MAX_PATH];

    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, path)))
    {
        std::wstring folder_path = std::wstring(path) + L"\\" + get_app_name();
        std::error_code ec;
        std::filesystem::path dir(folder_path);
        if (std::filesystem::exists(dir, ec))
        {
            std::filesystem::remove_all(dir, ec);
            if (ec)
            {
                LL_WARNS("Velopack") << "Failed to remove NSIS start menu directory: "
                    << ll_convert_wide_to_string(folder_path) << LL_ENDL;
            }
        }
    }

    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, 0, path)))
    {
        std::wstring shortcut_path = std::wstring(path) + L"\\" + get_app_name() + L".lnk";
        if (!DeleteFileW(shortcut_path.c_str()))
        {
            DWORD error = GetLastError();
            if (error != ERROR_FILE_NOT_FOUND)
            {
                LL_WARNS("Velopack") << "Failed to delete NSIS desktop shortcut: "
                    << ll_convert_wide_to_string(shortcut_path)
                    << " (error: " << error << ")" << LL_ENDL;
            }
        }
    }
}

static void parse_version(const wchar_t* version_str, int& major, int& minor, int& patch, uint64_t& build)
{
    major = minor = patch = 0;
    build = 0;
    if (!version_str) return;
    // Use swscanf for wide strings
    swscanf(version_str, L"%d.%d.%d.%llu", &major, &minor, &patch, &build);
}

bool get_nsis_uninstaller_path(wchar_t* path_buffer, DWORD bufSize, S32 cur_major_ver, S32 cur_minor_ver, S32 cur_patch_ver, U64 cur_build_ver)
{
    // Test for presence of NSIS viewer registration, then
    // attempt to read uninstall info
    std::wstring app_name_oneword = get_app_name_oneword();
    std::wstring uninstall_key_path = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + app_name_oneword;
    HKEY hkey;
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, uninstall_key_path.c_str(), 0, KEY_READ, &hkey);
    if (result != ERROR_SUCCESS)
    {
        return false;
    }

    // Read DisplayVersion
    wchar_t version_buf[64] = { 0 };
    DWORD version_buf_size = sizeof(version_buf);
    DWORD type = 0;
    LONG ver_rv = RegGetValueW(hkey, nullptr, L"DisplayVersion", RRF_RT_REG_SZ, &type, version_buf, &version_buf_size);

    if (ver_rv != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        return false;
    }

    int nsis_major = 0, nsis_minor = 0, nsis_patch = 0;
    uint64_t nsis_build = 0;
    parse_version(version_buf, nsis_major, nsis_minor, nsis_patch, nsis_build);

    // Compare numerically
    if ((nsis_major > cur_major_ver) ||
        (nsis_major == cur_major_ver && nsis_minor > cur_minor_ver) ||
        (nsis_major == cur_major_ver && nsis_minor == cur_minor_ver && nsis_patch > cur_patch_ver) ||
         // Assume that bigger build number means newer version, which is not always true but works for our purposes
        (nsis_major == cur_major_ver && nsis_minor == cur_minor_ver && nsis_patch == cur_patch_ver && nsis_build > cur_build_ver))
    {
        LL_INFOS() << "Found installed nsis version that is newer" << nsis_major << "." << nsis_minor << "." << nsis_patch << LL_ENDL;
        RegCloseKey(hkey);
        return false;
    }

    LONG rv = RegGetValueW(hkey, nullptr, L"UninstallString", RRF_RT_REG_SZ, &type, path_buffer, &bufSize);
    RegCloseKey(hkey);
    if (rv != ERROR_SUCCESS)
    {
        return false;
    }
    size_t len = wcslen(path_buffer);
    if (len > 0)
    {
        if (path_buffer[0] == L'\"')
        {
            // Likely to contain leading "
            memmove(path_buffer, path_buffer + 1, len * sizeof(wchar_t));
        }
        wchar_t* pos = wcsstr(path_buffer, L"uninst.exe");
        if (pos)
        {
            // Likely to contain trailing "
            pos[wcslen(L"uninst.exe")] = L'\0';
        }
    }
    std::error_code ec;
    std::filesystem::path path(path_buffer);
    if (!std::filesystem::exists(path, ec))
    {
        return false;
    }

    // Todo: check codesigning?

    return true;
}

static void unregister_protocol_handler(const std::wstring& protocol)
{
    std::wstring key_path = L"SOFTWARE\\Classes\\" + protocol;
    RegDeleteTreeW(HKEY_CURRENT_USER, key_path.c_str());
}

static void register_uninstall_info(const std::wstring& install_dir,
                                    const std::wstring& app_name,
                                    const std::wstring& version)
{
    std::wstring app_name_oneword = get_app_name_oneword();
    std::wstring key_path = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + app_name_oneword;
    HKEY hkey;

    if (RegCreateKeyExW(HKEY_CURRENT_USER, key_path.c_str(), 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL) == ERROR_SUCCESS)
    {
        std::wstring exe_path = install_dir + L"\\" + get_viewer_exe_name();
        // Update.exe lives one level above the current\ directory where the viewer exe runs
        std::filesystem::path update_exe = std::filesystem::path(install_dir).parent_path() / L"Update.exe";
        std::wstring uninstall_cmd = L"\"" + update_exe.wstring() + L"\" --uninstall";

        RegSetValueExW(hkey, L"DisplayName", 0, REG_SZ,
                      (BYTE*)app_name.c_str(), (DWORD)((app_name.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hkey, L"DisplayVersion", 0, REG_SZ,
                      (BYTE*)version.c_str(), (DWORD)((version.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hkey, L"Publisher", 0, REG_SZ,
                      (BYTE*)L"Linden Research, Inc.", 44);
        RegSetValueExW(hkey, L"UninstallString", 0, REG_SZ,
                      (BYTE*)uninstall_cmd.c_str(), (DWORD)((uninstall_cmd.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hkey, L"DisplayIcon", 0, REG_SZ,
                      (BYTE*)exe_path.c_str(), (DWORD)((exe_path.size() + 1) * sizeof(wchar_t)));

        std::wstring link_url = L"https://support.secondlife.com/contact-support/";
        RegSetValueExW(hkey, L"HelpLink", 0, REG_SZ,
            (BYTE*)link_url.c_str(), (DWORD)((link_url.size() + 1) * sizeof(wchar_t)));

        link_url = L"https://secondlife.com/whatis/";
        RegSetValueExW(hkey, L"URLInfoAbout", 0, REG_SZ,
            (BYTE*)link_url.c_str(), (DWORD)((link_url.size() + 1) * sizeof(wchar_t)));

        link_url = L"http://secondlife.com/support/downloads/";
        RegSetValueExW(hkey, L"URLUpdateInfo", 0, REG_SZ,
            (BYTE*)link_url.c_str(), (DWORD)((link_url.size() + 1) * sizeof(wchar_t)));

        DWORD no_modify = 1;
        RegSetValueExW(hkey, L"NoModify", 0, REG_DWORD, (BYTE*)&no_modify, sizeof(DWORD));
        RegSetValueExW(hkey, L"NoRepair", 0, REG_DWORD, (BYTE*)&no_modify, sizeof(DWORD));

        DWORD estimated_size = 120000;
        RegSetValueExW(hkey, L"EstimatedSize", 0, REG_DWORD, (BYTE*)&estimated_size, sizeof(DWORD));

        RegCloseKey(hkey);
    }
}

static void unregister_uninstall_info()
{
    std::wstring app_name_oneword = get_app_name_oneword();
    std::wstring key_path = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + app_name_oneword;
    RegDeleteTreeW(HKEY_CURRENT_USER, key_path.c_str());
}

static void create_shortcuts(const std::wstring& install_dir, const std::wstring& app_name)
{
    std::wstring exe_path = install_dir + L"\\" + get_viewer_exe_name();
    std::wstring start_menu_dir = get_start_menu_path() + L"\\" + app_name;
    std::wstring desktop_path = get_desktop_path();

    CreateDirectoryW(start_menu_dir.c_str(), NULL);

    create_shortcut(start_menu_dir + L"\\" + app_name + L".lnk",
                    exe_path, L"", app_name, exe_path);

    create_shortcut(desktop_path + L"\\" + app_name + L".lnk",
                    exe_path, L"", app_name, exe_path);
}

static void remove_shortcuts(const std::wstring& app_name)
{
    std::wstring start_menu_dir = get_start_menu_path() + L"\\" + app_name;
    std::wstring desktop_path = get_desktop_path();

    DeleteFileW((start_menu_dir + L"\\" + app_name + L".lnk").c_str());
    RemoveDirectoryW(start_menu_dir.c_str());
    DeleteFileW((desktop_path + L"\\" + app_name + L".lnk").c_str());
}

static void on_after_install(void* user_data, const char* app_version)
{
    std::wstring install_dir = get_install_dir();
    std::wstring app_name = get_app_name();
    std::wstring exe_path = install_dir + L"\\" + get_viewer_exe_name();

    int len = MultiByteToWideChar(CP_UTF8, 0, app_version, -1, NULL, 0);
    std::wstring version(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, app_version, -1, &version[0], len);

    register_protocol_handler(PROTOCOL_SECONDLIFE, L"URL:Second Life", exe_path);
    register_protocol_handler(PROTOCOL_GRID_INFO, L"URL:Second Life", exe_path);
    register_uninstall_info(install_dir, app_name, version);
    create_shortcuts(install_dir, app_name);
}

static void on_before_uninstall(void* user_data, const char* app_version)
{
    std::wstring app_name = get_app_name();

    unregister_protocol_handler(PROTOCOL_SECONDLIFE);
    unregister_protocol_handler(PROTOCOL_GRID_INFO);
    unregister_uninstall_info();
    remove_shortcuts(app_name);
}

static void on_log_message(void* user_data, const char* level, const char* message)
{
    OutputDebugStringA("[Velopack] ");
    OutputDebugStringA(level);
    OutputDebugStringA(": ");
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
}

#elif LL_DARWIN

// macOS-specific hooks
// TODO: Implement protocol handler registration via Launch Services
// TODO: Implement app bundle management

static void on_after_install(void* user_data, const char* app_version)
{
    // macOS handles protocol registration via Info.plist CFBundleURLTypes
    // No additional registration needed at runtime
    LL_INFOS("Velopack") << "macOS post-install hook called for version: " << app_version << LL_ENDL;
}

static void on_before_uninstall(void* user_data, const char* app_version)
{
    LL_INFOS("Velopack") << "macOS pre-uninstall hook called for version: " << app_version << LL_ENDL;
}

static void on_log_message(void* user_data, const char* level, const char* message)
{
    LL_INFOS("Velopack") << "[" << level << "] " << message << LL_ENDL;
}

#endif // LL_WINDOWS / LL_DARWIN

//
// Common progress callback
//

static void on_progress(void* user_data, size_t progress)
{
    if (updater().mProgressCallback)
    {
        updater().mProgressCallback(static_cast<int>(progress));
    }
}

static void on_vpk_log(void* p_user_data,
    const char* psz_level,
    const char* psz_message)
{
    LL_DEBUGS("Velopack") << ll_safe_string(psz_message) << LL_ENDL;
}

static int compare_running_version(const std::string& vvm_version)
{
    S32 major = 0, minor = 0, patch = 0;
    U64 build = 0;
    sscanf(vvm_version.c_str(), "%d.%d.%d.%llu", &major, &minor, &patch, &build);

    const LLVersionInfo& vi = LLVersionInfo::instance();
    if (vi.getMajor() != major) return vi.getMajor() < major ? -1 : 1;
    if (vi.getMinor() != minor) return vi.getMinor() < minor ? -1 : 1;
    if (vi.getPatch() != patch) return vi.getPatch() < patch ? -1 : 1;
    if (vi.getBuild() != build) return vi.getBuild() < build ? -1 : 1;
    return 0;
}

//
// Public API - Cross-platform
//

bool velopack_initialize()
{
    vpkc_set_logger(on_log_message, nullptr);
    vpkc_app_set_auto_apply_on_startup(false);

#if LL_WINDOWS || LL_DARWIN
    vpkc_app_set_hook_after_install(on_after_install);
    vpkc_app_set_hook_before_uninstall(on_before_uninstall);
#endif

    vpkc_app_run(nullptr);
    return true;
}

static void join_background_stage_thread()
{
    if (updater().mBackgroundStageThread.joinable())
    {
        updater().mBackgroundStageThread.join();
    }
}

static bool try_begin_update_job()
{
    if (updater().mBackgroundStageActive)
    {
        LL_INFOS("Velopack") << "Update staging already in progress, skipping duplicate request" << LL_ENDL;
        return false;
    }

    updater().mBackgroundStageActive = true;
    return true;
}

static void finish_update_job(const VelopackDownloadRequest& request, bool success)
{
    join_background_stage_thread();
    updater().mBackgroundStageActive = false;

    if (request.show_progress_notification)
    {
        dismiss_downloading_notification();
    }

    if (success && request.quit_when_ready && velopack_is_update_pending())
    {
        LL_INFOS("Velopack")
            << (request.required_update ? "Required" : "Optional")
            << " update downloaded, quitting to apply" << LL_ENDL;
        velopack_request_restart_after_update();
        LLAppViewer::instance()->requestQuit();
        return;
    }

    if (!success && request.required_update)
    {
        LL_WARNS("Velopack") << "Required update did not finish staging, returning to prompt" << LL_ENDL;
        show_required_update_prompt();
    }
}

static bool pre_download_assets(const std::vector<vpkc_asset_t*>& assets)
{
    for (vpkc_asset_t* target_asset : assets)
    {
        std::string asset_filename = target_asset->FileName ? target_asset->FileName : "";
        std::string safe_asset_filename = sanitize_asset_filename(asset_filename);
        if (safe_asset_filename.empty())
        {
            LL_WARNS("Velopack") << "Rejecting unsafe update asset filename: " << asset_filename << LL_ENDL;
            return false;
        }

        if (updater().mPreDownloadedAssets.count(safe_asset_filename))
        {
            continue;
        }

        std::string asset_url;
        auto url_it = updater().mAssetUrlMap.find(safe_asset_filename);
        if (url_it != updater().mAssetUrlMap.end())
        {
            asset_url = url_it->second;
        }
        else
        {
            std::string base = updater().mUpdateUrl;
            if (!base.empty() && base.back() == '/')
            {
                base.pop_back();
            }
            asset_url = base + "/" + safe_asset_filename;
        }

        std::string local_path = gDirUtilp->getExpandedFilename(LL_PATH_TEMP, safe_asset_filename);
        LL_INFOS("Velopack") << "Pre-downloading " << ll_safe_string(target_asset->Type)
                             << " asset: " << asset_url << " to " << local_path << LL_ENDL;

        if (!download_url_to_file(asset_url, local_path))
        {
            LL_WARNS("Velopack") << "Failed to pre-download update asset: " << safe_asset_filename << LL_ENDL;
            return false;
        }

        updater().mPreDownloadedAssets[safe_asset_filename] = local_path;
    }

    return true;
}

static bool prepare_update_manager(bool allow_downgrade)
{
    if (updater().mUpdateManager)
    {
        return true;
    }

    vpkc_update_options_t options = {};
    options.AllowVersionDowngrade = allow_downgrade;
    options.ExplicitChannel = nullptr;

    if (!updater().mUpdateSource)
    {
        updater().mUpdateSource = vpkc_new_source_custom_callback(
            custom_get_release_feed,
            custom_free_release_feed,
            custom_download_asset,
            nullptr);
    }

    if (!vpkc_new_update_manager_with_source(updater().mUpdateSource, &options, nullptr, &updater().mUpdateManager))
    {
        LL_WARNS("Velopack") << "Failed to create update manager" << LL_ENDL;
        return false;
    }

    return true;
}

static bool query_available_update(bool allow_downgrade, vpkc_update_info_t*& update_info)
{
    update_info = nullptr;

    if (updater().mUpdateUrl.empty())
    {
        LL_DEBUGS("Velopack") << "No update URL set, skipping update check" << LL_ENDL;
        return false;
    }

    if (!prepare_update_manager(allow_downgrade))
    {
        return false;
    }

    vpkc_update_check_t result = vpkc_check_for_updates(updater().mUpdateManager, &update_info);
    if (result != UPDATE_AVAILABLE || !update_info)
    {
        LL_DEBUGS("Velopack") << "No update available (result=" << result << ")" << LL_ENDL;
        return false;
    }

    return true;
}

static bool prepare_update_download(bool allow_downgrade, vpkc_update_info_t*& update_info)
{
    if (!query_available_update(allow_downgrade, update_info))
    {
        return false;
    }

    LL_DEBUGS("Velopack") << "Setting up detailed logging";
    vpkc_set_logger(on_vpk_log, nullptr);
    LL_CONT << LL_ENDL;
    LL_INFOS("Velopack") << "Update available, preparing download..." << LL_ENDL;

    bool has_deltas = update_info->DeltasToTargetCount > 0;
    std::vector<vpkc_asset_t*> first_pass;

    if (has_deltas)
    {
        for (size_t i = 0; i < update_info->DeltasToTargetCount; ++i)
        {
            first_pass.push_back(update_info->DeltasToTarget[i]);
        }
    }
    else
    {
        first_pass.push_back(update_info->TargetFullRelease);
    }

    if (!pre_download_assets(first_pass))
    {
        updater().mPreDownloadedAssets.clear();
        vpkc_free_update_info(update_info);
        update_info = nullptr;
        return false;
    }

    return true;
}

static bool stage_prepared_update(vpkc_update_info_t* update_info)
{
    bool has_deltas = update_info->DeltasToTargetCount > 0;

    LL_INFOS("Velopack") << "Pre-download complete, staging with Velopack in background" << LL_ENDL;
    if (vpkc_download_updates(updater().mUpdateManager, update_info, on_progress, nullptr))
    {
        std::lock_guard<std::mutex> lock(updater().mStateMutex);
        if (updater().mPendingUpdate)
        {
            vpkc_free_update_info(updater().mPendingUpdate);
        }
        updater().mPendingUpdate = update_info;
        LL_INFOS("Velopack") << "Update downloaded and pending" << LL_ENDL;
        return true;
    }

    if (has_deltas && update_info->TargetFullRelease)
    {
        LL_INFOS("Velopack") << "Delta update failed, attempting fallback to full release..." << LL_ENDL;
        if (pre_download_assets({ update_info->TargetFullRelease }) &&
            vpkc_download_updates(updater().mUpdateManager, update_info, on_progress, nullptr))
        {
            std::lock_guard<std::mutex> lock(updater().mStateMutex);
            if (updater().mPendingUpdate)
            {
                vpkc_free_update_info(updater().mPendingUpdate);
            }
            updater().mPendingUpdate = update_info;
            LL_INFOS("Velopack") << "Update downloaded via fallback" << LL_ENDL;
            return true;
        }

        char descr[512];
        vpkc_get_last_error(descr, sizeof(descr));
        LL_WARNS("Velopack") << "Fallback also failed: " << ll_safe_string((const char*)descr) << LL_ENDL;
    }
    else
    {
        char descr[512];
        vpkc_get_last_error(descr, sizeof(descr));
        LL_WARNS("Velopack") << "Update failed: " << ll_safe_string((const char*)descr) << LL_ENDL;
    }

    vpkc_free_update_info(update_info);
    return false;
}

static void start_background_stage(vpkc_update_info_t* update_info, const VelopackDownloadRequest request)
{
    join_background_stage_thread();
    updater().mBackgroundStageThread = std::thread([update_info, request]()
    {
        bool success = stage_prepared_update(update_info);
        if (success && request.quit_when_ready)
        {
            std::lock_guard<std::mutex> lock(updater().mStateMutex);
            updater().mRestartAfterUpdate = true;
        }
        updater().mAssetUrlMap.clear();
        updater().mPreDownloadedAssets.clear();

        LLAppViewer::instance()->postToMainCoro([request, success]()
        {
            finish_update_job(request, success);
        });
    });
}

static void velopack_download_update(const VelopackDownloadRequest& request)
{
    if (!try_begin_update_job())
    {
        return;
    }

    if (request.show_progress_notification)
    {
        show_downloading_notification(request.version);
    }

    LLCoros::instance().launch(request.required_update ? "VelopackRequiredStagePrep" : "VelopackOptionalStagePrep",
        [request]()
        {
            vpkc_update_info_t* update_info = nullptr;
            if (!prepare_update_download(request.allow_downgrade, update_info))
            {
                finish_update_job(request, false);
                return;
            }

            start_background_stage(update_info, request);
        });
}

static void on_downloading_closed(const LLSD& notification, const LLSD& response)
{
    updater().mDownloadingNotification = nullptr;
    std::string version;
    {
        std::lock_guard<std::mutex> lock(updater().mStateMutex);
        if (!updater().mRequiredUpdateInProgress)
        {
            return;
        }
        version = updater().mRequiredUpdateVersion;
    }

    if (!version.empty())
    {
        // User closed the downloading dialog during a required update — re-show it
        show_downloading_notification(version);
    }
}

static void show_downloading_notification(const std::string& version)
{
    LLSD args;
    args["VERSION"] = version;
    updater().mDownloadingNotification = LLNotificationsUtil::add("DownloadingUpdate", args, LLSD(), on_downloading_closed);
}

static void dismiss_downloading_notification()
{
    if (updater().mDownloadingNotification)
    {
        LLNotificationsUtil::cancel(updater().mDownloadingNotification);
        updater().mDownloadingNotification = nullptr;
    }
}

static void on_required_update_response(const LLSD& notification, const LLSD& response)
{
    std::string version = notification["substitutions"]["VERSION"].asString();
    LL_INFOS("Velopack") << "Required update acknowledged, starting download" << LL_ENDL;
    velopack_download_update({ true, true, true, true, version });
}

static void on_optional_update_response(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0) // "Install"
    {
        std::string version = notification["substitutions"]["VERSION"].asString();
        LL_INFOS("Velopack") << "User accepted optional update, starting download" << LL_ENDL;
        velopack_download_update({ false, true, true, false, version });
    }
    else
    {
        LL_INFOS("Velopack") << "User declined optional update (option=" << option << ")" << LL_ENDL;
    }
}

static void show_required_update_prompt()
{
    LLSD args;
    {
        std::lock_guard<std::mutex> lock(updater().mStateMutex);
        args["VERSION"] = updater().mRequiredUpdateVersion;
        args["URL"] = updater().mRequiredUpdateRelnotes;
    }
    LLNotificationsUtil::add("PauseForUpdate", args, LLSD(), on_required_update_response);
}

void velopack_check_for_updates(const std::string& required_version, const std::string& relnotes_url)
{
    if (updater().mUpdateUrl.empty())
    {
        LL_DEBUGS("Velopack") << "No update URL set, skipping update check" << LL_ENDL;
        return;
    }

    bool has_required = !required_version.empty();
    int ver_cmp = has_required ? compare_running_version(required_version) : 0;
    bool allow_downgrade = ver_cmp > 0;

    vpkc_update_info_t* update_info = nullptr;
    if (!query_available_update(allow_downgrade, update_info))
    {
        return;
    }

    std::string target_version = (update_info &&
                                  update_info->TargetFullRelease &&
                                  update_info->TargetFullRelease->Version)
        ? update_info->TargetFullRelease->Version
        : std::string();
    vpkc_free_update_info(update_info);

    if (target_version.empty())
    {
        LL_WARNS("Velopack") << "Update feed reported an update without a target version" << LL_ENDL;
        return;
    }

    bool is_required = has_required && ver_cmp < 0;
    if (is_required)
    {
        LL_INFOS("Velopack") << "Required update available from feed: " << target_version
                             << " (required floor " << required_version << ")" << LL_ENDL;
        {
            std::lock_guard<std::mutex> lock(updater().mStateMutex);
            updater().mRequiredUpdateInProgress = true;
            updater().mRequiredUpdateVersion = target_version;
            updater().mRequiredUpdateRelnotes = relnotes_url;
        }
        show_required_update_prompt();
        return;
    }

    U32 updater_setting = gSavedSettings.getU32("UpdaterServiceSetting");
    if (updater_setting == 0)
    {
        LL_INFOS("Velopack") << "Optional update to version " << target_version
                             << " skipped (UpdaterServiceSetting=0)" << LL_ENDL;
        return;
    }

    if (updater_setting == 3)
    {
        LL_INFOS("Velopack") << "Optional update to version " << target_version
                             << ", downloading automatically (UpdaterServiceSetting=3)" << LL_ENDL;
        velopack_download_update({ false, false, false, false, target_version });
        return;
    }

    LL_INFOS("Velopack") << "Optional update available (version " << target_version << "), prompting user" << LL_ENDL;
    LLSD args;
    args["VERSION"] = target_version;
    args["URL"] = relnotes_url;
    LLNotificationsUtil::add("PromptOptionalUpdate", args, LLSD(), on_optional_update_response);
}

std::string velopack_get_current_version()
{
    if (!updater().mUpdateManager)
    {
        return "";
    }

    char version[64];
    size_t len = vpkc_get_current_version(updater().mUpdateManager, version, sizeof(version));
    if (len > 0)
    {
        return std::string(version, len);
    }
    return "";
}

bool velopack_is_update_pending()
{
    std::lock_guard<std::mutex> lock(updater().mStateMutex);
    return updater().mPendingUpdate != nullptr;
}

bool velopack_is_required_update_in_progress()
{
    std::lock_guard<std::mutex> lock(updater().mStateMutex);
    return updater().mRequiredUpdateInProgress;
}

std::string velopack_get_required_update_version()
{
    std::lock_guard<std::mutex> lock(updater().mStateMutex);
    return updater().mRequiredUpdateVersion;
}

bool velopack_should_restart_after_update()
{
    std::lock_guard<std::mutex> lock(updater().mStateMutex);
    return updater().mRestartAfterUpdate;
}

void velopack_request_restart_after_update()
{
    std::lock_guard<std::mutex> lock(updater().mStateMutex);
    updater().mRestartAfterUpdate = true;
}

void velopack_wait_for_background_update()
{
    join_background_stage_thread();
    updater().mBackgroundStageActive = false;
}

void velopack_apply_pending_update(bool restart)
{
    vpkc_update_info_t* pending_update = nullptr;
    {
        std::lock_guard<std::mutex> lock(updater().mStateMutex);
        pending_update = updater().mPendingUpdate;
    }

    if (!updater().mUpdateManager || !pending_update || !pending_update->TargetFullRelease)
    {
        LL_WARNS("Velopack") << "Cannot apply update: no pending update or manager" << LL_ENDL;
        return;
    }

    LL_INFOS("Velopack") << "Applying pending update (restart=" << restart << ")" << LL_ENDL;
    vpkc_wait_exit_then_apply_updates(updater().mUpdateManager,
                                       pending_update->TargetFullRelease,
                                       false,
                                       restart,
                                       nullptr, 0);
}

void velopack_cleanup()
{
    velopack_wait_for_background_update();

    if (updater().mUpdateManager)
    {
        vpkc_free_update_manager(updater().mUpdateManager);
        updater().mUpdateManager = nullptr;
    }
    if (updater().mUpdateSource)
    {
        vpkc_free_source(updater().mUpdateSource);
        updater().mUpdateSource = nullptr;
    }
    {
        std::lock_guard<std::mutex> lock(updater().mStateMutex);
        if (updater().mPendingUpdate)
        {
            vpkc_free_update_info(updater().mPendingUpdate);
            updater().mPendingUpdate = nullptr;
        }
        updater().mRestartAfterUpdate = false;
        updater().mRequiredUpdateInProgress = false;
        updater().mRequiredUpdateVersion.clear();
        updater().mRequiredUpdateRelnotes.clear();
    }
    updater().mAssetUrlMap.clear();
    updater().mPreDownloadedAssets.clear();
}

void velopack_set_update_url(const std::string& url)
{
    updater().mUpdateUrl = url;
    LL_INFOS("Velopack") << "Update URL set to: " << url << LL_ENDL;
}

void velopack_set_progress_callback(std::function<void(int)> callback)
{
    updater().mProgressCallback = callback;
}

#endif // LL_VELOPACK
