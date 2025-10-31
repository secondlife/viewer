/**
 * @file llfilepicker.cpp
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

#include "llfilepicker.h"
#include "llworld.h"
#include "llviewerwindow.h"
#include "llkeyboard.h"
#include "lldir.h"
#include "llframetimer.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llwindow.h"   // beforeDialog()

#if LL_SDL_WINDOW
#include "llwindowsdl.h" // for some X/GTK utils to help with filepickers
#endif // LL_SDL

#if LL_LINUX
#include "llhttpconstants.h"    // file picker uses some of thes constants on Linux
#endif

//
// Globals
//

LLFilePicker LLFilePicker::sInstance;

#if LL_WINDOWS && !LL_SDL_WINDOW
#define SOUND_FILTER L"Sounds (*.wav)\0*.wav\0"
#define IMAGE_FILTER L"Images (*.tga; *.bmp; *.jpg; *.jpeg; *.png)\0*.tga;*.bmp;*.jpg;*.jpeg;*.png\0"
#define ANIM_FILTER L"Animations (*.bvh; *.anim)\0*.bvh;*.anim\0"
#define COLLADA_FILTER L"Scene (*.dae)\0*.dae\0"
#define GLTF_FILTER L"glTF (*.gltf; *.glb)\0*.gltf;*.glb\0"
#define XML_FILTER L"XML files (*.xml)\0*.xml\0"
#define SLOBJECT_FILTER L"Objects (*.slobject)\0*.slobject\0"
#define RAW_FILTER L"RAW files (*.raw)\0*.raw\0"
#define MODEL_FILTER L"Model files (*.dae, *.gltf, *.glb)\0*.dae;*.gltf;*.glb\0"
#define MATERIAL_FILTER L"GLTF Files (*.gltf; *.glb)\0*.gltf;*.glb\0"
#define HDRI_FILTER L"HDRI Files (*.exr)\0*.exr\0"
#define MATERIAL_TEXTURES_FILTER L"GLTF Import (*.gltf; *.glb; *.tga; *.bmp; *.jpg; *.jpeg; *.png)\0*.gltf;*.glb;*.tga;*.bmp;*.jpg;*.jpeg;*.png\0"
#define SCRIPT_FILTER L"Script files (*.lsl)\0*.lsl\0"
#define DICTIONARY_FILTER L"Dictionary files (*.dic; *.xcu)\0*.dic;*.xcu\0"
#endif

#ifdef LL_DARWIN
#include "llfilepicker_mac.h"
//#include <boost/algorithm/string/predicate.hpp>
#endif

//
// Implementation
//
LLFilePicker::LLFilePicker()
    : mCurrentFile(0),
      mLocked(false)

{
    reset();

#if LL_WINDOWS && !LL_SDL_WINDOW
    mOFN.lStructSize = sizeof(OPENFILENAMEW);
    mOFN.hwndOwner = NULL;  // Set later
    mOFN.hInstance = NULL;
    mOFN.lpstrCustomFilter = NULL;
    mOFN.nMaxCustFilter = 0;
    mOFN.lpstrFile = NULL;                          // set in open and close
    mOFN.nMaxFile = LL_MAX_PATH;
    mOFN.lpstrFileTitle = NULL;
    mOFN.nMaxFileTitle = 0;
    mOFN.lpstrInitialDir = NULL;
    mOFN.lpstrTitle = NULL;
    mOFN.Flags = 0;                                 // set in open and close
    mOFN.nFileOffset = 0;
    mOFN.nFileExtension = 0;
    mOFN.lpstrDefExt = NULL;
    mOFN.lCustData = 0L;
    mOFN.lpfnHook = NULL;
    mOFN.lpTemplateName = NULL;
    mFilesW[0] = '\0';
#elif LL_DARWIN && !LL_SDL_WINDOW
    mPickOptions = 0;
#endif

}

LLFilePicker::~LLFilePicker()
{
    // nothing
}

// utility function to check if access to local file system via file browser
// is enabled and if not, tidy up and indicate we're not allowed to do this.
bool LLFilePicker::check_local_file_access_enabled()
{
    // if local file browsing is turned off, return without opening dialog
    bool local_file_system_browsing_enabled = gSavedSettings.getBOOL("LocalFileSystemBrowsingEnabled");
    if ( ! local_file_system_browsing_enabled )
    {
        mFiles.clear();
        return false;
    }

    return true;
}

const std::string LLFilePicker::getFirstFile()
{
    mCurrentFile = 0;
    return getNextFile();
}

const std::string LLFilePicker::getNextFile()
{
    if (mCurrentFile >= getFileCount())
    {
        mLocked = false;
        return std::string();
    }
    else
    {
        return mFiles[mCurrentFile++];
    }
}

const std::string LLFilePicker::getCurFile()
{
    if (mCurrentFile >= getFileCount())
    {
        mLocked = false;
        return std::string();
    }
    else
    {
        return mFiles[mCurrentFile];
    }
}

void LLFilePicker::reset()
{
    mLocked = false;
    mFiles.clear();
    mCurrentFile = 0;
}

#if LL_SDL_WINDOW

namespace
{
    std::vector<SDL_DialogFileFilter> setupLoadFilter(LLFilePicker::ELoadFilter filter)
    {
        std::vector<SDL_DialogFileFilter> filter_vec;

        switch (filter)
        {
        case LLFilePicker::FFLOAD_ALL:
        case LLFilePicker::FFLOAD_EXE:
            filter_vec.push_back({ "All Files (*.*)", "*" });
            filter_vec.push_back({ "Sounds (*.wav)", "wav" });
            filter_vec.push_back({ "Images (*.tga; *.bmp; *.jpg; *.jpeg; *.png)", "tga;bmp;jpg;jpeg;png" });
            filter_vec.push_back({ "Animations (*.bvh; *.anim)", "bvh;anim" });
            filter_vec.push_back({ "GLTF Files (*.gltf; *.glb)", "gltf;glb" });
            break;
        case LLFilePicker::FFLOAD_WAV:
            filter_vec.push_back({ "Sounds (*.wav)", "wav" });
            break;
        case LLFilePicker::FFLOAD_IMAGE:
            filter_vec.push_back({ "Images (*.tga; *.bmp; *.jpg; *.jpeg; *.png)", "tga;bmp;jpg;jpeg;png" });
            break;
        case LLFilePicker::FFLOAD_ANIM:
            filter_vec.push_back({ "Animations (*.bvh; *.anim)", "bvh;anim" });
            break;
        case LLFilePicker::FFLOAD_GLTF:
            filter_vec.push_back({ "glTF (*.gltf; *.glb)", "gltf;glb" });
            break;
        case LLFilePicker::FFLOAD_COLLADA:
            filter_vec.push_back({ "Scene (*.dae)", "dae" });
            break;
        case LLFilePicker::FFLOAD_XML:
            filter_vec.push_back({ "XML files (*.xml)", "xml" });
            break;
        case LLFilePicker::FFLOAD_SLOBJECT:
            filter_vec.push_back({ "Objects (*.slobject)", "slobject" });
            break;
        case LLFilePicker::FFLOAD_RAW:
            filter_vec.push_back({ "RAW files (*.raw)", "raw" });
            break;
        case LLFilePicker::FFLOAD_MODEL:
            filter_vec.push_back({ "Model files (*.dae)", "dae" });
            break;
        case LLFilePicker::FFLOAD_MATERIAL:
            filter_vec.push_back({ "GLTF Files (*.gltf; *.glb)", "gltf;glb" });
            break;
        case LLFilePicker::FFLOAD_MATERIAL_TEXTURE:
            filter_vec.push_back({ "GLTF Import (*.gltf; *.glb; *.tga; *.bmp; *.jpg; *.jpeg; *.png)", "gltf;glb;tga;bmp;jpg;jpeg;png" });
            filter_vec.push_back({ "GLTF Files (*.gltf; *.glb)", "gltf;glb" });
            filter_vec.push_back({ "Images (*.tga; *.bmp; *.jpg; *.jpeg; *.png)", "tga;bmp;jpg;jpeg;png" });
            break;
        case LLFilePicker::FFLOAD_HDRI:
            filter_vec.push_back({ "HDRI Files (*.exr)", "exr" });
            break;
        case LLFilePicker::FFLOAD_SCRIPT:
            filter_vec.push_back({ "Script files (*.lsl)", "lsl" });
            break;
        case LLFilePicker::FFLOAD_DICTIONARY:
            filter_vec.push_back({ "Dictionary files (*.dic; *.xcu)", "dic;xcu" });
            break;
        default:
            break;
        }
        return filter_vec;
    }
}

bool LLFilePicker::getOpenFile(ELoadFilter filter, bool blocking)
{
    LL_ERRS() << "NOT IMPLEMENTED" << LL_ENDL;
    return false;
}

bool LLFilePicker::getOpenFileModeless(ELoadFilter filter,
    void (*callback)(bool, std::vector<std::string>&, void*),
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

    auto file_filters = setupLoadFilter(filter);

    reset();

    {
        struct LLSDLFileUserdata
        {
            LLSDLFileUserdata(void (*callback_func)(bool, std::vector<std::string>&, void*), void* callback_userdata)
                : mCallback(callback_func), mUserdata(callback_userdata)
            {
            }
            void (*mCallback)(bool, std::vector<std::string>&, void*);
            void* mUserdata;
        };

        auto sdl_callback = [](void* userdata, const char* const* filelist, int filter)
            {
                LLSDLFileUserdata* callback_struct = (LLSDLFileUserdata*)userdata;

                auto* callback_func = callback_struct->mCallback;
                auto* callback_data = callback_struct->mUserdata;
                delete callback_struct; // delete callback container

                std::vector<std::string> rtn;

                if (!filelist)
                {
                    LL_WARNS() << "Error during SDL file picking: " << SDL_GetError() << LL_ENDL;
                    callback_func(false, rtn, callback_data);
                    return;
                }
                else if (!*filelist)
                {
                    LL_INFOS() << "User did not select any file. Dialog likely cancelled." << LL_ENDL;
                    callback_func(false, rtn, callback_data);
                    return;
                }

                while (*filelist)
                {
                    rtn.push_back(std::string(*filelist));
                    filelist++;
                }

                callback_func(true, rtn, callback_data);

            };

        LLSDLFileUserdata* llfilecallback = new LLSDLFileUserdata(callback, userdata);

        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_FILTERS_POINTER, file_filters.data());
        SDL_SetNumberProperty(props, SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER, file_filters.size());
        SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_WINDOW_POINTER, SDL_GL_GetCurrentWindow());
        SDL_SetBooleanProperty(props, SDL_PROP_FILE_DIALOG_MANY_BOOLEAN, false);

        SDL_ShowFileDialogWithProperties(SDL_FILEDIALOG_OPENFILE, sdl_callback, llfilecallback, props);

        SDL_DestroyProperties(props);
    }
    return true;
}

bool LLFilePicker::getMultipleOpenFiles(ELoadFilter filter, bool blocking)
{
    LL_ERRS() << "NOT IMPLEMENTED" << LL_ENDL;
    return false;
}

bool LLFilePicker::getMultipleOpenFilesModeless(ELoadFilter filter,
    void (*callback)(bool, std::vector<std::string>&, void*),
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

    auto file_filters = setupLoadFilter(filter);

    reset();

    {
        struct LLSDLFileUserdata
        {
            LLSDLFileUserdata(void (*callback_func)(bool, std::vector<std::string>&, void*), void* callback_userdata)
                : mCallback(callback_func), mUserdata(callback_userdata)
            {
            }
            void (*mCallback)(bool, std::vector<std::string>&, void*);
            void* mUserdata;
        };

        auto sdl_callback = [](void* userdata, const char* const* filelist, int filter)
            {
                LLSDLFileUserdata* callback_struct = (LLSDLFileUserdata*)userdata;

                auto* callback_func = callback_struct->mCallback;
                auto* callback_data = callback_struct->mUserdata;
                delete callback_struct; // delete callback container

                std::vector<std::string> rtn;

                if (!filelist)
                {
                    LL_WARNS() << "Error during SDL file picking: " << SDL_GetError() << LL_ENDL;
                    callback_func(false, rtn, callback_data);
                    return;
                }
                else if (!*filelist)
                {
                    LL_INFOS() << "User did not select any file. Dialog likely cancelled." << LL_ENDL;
                    callback_func(false, rtn, callback_data);
                    return;
                }

                while (*filelist)
                {
                    rtn.push_back(std::string(*filelist));
                    filelist++;
                }

                callback_func(true, rtn, callback_data);

            };

        LLSDLFileUserdata* llfilecallback = new LLSDLFileUserdata(callback, userdata);

        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_FILTERS_POINTER, file_filters.data());
        SDL_SetNumberProperty(props, SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER, file_filters.size());
        SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_WINDOW_POINTER, SDL_GL_GetCurrentWindow());
        SDL_SetBooleanProperty(props, SDL_PROP_FILE_DIALOG_MANY_BOOLEAN, true);

        SDL_ShowFileDialogWithProperties(SDL_FILEDIALOG_OPENFILE, sdl_callback, llfilecallback, props);

        SDL_DestroyProperties(props);
    }

    return true;
}

bool LLFilePicker::getSaveFile(ESaveFilter filter, const std::string& filename, bool blocking)
{
    LL_ERRS() << "NOT IMPLEMENTED" << LL_ENDL;
    return false;
}

bool LLFilePicker::getSaveFileModeless(ESaveFilter filter,
    const std::string& filename,
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

    std::string default_filename;
    if (!filename.empty())
    {
        default_filename = filename;
    }

    std::vector<SDL_DialogFileFilter> file_filters;

    switch (filter)
    {
    case FFSAVE_ALL:
        file_filters.push_back({ "All Files (*.*)", "*" });
        file_filters.push_back({ "WAV Sounds (*.wav)", "wav" });
        file_filters.push_back({ "Targa, Bitmap Images (*.tga; *.bmp)", "tga;bmp" });
        break;
    case FFSAVE_WAV:
        if (default_filename.empty())
        {
            default_filename = "untitled.wav";
        }
        file_filters.push_back({ "WAV Sounds (*.wav)", "wav" });
        break;
    case FFSAVE_TGA:
        if (default_filename.empty())
        {
            default_filename = "untitled.tga";
        }
        file_filters.push_back({ "Targa Images (*.tga)", "tga" });
        break;
    case FFSAVE_BMP:
        if (default_filename.empty())
        {
            default_filename = "untitled.bmp";
        }
        file_filters.push_back({ "Bitmap Images (*.bmp)", "bmp" });
        break;
    case FFSAVE_PNG:
        if (default_filename.empty())
        {
            default_filename = "untitled.png";
        }
        file_filters.push_back({ "PNG Images (*.png)", "png" });
        break;
    case FFSAVE_TGAPNG:
        if (default_filename.empty())
        {
            //PNG by default
            default_filename = "untitled.png";
        }
        file_filters.push_back({ "PNG Images (*.png)", "png" });
        file_filters.push_back({ "Targa Images (*.tga)", "tga" });
        break;

    case FFSAVE_JPEG:
        if (default_filename.empty())
        {
            default_filename = "untitled.jpeg";
        }
        file_filters.push_back({ "JPEG Images (*.jpg *.jpeg)", "jpg;jpeg" });
        break;
    case FFSAVE_AVI:
        if (default_filename.empty())
        {
            default_filename = "untitled.avi";
        }
        file_filters.push_back({ "AVI Movie File (*.avi)", "avi" });
        break;
    case FFSAVE_ANIM:
        if (default_filename.empty())
        {
            default_filename = "untitled.xaf";
        }
        file_filters.push_back({ "XAF Anim File (*.xaf)", "xaf" });
        break;
    case FFSAVE_GLTF:
        if (default_filename.empty())
        {
            default_filename = "untitled.gltf";
        }
        file_filters.push_back({ "glTF Asset File (*.gltf)", "gltf" });
        break;
    case FFSAVE_XML:
        if (default_filename.empty())
        {
            default_filename = "untitled.xml";
        }
        file_filters.push_back({ "XML File (*.xml)", "xml" });
        break;
    case FFSAVE_COLLADA:
        if (default_filename.empty())
        {
            default_filename = "untitled.collada";
        }
        file_filters.push_back({ "COLLADA File (*.collada)", "collada" });
        break;
    case FFSAVE_RAW:
        if (default_filename.empty())
        {
            default_filename = "untitled.raw";
        }
        file_filters.push_back({ "RAW files (*.raw)", "raw" });
        break;
    case FFSAVE_J2C:
        if (default_filename.empty())
        {
            default_filename = "untitled.j2c";
        }
        file_filters.push_back({ "Compressed Images (*.j2c)", "j2c" });
        break;
    case FFSAVE_SCRIPT:
        if (default_filename.empty())
        {
            default_filename = "untitled.lsl";
        }
        file_filters.push_back({ "LSL Files (*.lsl)", "lsl" });
        break;
    default:
        return false;
    }

    reset();

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
                    LL_WARNS() << "Error during SDL file picking: " << SDL_GetError() << LL_ENDL;
                    callback_func(false, rtn, callback_data);
                    return;
                }
                else if (!*filelist)
                {
                    LL_INFOS() << "User did not select any file. Dialog likely cancelled." << LL_ENDL;
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
        SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_FILTERS_POINTER, file_filters.data());
        SDL_SetNumberProperty(props, SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER, file_filters.size());
        SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_WINDOW_POINTER, SDL_GL_GetCurrentWindow());

        // Disabled default file name functionality currently due to inconsistency between various SDL3 backends
        // if(!default_filename.empty())
        // {
        //     SDL_SetStringProperty(props, SDL_PROP_FILE_DIALOG_LOCATION_STRING, default_filename.c_str());
        // }
        SDL_SetBooleanProperty(props, SDL_PROP_FILE_DIALOG_MANY_BOOLEAN, false);

        SDL_ShowFileDialogWithProperties(SDL_FILEDIALOG_SAVEFILE, sdl_callback, llfilecallback, props);

        SDL_DestroyProperties(props);
    }

    return true;
}
#elif LL_WINDOWS

bool LLFilePicker::setupFilter(ELoadFilter filter)
{
    bool res = true;
    switch (filter)
    {
    case FFLOAD_ALL:
    case FFLOAD_EXE:
        mOFN.lpstrFilter = L"All Files (*.*)\0*.*\0" \
        SOUND_FILTER \
        IMAGE_FILTER \
        ANIM_FILTER \
        MATERIAL_FILTER \
        L"\0";
        break;
    case FFLOAD_WAV:
        mOFN.lpstrFilter = SOUND_FILTER \
            L"\0";
        break;
    case FFLOAD_IMAGE:
        mOFN.lpstrFilter = IMAGE_FILTER \
            L"\0";
        break;
    case FFLOAD_ANIM:
        mOFN.lpstrFilter = ANIM_FILTER \
            L"\0";
        break;
    case FFLOAD_GLTF:
        mOFN.lpstrFilter = GLTF_FILTER \
            L"\0";
        break;
    case FFLOAD_COLLADA:
        mOFN.lpstrFilter = COLLADA_FILTER \
            L"\0";
        break;
    case FFLOAD_XML:
        mOFN.lpstrFilter = XML_FILTER \
            L"\0";
        break;
    case FFLOAD_SLOBJECT:
        mOFN.lpstrFilter = SLOBJECT_FILTER \
            L"\0";
        break;
    case FFLOAD_RAW:
        mOFN.lpstrFilter = RAW_FILTER \
            L"\0";
        break;
    case FFLOAD_MODEL:
        mOFN.lpstrFilter = MODEL_FILTER \
            COLLADA_FILTER \
            MATERIAL_FILTER \
            L"\0";
        break;
    case FFLOAD_MATERIAL:
        mOFN.lpstrFilter = MATERIAL_FILTER \
            L"\0";
        break;
    case FFLOAD_MATERIAL_TEXTURE:
        mOFN.lpstrFilter = MATERIAL_TEXTURES_FILTER \
            MATERIAL_FILTER \
            IMAGE_FILTER \
            L"\0";
        break;
    case FFLOAD_HDRI:
        mOFN.lpstrFilter = HDRI_FILTER \
            L"\0";
        break;
    case FFLOAD_SCRIPT:
        mOFN.lpstrFilter = SCRIPT_FILTER \
            L"\0";
        break;
    case FFLOAD_DICTIONARY:
        mOFN.lpstrFilter = DICTIONARY_FILTER \
            L"\0";
        break;
    default:
        res = false;
        break;
    }
    return res;
}

bool LLFilePicker::getOpenFile(ELoadFilter filter, bool blocking)
{
    if (mLocked)
    {
        return false;
    }
    bool success = false;

    // if local file browsing is turned off, return without opening dialog
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    // don't provide default file selection
    mFilesW[0] = '\0';

    mOFN.hwndOwner = (HWND)gViewerWindow->getPlatformWindow();
    mOFN.lpstrFile = mFilesW;
    mOFN.nMaxFile = SINGLE_FILENAME_BUFFER_SIZE;
    mOFN.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR ;
    mOFN.nFilterIndex = 1;

    setupFilter(filter);

    if (blocking)
    {
        // Modal, so pause agent
        send_agent_pause();
    }

    reset();

    // NOTA BENE: hitting the file dialog triggers a window focus event, destroying the selection manager!!
    success = GetOpenFileName(&mOFN);
    if (success)
    {
        std::string filename = ll_convert<std::string>(std::wstring(mFilesW));
        mFiles.push_back(filename);
    }

    if (blocking)
    {
        send_agent_resume();
        // Account for the fact that the app has been stalled.
        LLFrameTimer::updateFrameTime();
    }

    return success;
}

bool LLFilePicker::getOpenFileModeless(ELoadFilter filter,
                                       void (*callback)(bool, std::vector<std::string> &, void*),
                                       void *userdata)
{
    // not supposed to be used yet, use LLFilePickerThread
    LL_ERRS() << "NOT IMPLEMENTED" << LL_ENDL;
    return false;
}

bool LLFilePicker::getMultipleOpenFiles(ELoadFilter filter, bool blocking)
{
    if( mLocked )
    {
        return false;
    }
    bool success = false;

    // if local file browsing is turned off, return without opening dialog
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    // don't provide default file selection
    mFilesW[0] = '\0';

    mOFN.hwndOwner = (HWND)gViewerWindow->getPlatformWindow();
    mOFN.lpstrFile = mFilesW;
    mOFN.nFilterIndex = 1;
    mOFN.nMaxFile = FILENAME_BUFFER_SIZE;
    mOFN.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR |
        OFN_EXPLORER | OFN_ALLOWMULTISELECT;

    setupFilter(filter);

    reset();

    if (blocking)
    {
        // Modal, so pause agent
        send_agent_pause();
    }

    // NOTA BENE: hitting the file dialog triggers a window focus event, destroying the selection manager!!
    success = GetOpenFileName(&mOFN); // pauses until ok or cancel.
    if( success )
    {
        // The getopenfilename api doesn't tell us if we got more than
        // one file, so we have to test manually by checking string
        // lengths.
        if( wcslen(mOFN.lpstrFile) > mOFN.nFileOffset ) /*Flawfinder: ignore*/
        {
            std::string filename = ll_convert<std::string>(std::wstring(mFilesW));
            mFiles.push_back(filename);
        }
        else
        {
            mLocked = true;
            WCHAR* tptrw = mFilesW;
            std::string dirname;
            while(1)
            {
                if (*tptrw == 0 && *(tptrw+1) == 0) // double '\0'
                    break;
                if (*tptrw == 0)
                    tptrw++; // shouldn't happen?
                std::string filename = ll_convert<std::string>(std::wstring(tptrw));
                if (dirname.empty())
                    dirname = filename + "\\";
                else
                    mFiles.push_back(dirname + filename);
                tptrw += wcslen(tptrw);
            }
        }
    }

    if (blocking)
    {
        send_agent_resume();
    }

    // Account for the fact that the app has been stalled.
    LLFrameTimer::updateFrameTime();
    return success;
}

