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
#include <unordered_map>
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

// Common state
static std::string sUpdateUrl;
static std::function<void(int)> sProgressCallback;
static vpkc_update_manager_t* sUpdateManager = nullptr;
static vpkc_update_info_t* sPendingUpdate = nullptr;        // Downloaded, ready to apply
static vpkc_update_info_t* sPendingCheckInfo = nullptr;     // Checked, awaiting user response
static vpkc_update_source_t* sUpdateSource = nullptr;
static LLNotificationPtr sDownloadingNotification;
static bool sRestartAfterUpdate = false;
static bool sIsRequired = false;                             // Is the pending check a required update?
static std::string sReleaseNotesUrl;
static std::string sTargetVersion;                           // Velopack's actual target version

// Forward declarations
static void show_required_update_prompt();
static void show_downloading_notification(const std::string& version);
static void ensure_update_manager(bool allow_downgrade);
static void velopack_download_pending_update();
static std::unordered_map<std::string, std::string> sAssetUrlMap; // basename -> original absolute URL

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
                sAssetUrlMap[basename] = filename;
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
    std::string base = sUpdateUrl;
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

static std::string sPreDownloadedAssetPath;

static bool custom_download_asset(void* user_data,
                                   const vpkc_asset_t* asset,
                                   const char* local_path,
                                   size_t progress_callback_id)
{
    // The asset has already been downloaded at the coroutine level (before vpkc_download_updates).
    // This callback just copies the pre-downloaded file to where Velopack expects it.
    // We cannot use getRawAndSuspend here — coroutine context is lost through the Rust FFI boundary.
    if (sPreDownloadedAssetPath.empty())
    {
        LL_WARNS("Velopack") << "No pre-downloaded asset available" << LL_ENDL;
        return false;
    }

    std::string filename = asset->FileName ? asset->FileName : "";
    LL_INFOS("Velopack") << "Download asset callback: filename=" << filename
        << " local_path=" << local_path
        << " size=" << asset->Size << LL_ENDL;
    vpkc_source_report_progress(progress_callback_id, 0);

    std::ifstream src(sPreDownloadedAssetPath, std::ios::binary);
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
    // Match viewer_manifest.py app_name() logic: release channel uses "Viewer"
    // suffix instead of "Release" for display purposes (shortcuts, uninstall, etc.)
    std::wstring channel = LL_TO_WSTRING(LL_VIEWER_CHANNEL);
    std::wstring release_suffix = L" Release";
    if (channel.size() >= release_suffix.size() &&
        channel.compare(channel.size() - release_suffix.size(), release_suffix.size(), release_suffix) == 0)
    {
        channel.replace(channel.size() - release_suffix.size(), release_suffix.size(), L" Viewer");
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

static HRESULT create_shortcut(const std::wstring& shortcut_path,
                            const std::wstring& target_path,
                            const std::wstring& arguments,
                            const std::wstring& description,
                            const std::wstring& icon_path)
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) return hr;

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
    return hr;
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

static bool get_shortcut_target(const std::wstring& lnk_path, std::wstring& target_path_str)
{
    // Resolve the shortcut to check its target
    wchar_t target_path[MAX_PATH] = { 0 };
    HRESULT hr = CoInitialize(NULL);
    bool res = false;
    if (SUCCEEDED(hr))
    {
        IShellLinkW* psl = nullptr;
        hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl);
        if (SUCCEEDED(hr))
        {
            IPersistFile* ppf = nullptr;
            hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
            if (SUCCEEDED(hr))
            {
                hr = ppf->Load(lnk_path.c_str(), STGM_READ);
                if (SUCCEEDED(hr))
                {
                    // Resolve() in theory may retarget to a different executable,
                    // but we should be fine as long as folder stays the same,
                    // otherwise we shouldn't clear it.
                    hr = psl->Resolve(NULL, SLR_NO_UI | SLR_NOUPDATE | SLR_ANY_MATCH);
                    if (SUCCEEDED(hr))
                    {
                        hr = psl->GetPath(target_path, MAX_PATH, nullptr, 0);
                        if (SUCCEEDED(hr))
                        {
                            target_path_str = std::wstring(target_path);
                            res = true;
                        }
                    }
                }
                ppf->Release();
            }
            psl->Release();
        }
        CoUninitialize();
    }
    return res;
}

