/**
 * @file lldirpicker.cpp
 * @brief OS-specific file picker
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "lldirpicker.h"
#include "llworld.h"
#include "llviewerwindow.h"
#include "llkeyboard.h"
#include "lldir.h"
#include "llframetimer.h"
#include "lltrans.h"
#include "llwindow.h"   // beforeDialog()
#include "llviewercontrol.h"
#include "llwin32headers.h"

#if LL_SDL_WINDOW
#include "SDL3/SDL.h"
#endif

#if LL_LINUX || LL_DARWIN
# include "llfilepicker.h"
#endif

#if LL_WINDOWS
#include <shlobj.h>
#endif

//
// Implementation
//

// utility function to check if access to local file system via file browser
// is enabled and if not, tidy up and indicate we're not allowed to do this.
bool LLDirPicker::check_local_file_access_enabled()
{
    // if local file browsing is turned off, return without opening dialog
    bool local_file_system_browsing_enabled = gSavedSettings.getBOOL("LocalFileSystemBrowsingEnabled");
    if ( ! local_file_system_browsing_enabled )
    {
        mDir.clear();   // Windows
        mFileName = NULL; // Mac/Linux
        return false;
    }

    return true;
}

#if LL_SDL_WINDOW

LLDirPicker::LLDirPicker() :
    mFileName(NULL),
    mLocked(false)
{
    reset();
}

LLDirPicker::~LLDirPicker()
{
}

void LLDirPicker::reset()
{
}

bool LLDirPicker::getDir(std::string* filename, bool blocking)
{
    return false;
}

std::string LLDirPicker::getDirName()
{
    return {};
}

bool LLDirPicker::getDirModeless(std::string* filename,
    void (*callback)(bool, std::string&, void*),
    void* userdata)
{
    if (mLocked)
    {
        return false;
    }

    // if local file browsing is turned off, return without opening dialog
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    {
        struct LLSDLFileUserdata
        {
            LLSDLFileUserdata(void (*callback_func)(bool, std::string&, void*), void* callback_userdata)
                : mCallback(callback_func), mUserdata(callback_userdata)
            {
            }
            void (*mCallback)(bool, std::string&, void*);
            void* mUserdata;
        };

        auto sdl_callback = [](void* userdata, const char* const* filelist, int filter)
            {
                LLSDLFileUserdata* callback_struct = (LLSDLFileUserdata*)userdata;

                auto* callback_func = callback_struct->mCallback;
                auto* callback_data = callback_struct->mUserdata;
                delete callback_struct; // delete callback container

                std::string rtn;
                if (!filelist)
                {
                    LL_WARNS() << "Error during SDL folder picking: " << SDL_GetError() << LL_ENDL;
                    callback_func(false, rtn, callback_data);
                    return;
                }
                else if (!*filelist)
                {
                    LL_INFOS() << "User did not select any folders. Dialog likely cancelled." << LL_ENDL;
                    callback_func(false, rtn, callback_data);
                    return;
                }

                while (*filelist) {
                    rtn = std::string(*filelist);
                    break;
                }
                callback_func(true, rtn, callback_data);

            };

        LLSDLFileUserdata* llfilecallback = new LLSDLFileUserdata(callback, userdata);

        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_WINDOW_POINTER, SDL_GL_GetCurrentWindow());
        SDL_ShowFileDialogWithProperties(SDL_FILEDIALOG_OPENFOLDER, sdl_callback, llfilecallback, props);

        SDL_DestroyProperties(props);
    }

    return true;
}


#elif LL_WINDOWS

LLDirPicker::LLDirPicker() :
    mFileName(NULL),
    mLocked(false),
    pDialog(NULL)
{
}

LLDirPicker::~LLDirPicker()
{
    mEventListener.disconnect();
}

void LLDirPicker::reset()
{
    if (pDialog)
    {
        IFileDialog* p_file_dialog = (IFileDialog*)pDialog;
        p_file_dialog->Close(S_FALSE);
        pDialog = NULL;
    }
}

bool LLDirPicker::getDir(std::string* filename, bool blocking)
{
    if (mLocked)
    {
        return false;
    }

    // if local file browsing is turned off, return without opening dialog
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    bool success = false;

    if (blocking)
    {
        // Modal, so pause agent
        send_agent_pause();
    }
    else if (!mEventListener.connected())
    {
        mEventListener = LLEventPumps::instance().obtain("LLApp").listen(
            "DirPicker",
            [this](const LLSD& stat)
            {
                std::string status(stat["status"]);
                if (status != "running")
                {
                    reset();
                }
                return false;
            });
    }

    ::OleInitialize(NULL);

    IFileDialog* p_file_dialog;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&p_file_dialog))))
    {
        DWORD dwOptions;
        if (SUCCEEDED(p_file_dialog->GetOptions(&dwOptions)))
        {
            p_file_dialog->SetOptions(dwOptions | FOS_PICKFOLDERS);
        }
        HWND owner = (HWND)gViewerWindow->getPlatformWindow();
        pDialog = p_file_dialog;
        if (SUCCEEDED(p_file_dialog->Show(owner)))
        {
            IShellItem* psi;
            if (SUCCEEDED(p_file_dialog->GetResult(&psi)))
            {
                wchar_t* pwstr = NULL;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pwstr)))
                {
                    mDir = ll_convert_wide_to_string(pwstr);
                    CoTaskMemFree(pwstr);
                    success = true;
                }
                psi->Release();
            }
        }
        pDialog = NULL;
        p_file_dialog->Release();
    }

    ::OleUninitialize();

    if (blocking)
    {
        send_agent_resume();
    }

    // Account for the fact that the app has been stalled.
    LLFrameTimer::updateFrameTime();
    return success;
}

bool LLDirPicker::getDirModeless(std::string* filename,
    void (*callback)(bool, std::string&, void*),
    void* userdata)
{
    return false;
}

std::string LLDirPicker::getDirName()
{
    return mDir;
}

/////////////////////////////////////////////DARWIN
#elif LL_DARWIN

LLDirPicker::LLDirPicker() :
mFileName(NULL),
mLocked(false)
{
    mFilePicker = new LLFilePicker();
    reset();
}

LLDirPicker::~LLDirPicker()
{
    delete mFilePicker;
}

void LLDirPicker::reset()
{
    if (mFilePicker)
        mFilePicker->reset();
}


bool LLDirPicker::getDir(std::string* filename, bool blocking)
{
    LLFilePicker::ELoadFilter filter=LLFilePicker::FFLOAD_DIRECTORY;

    return mFilePicker->getOpenFile(filter, true);
}

bool LLDirPicker::getDirModeless(std::string* filename,
    void (*callback)(bool, std::string&, void*),
    void* userdata)
{
    return false;
}

std::string LLDirPicker::getDirName()
{
    return mFilePicker->getFirstFile();
}

#elif LL_LINUX

LLDirPicker::LLDirPicker() :
    mFileName(NULL),
    mLocked(false)
{
    mFilePicker = new LLFilePicker();
    reset();
}

LLDirPicker::~LLDirPicker()
{
    delete mFilePicker;
}


void LLDirPicker::reset()
{
    if (mFilePicker)
        mFilePicker->reset();
}

bool LLDirPicker::getDir(std::string* filename, bool blocking)
{
    reset();

    // if local file browsing is turned off, return without opening dialog
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    return false;
}

bool LLDirPicker::getDirModeless(std::string* filename,
    void (*callback)(bool, std::string&, void*),
    void* userdata)
{
    return false;
}

std::string LLDirPicker::getDirName()
{
    if (mFilePicker)
    {
        return mFilePicker->getFirstFile();
    }
    return "";
}

#else // not implemented

LLDirPicker::LLDirPicker()
{
    reset();
}

LLDirPicker::~LLDirPicker()
{
}


void LLDirPicker::reset()
{
}

bool LLDirPicker::getDir(std::string* filename, bool blocking)
{
    return false;
}

bool LLDirPicker::getDirModeless(std::string* filename,
    void (*callback)(bool, std::string&, void*),
    void* userdata)
{
    return false;
}

std::string LLDirPicker::getDirName()
{
    return "";
}

#endif


LLMutex* LLDirPickerThread::sMutex = NULL;
std::queue<LLDirPickerThread*> LLDirPickerThread::sDeadQ;

void LLDirPickerThread::getFile()
{
#if LL_SDL_WINDOW
    runModeless();
#elif LL_WINDOWS
    start();
#else
    run();
#endif
}

//virtual
void LLDirPickerThread::run()
{
#if LL_WINDOWS
    bool blocking = false;
#else
    bool blocking = true; // modal
#endif

    LLDirPicker picker;

    if (picker.getDir(&mProposedName, blocking))
    {
        mResponses.push_back(picker.getDirName());
    }

    {
        LLMutexLock lock(sMutex);
        sDeadQ.push(this);
    }

}

void LLDirPickerThread::runModeless()
{
    LLDirPicker picker;
    bool result = picker.getDirModeless(&mProposedName, modelessStringCallback, this);
    if (!result)
    {
        LLMutexLock lock(sMutex);
        sDeadQ.push(this);
    }
}

void LLDirPickerThread::modelessStringCallback(bool success,
    std::string& response,
    void* user_data)
{
    LLDirPickerThread* picker = (LLDirPickerThread*)user_data;
    {
        LLMutexLock lock(sMutex);
        if (success)
        {
            picker->mResponses.push_back(response);
        }
        sDeadQ.push(picker);
    }
}

//static
void LLDirPickerThread::initClass()
{
    sMutex = new LLMutex();
}

//static
void LLDirPickerThread::cleanupClass()
{
    clearDead();

    delete sMutex;
    sMutex = NULL;
}

//static
void LLDirPickerThread::clearDead()
{
    if (!sDeadQ.empty())
    {
        LLMutexLock lock(sMutex);
        while (!sDeadQ.empty())
        {
            LLDirPickerThread* thread = sDeadQ.front();
            thread->notify(thread->mResponses);
            delete thread;
            sDeadQ.pop();
        }
    }
}

LLDirPickerThread::LLDirPickerThread(const dir_picked_signal_t::slot_type& cb, const std::string &proposed_name)
    : LLThread("dir picker"),
    mFilePickedSignal(NULL)
{
    mFilePickedSignal = new dir_picked_signal_t();
    mFilePickedSignal->connect(cb);
}

LLDirPickerThread::~LLDirPickerThread()
{
    delete mFilePickedSignal;
}

void LLDirPickerThread::notify(const std::vector<std::string>& filenames)
{
    if (!filenames.empty())
    {
        if (mFilePickedSignal)
        {
            (*mFilePickedSignal)(filenames, mProposedName);
        }
    }
}