bool LLFilePicker::getMultipleOpenFilesModeless(ELoadFilter filter,
                                                void (*callback)(bool, std::vector<std::string> &, void*),
                                                void *userdata )
{
    // not supposed to be used yet, use LLFilePickerThread
    LL_ERRS() << "NOT IMPLEMENTED" << LL_ENDL;
    return false;
}

bool LLFilePicker::getSaveFile(ESaveFilter filter, const std::string& filename, bool blocking)
{
    if( mLocked )
    {
        return false;
    }
    bool success = false;

    // if local file browsing is turned off, return without opening dialog
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    mOFN.lpstrFile = mFilesW;
    if (!filename.empty())
    {
        std::wstring tstring = ll_convert<std::wstring>(filename);
        wcsncpy(mFilesW, tstring.c_str(), FILENAME_BUFFER_SIZE);    }   /*Flawfinder: ignore*/
    else
    {
        mFilesW[0] = '\0';
    }
    mOFN.hwndOwner = (HWND)gViewerWindow->getPlatformWindow();

    switch( filter )
    {
    case FFSAVE_ALL:
        mOFN.lpstrDefExt = NULL;
        mOFN.lpstrFilter =
            L"All Files (*.*)\0*.*\0" \
            L"WAV Sounds (*.wav)\0*.wav\0" \
            L"Targa, Bitmap Images (*.tga; *.bmp)\0*.tga;*.bmp\0" \
            L"\0";
        break;
    case FFSAVE_WAV:
        if (filename.empty())
        {
            wcsncpy( mFilesW,L"untitled.wav", FILENAME_BUFFER_SIZE);    /*Flawfinder: ignore*/
        }
        mOFN.lpstrDefExt = L"wav";
        mOFN.lpstrFilter =
            L"WAV Sounds (*.wav)\0*.wav\0" \
            L"\0";
        break;
    case FFSAVE_TGA:
        if (filename.empty())
        {
            wcsncpy( mFilesW,L"untitled.tga", FILENAME_BUFFER_SIZE);    /*Flawfinder: ignore*/
        }
        mOFN.lpstrDefExt = L"tga";
        mOFN.lpstrFilter =
            L"Targa Images (*.tga)\0*.tga\0" \
            L"\0";
        break;
    case FFSAVE_BMP:
        if (filename.empty())
        {
            wcsncpy( mFilesW,L"untitled.bmp", FILENAME_BUFFER_SIZE);    /*Flawfinder: ignore*/
        }
        mOFN.lpstrDefExt = L"bmp";
        mOFN.lpstrFilter =
            L"Bitmap Images (*.bmp)\0*.bmp\0" \
            L"\0";
        break;
    case FFSAVE_PNG:
        if (filename.empty())
        {
            wcsncpy( mFilesW,L"untitled.png", FILENAME_BUFFER_SIZE);    /*Flawfinder: ignore*/
        }
        mOFN.lpstrDefExt = L"png";
        mOFN.lpstrFilter =
            L"PNG Images (*.png)\0*.png\0" \
            L"\0";
        break;
    case FFSAVE_TGAPNG:
        if (filename.empty())
        {
            wcsncpy( mFilesW,L"untitled.png", FILENAME_BUFFER_SIZE);    /*Flawfinder: ignore*/
            //PNG by default
        }
        mOFN.lpstrDefExt = L"png";
        mOFN.lpstrFilter =
            L"PNG Images (*.png)\0*.png\0" \
            L"Targa Images (*.tga)\0*.tga\0" \
            L"\0";
        break;

    case FFSAVE_JPEG:
        if (filename.empty())
        {
            wcsncpy( mFilesW,L"untitled.jpeg", FILENAME_BUFFER_SIZE);   /*Flawfinder: ignore*/
        }
        mOFN.lpstrDefExt = L"jpg";
        mOFN.lpstrFilter =
            L"JPEG Images (*.jpg *.jpeg)\0*.jpg;*.jpeg\0" \
            L"\0";
        break;
    case FFSAVE_AVI:
        if (filename.empty())
        {
            wcsncpy( mFilesW,L"untitled.avi", FILENAME_BUFFER_SIZE);    /*Flawfinder: ignore*/
        }
        mOFN.lpstrDefExt = L"avi";
        mOFN.lpstrFilter =
            L"AVI Movie File (*.avi)\0*.avi\0" \
            L"\0";
        break;
    case FFSAVE_ANIM:
        if (filename.empty())
        {
            wcsncpy( mFilesW,L"untitled.xaf", FILENAME_BUFFER_SIZE);    /*Flawfinder: ignore*/
        }
        mOFN.lpstrDefExt = L"xaf";
        mOFN.lpstrFilter =
            L"XAF Anim File (*.xaf)\0*.xaf\0" \
            L"\0";
        break;
    case FFSAVE_GLTF:
        if (filename.empty())
        {
            wcsncpy( mFilesW,L"untitled.gltf", FILENAME_BUFFER_SIZE);   /*Flawfinder: ignore*/
        }
        mOFN.lpstrDefExt = L"gltf";
        mOFN.lpstrFilter =
            L"glTF Asset File (*.gltf)\0*.gltf\0" \
            L"\0";
        break;
    case FFSAVE_XML:
        if (filename.empty())
        {
            wcsncpy( mFilesW,L"untitled.xml", FILENAME_BUFFER_SIZE);    /*Flawfinder: ignore*/
        }

        mOFN.lpstrDefExt = L"xml";
        mOFN.lpstrFilter =
            L"XML File (*.xml)\0*.xml\0" \
            L"\0";
        break;
    case FFSAVE_COLLADA:
        if (filename.empty())
        {
            wcsncpy( mFilesW,L"untitled.collada", FILENAME_BUFFER_SIZE);    /*Flawfinder: ignore*/
        }
        mOFN.lpstrDefExt = L"collada";
        mOFN.lpstrFilter =
            L"COLLADA File (*.collada)\0*.collada\0" \
            L"\0";
        break;
    case FFSAVE_RAW:
        if (filename.empty())
        {
            wcsncpy( mFilesW,L"untitled.raw", FILENAME_BUFFER_SIZE);    /*Flawfinder: ignore*/
        }
        mOFN.lpstrDefExt = L"raw";
        mOFN.lpstrFilter =  RAW_FILTER \
                            L"\0";
        break;
    case FFSAVE_J2C:
        if (filename.empty())
        {
            wcsncpy( mFilesW,L"untitled.j2c", FILENAME_BUFFER_SIZE);
        }
        mOFN.lpstrDefExt = L"j2c";
        mOFN.lpstrFilter =
            L"Compressed Images (*.j2c)\0*.j2c\0" \
            L"\0";
        break;
    case FFSAVE_SCRIPT:
        if (filename.empty())
        {
            wcsncpy( mFilesW,L"untitled.lsl", FILENAME_BUFFER_SIZE);
        }
        mOFN.lpstrDefExt = L"txt";
        mOFN.lpstrFilter = L"LSL Files (*.lsl)\0*.lsl\0" L"\0";
        break;
    default:
        return false;
    }


    mOFN.nMaxFile = SINGLE_FILENAME_BUFFER_SIZE;
    mOFN.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;

    reset();

    if (blocking)
    {
        // Modal, so pause agent
        send_agent_pause();
    }

    {
        // NOTA BENE: hitting the file dialog triggers a window focus event, destroying the selection manager!!
        try
        {
            success = GetSaveFileName(&mOFN);
            if (success)
            {
                std::string filename = ll_convert<std::string>(std::wstring(mFilesW));
                mFiles.push_back(filename);
            }
        }
        catch (...)
        {
            LOG_UNHANDLED_EXCEPTION("");
        }
        gKeyboard->resetKeys();
    }

    if (blocking)
    {
        send_agent_resume();
    }

    // Account for the fact that the app has been stalled.
    LLFrameTimer::updateFrameTime();
    return success;
}