static bool paths_are_equal(const std::wstring& path1, const std::wstring& path2)
{
    try
    {
        std::error_code ec1, ec2;
        std::filesystem::path p1(path1);
        std::filesystem::path p2(path2);

        // Get canonical (absolute, normalized) paths
        auto canonical1 = std::filesystem::canonical(p1, ec1);
        auto canonical2 = std::filesystem::canonical(p2, ec2);

        // If either path doesn't exist, fall back to string comparison
        if (ec1 || ec2)
        {
            // Normalize case for comparison
            std::wstring lower1 = path1;
            std::wstring lower2 = path2;
            std::transform(lower1.begin(), lower1.end(), lower1.begin(), ::towlower);
            std::transform(lower2.begin(), lower2.end(), lower2.begin(), ::towlower);
            while (!lower1.empty() && (lower1.back() == L'\\' || lower1.back() == L'/'))
            {
                lower1.pop_back();
            }
            while (!lower2.empty() && (lower2.back() == L'\\' || lower2.back() == L'/'))
            {
                lower2.pop_back();
            }
            return lower1 == lower2;
        }

        // Use filesystem::equivalent which handles all path variations
        return std::filesystem::equivalent(canonical1, canonical2, ec1);
    }
    catch (const std::filesystem::filesystem_error&)
    {
        // Fallback to case-insensitive string comparison
        std::wstring lower1 = path1;
        std::wstring lower2 = path2;
        std::transform(lower1.begin(), lower1.end(), lower1.begin(), ::towlower);
        std::transform(lower2.begin(), lower2.end(), lower2.begin(), ::towlower);
        while (!lower1.empty() && (lower1.back() == L'\\' || lower1.back() == L'/'))
        {
            lower1.pop_back();
        }
        while (!lower2.empty() && (lower2.back() == L'\\' || lower2.back() == L'/'))
        {
            lower2.pop_back();
        }
        return lower1 == lower2;
    }
}

