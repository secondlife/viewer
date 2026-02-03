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
static vpkc_update_info_t* sPendingUpdate = nullptr;

//
// Platform-specific helpers and hooks
//

#if LL_WINDOWS

static const wchar_t* PROTOCOL_SECONDLIFE = L"secondlife";
static const wchar_t* PROTOCOL_GRID_INFO = L"x-grid-location-info";
static const wchar_t* VIEWER_EXE_NAME = L"SecondLifeViewer.exe";

static std::wstring get_install_dir()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    return path;
}

static std::wstring get_app_name()
{
    // LL_VIEWER_CHANNEL is defined at compile time via CMake (e.g., "Second Life Test")
    return LL_TO_WSTRING(LL_VIEWER_CHANNEL);
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
        std::wstring exe_path = install_dir + L"\\" + VIEWER_EXE_NAME;
        std::wstring uninstall_cmd = L"\"" + install_dir + L"\\Update.exe\" --uninstall";

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
    std::wstring exe_path = install_dir + L"\\" + VIEWER_EXE_NAME;
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
    std::wstring exe_path = install_dir + L"\\" + VIEWER_EXE_NAME;

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
    if (sProgressCallback)
    {
        sProgressCallback(static_cast<int>(progress));
    }
}

//
// Public API - Cross-platform
//

bool velopack_initialize()
{
    vpkc_set_logger(on_log_message, nullptr);

#if LL_WINDOWS || LL_DARWIN
    vpkc_app_set_hook_after_install(on_after_install);
    vpkc_app_set_hook_before_uninstall(on_before_uninstall);
#endif

    vpkc_app_run(nullptr);
    return true;
}

void velopack_check_for_updates()
{
    if (sUpdateUrl.empty())
    {
        LL_DEBUGS("Velopack") << "No update URL set, skipping update check" << LL_ENDL;
        return;
    }

    if (!sUpdateManager)
    {
        vpkc_update_options_t options = {};
        options.AllowVersionDowngrade = false;
        options.ExplicitChannel = nullptr;

        if (!vpkc_new_update_manager(sUpdateUrl.c_str(), &options, nullptr, &sUpdateManager))
        {
            LL_WARNS("Velopack") << "Failed to create update manager" << LL_ENDL;
            return;
        }
    }

    vpkc_update_info_t* update_info = nullptr;
    vpkc_update_check_t result = vpkc_check_for_updates(sUpdateManager, &update_info);

    if (result == UPDATE_AVAILABLE && update_info)
    {
        LL_INFOS("Velopack") << "Update available, downloading..." << LL_ENDL;
        if (vpkc_download_updates(sUpdateManager, update_info, on_progress, nullptr))
        {
            if (sPendingUpdate)
            {
                vpkc_free_update_info(sPendingUpdate);
            }
            sPendingUpdate = update_info;
            LL_INFOS("Velopack") << "Update downloaded and pending" << LL_ENDL;
        }
        else
        {
            LL_WARNS("Velopack") << "Failed to download update" << LL_ENDL;
            vpkc_free_update_info(update_info);
        }
    }
    else
    {
        LL_DEBUGS("Velopack") << "No update available (result=" << result << ")" << LL_ENDL;
    }
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