bool LLFilePicker::getSaveFileModeless(ESaveFilter filter,
                                       const std::string& filename,
                                       void (*callback)(bool, std::string&, void*),
                                       void *userdata)
{
    // not supposed to be used yet, use LLFilePickerThread
    LL_ERRS() << "NOT IMPLEMENTED" << LL_ENDL;
    return false;
}

#elif LL_DARWIN

std::unique_ptr<std::vector<std::string>> LLFilePicker::navOpenFilterProc(ELoadFilter filter) //(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes filterMode)
{
    std::unique_ptr<std::vector<std::string>> allowedv(new std::vector< std::string >);
    switch(filter)
    {
        case FFLOAD_ALL:
        case FFLOAD_EXE:
            allowedv->push_back("app");
            allowedv->push_back("exe");
            allowedv->push_back("wav");
            allowedv->push_back("bvh");
            allowedv->push_back("anim");
            allowedv->push_back("dae");
            allowedv->push_back("raw");
            allowedv->push_back("lsl");
            allowedv->push_back("dic");
            allowedv->push_back("xcu");
            allowedv->push_back("gif");
            allowedv->push_back("gltf");
            allowedv->push_back("glb");
        case FFLOAD_IMAGE:
            allowedv->push_back("jpg");
            allowedv->push_back("jpeg");
            allowedv->push_back("bmp");
            allowedv->push_back("tga");
            allowedv->push_back("bmpf");
            allowedv->push_back("tpic");
            allowedv->push_back("png");
            break;
            break;
        case FFLOAD_WAV:
            allowedv->push_back("wav");
            break;
        case FFLOAD_ANIM:
            allowedv->push_back("bvh");
            allowedv->push_back("anim");
            break;
        case FFLOAD_GLTF:
        case FFLOAD_MATERIAL:
            allowedv->push_back("gltf");
            allowedv->push_back("glb");
            break;
        case FFLOAD_HDRI:
            allowedv->push_back("exr");
        case FFLOAD_MODEL:
            allowedv->push_back("gltf");
            allowedv->push_back("glb");
        case FFLOAD_COLLADA:
            allowedv->push_back("dae");
            break;
        case FFLOAD_XML:
            allowedv->push_back("xml");
            break;
        case FFLOAD_RAW:
            allowedv->push_back("raw");
            break;
        case FFLOAD_SCRIPT:
            allowedv->push_back("lsl");
            break;
        case FFLOAD_DICTIONARY:
            allowedv->push_back("dic");
            allowedv->push_back("xcu");
            break;
        case FFLOAD_DIRECTORY:
            break;
        default:
            LL_WARNS() << "Unsupported format." << LL_ENDL;
    }

    return allowedv;
}