void clear_nsis_links(const std::string& nsis_folder_path)
{
    wchar_t path[MAX_PATH];
    std::wstring app_name = get_app_name();

    // 1. The 'start' shortcuts set by nsis would be global, like app shortcut:
    // C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Second Life Viewer\Second Life Viewer.lnk
    // But it isn't just one link, it's a whole directory that needs to be removed.
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, path)))
    {
        std::wstring start_menu_path = path;
        std::wstring folder_path   = start_menu_path + L"\\" + app_name;

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

    // 2. Desktop link, also a global one.
    // C:\Users\Public\Desktop\Second Life Viewer.lnk
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, 0, path)))
    {
        std::wstring desktop_path = path;
        std::wstring shortcut_path = desktop_path + L"\\" + app_name + L".lnk";
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

    // 3. Taskbar link, which is user-specific and located at:
    // %AppData%\Microsoft\Internet Explorer\Quick Launch\User Pinned\TaskBar\Second Life.lnk
    // Note that it can be a link to velopack already, but since
    // we aren't removing, but recreating, it shouldn't be an issue.
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path)))
    {
        std::wstring taskbar_path = path;
        // Hardcoded, because this name is the default window name and isn't going to change in NSIS
        taskbar_path += L"\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\Second Life.lnk";

        if (PathFileExistsW(taskbar_path.c_str()))
        {
            // First try to overwrite shortcut
            std::wstring current_exe = ll_convert<std::wstring>(gDirUtilp->getExecutablePathAndName());

            HRESULT hr = create_shortcut(taskbar_path, current_exe, L"", app_name, current_exe);
            if (FAILED(hr))
            {
                LL_WARNS("Velopack") << "Failed to update taskbar shortcut, error " << std::hex << hr << LL_ENDL;
                // Try deleting and if possible recreating separately. We should not leave hanging shortcuts behind.
                // It is not warranted that the shortcut points to NSIS, so check the destination first.
                std::wstring target_path;
                if (get_shortcut_target(taskbar_path, target_path))
                {
                    // Strip the filename part to get just the folder path
                    std::filesystem::path trim_path(target_path);
                    if (trim_path.has_filename())
                    {
                        target_path = trim_path.parent_path().wstring();
                    }
                    std::wstring nsis_path_w = ll_convert<std::wstring>(nsis_folder_path);
                    if (paths_are_equal(nsis_path_w, target_path))
                    {
                        if (!DeleteFileW(taskbar_path.c_str()))
                        {
                            DWORD error = GetLastError();
                            if (error != ERROR_FILE_NOT_FOUND)
                            {
                                LL_WARNS("Velopack") << "Failed to delete NSIS taskbar shortcut: "
                                    << ll_convert_wide_to_string(taskbar_path)
                                    << " (error: " << error << ")" << LL_ENDL;
                            }
                        }
                        else
                        {
                            // Deleted user created link, recreate it with new target if possible.
                            LL_INFOS("Velopack") << "Updating taskbar shortcut to point to current exe" << LL_ENDL;
                            HRESULT hr = create_shortcut(taskbar_path, current_exe, L"", app_name, current_exe);
                            if (FAILED(hr))
                            {
                                LL_WARNS("Velopack") << "Failed to re-create taskbar shortcut, error: " << std::hex << hr << ", NSIS shortcut was removed" << LL_ENDL;
                            }
                        }
                    }
                }
            }
            else
            {
                LL_INFOS("Velopack") << "Successfully updated taskbar shortcut to point to current exe" << LL_ENDL;
            }
        }
        // else user didn't create a taskbar shortcut.
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

bool get_nsis_version(
    int& nsis_major,
    int& nsis_minor,
    int& nsis_patch,
    uint64_t& nsis_build,
    std::string& nsis_path)
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

    parse_version(version_buf, nsis_major, nsis_minor, nsis_patch, nsis_build);

    // Make sure it actually exists and not a dead entry.
    wchar_t path_buffer[MAX_PATH] = { 0 };
    DWORD path_buf_size = sizeof(path_buffer);
    LONG rv = RegGetValueW(hkey, nullptr, L"UninstallString", RRF_RT_REG_SZ, &type, path_buffer, &path_buf_size);
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
    // check if uninstaller exists
    if (!std::filesystem::exists(path, ec))
    {
        return false;
    }

    // We found a valid NSIS installation, trim the path to get the folder
    wchar_t* pos = wcsstr(path_buffer, L"uninst.exe");
    if (pos)
    {
        // Trim uninst.exe from the path by setting null terminator before it
        *pos = L'\0';
    }
    // Note that it has a trailing backslash.
    nsis_path = ll_convert_wide_to_string(path_buffer);

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
    // Clears velopack's recently created 'uninstall' registry entry.
    // We are going to use a custom one.
    // Note that velopack doesn't know about our custom entry.
    std::wstring key_path = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + app_name_oneword;
    RegDeleteTreeW(HKEY_CURRENT_USER, key_path.c_str());
    // Use a unique key name to avoid conflicts with any existing NSIS-based uninstall info,
    // which can cause only one of the two entries to show up in the Add/Remove Programs list.
    // The UI will show DisplayName, so the key name itself is not important to be user-friendly.
    key_path = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Vlpk" + app_name_oneword;
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

        link_url = L"https://secondlife.com/support/downloads/";
        RegSetValueExW(hkey, L"URLUpdateInfo", 0, REG_SZ,
            (BYTE*)link_url.c_str(), (DWORD)((link_url.size() + 1) * sizeof(wchar_t)));

        DWORD no_modify = 1;
        RegSetValueExW(hkey, L"NoModify", 0, REG_DWORD, (BYTE*)&no_modify, sizeof(DWORD));
        RegSetValueExW(hkey, L"NoRepair", 0, REG_DWORD, (BYTE*)&no_modify, sizeof(DWORD));

        // Format YYYYMMDD
        wchar_t dateStr[9];
        time_t t = time(NULL);
        struct tm tm;
        localtime_s(&tm, &t);
        wcsftime(dateStr, 9, L"%Y%m%d", &tm);
        RegSetValueExW(hkey, L"InstallDate", 0, REG_SZ, (BYTE*)dateStr, (DWORD)((wcslen(dateStr) + 1) * sizeof(wchar_t))); // Let Windows fill in the install date

        // 800 MB, inaccurate, but for a rough idea.
        // We can check folder size here, but it would take time and
        // information is of low importance.
        DWORD estimated_size = 800000;
        RegSetValueExW(hkey, L"EstimatedSize", 0, REG_DWORD, (BYTE*)&estimated_size, sizeof(DWORD));

        RegCloseKey(hkey);
    }
}