bool LLFilePicker::doNavChooseDialog(ELoadFilter filter)
{
    // if local file browsing is turned off, return without opening dialog
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    gViewerWindow->getWindow()->beforeDialog();

    std::unique_ptr<std::vector<std::string>> allowed_types = navOpenFilterProc(filter);

    std::unique_ptr<std::vector<std::string>> filev  = doLoadDialog(allowed_types.get(),
                                                    mPickOptions);

    gViewerWindow->getWindow()->afterDialog();


    if (filev && filev->size() > 0)
    {
        mFiles.insert(mFiles.end(), filev->begin(), filev->end());
        return true;
    }

    return false;
}

bool LLFilePicker::doNavChooseDialogModeless(ELoadFilter filter,
                                                void (*callback)(bool, std::vector<std::string> &,void*),
                                                void *userdata)
{
    // if local file browsing is turned off, return without opening dialog
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    std::unique_ptr<std::vector<std::string>> allowed_types=navOpenFilterProc(filter);

    doLoadDialogModeless(allowed_types.get(),
                                                    mPickOptions,
                                                    callback,
                                                    userdata);

    return true;
}

void set_nav_save_data(LLFilePicker::ESaveFilter filter, std::string &extension, std::string &type, std::string &creator)
{
    switch (filter)
    {
        case LLFilePicker::FFSAVE_WAV:
            type = "WAVE";
            creator = "TVOD";
            extension = "wav";
            break;
        case LLFilePicker::FFSAVE_TGA:
            type = "TPIC";
            creator = "prvw";
            extension = "tga";
            break;
        case LLFilePicker::FFSAVE_TGAPNG:
            type = "PNG";
            creator = "prvw";
            extension = "png,tga";
            break;
        case LLFilePicker::FFSAVE_BMP:
            type = "BMPf";
            creator = "prvw";
            extension = "bmp";
            break;
        case LLFilePicker::FFSAVE_JPEG:
            type = "JPEG";
            creator = "prvw";
            extension = "jpeg";
            break;
        case LLFilePicker::FFSAVE_PNG:
            type = "PNG ";
            creator = "prvw";
            extension = "png";
            break;
        case LLFilePicker::FFSAVE_AVI:
            type = "\?\?\?\?";
            creator = "\?\?\?\?";
            extension = "mov";
            break;

        case LLFilePicker::FFSAVE_ANIM:
            type = "\?\?\?\?";
            creator = "\?\?\?\?";
            extension = "xaf";
            break;
        case LLFilePicker::FFSAVE_GLTF:
            type = "\?\?\?\?";
            creator = "\?\?\?\?";
            extension = "gltf";
            break;

        case LLFilePicker::FFSAVE_XML:
            type = "\?\?\?\?";
            creator = "\?\?\?\?";
            extension = "xml";
            break;

        case LLFilePicker::FFSAVE_RAW:
            type = "\?\?\?\?";
            creator = "\?\?\?\?";
            extension = "raw";
            break;

        case LLFilePicker::FFSAVE_J2C:
            type = "\?\?\?\?";
            creator = "prvw";
            extension = "j2c";
            break;

        case LLFilePicker::FFSAVE_SCRIPT:
            type = "LSL ";
            creator = "\?\?\?\?";
            extension = "lsl";
            break;

        case LLFilePicker::FFSAVE_ALL:
        default:
            type = "\?\?\?\?";
            creator = "\?\?\?\?";
            extension = "";
            break;
    }
}

bool LLFilePicker::doNavSaveDialog(ESaveFilter filter, const std::string& filename)
{
    // Setup the type, creator, and extension
    std::string     extension, type, creator;

    set_nav_save_data(filter, extension, type, creator);

    std::string namestring = filename;
    if (namestring.empty()) namestring="Untitled";

    gViewerWindow->getWindow()->beforeDialog();

    // Run the dialog
    std::unique_ptr<std::string> filev = doSaveDialog(&namestring,
                 &type,
                 &creator,
                 &extension,
                 mPickOptions);

    gViewerWindow->getWindow()->afterDialog();

    if ( filev && !filev->empty() )
    {
        mFiles.push_back(*filev);
        return true;
    }

    return false;
}

bool LLFilePicker::doNavSaveDialogModeless(ESaveFilter filter,
                                              const std::string& filename,
                                              void (*callback)(bool, std::string&, void*),
                                              void *userdata)
{
    // Setup the type, creator, and extension
    std::string        extension, type, creator;

    set_nav_save_data(filter, extension, type, creator);

    std::string namestring = filename;
    if (namestring.empty()) namestring="Untitled";

    // Run the dialog
    doSaveDialogModeless(&namestring,
                 &type,
                 &creator,
                 &extension,
                 mPickOptions,
                 callback,
                 userdata);
    return true;
}