static void unregister_uninstall_info()
{
    std::wstring app_name_oneword = get_app_name_oneword();
    std::wstring key_path = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Vlpk" + app_name_oneword;
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

static void on_first_run(void* p_user_data, const char* app_version)
{
    // Velopack first executes 'after install' hook, then writes registry,
    // then executes 'on first run' hook.
    // As we need to clear velopack's 'uninstall' registry entry and use
    // our own, clean it here instead of on_after_install.

    std::wstring install_dir = get_install_dir();
    std::wstring app_name = get_app_name();

    int len = MultiByteToWideChar(CP_UTF8, 0, app_version, -1, NULL, 0);
    std::wstring version(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, app_version, -1, &version[0], len);

    register_uninstall_info(install_dir, app_name, version);
}

static void on_after_install(void* user_data, const char* app_version)
{
    std::wstring install_dir = get_install_dir();
    std::wstring app_name = get_app_name();
    std::wstring exe_path = install_dir + L"\\" + get_viewer_exe_name();

    register_protocol_handler(PROTOCOL_SECONDLIFE, L"URL:Second Life", exe_path);
    register_protocol_handler(PROTOCOL_GRID_INFO, L"URL:Second Life", exe_path);
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

static void on_first_run(void* user_data, const char* app_version)
{
}

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
    if (sProgressCallback)
    {
        sProgressCallback(static_cast<int>(progress));
    }
}

static void on_vpk_log(void* p_user_data,
    const char* psz_level,
    const char* psz_message)
{
    LL_DEBUGS("Velopack") << ll_safe_string(psz_message) << LL_ENDL;
}

//
// Version comparison helper
//

// Compare running version against a VVM version string "major.minor.patch.build".
// Returns -1 if running < vvm, 0 if equal, 1 if running > vvm.
static int compare_running_version(const std::string& vvm_version)
{
    S32 major = 0, minor = 0, patch = 0;
    U64 build = 0;
    sscanf(vvm_version.c_str(), "%d.%d.%d.%llu", &major, &minor, &patch, &build);

    const LLVersionInfo& vi = LLVersionInfo::instance();
    S32 cur_major = vi.getMajor();
    S32 cur_minor = vi.getMinor();
    S32 cur_patch = vi.getPatch();
    U64 cur_build = vi.getBuild();

    if (cur_major != major) return cur_major < major ? -1 : 1;
    if (cur_minor != minor) return cur_minor < minor ? -1 : 1;
    if (cur_patch != patch) return cur_patch < patch ? -1 : 1;
    if (cur_build != build) return cur_build < build ? -1 : 1;
    return 0;
}

//
// Update manager lifecycle
//

static void ensure_update_manager(bool allow_downgrade)
{
    if (sUpdateManager)
        return;

    vpkc_update_options_t options = {};
    options.AllowVersionDowngrade = allow_downgrade;
    options.ExplicitChannel = nullptr;

    if (!sUpdateSource)
    {
        sUpdateSource = vpkc_new_source_custom_callback(
            custom_get_release_feed,
            custom_free_release_feed,
            custom_download_asset,
            nullptr);
    }

    vpkc_locator_config_t* locator_ptr = nullptr;

#if LL_DARWIN
    // Try auto-detection first (works when the app bundle was packaged by vpk
    // and has UpdateMac + sq.version already present)
    if (!vpkc_new_update_manager_with_source(sUpdateSource, &options, nullptr, &sUpdateManager))
    {
        char err[512];
        vpkc_get_last_error(err, sizeof(err));
        LL_INFOS("Velopack") << "Auto-detect failed (" << ll_safe_string(err)
                             << "), falling back to explicit locator" << LL_ENDL;

        // Auto-detection failed — construct an explicit locator.
        // This handles legacy DMG installs that don't have Velopack's
        // install state (UpdateMac, sq.version) in the bundle.
        vpkc_locator_config_t locator = {};

        // The executable lives at <bundle>/Contents/MacOS/<exe>
        // The app bundle root is two levels up from the executable directory.
        std::string exe_dir = gDirUtilp->getExecutableDir();
        std::string bundle_root = exe_dir + "/../..";
        char resolved[PATH_MAX];
        if (realpath(bundle_root.c_str(), resolved))
        {
            bundle_root = resolved;
        }

        // Construct a version string in Velopack SemVer format: major.minor.patch-build
        const LLVersionInfo& vi = LLVersionInfo::instance();
        std::string current_version = llformat("%d.%d.%d-%llu",
            vi.getMajor(), vi.getMinor(), vi.getPatch(), vi.getBuild());

        // Create a minimal sq.version manifest so Velopack knows our version.
        // Proper vpk-packaged builds have this in the bundle already.
        std::string manifest_path = gDirUtilp->getExpandedFilename(LL_PATH_TEMP, "sq.version");
        {
            std::string app_name = LLVersionInfo::instance().getChannel();
            std::string pack_id = app_name;
            pack_id.erase(std::remove(pack_id.begin(), pack_id.end(), ' '), pack_id.end());

            std::string nuspec = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                "<package xmlns=\"http://schemas.microsoft.com/packaging/2010/07/nuspec.xsd\">\n"
                "  <metadata>\n"
                "    <id>" + pack_id + "</id>\n"
                "    <version>" + current_version + "</version>\n"
                "    <title>" + app_name + "</title>\n"
                "  </metadata>\n"
                "</package>\n";

            llofstream manifest_file(manifest_path, std::ios::trunc);
            if (manifest_file.is_open())
            {
                manifest_file << nuspec;
                manifest_file.close();
            }
        }

        std::string packages_dir = gDirUtilp->getExpandedFilename(LL_PATH_TEMP, "velopack-packages");
        LLFile::mkdir(packages_dir);

        locator.RootAppDir = const_cast<char*>(bundle_root.c_str());
        locator.CurrentBinaryDir = const_cast<char*>(exe_dir.c_str());
        locator.ManifestPath = const_cast<char*>(manifest_path.c_str());
        locator.PackagesDir = const_cast<char*>(packages_dir.c_str());
        locator.UpdateExePath = nullptr;
        locator.IsPortable = false;

        locator_ptr = &locator;

        LL_INFOS("Velopack") << "Explicit locator: RootAppDir=" << bundle_root
                             << " CurrentBinaryDir=" << exe_dir
                             << " Version=" << current_version << LL_ENDL;

        if (!vpkc_new_update_manager_with_source(sUpdateSource, &options, locator_ptr, &sUpdateManager))
        {
            char err2[512];
            vpkc_get_last_error(err2, sizeof(err2));
            LL_WARNS("Velopack") << "Failed to create update manager: " << ll_safe_string(err2) << LL_ENDL;
        }
    }
    return;
#endif

    // Windows: Velopack auto-detection works because the viewer is installed
    // by Velopack's Setup.exe which creates the proper install structure.
    if (!vpkc_new_update_manager_with_source(sUpdateSource, &options, nullptr, &sUpdateManager))
    {
        char err[512];
        vpkc_get_last_error(err, sizeof(err));
        LL_WARNS("Velopack") << "Failed to create update manager: " << ll_safe_string(err) << LL_ENDL;
    }
}

//
// Public API - Cross-platform
//

bool velopack_initialize()
{
    vpkc_set_logger(on_log_message, nullptr);
    vpkc_app_set_auto_apply_on_startup(false);

#if LL_WINDOWS || LL_DARWIN
    vpkc_app_set_hook_first_run(on_first_run);
    vpkc_app_set_hook_after_install(on_after_install);
    vpkc_app_set_hook_before_uninstall(on_before_uninstall);
#endif

    vpkc_app_run(nullptr);
    return true;
}

// Downloads the update that was found during the check phase.
// Operates on sPendingCheckInfo which was set by velopack_check_for_updates.
static void velopack_download_pending_update()
{
    if (!sUpdateManager || !sPendingCheckInfo)
    {
        LL_WARNS("Velopack") << "No pending check info to download" << LL_ENDL;
        return;
    }

    LL_DEBUGS("Velopack") << "Setting up detailed logging";
    vpkc_set_logger(on_vpk_log, nullptr);
    LL_CONT << LL_ENDL;
    LL_INFOS("Velopack") << "Downloading update..." << LL_ENDL;

    // Pre-download the nupkg at the coroutine level where getRawAndSuspend works.
    // The download callback inside the Rust FFI cannot use coroutine HTTP.
    std::string asset_filename = sPendingCheckInfo->TargetFullRelease->FileName
        ? sPendingCheckInfo->TargetFullRelease->FileName : "";
    std::string asset_url;
    auto url_it = sAssetUrlMap.find(asset_filename);
    if (url_it != sAssetUrlMap.end())
    {
        asset_url = url_it->second;
    }
    else
    {
        std::string base = sUpdateUrl;
        if (!base.empty() && base.back() == '/')
            base.pop_back();
        asset_url = base + "/" + asset_filename;
    }

    sPreDownloadedAssetPath = gDirUtilp->getExpandedFilename(LL_PATH_TEMP, asset_filename);
    LL_INFOS("Velopack") << "Pre-downloading " << asset_url
        << " to " << sPreDownloadedAssetPath << LL_ENDL;

    if (!download_url_to_file(asset_url, sPreDownloadedAssetPath))
    {
        LL_WARNS("Velopack") << "Failed to pre-download update asset" << LL_ENDL;
        sPreDownloadedAssetPath.clear();
        return;
    }

    LL_INFOS("Velopack") << "Pre-download complete, handing to Velopack" << LL_ENDL;
    if (vpkc_download_updates(sUpdateManager, sPendingCheckInfo, on_progress, nullptr))
    {
        if (sPendingUpdate)
        {
            vpkc_free_update_info(sPendingUpdate);
        }
        sPendingUpdate = sPendingCheckInfo;
        sPendingCheckInfo = nullptr; // Ownership transferred
        LL_INFOS("Velopack") << "Update downloaded and pending" << LL_ENDL;
    }
    else
    {
        char descr[512];
        vpkc_get_last_error(descr, sizeof(descr));
        LL_WARNS("Velopack") << "Failed to download update: " << ll_safe_string((const char*)descr) << LL_ENDL;
    }
}

static void on_downloading_closed(const LLSD& notification, const LLSD& response)
{
    sDownloadingNotification = nullptr;
    if (sIsRequired)
    {
        // User closed the downloading dialog during a required update — re-show it
        show_downloading_notification(sTargetVersion);
    }
}

static void show_downloading_notification(const std::string& version)
{
    LLSD args;
    args["VERSION"] = version;
    sDownloadingNotification = LLNotificationsUtil::add("DownloadingUpdate", args, LLSD(), on_downloading_closed);
}

static void dismiss_downloading_notification()
{
    if (sDownloadingNotification)
    {
        LLNotificationsUtil::cancel(sDownloadingNotification);
        sDownloadingNotification = nullptr;
    }
}

static void on_required_update_response(const LLSD& notification, const LLSD& response)
{
    LL_INFOS("Velopack") << "Required update acknowledged, starting download" << LL_ENDL;
    show_downloading_notification(sTargetVersion);
    LLCoros::instance().launch("VelopackRequiredUpdate", []()
    {
        velopack_download_pending_update();
        dismiss_downloading_notification();
        if (velopack_is_update_pending())
        {
            LL_INFOS("Velopack") << "Required update downloaded, quitting to apply" << LL_ENDL;
            velopack_request_restart_after_update();
            LLAppViewer::instance()->requestQuit();
        }
    });
}

static void on_optional_update_response(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0) // "Install"
    {
        LL_INFOS("Velopack") << "User accepted optional update, starting download" << LL_ENDL;
        show_downloading_notification(sTargetVersion);
        LLCoros::instance().launch("VelopackOptionalUpdate", []()
        {
            velopack_download_pending_update();
            dismiss_downloading_notification();
            if (velopack_is_update_pending())
            {
                LL_INFOS("Velopack") << "Optional update downloaded, quitting to apply" << LL_ENDL;
                velopack_request_restart_after_update();
                LLAppViewer::instance()->requestQuit();
            }
        });
    }
    else
    {
        LL_INFOS("Velopack") << "User declined optional update (option=" << option << ")" << LL_ENDL;
        // Free the check info since user declined
        if (sPendingCheckInfo)
        {
            vpkc_free_update_info(sPendingCheckInfo);
            sPendingCheckInfo = nullptr;
        }
    }
}