bool LLFilePicker::getOpenFile(ELoadFilter filter, bool blocking)
{
    if( mLocked )
        return false;

    bool success = false;

    // if local file browsing is turned off, return without opening dialog
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    reset();

    mPickOptions &= ~F_MULTIPLE;
    mPickOptions |= F_FILE;

    if (filter == FFLOAD_DIRECTORY) //This should only be called from lldirpicker.
    {
        mPickOptions |= ( F_NAV_SUPPORT | F_DIRECTORY );
        mPickOptions &= ~F_FILE;
    }

    if (filter == FFLOAD_ALL)   // allow application bundles etc. to be traversed; important for DEV-16869, but generally useful
    {
        mPickOptions |= F_NAV_SUPPORT;
    }

    if (blocking) // always true for linux/mac
    {
        // Modal, so pause agent
        send_agent_pause();
    }


    success = doNavChooseDialog(filter);

    if (success)
    {
        if (!getFileCount())
            success = false;
    }

    if (blocking)
    {
        send_agent_resume();
        // Account for the fact that the app has been stalled.
        LLFrameTimer::updateFrameTime();
    }

    return success;
}


bool LLFilePicker::getOpenFileModeless(ELoadFilter filter,
                                       void (*callback)(bool, std::vector<std::string> &, void*),
                                       void *userdata)
{
    if (mLocked)
        return false;

    // if local file browsing is turned off, return without opening dialog
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    reset();

    mPickOptions &= ~F_MULTIPLE;
    mPickOptions |= F_FILE;

    if (filter == FFLOAD_DIRECTORY) //This should only be called from lldirpicker.
    {

        mPickOptions |= ( F_NAV_SUPPORT | F_DIRECTORY );
        mPickOptions &= ~F_FILE;
    }

    if (filter == FFLOAD_ALL)    // allow application bundles etc. to be traversed; important for DEV-16869, but generally useful
    {
        mPickOptions |= F_NAV_SUPPORT;
    }

    return doNavChooseDialogModeless(filter, callback, userdata);
}

bool LLFilePicker::getMultipleOpenFiles(ELoadFilter filter, bool blocking)
{
    if (mLocked)
        return false;

    // if local file browsing is turned off, return without opening dialog
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    bool success = false;

    reset();

    mPickOptions |= F_FILE;

    mPickOptions |= F_MULTIPLE;

    if (blocking) // always true for linux/mac
    {
        // Modal, so pause agent
        send_agent_pause();
    }

    success = doNavChooseDialog(filter);

    if (blocking)
    {
        send_agent_resume();
    }

    if (success)
    {
        if (!getFileCount())
            success = false;
        if (getFileCount() > 1)
            mLocked = true;
    }

    // Account for the fact that the app has been stalled.
    LLFrameTimer::updateFrameTime();
    return success;
}


bool LLFilePicker::getMultipleOpenFilesModeless(ELoadFilter filter,
                                                void (*callback)(bool, std::vector<std::string> &, void*),
                                                void *userdata )
{
    if (mLocked)
        return false;

    // if local file browsing is turned off, return without opening dialog
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    reset();

    mPickOptions |= F_FILE;

    mPickOptions |= F_MULTIPLE;

    return doNavChooseDialogModeless(filter, callback, userdata);
}

bool LLFilePicker::getSaveFile(ESaveFilter filter, const std::string& filename, bool blocking)
{

    if (mLocked)
        return false;

    bool success = false;

    // if local file browsing is turned off, return without opening dialog
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    reset();

    mPickOptions &= ~F_MULTIPLE;

    if (blocking)
    {
        // Modal, so pause agent
        send_agent_pause();
    }

    success = doNavSaveDialog(filter, filename);

    if (success)
    {
        if (!getFileCount())
            success = false;
    }

    if (blocking)
    {
        send_agent_resume();
    }

    // Account for the fact that the app has been stalled.
    LLFrameTimer::updateFrameTime();
    return success;
}