static void show_required_update_prompt()
{
    LLSD args;
    args["VERSION"] = sTargetVersion;
    args["URL"] = sReleaseNotesUrl;
    LLNotificationsUtil::add("PauseForUpdate", args, LLSD(), on_required_update_response);
}

void velopack_check_for_updates(const std::string& required_version, const std::string& relnotes_url)
{
    if (sUpdateUrl.empty())
    {
        LL_DEBUGS("Velopack") << "No update URL set, skipping update check" << LL_ENDL;
        return;
    }

    // Allow downgrades only for rollbacks: VVM requires a version that's
    // strictly lower than what we're running (e.g., a retracted build).
    bool has_required = !required_version.empty();
    int ver_cmp = has_required ? compare_running_version(required_version) : 0;
    bool allow_downgrade = ver_cmp > 0; // running > required → rollback scenario
    ensure_update_manager(allow_downgrade);
    if (!sUpdateManager)
        return;

    // Ask Velopack to check its feed — this is the source of truth
    vpkc_update_info_t* update_info = nullptr;
    vpkc_update_check_t result = vpkc_check_for_updates(sUpdateManager, &update_info);

    if (result != UPDATE_AVAILABLE || !update_info)
    {
        LL_INFOS("Velopack") << "No update available from feed (result=" << result << ")" << LL_ENDL;
        return;
    }

    // Extract the actual target version from Velopack's feed
    std::string target_version = update_info->TargetFullRelease->Version
        ? update_info->TargetFullRelease->Version : "";
    LL_INFOS("Velopack") << "Update available: " << target_version
                         << " (required_version=" << required_version << ")" << LL_ENDL;

    // Store state for the prompt/download phase
    sReleaseNotesUrl = relnotes_url;
    sTargetVersion = target_version;
    if (sPendingCheckInfo)
    {
        vpkc_free_update_info(sPendingCheckInfo);
    }
    sPendingCheckInfo = update_info;

    // Determine if this is mandatory: running version is below VVM's required floor
    bool is_required = ver_cmp < 0; // running < required → must update
    sIsRequired = is_required;

    if (is_required)
    {
        LL_INFOS("Velopack") << "Required update (running below " << required_version
                             << "), prompting user for " << target_version << LL_ENDL;
        show_required_update_prompt();
        return;
    }

    // Optional update — check user preference
    U32 updater_setting = gSavedSettings.getU32("UpdaterServiceSetting");

    if (updater_setting == 3)
    {
        // "Install each update automatically" — download silently, apply on quit
        LL_INFOS("Velopack") << "Optional update to " << target_version
                             << ", downloading automatically (UpdaterServiceSetting=3)" << LL_ENDL;
        velopack_download_pending_update();
        return;
    }

    // Default / value 1: "Ask me when an optional update is ready to install"
    LL_INFOS("Velopack") << "Optional update available (" << target_version << "), prompting user" << LL_ENDL;
    LLSD args;
    args["VERSION"] = target_version;
    args["URL"] = relnotes_url;
    LLNotificationsUtil::add("PromptOptionalUpdate", args, LLSD(), on_optional_update_response);
}