bool LLFilePicker::getSaveFileModeless(ESaveFilter filter,
                                       const std::string& filename,
                                       void (*callback)(bool, std::string&, void*),
                                       void *userdata)
{
    if (mLocked)
        return false;

    // if local file browsing is turned off, return without opening dialog
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    reset();

    mPickOptions &= ~F_MULTIPLE;

    return doNavSaveDialogModeless(filter, filename, callback, userdata);
}
//END LL_DARWIN

#elif LL_LINUX

// Hacky stubs designed to facilitate fake getSaveFile and getOpenFile with
// static results, when we don't have a real filepicker.

bool LLFilePicker::getSaveFile( ESaveFilter filter, const std::string& filename, bool blocking )
{
    // if local file browsing is turned off, return without opening dialog
    // (Even though this is a stub, I think we still should not return anything at all)
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    reset();

    LL_INFOS() << "getSaveFile suggested filename is [" << filename
        << "]" << LL_ENDL;
    if (!filename.empty())
    {
        mFiles.push_back(gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter() + filename);
        return true;
    }
    return false;
}

bool LLFilePicker::getSaveFileModeless(ESaveFilter filter,
                                       const std::string& filename,
                                       void (*callback)(bool, std::string&, void*),
                                       void *userdata)
{
    LL_ERRS() << "NOT IMPLEMENTED" << LL_ENDL;
    return false;
}

bool LLFilePicker::getOpenFile( ELoadFilter filter, bool blocking )
{
    // if local file browsing is turned off, return without opening dialog
    // (Even though this is a stub, I think we still should not return anything at all)
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    reset();

    // HACK: Static filenames for 'open' until we implement filepicker
    std::string filename = gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter() + "upload";
    switch (filter)
    {
    case FFLOAD_WAV: filename += ".wav"; break;
    case FFLOAD_IMAGE: filename += ".tga"; break;
    case FFLOAD_ANIM: filename += ".bvh"; break;
    default: break;
    }
    mFiles.push_back(filename);
    LL_INFOS() << "getOpenFile: Will try to open file: " << filename << LL_ENDL;
    return true;
}

bool LLFilePicker::getOpenFileModeless(ELoadFilter filter,
                                       void (*callback)(bool, std::vector<std::string> &, void*),
                                       void *userdata)
{
    LL_ERRS() << "NOT IMPLEMENTED" << LL_ENDL;
    return false;
}

bool LLFilePicker::getMultipleOpenFiles( ELoadFilter filter, bool blocking)
{
    // if local file browsing is turned off, return without opening dialog
    // (Even though this is a stub, I think we still should not return anything at all)
    if (!check_local_file_access_enabled())
    {
        return false;
    }

    reset();
    return false;
}

bool LLFilePicker::getMultipleOpenFilesModeless(ELoadFilter filter,
                                                void (*callback)(bool, std::vector<std::string> &, void*),
                                                void *userdata )
{
    LL_ERRS() << "NOT IMPLEMENTED" << LL_ENDL;
    return false;
}

#else // not implemented

bool LLFilePicker::getSaveFile( ESaveFilter filter, const std::string& filename )
{
    reset();
    return false;
}

bool LLFilePicker::getOpenFile( ELoadFilter filter )
{
    reset();
    return false;
}

bool LLFilePicker::getMultipleOpenFiles( ELoadFilter filter, bool blocking)
{
    reset();
    return false;
}

#endif // LL_LINUX