std::string velopack_get_current_version()
{
    if (!sUpdateManager)
    {
        return "";
    }

    char version[64];
    size_t len = vpkc_get_current_version(sUpdateManager, version, sizeof(version));
    if (len > 0)
    {
        return std::string(version, len);
    }
    return "";
}

bool velopack_is_update_pending()
{
    return sPendingUpdate != nullptr;
}

bool velopack_is_required_update_in_progress()
{
    return sIsRequired && sPendingCheckInfo != nullptr;
}

std::string velopack_get_required_update_version()
{
    return sTargetVersion;
}

bool velopack_should_restart_after_update()
{
    return sRestartAfterUpdate;
}

void velopack_request_restart_after_update()
{
    sRestartAfterUpdate = true;
}

void velopack_apply_pending_update(bool restart)
{
    if (!sUpdateManager || !sPendingUpdate || !sPendingUpdate->TargetFullRelease)
    {
        LL_WARNS("Velopack") << "Cannot apply update: no pending update or manager" << LL_ENDL;
        return;
    }

    LL_INFOS("Velopack") << "Applying pending update (restart=" << restart << ")" << LL_ENDL;
    vpkc_wait_exit_then_apply_updates(sUpdateManager,
                                       sPendingUpdate->TargetFullRelease,
                                       false,
                                       restart,
                                       nullptr, 0);
}

void velopack_cleanup()
{
    if (sUpdateManager)
    {
        vpkc_free_update_manager(sUpdateManager);
        sUpdateManager = nullptr;
    }
    if (sUpdateSource)
    {
        vpkc_free_source(sUpdateSource);
        sUpdateSource = nullptr;
    }
    if (sPendingUpdate)
    {
        vpkc_free_update_info(sPendingUpdate);
        sPendingUpdate = nullptr;
    }
    if (sPendingCheckInfo)
    {
        vpkc_free_update_info(sPendingCheckInfo);
        sPendingCheckInfo = nullptr;
    }
    sAssetUrlMap.clear();
}

void velopack_set_update_url(const std::string& url)
{
    sUpdateUrl = url;
    LL_INFOS("Velopack") << "Update URL set to: " << url << LL_ENDL;
}

void velopack_set_progress_callback(std::function<void(int)> callback)
{
    sProgressCallback = callback;
}

#endif // LL_VELOPACK
