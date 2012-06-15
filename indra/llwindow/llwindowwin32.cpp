/** 
 * @file llwindowwin32.cpp
 * @brief Platform-dependent implementation of llwindow
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

#include "linden_common.h"

#if LL_WINDOWS && !LL_MESA_HEADLESS

#include "llwindowwin32.h"

// LLWindow library includes
#include "llkeyboardwin32.h"
#include "lldragdropwin32.h"
#include "llpreeditor.h"
#include "llwindowcallbacks.h"

// Linden library includes
#include "llerror.h"
#include "llgl.h"
#include "llstring.h"
#include "lldir.h"
#include "llglslshader.h"

// System includes
#include <commdlg.h>
#include <WinUser.h>
#include <mapi.h>
#include <process.h>	// for _spawn
#include <shellapi.h>
#include <fstream>
#include <Imm.h>

// Require DirectInput version 8
#define DIRECTINPUT_VERSION 0x0800

#include <dinput.h>
#include <Dbt.h.>

#include "llmemtype.h"
// culled from winuser.h
#ifndef WM_MOUSEWHEEL /* Added to be compatible with later SDK's */
const S32	WM_MOUSEWHEEL = 0x020A;
#endif
#ifndef WHEEL_DELTA /* Added to be compatible with later SDK's */
const S32	WHEEL_DELTA = 120;     /* Value for rolling one detent */
#endif
const S32	MAX_MESSAGE_PER_UPDATE = 20;
const S32	BITS_PER_PIXEL = 32;
const S32	MAX_NUM_RESOLUTIONS = 32;
const F32	ICON_FLASH_TIME = 0.5f;

extern BOOL gDebugWindowProc;

LPWSTR gIconResource = IDI_APPLICATION;

LLW32MsgCallback gAsyncMsgCallback = NULL;

//
// LLWindowWin32
//

void show_window_creation_error(const std::string& title)
{
	LL_WARNS("Window") << title << LL_ENDL;
}

//static
BOOL LLWindowWin32::sIsClassRegistered = FALSE;

BOOL	LLWindowWin32::sLanguageTextInputAllowed = TRUE;
BOOL	LLWindowWin32::sWinIMEOpened = FALSE;
HKL		LLWindowWin32::sWinInputLocale = 0;
DWORD	LLWindowWin32::sWinIMEConversionMode = IME_CMODE_NATIVE;
DWORD	LLWindowWin32::sWinIMESentenceMode = IME_SMODE_AUTOMATIC;
LLCoordWindow LLWindowWin32::sWinIMEWindowPosition(-1,-1);

// The following class LLWinImm delegates Windows IMM APIs.
// We need this because some language versions of Windows,
// e.g., US version of Windows XP, doesn't install IMM32.DLL
// as a default, and we can't link against imm32.lib statically.
// I believe DLL loading of this type is best suited to do
// in a static initialization of a class.  What I'm not sure is
// whether it follows the Linden Conding Standard... 
// See http://wiki.secondlife.com/wiki/Coding_standards#Static_Members

class LLWinImm
{
public:
	static bool		isAvailable() { return sTheInstance.mHImmDll != NULL; }

public:
	// Wrappers for IMM API.
	static BOOL		isIME(HKL hkl);															
	static HWND		getDefaultIMEWnd(HWND hwnd);
	static HIMC		getContext(HWND hwnd);													
	static BOOL		releaseContext(HWND hwnd, HIMC himc);
	static BOOL		getOpenStatus(HIMC himc);												
	static BOOL		setOpenStatus(HIMC himc, BOOL status);									
	static BOOL		getConversionStatus(HIMC himc, LPDWORD conversion, LPDWORD sentence);	
	static BOOL		setConversionStatus(HIMC himc, DWORD conversion, DWORD sentence);		
	static BOOL		getCompositionWindow(HIMC himc, LPCOMPOSITIONFORM form);					
	static BOOL		setCompositionWindow(HIMC himc, LPCOMPOSITIONFORM form);					
	static LONG		getCompositionString(HIMC himc, DWORD index, LPVOID data, DWORD length);
	static BOOL		setCompositionString(HIMC himc, DWORD index, LPVOID pComp, DWORD compLength, LPVOID pRead, DWORD readLength);
	static BOOL		setCompositionFont(HIMC himc, LPLOGFONTW logfont);
	static BOOL		setCandidateWindow(HIMC himc, LPCANDIDATEFORM candidate_form);
	static BOOL		notifyIME(HIMC himc, DWORD action, DWORD index, DWORD value);

private:
	LLWinImm();
	~LLWinImm();

private:
	// Pointers to IMM API.
	BOOL	 	(WINAPI *mImmIsIME)(HKL);
	HWND		(WINAPI *mImmGetDefaultIMEWnd)(HWND);
	HIMC		(WINAPI *mImmGetContext)(HWND);
	BOOL		(WINAPI *mImmReleaseContext)(HWND, HIMC);
	BOOL		(WINAPI *mImmGetOpenStatus)(HIMC);
	BOOL		(WINAPI *mImmSetOpenStatus)(HIMC, BOOL);
	BOOL		(WINAPI *mImmGetConversionStatus)(HIMC, LPDWORD, LPDWORD);
	BOOL		(WINAPI *mImmSetConversionStatus)(HIMC, DWORD, DWORD);
	BOOL		(WINAPI *mImmGetCompostitionWindow)(HIMC, LPCOMPOSITIONFORM);
	BOOL		(WINAPI *mImmSetCompostitionWindow)(HIMC, LPCOMPOSITIONFORM);
	LONG		(WINAPI *mImmGetCompositionString)(HIMC, DWORD, LPVOID, DWORD);
	BOOL		(WINAPI *mImmSetCompositionString)(HIMC, DWORD, LPVOID, DWORD, LPVOID, DWORD);
	BOOL		(WINAPI *mImmSetCompositionFont)(HIMC, LPLOGFONTW);
	BOOL		(WINAPI *mImmSetCandidateWindow)(HIMC, LPCANDIDATEFORM);
	BOOL		(WINAPI *mImmNotifyIME)(HIMC, DWORD, DWORD, DWORD);

private:
	HMODULE		mHImmDll;
	static LLWinImm sTheInstance;
};

LLWinImm LLWinImm::sTheInstance;

LLWinImm::LLWinImm() : mHImmDll(NULL)
{
	// Check system metrics 
	if ( !GetSystemMetrics( SM_DBCSENABLED ) )
		return;
	

	mHImmDll = LoadLibraryA("Imm32");
	if (mHImmDll != NULL)
	{
		mImmIsIME               = (BOOL (WINAPI *)(HKL))                    GetProcAddress(mHImmDll, "ImmIsIME");
		mImmGetDefaultIMEWnd	= (HWND (WINAPI *)(HWND))					GetProcAddress(mHImmDll, "ImmGetDefaultIMEWnd");
		mImmGetContext          = (HIMC (WINAPI *)(HWND))                   GetProcAddress(mHImmDll, "ImmGetContext");
		mImmReleaseContext      = (BOOL (WINAPI *)(HWND, HIMC))             GetProcAddress(mHImmDll, "ImmReleaseContext");
		mImmGetOpenStatus       = (BOOL (WINAPI *)(HIMC))                   GetProcAddress(mHImmDll, "ImmGetOpenStatus");
		mImmSetOpenStatus       = (BOOL (WINAPI *)(HIMC, BOOL))             GetProcAddress(mHImmDll, "ImmSetOpenStatus");
		mImmGetConversionStatus = (BOOL (WINAPI *)(HIMC, LPDWORD, LPDWORD)) GetProcAddress(mHImmDll, "ImmGetConversionStatus");
		mImmSetConversionStatus = (BOOL (WINAPI *)(HIMC, DWORD, DWORD))     GetProcAddress(mHImmDll, "ImmSetConversionStatus");
		mImmGetCompostitionWindow = (BOOL (WINAPI *)(HIMC, LPCOMPOSITIONFORM))   GetProcAddress(mHImmDll, "ImmGetCompositionWindow");
		mImmSetCompostitionWindow = (BOOL (WINAPI *)(HIMC, LPCOMPOSITIONFORM))   GetProcAddress(mHImmDll, "ImmSetCompositionWindow");
		mImmGetCompositionString= (LONG (WINAPI *)(HIMC, DWORD, LPVOID, DWORD))					GetProcAddress(mHImmDll, "ImmGetCompositionStringW");
		mImmSetCompositionString= (BOOL (WINAPI *)(HIMC, DWORD, LPVOID, DWORD, LPVOID, DWORD))	GetProcAddress(mHImmDll, "ImmSetCompositionStringW");
		mImmSetCompositionFont  = (BOOL (WINAPI *)(HIMC, LPLOGFONTW))		GetProcAddress(mHImmDll, "ImmSetCompositionFontW");
		mImmSetCandidateWindow  = (BOOL (WINAPI *)(HIMC, LPCANDIDATEFORM))  GetProcAddress(mHImmDll, "ImmSetCandidateWindow");
		mImmNotifyIME			= (BOOL (WINAPI *)(HIMC, DWORD, DWORD, DWORD))	GetProcAddress(mHImmDll, "ImmNotifyIME");

		if (mImmIsIME == NULL ||
			mImmGetDefaultIMEWnd == NULL ||
			mImmGetContext == NULL ||
			mImmReleaseContext == NULL ||
			mImmGetOpenStatus == NULL ||
			mImmSetOpenStatus == NULL ||
			mImmGetConversionStatus == NULL ||
			mImmSetConversionStatus == NULL ||
			mImmGetCompostitionWindow == NULL ||
			mImmSetCompostitionWindow == NULL ||
			mImmGetCompositionString == NULL ||
			mImmSetCompositionString == NULL ||
			mImmSetCompositionFont == NULL ||
			mImmSetCandidateWindow == NULL ||
			mImmNotifyIME == NULL)
		{
			// If any of the above API entires are not found, we can't use IMM API.  
			// So, turn off the IMM support.  We should log some warning message in 
			// the case, since it is very unusual; these APIs are available from 
			// the beginning, and all versions of IMM32.DLL should have them all.  
			// Unfortunately, this code may be executed before initialization of 
			// the logging channel (llwarns), and we can't do it here...  Yes, this 
			// is one of disadvantages to use static constraction to DLL loading. 
			FreeLibrary(mHImmDll);
			mHImmDll = NULL;

			// If we unload the library, make sure all the function pointers are cleared
			mImmIsIME = NULL;
			mImmGetDefaultIMEWnd = NULL;
			mImmGetContext = NULL;
			mImmReleaseContext = NULL;
			mImmGetOpenStatus = NULL;
			mImmSetOpenStatus = NULL;
			mImmGetConversionStatus = NULL;
			mImmSetConversionStatus = NULL;
			mImmGetCompostitionWindow = NULL;
			mImmSetCompostitionWindow = NULL;
			mImmGetCompositionString = NULL;
			mImmSetCompositionString = NULL;
			mImmSetCompositionFont = NULL;
			mImmSetCandidateWindow = NULL;
			mImmNotifyIME = NULL;
		}
	}
}


// static 
BOOL	LLWinImm::isIME(HKL hkl)															
{ 
	if ( sTheInstance.mImmIsIME )
		return sTheInstance.mImmIsIME(hkl); 
	return FALSE;
}

// static 
HIMC		LLWinImm::getContext(HWND hwnd)
{
	if ( sTheInstance.mImmGetContext )
		return sTheInstance.mImmGetContext(hwnd); 
	return 0;
}

//static 
BOOL		LLWinImm::releaseContext(HWND hwnd, HIMC himc)
{ 
	if ( sTheInstance.mImmIsIME )
		return sTheInstance.mImmReleaseContext(hwnd, himc); 
	return FALSE;
}

// static 
BOOL		LLWinImm::getOpenStatus(HIMC himc)
{ 
	if ( sTheInstance.mImmGetOpenStatus )
		return sTheInstance.mImmGetOpenStatus(himc); 
	return FALSE;
}

// static 
BOOL		LLWinImm::setOpenStatus(HIMC himc, BOOL status)									
{ 
	if ( sTheInstance.mImmSetOpenStatus )
		return sTheInstance.mImmSetOpenStatus(himc, status); 
	return FALSE;
}

// static 
BOOL		LLWinImm::getConversionStatus(HIMC himc, LPDWORD conversion, LPDWORD sentence)	
{ 
	if ( sTheInstance.mImmGetConversionStatus )
		return sTheInstance.mImmGetConversionStatus(himc, conversion, sentence); 
	return FALSE;
}

// static 
BOOL		LLWinImm::setConversionStatus(HIMC himc, DWORD conversion, DWORD sentence)		
{ 
	if ( sTheInstance.mImmSetConversionStatus )
		return sTheInstance.mImmSetConversionStatus(himc, conversion, sentence); 
	return FALSE;
}

// static 
BOOL		LLWinImm::getCompositionWindow(HIMC himc, LPCOMPOSITIONFORM form)					
{ 
	if ( sTheInstance.mImmGetCompostitionWindow )
		return sTheInstance.mImmGetCompostitionWindow(himc, form);	
	return FALSE;
}

// static 
BOOL		LLWinImm::setCompositionWindow(HIMC himc, LPCOMPOSITIONFORM form)					
{ 
	if ( sTheInstance.mImmSetCompostitionWindow )
		return sTheInstance.mImmSetCompostitionWindow(himc, form);	
	return FALSE;
}


// static 
LONG		LLWinImm::getCompositionString(HIMC himc, DWORD index, LPVOID data, DWORD length)					
{ 
	if ( sTheInstance.mImmGetCompositionString )
		return sTheInstance.mImmGetCompositionString(himc, index, data, length);	
	return FALSE;
}


// static 
BOOL		LLWinImm::setCompositionString(HIMC himc, DWORD index, LPVOID pComp, DWORD compLength, LPVOID pRead, DWORD readLength)					
{ 
	if ( sTheInstance.mImmSetCompositionString )
		return sTheInstance.mImmSetCompositionString(himc, index, pComp, compLength, pRead, readLength);	
	return FALSE;
}

// static 
BOOL		LLWinImm::setCompositionFont(HIMC himc, LPLOGFONTW pFont)					
{ 
	if ( sTheInstance.mImmSetCompositionFont )
		return sTheInstance.mImmSetCompositionFont(himc, pFont);	
	return FALSE;
}

// static 
BOOL		LLWinImm::setCandidateWindow(HIMC himc, LPCANDIDATEFORM form)					
{ 
	if ( sTheInstance.mImmSetCandidateWindow )
		return sTheInstance.mImmSetCandidateWindow(himc, form);	
	return FALSE;
}

// static 
BOOL		LLWinImm::notifyIME(HIMC himc, DWORD action, DWORD index, DWORD value)					
{ 
	if ( sTheInstance.mImmNotifyIME )
		return sTheInstance.mImmNotifyIME(himc, action, index, value);	
	return FALSE;
}




// ----------------------------------------------------------------------------------------
LLWinImm::~LLWinImm()
{
	if (mHImmDll != NULL)
	{
		FreeLibrary(mHImmDll);
		mHImmDll = NULL;
	}
}


LLWindowWin32::LLWindowWin32(LLWindowCallbacks* callbacks,
							 const std::string& title, const std::string& name, S32 x, S32 y, S32 width,
							 S32 height, U32 flags, 
							 BOOL fullscreen, BOOL clearBg,
							 BOOL disable_vsync, BOOL use_gl,
							 BOOL ignore_pixel_depth,
							 U32 fsaa_samples)
	: LLWindow(callbacks, fullscreen, flags)
{
	
	//MAINT-516 -- force a load of opengl32.dll just in case windows went sideways 
	LoadLibrary(L"opengl32.dll");

	mFSAASamples = fsaa_samples;
	mIconResource = gIconResource;
	mOverrideAspectRatio = 0.f;
	mNativeAspectRatio = 0.f;
	mMousePositionModified = FALSE;
	mInputProcessingPaused = FALSE;
	mPreeditor = NULL;
	mKeyCharCode = 0;
	mKeyScanCode = 0;
	mKeyVirtualKey = 0;
	mhDC = NULL;
	mhRC = NULL;

	// Initialize the keyboard
	gKeyboard = new LLKeyboardWin32();
	gKeyboard->setCallbacks(callbacks);

	// Initialize the Drag and Drop functionality
	mDragDrop = new LLDragDropWin32;

	// Initialize (boot strap) the Language text input management,
	// based on the system's (user's) default settings.
	allowLanguageTextInput(mPreeditor, FALSE);

	WNDCLASS		wc;
	RECT			window_rect;

	// Set the window title
	if (title.empty())
	{
		mWindowTitle = new WCHAR[50];
		wsprintf(mWindowTitle, L"OpenGL Window");
	}
	else
	{
		mWindowTitle = new WCHAR[256]; // Assume title length < 255 chars.
		mbstowcs(mWindowTitle, title.c_str(), 255);
		mWindowTitle[255] = 0;
	}

	// Set the window class name
	if (name.empty())
	{
		mWindowClassName = new WCHAR[50];
		wsprintf(mWindowClassName, L"OpenGL Window");
	}
	else
	{
		mWindowClassName = new WCHAR[256]; // Assume title length < 255 chars.
		mbstowcs(mWindowClassName, name.c_str(), 255);
		mWindowClassName[255] = 0;
	}


	// We're not clipping yet
	SetRect( &mOldMouseClip, 0, 0, 0, 0 );

	// Make an instance of our window then define the window class
	mhInstance = GetModuleHandle(NULL);
	mWndProc = NULL;

	mSwapMethod = SWAP_METHOD_UNDEFINED;

	// No WPARAM yet.
	mLastSizeWParam = 0;

	// Windows GDI rects don't include rightmost pixel
	window_rect.left = (long) 0;
	window_rect.right = (long) width;
	window_rect.top = (long) 0;
	window_rect.bottom = (long) height;

	// Grab screen size to sanitize the window
	S32 window_border_y = GetSystemMetrics(SM_CYBORDER);
	S32 virtual_screen_x = GetSystemMetrics(SM_XVIRTUALSCREEN); 
	S32 virtual_screen_y = GetSystemMetrics(SM_YVIRTUALSCREEN); 
	S32 virtual_screen_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	S32 virtual_screen_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	if (x < virtual_screen_x) x = virtual_screen_x;
	if (y < virtual_screen_y - window_border_y) y = virtual_screen_y - window_border_y;

	if (x + width > virtual_screen_x + virtual_screen_width) x = virtual_screen_x + virtual_screen_width - width;
	if (y + height > virtual_screen_y + virtual_screen_height) y = virtual_screen_y + virtual_screen_height - height;

	if (!sIsClassRegistered)
	{
		// Force redraw when resized and create a private device context

		// Makes double click messages.
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;

		// Set message handler function
		wc.lpfnWndProc = (WNDPROC) mainWindowProc;

		// unused
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;

		wc.hInstance = mhInstance;
		wc.hIcon = LoadIcon(mhInstance, mIconResource);

		// We will set the cursor ourselves
		wc.hCursor = NULL;

		// background color is not used
		if (clearBg)
		{
			wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
		}
		else
		{
			wc.hbrBackground = (HBRUSH) NULL;
		}

		// we don't use windows menus
		wc.lpszMenuName = NULL;

		wc.lpszClassName = mWindowClassName;

		if (!RegisterClass(&wc))
		{
			OSMessageBox(mCallbacks->translateString("MBRegClassFailed"), 
				mCallbacks->translateString("MBError"), OSMB_OK);
			return;
		}
		sIsClassRegistered = TRUE;
	}

	//-----------------------------------------------------------------------
	// Get the current refresh rate
	//-----------------------------------------------------------------------

	DEVMODE dev_mode;
	::ZeroMemory(&dev_mode, sizeof(DEVMODE));
	dev_mode.dmSize = sizeof(DEVMODE);
	DWORD current_refresh;
	if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dev_mode))
	{
		current_refresh = dev_mode.dmDisplayFrequency;
		mNativeAspectRatio = ((F32)dev_mode.dmPelsWidth) / ((F32)dev_mode.dmPelsHeight);
	}
	else
	{
		current_refresh = 60;
	}

	//-----------------------------------------------------------------------
	// Drop resolution and go fullscreen
	// use a display mode with our desired size and depth, with a refresh
	// rate as close at possible to the users' default
	//-----------------------------------------------------------------------
	if (mFullscreen)
	{
		BOOL success = FALSE;
		DWORD closest_refresh = 0;

		for (S32 mode_num = 0;; mode_num++)
		{
			if (!EnumDisplaySettings(NULL, mode_num, &dev_mode))
			{
				break;
			}

			if (dev_mode.dmPelsWidth == width &&
				dev_mode.dmPelsHeight == height &&
				dev_mode.dmBitsPerPel == BITS_PER_PIXEL)
			{
				success = TRUE;
				if ((dev_mode.dmDisplayFrequency - current_refresh)
					< (closest_refresh - current_refresh))
				{
					closest_refresh = dev_mode.dmDisplayFrequency;
				}
			}
		}

		if (closest_refresh == 0)
		{
			LL_WARNS("Window") << "Couldn't find display mode " << width << " by " << height << " at " << BITS_PER_PIXEL << " bits per pixel" << LL_ENDL;
			//success = FALSE;

			if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dev_mode))
			{
				success = FALSE;
			}
			else
			{
				if (dev_mode.dmBitsPerPel == BITS_PER_PIXEL)
				{
					LL_WARNS("Window") << "Current BBP is OK falling back to that" << LL_ENDL;
					window_rect.right=width=dev_mode.dmPelsWidth;
					window_rect.bottom=height=dev_mode.dmPelsHeight;
					success = TRUE;
				}
				else
				{
					LL_WARNS("Window") << "Current BBP is BAD" << LL_ENDL;
					success = FALSE;
				}
			}
		}

		// If we found a good resolution, use it.
		if (success)
		{
			success = setDisplayResolution(width, height, BITS_PER_PIXEL, closest_refresh);
		}

		// Keep a copy of the actual current device mode in case we minimize 
		// and change the screen resolution.   JC
		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dev_mode);

		// If it failed, we don't want to run fullscreen
		if (success)
		{
			mFullscreen = TRUE;
			mFullscreenWidth   = dev_mode.dmPelsWidth;
			mFullscreenHeight  = dev_mode.dmPelsHeight;
			mFullscreenBits    = dev_mode.dmBitsPerPel;
			mFullscreenRefresh = dev_mode.dmDisplayFrequency;

			LL_INFOS("Window") << "Running at " << dev_mode.dmPelsWidth
				<< "x"   << dev_mode.dmPelsHeight
				<< "x"   << dev_mode.dmBitsPerPel
				<< " @ " << dev_mode.dmDisplayFrequency
				<< LL_ENDL;
		}
		else
		{
			mFullscreen = FALSE;
			mFullscreenWidth   = -1;
			mFullscreenHeight  = -1;
			mFullscreenBits    = -1;
			mFullscreenRefresh = -1;

			std::map<std::string,std::string> args;
			args["[WIDTH]"] = llformat("%d", width);
			args["[HEIGHT]"] = llformat ("%d", height);
			OSMessageBox(mCallbacks->translateString("MBFullScreenErr", args),
				mCallbacks->translateString("MBError"), OSMB_OK);
		}
	}

	// TODO: add this after resolving _WIN32_WINNT issue
	//	if (!fullscreen)
	//	{
	//		TRACKMOUSEEVENT track_mouse_event;
	//		track_mouse_event.cbSize = sizeof( TRACKMOUSEEVENT );
	//		track_mouse_event.dwFlags = TME_LEAVE;
	//		track_mouse_event.hwndTrack = mWindowHandle;
	//		track_mouse_event.dwHoverTime = HOVER_DEFAULT;
	//		TrackMouseEvent( &track_mouse_event ); 
	//	}


	//-----------------------------------------------------------------------
	// Create GL drawing context
	//-----------------------------------------------------------------------
	LLCoordScreen windowPos(x,y);
	LLCoordScreen windowSize(window_rect.right - window_rect.left,
							 window_rect.bottom - window_rect.top);
	if (!switchContext(mFullscreen, windowSize, TRUE, &windowPos))
	{
		return;
	}
	
	//start with arrow cursor
	initCursors();
	setCursor( UI_CURSOR_ARROW );

	// Initialize (boot strap) the Language text input management,
	// based on the system's (or user's) default settings.
	allowLanguageTextInput(NULL, FALSE);
}


LLWindowWin32::~LLWindowWin32()
{
	delete mDragDrop;

	delete [] mWindowTitle;
	mWindowTitle = NULL;

	delete [] mSupportedResolutions;
	mSupportedResolutions = NULL;

	delete mWindowClassName;
	mWindowClassName = NULL;
}

void LLWindowWin32::show()
{
	ShowWindow(mWindowHandle, SW_SHOW);
	SetForegroundWindow(mWindowHandle);
	SetFocus(mWindowHandle);
}

void LLWindowWin32::hide()
{
	setMouseClipping(FALSE);
	ShowWindow(mWindowHandle, SW_HIDE);
}

//virtual
void LLWindowWin32::minimize()
{
	setMouseClipping(FALSE);
	showCursor();
	ShowWindow(mWindowHandle, SW_MINIMIZE);
}

//virtual
void LLWindowWin32::restore()
{
	ShowWindow(mWindowHandle, SW_RESTORE);
	SetForegroundWindow(mWindowHandle);
	SetFocus(mWindowHandle);
}


// close() destroys all OS-specific code associated with a window.
// Usually called from LLWindowManager::destroyWindow()
void LLWindowWin32::close()
{
	LL_DEBUGS("Window") << "Closing LLWindowWin32" << LL_ENDL;
	// Is window is already closed?
	if (!mWindowHandle)
	{
		return;
	}

	mDragDrop->reset();

	// Make sure cursor is visible and we haven't mangled the clipping state.
	setMouseClipping(FALSE);
	showCursor();

	// Go back to screen mode written in the registry.
	if (mFullscreen)
	{
		resetDisplayResolution();
	}

	// Clean up remaining GL state
	LL_DEBUGS("Window") << "Shutting down GL" << LL_ENDL;
	gGLManager.shutdownGL();

	LL_DEBUGS("Window") << "Releasing Context" << LL_ENDL;
	if (mhRC)
	{
		if (!wglMakeCurrent(NULL, NULL))
		{
			LL_WARNS("Window") << "Release of DC and RC failed" << LL_ENDL;
		}

		if (!wglDeleteContext(mhRC))
		{
			LL_WARNS("Window") << "Release of rendering context failed" << LL_ENDL;
		}

		mhRC = NULL;
	}

	// Restore gamma to the system values.
	restoreGamma();

	if (mhDC && !ReleaseDC(mWindowHandle, mhDC))
	{
		LL_WARNS("Window") << "Release of ghDC failed" << LL_ENDL;
		mhDC = NULL;
	}

	LL_DEBUGS("Window") << "Destroying Window" << LL_ENDL;
	
	// Don't process events in our mainWindowProc any longer.
	SetWindowLong(mWindowHandle, GWL_USERDATA, NULL);

	// Make sure we don't leave a blank toolbar button.
	ShowWindow(mWindowHandle, SW_HIDE);

	// This causes WM_DESTROY to be sent *immediately*
	if (!DestroyWindow(mWindowHandle))
	{
		OSMessageBox(mCallbacks->translateString("MBDestroyWinFailed"),
			mCallbacks->translateString("MBShutdownErr"),
			OSMB_OK);
	}

	mWindowHandle = NULL;
}

BOOL LLWindowWin32::isValid()
{
	return (mWindowHandle != NULL);
}

BOOL LLWindowWin32::getVisible()
{
	return (mWindowHandle && IsWindowVisible(mWindowHandle));
}

BOOL LLWindowWin32::getMinimized()
{
	return (mWindowHandle && IsIconic(mWindowHandle));
}

BOOL LLWindowWin32::getMaximized()
{
	return (mWindowHandle && IsZoomed(mWindowHandle));
}

BOOL LLWindowWin32::maximize()
{
	BOOL success = FALSE;
	if (!mWindowHandle) return success;

	WINDOWPLACEMENT placement;
	placement.length = sizeof(WINDOWPLACEMENT);

	success = GetWindowPlacement(mWindowHandle, &placement);
	if (!success) return success;

	placement.showCmd = SW_MAXIMIZE;

	success = SetWindowPlacement(mWindowHandle, &placement);
	return success;
}

BOOL LLWindowWin32::getFullscreen()
{
	return mFullscreen;
}

BOOL LLWindowWin32::getPosition(LLCoordScreen *position)
{
	RECT window_rect;

	if (!mWindowHandle ||
		!GetWindowRect(mWindowHandle, &window_rect) ||
		NULL == position)
	{
		return FALSE;
	}

	position->mX = window_rect.left;
	position->mY = window_rect.top;
	return TRUE;
}

BOOL LLWindowWin32::getSize(LLCoordScreen *size)
{
	RECT window_rect;

	if (!mWindowHandle ||
		!GetWindowRect(mWindowHandle, &window_rect) ||
		NULL == size)
	{
		return FALSE;
	}

	size->mX = window_rect.right - window_rect.left;
	size->mY = window_rect.bottom - window_rect.top;
	return TRUE;
}

BOOL LLWindowWin32::getSize(LLCoordWindow *size)
{
	RECT client_rect;

	if (!mWindowHandle ||
		!GetClientRect(mWindowHandle, &client_rect) ||
		NULL == size)
	{
		return FALSE;
	}

	size->mX = client_rect.right - client_rect.left;
	size->mY = client_rect.bottom - client_rect.top;
	return TRUE;
}

BOOL LLWindowWin32::setPosition(const LLCoordScreen position)
{
	LLCoordScreen size;

	if (!mWindowHandle)
	{
		return FALSE;
	}
	getSize(&size);
	moveWindow(position, size);
	return TRUE;
}

BOOL LLWindowWin32::setSizeImpl(const LLCoordScreen size)
{
	LLCoordScreen position;

	getPosition(&position);
	if (!mWindowHandle)
	{
		return FALSE;
	}

	WINDOWPLACEMENT placement;
	placement.length = sizeof(WINDOWPLACEMENT);

	if (!GetWindowPlacement(mWindowHandle, &placement)) return FALSE;

	placement.showCmd = SW_RESTORE;

	if (!SetWindowPlacement(mWindowHandle, &placement)) return FALSE;

	moveWindow(position, size);
	return TRUE;
}

BOOL LLWindowWin32::setSizeImpl(const LLCoordWindow size)
{
	RECT window_rect = {0, 0, size.mX, size.mY };
	DWORD dw_ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD dw_style = WS_OVERLAPPEDWINDOW;

	AdjustWindowRectEx(&window_rect, dw_style, FALSE, dw_ex_style);

	return setSizeImpl(LLCoordScreen(window_rect.right - window_rect.left, window_rect.bottom - window_rect.top));
}

// changing fullscreen resolution
BOOL LLWindowWin32::switchContext(BOOL fullscreen, const LLCoordScreen &size, BOOL disable_vsync, const LLCoordScreen * const posp)
{
	GLuint	pixel_format;
	DEVMODE dev_mode;
	::ZeroMemory(&dev_mode, sizeof(DEVMODE));
	dev_mode.dmSize = sizeof(DEVMODE);
	DWORD	current_refresh;
	DWORD	dw_ex_style;
	DWORD	dw_style;
	RECT	window_rect = {0, 0, 0, 0};
	S32 width = size.mX;
	S32 height = size.mY;
	BOOL auto_show = FALSE;

	if (mhRC)	
	{
		auto_show = TRUE;
		resetDisplayResolution();
	}

	if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dev_mode))
	{
		current_refresh = dev_mode.dmDisplayFrequency;
	}
	else
	{
		current_refresh = 60;
	}

	gGLManager.shutdownGL();
	//destroy gl context
	if (mhRC)
	{
		if (!wglMakeCurrent(NULL, NULL))
		{
			LL_WARNS("Window") << "Release of DC and RC failed" << LL_ENDL;
		}

		if (!wglDeleteContext(mhRC))
		{
			LL_WARNS("Window") << "Release of rendering context failed" << LL_ENDL;
		}

		mhRC = NULL;
	}

	if (fullscreen)
	{
		mFullscreen = TRUE;
		BOOL success = FALSE;
		DWORD closest_refresh = 0;

		for (S32 mode_num = 0;; mode_num++)
		{
			if (!EnumDisplaySettings(NULL, mode_num, &dev_mode))
			{
				break;
			}

			if (dev_mode.dmPelsWidth == width &&
				dev_mode.dmPelsHeight == height &&
				dev_mode.dmBitsPerPel == BITS_PER_PIXEL)
			{
				success = TRUE;
				if ((dev_mode.dmDisplayFrequency - current_refresh)
					< (closest_refresh - current_refresh))
				{
					closest_refresh = dev_mode.dmDisplayFrequency;
				}
			}
		}

		if (closest_refresh == 0)
		{
			LL_WARNS("Window") << "Couldn't find display mode " << width << " by " << height << " at " << BITS_PER_PIXEL << " bits per pixel" << LL_ENDL;
			return FALSE;
		}

		// If we found a good resolution, use it.
		if (success)
		{
			success = setDisplayResolution(width, height, BITS_PER_PIXEL, closest_refresh);
		}

		// Keep a copy of the actual current device mode in case we minimize 
		// and change the screen resolution.   JC
		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dev_mode);

		if (success)
		{
			mFullscreen = TRUE;
			mFullscreenWidth   = dev_mode.dmPelsWidth;
			mFullscreenHeight  = dev_mode.dmPelsHeight;
			mFullscreenBits    = dev_mode.dmBitsPerPel;
			mFullscreenRefresh = dev_mode.dmDisplayFrequency;

			LL_INFOS("Window") << "Running at " << dev_mode.dmPelsWidth
				<< "x"   << dev_mode.dmPelsHeight
				<< "x"   << dev_mode.dmBitsPerPel
				<< " @ " << dev_mode.dmDisplayFrequency
				<< LL_ENDL;

			window_rect.left = (long) 0;
			window_rect.right = (long) width;			// Windows GDI rects don't include rightmost pixel
			window_rect.top = (long) 0;
			window_rect.bottom = (long) height;
			dw_ex_style = WS_EX_APPWINDOW;
			dw_style = WS_POPUP;

			// Move window borders out not to cover window contents.
			// This converts client rect to window rect, i.e. expands it by the window border size.
			AdjustWindowRectEx(&window_rect, dw_style, FALSE, dw_ex_style);
		}
		// If it failed, we don't want to run fullscreen
		else
		{
			mFullscreen = FALSE;
			mFullscreenWidth   = -1;
			mFullscreenHeight  = -1;
			mFullscreenBits    = -1;
			mFullscreenRefresh = -1;

			LL_INFOS("Window") << "Unable to run fullscreen at " << width << "x" << height << LL_ENDL;
			return FALSE;
		}
	}
	else
	{
		mFullscreen = FALSE;
		window_rect.left = (long) (posp ? posp->mX : 0);
		window_rect.right = (long) width + window_rect.left;			// Windows GDI rects don't include rightmost pixel
		window_rect.top = (long) (posp ? posp->mY : 0);
		window_rect.bottom = (long) height + window_rect.top;
		// Window with an edge
		dw_ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dw_style = WS_OVERLAPPEDWINDOW;
	}


	// don't post quit messages when destroying old windows
	mPostQuit = FALSE;

	// create window
	DestroyWindow(mWindowHandle);
	mWindowHandle = CreateWindowEx(dw_ex_style,
		mWindowClassName,
		mWindowTitle,
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dw_style,
		window_rect.left,								// x pos
		window_rect.top,								// y pos
		window_rect.right - window_rect.left,			// width
		window_rect.bottom - window_rect.top,			// height
		NULL,
		NULL,
		mhInstance,
		NULL);

	LL_INFOS("Window") << "window is created." << llendl ;

	//-----------------------------------------------------------------------
	// Create GL drawing context
	//-----------------------------------------------------------------------
	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR), 
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, 
			PFD_TYPE_RGBA,
			BITS_PER_PIXEL,
			0, 0, 0, 0, 0, 0,	// RGB bits and shift, unused
			8,					// alpha bits
			0,					// alpha shift
			0,					// accum bits
			0, 0, 0, 0,			// accum RGBA
			24,					// depth bits
			8,					// stencil bits, avi added for stencil test
			0,
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
	};

	if (!(mhDC = GetDC(mWindowHandle)))
	{
		close();
		OSMessageBox(mCallbacks->translateString("MBDevContextErr"),
			mCallbacks->translateString("MBError"), OSMB_OK);
		return FALSE;
	}

	LL_INFOS("Window") << "Device context retrieved." << llendl ;

	if (!(pixel_format = ChoosePixelFormat(mhDC, &pfd)))
	{
		close();
		OSMessageBox(mCallbacks->translateString("MBPixelFmtErr"),
			mCallbacks->translateString("MBError"), OSMB_OK);
		return FALSE;
	}

	LL_INFOS("Window") << "Pixel format chosen." << llendl ;

	// Verify what pixel format we actually received.
	if (!DescribePixelFormat(mhDC, pixel_format, sizeof(PIXELFORMATDESCRIPTOR),
		&pfd))
	{
		close();
		OSMessageBox(mCallbacks->translateString("MBPixelFmtDescErr"),
			mCallbacks->translateString("MBError"), OSMB_OK);
		return FALSE;
	}

	// (EXP-1765) dump pixel data to see if there is a pattern that leads to unreproducible crash
	LL_INFOS("Window") << "--- begin pixel format dump ---" << llendl ;
	LL_INFOS("Window") << "pixel_format is " << pixel_format << llendl ;
	LL_INFOS("Window") << "pfd.nSize:            " << pfd.nSize << llendl ;
	LL_INFOS("Window") << "pfd.nVersion:         " << pfd.nVersion << llendl ;
	LL_INFOS("Window") << "pfd.dwFlags:          0x" << std::hex << pfd.dwFlags << std::dec << llendl ;
	LL_INFOS("Window") << "pfd.iPixelType:       " << (int)pfd.iPixelType << llendl ;
	LL_INFOS("Window") << "pfd.cColorBits:       " << (int)pfd.cColorBits << llendl ;
	LL_INFOS("Window") << "pfd.cRedBits:         " << (int)pfd.cRedBits << llendl ;
	LL_INFOS("Window") << "pfd.cRedShift:        " << (int)pfd.cRedShift << llendl ;
	LL_INFOS("Window") << "pfd.cGreenBits:       " << (int)pfd.cGreenBits << llendl ;
	LL_INFOS("Window") << "pfd.cGreenShift:      " << (int)pfd.cGreenShift << llendl ;
	LL_INFOS("Window") << "pfd.cBlueBits:        " << (int)pfd.cBlueBits << llendl ;
	LL_INFOS("Window") << "pfd.cBlueShift:       " << (int)pfd.cBlueShift << llendl ;
	LL_INFOS("Window") << "pfd.cAlphaBits:       " << (int)pfd.cAlphaBits << llendl ;
	LL_INFOS("Window") << "pfd.cAlphaShift:      " << (int)pfd.cAlphaShift << llendl ;
	LL_INFOS("Window") << "pfd.cAccumBits:       " << (int)pfd.cAccumBits << llendl ;
	LL_INFOS("Window") << "pfd.cAccumRedBits:    " << (int)pfd.cAccumRedBits << llendl ;
	LL_INFOS("Window") << "pfd.cAccumGreenBits:  " << (int)pfd.cAccumGreenBits << llendl ;
	LL_INFOS("Window") << "pfd.cAccumBlueBits:   " << (int)pfd.cAccumBlueBits << llendl ;
	LL_INFOS("Window") << "pfd.cAccumAlphaBits:  " << (int)pfd.cAccumAlphaBits << llendl ;
	LL_INFOS("Window") << "pfd.cDepthBits:       " << (int)pfd.cDepthBits << llendl ;
	LL_INFOS("Window") << "pfd.cStencilBits:     " << (int)pfd.cStencilBits << llendl ;
	LL_INFOS("Window") << "pfd.cAuxBuffers:      " << (int)pfd.cAuxBuffers << llendl ;
	LL_INFOS("Window") << "pfd.iLayerType:       " << (int)pfd.iLayerType << llendl ;
	LL_INFOS("Window") << "pfd.bReserved:        " << (int)pfd.bReserved << llendl ;
	LL_INFOS("Window") << "pfd.dwLayerMask:      " << pfd.dwLayerMask << llendl ;
	LL_INFOS("Window") << "pfd.dwVisibleMask:    " << pfd.dwVisibleMask << llendl ;
	LL_INFOS("Window") << "pfd.dwDamageMask:     " << pfd.dwDamageMask << llendl ;
	LL_INFOS("Window") << "--- end pixel format dump ---" << llendl ;

	if (pfd.cColorBits < 32)
	{
		close();
		OSMessageBox(mCallbacks->translateString("MBTrueColorWindow"),
			mCallbacks->translateString("MBError"), OSMB_OK);
		return FALSE;
	}

	if (pfd.cAlphaBits < 8)
	{
		close();
		OSMessageBox(mCallbacks->translateString("MBAlpha"),
			mCallbacks->translateString("MBError"), OSMB_OK);
		return FALSE;
	}

	if (!SetPixelFormat(mhDC, pixel_format, &pfd))
	{
		close();
		OSMessageBox(mCallbacks->translateString("MBPixelFmtSetErr"),
			mCallbacks->translateString("MBError"), OSMB_OK);
		return FALSE;
	}

	if (!(mhRC = wglCreateContext(mhDC)))
	{
		close();
		OSMessageBox(mCallbacks->translateString("MBGLContextErr"),
			mCallbacks->translateString("MBError"), OSMB_OK);
		return FALSE;
	}

	if (!wglMakeCurrent(mhDC, mhRC))
	{
		close();
		OSMessageBox(mCallbacks->translateString("MBGLContextActErr"),
			mCallbacks->translateString("MBError"), OSMB_OK);
		return FALSE;
	}

	LL_INFOS("Window") << "Drawing context is created." << llendl ;

	gGLManager.initWGL();
	
	if (wglChoosePixelFormatARB)
	{
		// OK, at this point, use the ARB wglChoosePixelFormatsARB function to see if we
		// can get exactly what we want.
		GLint attrib_list[256];
		S32 cur_attrib = 0;

		attrib_list[cur_attrib++] = WGL_DEPTH_BITS_ARB;
		attrib_list[cur_attrib++] = 24;

		attrib_list[cur_attrib++] = WGL_STENCIL_BITS_ARB;
		attrib_list[cur_attrib++] = 8;

		attrib_list[cur_attrib++] = WGL_DRAW_TO_WINDOW_ARB;
		attrib_list[cur_attrib++] = GL_TRUE;

		attrib_list[cur_attrib++] = WGL_ACCELERATION_ARB;
		attrib_list[cur_attrib++] = WGL_FULL_ACCELERATION_ARB;

		attrib_list[cur_attrib++] = WGL_SUPPORT_OPENGL_ARB;
		attrib_list[cur_attrib++] = GL_TRUE;

		attrib_list[cur_attrib++] = WGL_DOUBLE_BUFFER_ARB;
		attrib_list[cur_attrib++] = GL_TRUE;

		attrib_list[cur_attrib++] = WGL_COLOR_BITS_ARB;
		attrib_list[cur_attrib++] = 24;

		attrib_list[cur_attrib++] = WGL_ALPHA_BITS_ARB;
		attrib_list[cur_attrib++] = 8;

		U32 end_attrib = 0;
		if (mFSAASamples > 0)
		{
			end_attrib = cur_attrib;
			attrib_list[cur_attrib++] = WGL_SAMPLE_BUFFERS_ARB;
			attrib_list[cur_attrib++] = GL_TRUE;

			attrib_list[cur_attrib++] = WGL_SAMPLES_ARB;
			attrib_list[cur_attrib++] = mFSAASamples;
		}

		// End the list
		attrib_list[cur_attrib++] = 0;

		GLint pixel_formats[256];
		U32 num_formats = 0;

		// First we try and get a 32 bit depth pixel format
		BOOL result = wglChoosePixelFormatARB(mhDC, attrib_list, NULL, 256, pixel_formats, &num_formats);
		
		while(!result && mFSAASamples > 0) 
		{
			llwarns << "FSAASamples: " << mFSAASamples << " not supported." << llendl ;

			mFSAASamples /= 2 ; //try to decrease sample pixel number until to disable anti-aliasing
			if(mFSAASamples < 2)
			{
				mFSAASamples = 0 ;
			}

			if (mFSAASamples > 0)
			{
				attrib_list[end_attrib + 3] = mFSAASamples;
			}
			else
			{
				cur_attrib = end_attrib ;
				end_attrib = 0 ;
				attrib_list[cur_attrib++] = 0 ; //end
			}
			result = wglChoosePixelFormatARB(mhDC, attrib_list, NULL, 256, pixel_formats, &num_formats);

			if(result)
			{
				llwarns << "Only support FSAASamples: " << mFSAASamples << llendl ;
			}
		}

		if (!result)
		{
			llwarns << "mFSAASamples: " << mFSAASamples << llendl ;

			close();
			show_window_creation_error("Error after wglChoosePixelFormatARB 32-bit");
			return FALSE;
		}

		if (!num_formats)
		{
			if (end_attrib > 0)
			{
				LL_INFOS("Window") << "No valid pixel format for " << mFSAASamples << "x anti-aliasing." << LL_ENDL;
				attrib_list[end_attrib] = 0;

				BOOL result = wglChoosePixelFormatARB(mhDC, attrib_list, NULL, 256, pixel_formats, &num_formats);
				if (!result)
				{
					close();
					show_window_creation_error("Error after wglChoosePixelFormatARB 32-bit no AA");
					return FALSE;
				}
			}

			if (!num_formats)
			{
				LL_INFOS("Window") << "No 32 bit z-buffer, trying 24 bits instead" << LL_ENDL;
				// Try 24-bit format
				attrib_list[1] = 24;
				BOOL result = wglChoosePixelFormatARB(mhDC, attrib_list, NULL, 256, pixel_formats, &num_formats);
				if (!result)
				{
					close();
					show_window_creation_error("Error after wglChoosePixelFormatARB 24-bit");
					return FALSE;
				}

				if (!num_formats)
				{
					LL_WARNS("Window") << "Couldn't get 24 bit z-buffer,trying 16 bits instead!" << LL_ENDL;
					attrib_list[1] = 16;
					BOOL result = wglChoosePixelFormatARB(mhDC, attrib_list, NULL, 256, pixel_formats, &num_formats);
					if (!result || !num_formats)
					{
						close();
						show_window_creation_error("Error after wglChoosePixelFormatARB 16-bit");
						return FALSE;
					}
				}
			}

			LL_INFOS("Window") << "Choosing pixel formats: " << num_formats << " pixel formats returned" << LL_ENDL;
		}

		LL_INFOS("Window") << "pixel formats done." << llendl ;

		S32 swap_method = 0;
		S32 cur_format = num_formats-1;
		GLint swap_query = WGL_SWAP_METHOD_ARB;

		BOOL found_format = FALSE;

		while (!found_format && wglGetPixelFormatAttribivARB(mhDC, pixel_format, 0, 1, &swap_query, &swap_method))
		{
			if (swap_method == WGL_SWAP_UNDEFINED_ARB || cur_format <= 0)
			{
				found_format = TRUE;
			}
			else
			{
				--cur_format;
			}
		}
		
		pixel_format = pixel_formats[cur_format];
		
		if (mhDC != 0)											// Does The Window Have A Device Context?
		{
			wglMakeCurrent(mhDC, 0);							// Set The Current Active Rendering Context To Zero
			if (mhRC != 0)										// Does The Window Have A Rendering Context?
			{
				wglDeleteContext (mhRC);							// Release The Rendering Context
				mhRC = 0;										// Zero The Rendering Context

			}
			ReleaseDC (mWindowHandle, mhDC);						// Release The Device Context
			mhDC = 0;											// Zero The Device Context
		}
		DestroyWindow (mWindowHandle);									// Destroy The Window
		

		mWindowHandle = CreateWindowEx(dw_ex_style,
			mWindowClassName,
			mWindowTitle,
			WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dw_style,
			window_rect.left,								// x pos
			window_rect.top,								// y pos
			window_rect.right - window_rect.left,			// width
			window_rect.bottom - window_rect.top,			// height
			NULL,
			NULL,
			mhInstance,
			NULL);

		LL_INFOS("Window") << "recreate window done." << llendl ;

		if (!(mhDC = GetDC(mWindowHandle)))
		{
			close();
			OSMessageBox(mCallbacks->translateString("MBDevContextErr"), mCallbacks->translateString("MBError"), OSMB_OK);
			return FALSE;
		}

		if (!SetPixelFormat(mhDC, pixel_format, &pfd))
		{
			close();
			OSMessageBox(mCallbacks->translateString("MBPixelFmtSetErr"),
				mCallbacks->translateString("MBError"), OSMB_OK);
			return FALSE;
		}

		if (wglGetPixelFormatAttribivARB(mhDC, pixel_format, 0, 1, &swap_query, &swap_method))
		{
			switch (swap_method)
			{
			case WGL_SWAP_EXCHANGE_ARB:
				mSwapMethod = SWAP_METHOD_EXCHANGE;
				LL_DEBUGS("Window") << "Swap Method: Exchange" << LL_ENDL;
				break;
			case WGL_SWAP_COPY_ARB:
				mSwapMethod = SWAP_METHOD_COPY;
				LL_DEBUGS("Window") << "Swap Method: Copy" << LL_ENDL;
				break;
			case WGL_SWAP_UNDEFINED_ARB:
				mSwapMethod = SWAP_METHOD_UNDEFINED;
				LL_DEBUGS("Window") << "Swap Method: Undefined" << LL_ENDL;
				break;
			default:
				mSwapMethod = SWAP_METHOD_UNDEFINED;
				LL_DEBUGS("Window") << "Swap Method: Unknown" << LL_ENDL;
				break;
			}
		}		
	}
	else
	{
		LL_WARNS("Window") << "No wgl_ARB_pixel_format extension, using default ChoosePixelFormat!" << LL_ENDL;
	}

	// Verify what pixel format we actually received.
	if (!DescribePixelFormat(mhDC, pixel_format, sizeof(PIXELFORMATDESCRIPTOR),
		&pfd))
	{
		close();
		OSMessageBox(mCallbacks->translateString("MBPixelFmtDescErr"), mCallbacks->translateString("MBError"), OSMB_OK);
		return FALSE;
	}

	LL_INFOS("Window") << "GL buffer: Color Bits " << S32(pfd.cColorBits) 
		<< " Alpha Bits " << S32(pfd.cAlphaBits)
		<< " Depth Bits " << S32(pfd.cDepthBits) 
		<< LL_ENDL;

	// make sure we have 32 bits per pixel
	if (pfd.cColorBits < 32 || GetDeviceCaps(mhDC, BITSPIXEL) < 32)
	{
		close();
		OSMessageBox(mCallbacks->translateString("MBTrueColorWindow"), mCallbacks->translateString("MBError"), OSMB_OK);
		return FALSE;
	}

	if (pfd.cAlphaBits < 8)
	{
		close();
		OSMessageBox(mCallbacks->translateString("MBAlpha"), mCallbacks->translateString("MBError"), OSMB_OK);
		return FALSE;
	}

	mhRC = 0;
	if (wglCreateContextAttribsARB)
	{ //attempt to create a specific versioned context
		S32 attribs[] = 
		{ //start at 4.2
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 2,
			WGL_CONTEXT_PROFILE_MASK_ARB,  LLRender::sGLCoreProfile ? WGL_CONTEXT_CORE_PROFILE_BIT_ARB : WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
			WGL_CONTEXT_FLAGS_ARB, gDebugGL ? WGL_CONTEXT_DEBUG_BIT_ARB : 0,
			0
		};

		bool done = false;
		while (!done)
		{
			mhRC = wglCreateContextAttribsARB(mhDC, mhRC, attribs);

			if (!mhRC)
			{
				if (attribs[3] > 0)
				{ //decrement minor version
					attribs[3]--;
				}
				else if (attribs[1] > 3)
				{ //decrement major version and start minor version over at 3
					attribs[1]--;
					attribs[3] = 3;
				}
				else
				{ //we reached 3.0 and still failed, bail out
					done = true;
				}
			}
			else
			{
				llinfos << "Created OpenGL " << llformat("%d.%d", attribs[1], attribs[3]) << 
					(LLRender::sGLCoreProfile ? " core" : " compatibility") << " context." << llendl;
				done = true;

				if (LLRender::sGLCoreProfile)
				{
					LLGLSLShader::sNoFixedFunction = true;
				}
			}
		}
	}

	if (!mhRC && !(mhRC = wglCreateContext(mhDC)))
	{
		close();
		OSMessageBox(mCallbacks->translateString("MBGLContextErr"), mCallbacks->translateString("MBError"), OSMB_OK);
		return FALSE;
	}

	if (!wglMakeCurrent(mhDC, mhRC))
	{
		close();
		OSMessageBox(mCallbacks->translateString("MBGLContextActErr"), mCallbacks->translateString("MBError"), OSMB_OK);
		return FALSE;
	}

	if (!gGLManager.initGL())
	{
		close();
		OSMessageBox(mCallbacks->translateString("MBVideoDrvErr"), mCallbacks->translateString("MBError"), OSMB_OK);
		return FALSE;
	}
	
	// Disable vertical sync for swap
	if (disable_vsync && wglSwapIntervalEXT)
	{
		LL_DEBUGS("Window") << "Disabling vertical sync" << LL_ENDL;
		wglSwapIntervalEXT(0);
	}
	else
	{
		LL_DEBUGS("Window") << "Keeping vertical sync" << LL_ENDL;
	}

	SetWindowLong(mWindowHandle, GWL_USERDATA, (U32)this);

	// register this window as handling drag/drop events from the OS
	DragAcceptFiles( mWindowHandle, TRUE );

	mDragDrop->init( mWindowHandle );
	
	//register joystick timer callback
	SetTimer( mWindowHandle, 0, 1000 / 30, NULL ); // 30 fps timer

	// ok to post quit messages now
	mPostQuit = TRUE;

	if (auto_show)
	{
		show();
		glClearColor(0.0f, 0.0f, 0.0f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT);
		swapBuffers();
	}

	return TRUE;
}

void LLWindowWin32::moveWindow( const LLCoordScreen& position, const LLCoordScreen& size )
{
	if( mIsMouseClipping )
	{
		RECT client_rect_in_screen_space;
		if( getClientRectInScreenSpace( &client_rect_in_screen_space ) )
		{
			ClipCursor( &client_rect_in_screen_space );
		}
	}

	// if the window was already maximized, MoveWindow seems to still set the maximized flag even if
	// the window is smaller than maximized.
	// So we're going to do a restore first (which is a ShowWindow call) (SL-44655).

	// THIS CAUSES DEV-15484 and DEV-15949 
	//ShowWindow(mWindowHandle, SW_RESTORE);
	// NOW we can call MoveWindow
	MoveWindow(mWindowHandle, position.mX, position.mY, size.mX, size.mY, TRUE);
}

BOOL LLWindowWin32::setCursorPosition(const LLCoordWindow position)
{
	mMousePositionModified = TRUE;
	if (!mWindowHandle)
	{
		return FALSE;
	}


	// Inform the application of the new mouse position (needed for per-frame
	// hover/picking to function).
	mCallbacks->handleMouseMove(this, position.convert(), (MASK)0);
	
	// DEV-18951 VWR-8524 Camera moves wildly when alt-clicking.
	// Because we have preemptively notified the application of the new
	// mouse position via handleMouseMove() above, we need to clear out
	// any stale mouse move events.  RN/JC
	MSG msg;
	while (PeekMessage(&msg, NULL, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE))
	{ }

	LLCoordScreen screen_pos(position.convert());
	return ::SetCursorPos(screen_pos.mX, screen_pos.mY);
}

BOOL LLWindowWin32::getCursorPosition(LLCoordWindow *position)
{
	POINT cursor_point;

	if (!mWindowHandle 
		|| !GetCursorPos(&cursor_point)
		|| !position)
	{
		return FALSE;
	}

	*position = LLCoordScreen(cursor_point.x, cursor_point.y).convert();
	return TRUE;
}

void LLWindowWin32::hideCursor()
{
	while (ShowCursor(FALSE) >= 0)
	{
		// nothing, wait for cursor to push down
	}
	mCursorHidden = TRUE;
	mHideCursorPermanent = TRUE;
}

void LLWindowWin32::showCursor()
{
	// makes sure the cursor shows up
	while (ShowCursor(TRUE) < 0)
	{
		// do nothing, wait for cursor to pop out
	}
	mCursorHidden = FALSE;
	mHideCursorPermanent = FALSE;
}

void LLWindowWin32::showCursorFromMouseMove()
{
	if (!mHideCursorPermanent)
	{
		showCursor();
	}
}

void LLWindowWin32::hideCursorUntilMouseMove()
{
	if (!mHideCursorPermanent)
	{
		hideCursor();
		mHideCursorPermanent = FALSE;
	}
}

BOOL LLWindowWin32::isCursorHidden()
{
	return mCursorHidden;
}


HCURSOR LLWindowWin32::loadColorCursor(LPCTSTR name)
{
	return (HCURSOR)LoadImage(mhInstance,
							  name,
							  IMAGE_CURSOR,
							  0,	// default width
							  0,	// default height
							  LR_DEFAULTCOLOR);
}


void LLWindowWin32::initCursors()
{
	mCursor[ UI_CURSOR_ARROW ]		= LoadCursor(NULL, IDC_ARROW);
	mCursor[ UI_CURSOR_WAIT ]		= LoadCursor(NULL, IDC_WAIT);
	mCursor[ UI_CURSOR_HAND ]		= LoadCursor(NULL, IDC_HAND);
	mCursor[ UI_CURSOR_IBEAM ]		= LoadCursor(NULL, IDC_IBEAM);
	mCursor[ UI_CURSOR_CROSS ]		= LoadCursor(NULL, IDC_CROSS);
	mCursor[ UI_CURSOR_SIZENWSE ]	= LoadCursor(NULL, IDC_SIZENWSE);
	mCursor[ UI_CURSOR_SIZENESW ]	= LoadCursor(NULL, IDC_SIZENESW);
	mCursor[ UI_CURSOR_SIZEWE ]		= LoadCursor(NULL, IDC_SIZEWE);  
	mCursor[ UI_CURSOR_SIZENS ]		= LoadCursor(NULL, IDC_SIZENS);  
	mCursor[ UI_CURSOR_NO ]			= LoadCursor(NULL, IDC_NO);
	mCursor[ UI_CURSOR_WORKING ]	= LoadCursor(NULL, IDC_APPSTARTING); 

	HMODULE module = GetModuleHandle(NULL);
	mCursor[ UI_CURSOR_TOOLGRAB ]	= LoadCursor(module, TEXT("TOOLGRAB"));
	mCursor[ UI_CURSOR_TOOLLAND ]	= LoadCursor(module, TEXT("TOOLLAND"));
	mCursor[ UI_CURSOR_TOOLFOCUS ]	= LoadCursor(module, TEXT("TOOLFOCUS"));
	mCursor[ UI_CURSOR_TOOLCREATE ]	= LoadCursor(module, TEXT("TOOLCREATE"));
	mCursor[ UI_CURSOR_ARROWDRAG ]	= LoadCursor(module, TEXT("ARROWDRAG"));
	mCursor[ UI_CURSOR_ARROWCOPY ]	= LoadCursor(module, TEXT("ARROWCOPY"));
	mCursor[ UI_CURSOR_ARROWDRAGMULTI ]	= LoadCursor(module, TEXT("ARROWDRAGMULTI"));
	mCursor[ UI_CURSOR_ARROWCOPYMULTI ]	= LoadCursor(module, TEXT("ARROWCOPYMULTI"));
	mCursor[ UI_CURSOR_NOLOCKED ]	= LoadCursor(module, TEXT("NOLOCKED"));
	mCursor[ UI_CURSOR_ARROWLOCKED ]= LoadCursor(module, TEXT("ARROWLOCKED"));
	mCursor[ UI_CURSOR_GRABLOCKED ]	= LoadCursor(module, TEXT("GRABLOCKED"));
	mCursor[ UI_CURSOR_TOOLTRANSLATE ]	= LoadCursor(module, TEXT("TOOLTRANSLATE"));
	mCursor[ UI_CURSOR_TOOLROTATE ]	= LoadCursor(module, TEXT("TOOLROTATE")); 
	mCursor[ UI_CURSOR_TOOLSCALE ]	= LoadCursor(module, TEXT("TOOLSCALE"));
	mCursor[ UI_CURSOR_TOOLCAMERA ]	= LoadCursor(module, TEXT("TOOLCAMERA"));
	mCursor[ UI_CURSOR_TOOLPAN ]	= LoadCursor(module, TEXT("TOOLPAN"));
	mCursor[ UI_CURSOR_TOOLZOOMIN ] = LoadCursor(module, TEXT("TOOLZOOMIN"));
	mCursor[ UI_CURSOR_TOOLPICKOBJECT3 ] = LoadCursor(module, TEXT("TOOLPICKOBJECT3"));
	mCursor[ UI_CURSOR_PIPETTE ] = LoadCursor(module, TEXT("TOOLPIPETTE"));
	mCursor[ UI_CURSOR_TOOLSIT ]	= LoadCursor(module, TEXT("TOOLSIT"));
	mCursor[ UI_CURSOR_TOOLBUY ]	= LoadCursor(module, TEXT("TOOLBUY"));
	mCursor[ UI_CURSOR_TOOLOPEN ]	= LoadCursor(module, TEXT("TOOLOPEN"));

	// Color cursors
	mCursor[ UI_CURSOR_TOOLPLAY ]		= loadColorCursor(TEXT("TOOLPLAY"));
	mCursor[ UI_CURSOR_TOOLPAUSE ]		= loadColorCursor(TEXT("TOOLPAUSE"));
	mCursor[ UI_CURSOR_TOOLMEDIAOPEN ]	= loadColorCursor(TEXT("TOOLMEDIAOPEN"));

	// Note: custom cursors that are not found make LoadCursor() return NULL.
	for( S32 i = 0; i < UI_CURSOR_COUNT; i++ )
	{
		if( !mCursor[i] )
		{
			mCursor[i] = LoadCursor(NULL, IDC_ARROW);
		}
	}
}



void LLWindowWin32::updateCursor()
{
	if (mNextCursor == UI_CURSOR_ARROW
		&& mBusyCount > 0)
	{
		mNextCursor = UI_CURSOR_WORKING;
	}

	if( mCurrentCursor != mNextCursor )
	{
		mCurrentCursor = mNextCursor;
		SetCursor( mCursor[mNextCursor] );
	}
}

ECursorType LLWindowWin32::getCursor() const
{
	return mCurrentCursor;
}

void LLWindowWin32::captureMouse()
{
	SetCapture(mWindowHandle);
}

void LLWindowWin32::releaseMouse()
{
	// *NOTE:Mani ReleaseCapture will spawn new windows messages...
	// which will in turn call our MainWindowProc. It therefore requires
	// pausing *and more importantly resumption* of the mainlooptimeout...
	// just like DispatchMessage below.
	mCallbacks->handlePauseWatchdog(this);
	ReleaseCapture();
	mCallbacks->handleResumeWatchdog(this);
}


void LLWindowWin32::delayInputProcessing()
{
	mInputProcessingPaused = TRUE;
}

void LLWindowWin32::gatherInput()
{
	MSG		msg;
	int		msg_count = 0;

	LLMemType m1(LLMemType::MTYPE_GATHER_INPUT);

	while ((msg_count < MAX_MESSAGE_PER_UPDATE) && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		mCallbacks->handlePingWatchdog(this, "Main:TranslateGatherInput");
		TranslateMessage(&msg);

		// turn watchdog off in here to not fail if windows is doing something wacky
		mCallbacks->handlePauseWatchdog(this);
		DispatchMessage(&msg);
		mCallbacks->handleResumeWatchdog(this);
		msg_count++;

		if ( mInputProcessingPaused )
		{
			break;
		}
		/* Attempted workaround for problem where typing fast and hitting
		   return would result in only part of the text being sent. JC

		BOOL key_posted = TranslateMessage(&msg);
		DispatchMessage(&msg);
		msg_count++;

		// If a key was translated, a WM_CHAR might have been posted to the end
		// of the event queue.  We need it immediately.
		if (key_posted && msg.message == WM_KEYDOWN)
		{
			if (PeekMessage(&msg, NULL, WM_CHAR, WM_CHAR, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				msg_count++;
			}
		}
		*/
		mCallbacks->handlePingWatchdog(this, "Main:AsyncCallbackGatherInput");
		// For async host by name support.  Really hacky.
		if (gAsyncMsgCallback && (LL_WM_HOST_RESOLVED == msg.message))
		{
			gAsyncMsgCallback(msg);
		}
	}

	mInputProcessingPaused = FALSE;

	updateCursor();

	// clear this once we've processed all mouse messages that might have occurred after
	// we slammed the mouse position
	mMousePositionModified = FALSE;
}

static LLFastTimer::DeclareTimer FTM_KEYHANDLER("Handle Keyboard");
static LLFastTimer::DeclareTimer FTM_MOUSEHANDLER("Handle Mouse");

LRESULT CALLBACK LLWindowWin32::mainWindowProc(HWND h_wnd, UINT u_msg, WPARAM w_param, LPARAM l_param)
{
	// Ignore clicks not originated in the client area, i.e. mouse-up events not preceded with a WM_LBUTTONDOWN.
	// This helps prevent avatar walking after maximizing the window by double-clicking the title bar.
	static bool sHandleLeftMouseUp = true;

	LLWindowWin32 *window_imp = (LLWindowWin32 *)GetWindowLong(h_wnd, GWL_USERDATA);


	if (NULL != window_imp)
	{
		window_imp->mCallbacks->handleResumeWatchdog(window_imp);
		window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:StartWndProc");
		// Has user provided their own window callback?
		if (NULL != window_imp->mWndProc)
		{
			if (!window_imp->mWndProc(h_wnd, u_msg, w_param, l_param))
			{
				// user has handled window message
				return 0;
			}
		}

		window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:PreSwitchWndProc");
		
		// Juggle to make sure we can get negative positions for when
		// mouse is outside window.
		LLCoordWindow window_coord((S32)(S16)LOWORD(l_param), (S32)(S16)HIWORD(l_param));

		// This doesn't work, as LOWORD returns unsigned short.
		//LLCoordWindow window_coord(LOWORD(l_param), HIWORD(l_param));
		LLCoordGL gl_coord;

		// pass along extended flag in mask
		MASK mask = (l_param>>16 & KF_EXTENDED) ? MASK_EXTENDED : 0x0;
		BOOL eat_keystroke = TRUE;

		switch(u_msg)
		{
			RECT	update_rect;
			S32		update_width;
			S32		update_height;

		case WM_TIMER:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_TIMER");
			window_imp->mCallbacks->handleTimerEvent(window_imp);
			break;

		case WM_DEVICECHANGE:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_DEVICECHANGE");
			if (gDebugWindowProc)
			{
				llinfos << "  WM_DEVICECHANGE: wParam=" << w_param 
						<< "; lParam=" << l_param << llendl;
			}
			if (w_param == DBT_DEVNODES_CHANGED || w_param == DBT_DEVICEARRIVAL)
			{
				if (window_imp->mCallbacks->handleDeviceChange(window_imp))
				{
					return 0;
				}
			}
			break;

		case WM_PAINT:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_PAINT");
			GetUpdateRect(window_imp->mWindowHandle, &update_rect, FALSE);
			update_width = update_rect.right - update_rect.left + 1;
			update_height = update_rect.bottom - update_rect.top + 1;
			window_imp->mCallbacks->handlePaint(window_imp, update_rect.left, update_rect.top,
				update_width, update_height);
			break;
		case WM_PARENTNOTIFY:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_PARENTNOTIFY");
			u_msg = u_msg;
			break;

		case WM_SETCURSOR:
			// This message is sent whenever the cursor is moved in a window.
			// You need to set the appropriate cursor appearance.

			// Only take control of cursor over client region of window
			// This allows Windows(tm) to handle resize cursors, etc.
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_SETCURSOR");
			if (LOWORD(l_param) == HTCLIENT)
			{
				SetCursor(window_imp->mCursor[ window_imp->mCurrentCursor] );
				return 0;
			}
			break;

		case WM_ENTERMENULOOP:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_ENTERMENULOOP");
			window_imp->mCallbacks->handleWindowBlock(window_imp);
			break;

		case WM_EXITMENULOOP:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_EXITMENULOOP");
			window_imp->mCallbacks->handleWindowUnblock(window_imp);
			break;

		case WM_ACTIVATEAPP:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_ACTIVATEAPP");
			{
				// This message should be sent whenever the app gains or loses focus.
				BOOL activating = (BOOL) w_param;
				BOOL minimized = window_imp->getMinimized();

				if (gDebugWindowProc)
				{
					LL_INFOS("Window") << "WINDOWPROC ActivateApp "
						<< " activating " << S32(activating)
						<< " minimized " << S32(minimized)
						<< " fullscreen " << S32(window_imp->mFullscreen)
						<< LL_ENDL;
				}

				if (window_imp->mFullscreen)
				{
					// When we run fullscreen, restoring or minimizing the app needs 
					// to switch the screen resolution
					if (activating)
					{
						window_imp->setFullscreenResolution();
						window_imp->restore();
					}
					else
					{
						window_imp->minimize();
						window_imp->resetDisplayResolution();
					}
				}

				window_imp->mCallbacks->handleActivateApp(window_imp, activating);

				break;
			}

		case WM_ACTIVATE:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_ACTIVATE");
			{
				// Can be one of WA_ACTIVE, WA_CLICKACTIVE, or WA_INACTIVE
				BOOL activating = (LOWORD(w_param) != WA_INACTIVE);

				BOOL minimized = BOOL(HIWORD(w_param));

				if (!activating && LLWinImm::isAvailable() && window_imp->mPreeditor)
				{
					window_imp->interruptLanguageTextInput();
				}

				// JC - I'm not sure why, but if we don't report that we handled the 
				// WM_ACTIVATE message, the WM_ACTIVATEAPP messages don't work 
				// properly when we run fullscreen.
				if (gDebugWindowProc)
				{
					LL_INFOS("Window") << "WINDOWPROC Activate "
						<< " activating " << S32(activating) 
						<< " minimized " << S32(minimized)
						<< LL_ENDL;
				}

				// Don't handle this.
				break;
			}

		case WM_QUERYOPEN:
			// TODO: use this to return a nice icon
			break;

		case WM_SYSCOMMAND:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_SYSCOMMAND");
			switch(w_param)
			{
			case SC_KEYMENU: 
				// Disallow the ALT key from triggering the default system menu.
				return 0;		

			case SC_SCREENSAVE:
			case SC_MONITORPOWER:
				// eat screen save messages and prevent them!
				return 0;
			}
			break;

		case WM_CLOSE:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_CLOSE");
			// Will the app allow the window to close?
			if (window_imp->mCallbacks->handleCloseRequest(window_imp))
			{
				// Get the app to initiate cleanup.
				window_imp->mCallbacks->handleQuit(window_imp);
				// The app is responsible for calling destroyWindow when done with GL
			}
			return 0;

		case WM_DESTROY:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_DESTROY");
			if (window_imp->shouldPostQuit())
			{
				PostQuitMessage(0);  // Posts WM_QUIT with an exit code of 0
			}
			return 0;

		case WM_COMMAND:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_COMMAND");
			if (!HIWORD(w_param)) // this message is from a menu
			{
				window_imp->mCallbacks->handleMenuSelect(window_imp, LOWORD(w_param));
			}
			break;

		case WM_SYSKEYDOWN:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_SYSKEYDOWN");
			// allow system keys, such as ALT-F4 to be processed by Windows
			eat_keystroke = FALSE;
		case WM_KEYDOWN:
			window_imp->mKeyCharCode = 0; // don't know until wm_char comes in next
			window_imp->mKeyScanCode = ( l_param >> 16 ) & 0xff;
			window_imp->mKeyVirtualKey = w_param;

			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_KEYDOWN");
			{
				if (gDebugWindowProc)
				{
					LL_INFOS("Window") << "Debug WindowProc WM_KEYDOWN "
						<< " key " << S32(w_param) 
						<< LL_ENDL;
				}
				if(gKeyboard->handleKeyDown(w_param, mask) && eat_keystroke)
				{
					return 0;
				}
				// pass on to windows if we didn't handle it
				break;
			}
		case WM_SYSKEYUP:
			eat_keystroke = FALSE;
		case WM_KEYUP:
		{
			window_imp->mKeyScanCode = ( l_param >> 16 ) & 0xff;
			window_imp->mKeyVirtualKey = w_param;

			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_KEYUP");
			LLFastTimer t2(FTM_KEYHANDLER);

			if (gDebugWindowProc)
			{
				LL_INFOS("Window") << "Debug WindowProc WM_KEYUP "
					<< " key " << S32(w_param) 
					<< LL_ENDL;
			}
			if (gKeyboard->handleKeyUp(w_param, mask) && eat_keystroke)
			{
				return 0;
			}

			// pass on to windows
			break;
		}
		case WM_IME_SETCONTEXT:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_IME_SETCONTEXT");
			if (gDebugWindowProc)
			{
				llinfos << "WM_IME_SETCONTEXT" << llendl;
			}
			if (LLWinImm::isAvailable() && window_imp->mPreeditor)
			{
				l_param &= ~ISC_SHOWUICOMPOSITIONWINDOW;
				// Invoke DefWinProc with the modified LPARAM.
			}
			break;

		case WM_IME_STARTCOMPOSITION:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_IME_STARTCOMPOSITION");
			if (gDebugWindowProc)
			{
				llinfos << "WM_IME_STARTCOMPOSITION" << llendl;
			}
			if (LLWinImm::isAvailable() && window_imp->mPreeditor)
			{
				window_imp->handleStartCompositionMessage();
				return 0;
			}
			break;

		case WM_IME_ENDCOMPOSITION:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_IME_ENDCOMPOSITION");
			if (gDebugWindowProc)
			{
				llinfos << "WM_IME_ENDCOMPOSITION" << llendl;
			}
			if (LLWinImm::isAvailable() && window_imp->mPreeditor)
			{
				return 0;
			}
			break;

		case WM_IME_COMPOSITION:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_IME_COMPOSITION");
			if (gDebugWindowProc)
			{
				llinfos << "WM_IME_COMPOSITION" << llendl;
			}
			if (LLWinImm::isAvailable() && window_imp->mPreeditor)
			{
				window_imp->handleCompositionMessage(l_param);
				return 0;
			}
			break;

		case WM_IME_REQUEST:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_IME_REQUEST");
			if (gDebugWindowProc)
			{
				llinfos << "WM_IME_REQUEST" << llendl;
			}
			if (LLWinImm::isAvailable() && window_imp->mPreeditor)
			{
				LRESULT result = 0;
				if (window_imp->handleImeRequests(w_param, l_param, &result))
				{
					return result;
				}
			}
			break;

		case WM_CHAR:
			window_imp->mKeyCharCode = w_param;

			// Should really use WM_UNICHAR eventually, but it requires a specific Windows version and I need
			// to figure out how that works. - Doug
			//
			// ... Well, I don't think so.
			// How it works is explained in Win32 API document, but WM_UNICHAR didn't work
			// as specified at least on Windows XP SP1 Japanese version.  I have never used
			// it since then, and I'm not sure whether it has been fixed now, but I don't think
			// it is worth trying.  The good old WM_CHAR works just fine even for supplementary
			// characters.  We just need to take care of surrogate pairs sent as two WM_CHAR's
			// by ourselves.  It is not that tough.  -- Alissa Sabre @ SL
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_CHAR");
			if (gDebugWindowProc)
			{
				LL_INFOS("Window") << "Debug WindowProc WM_CHAR "
					<< " key " << S32(w_param) 
					<< LL_ENDL;
			}
			// Even if LLWindowCallbacks::handleUnicodeChar(llwchar, BOOL) returned FALSE,
			// we *did* processed the event, so I believe we should not pass it to DefWindowProc...
			window_imp->handleUnicodeUTF16((U16)w_param, gKeyboard->currentMask(FALSE));
			return 0;

		case WM_NCLBUTTONDOWN:
			{
				window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_NCLBUTTONDOWN");
				// A click in a non-client area, e.g. title bar or window border.
				sHandleLeftMouseUp = false;
			}
			break;

		case WM_LBUTTONDOWN:
			{
				window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_LBUTTONDOWN");
				LLFastTimer t2(FTM_MOUSEHANDLER);
				sHandleLeftMouseUp = true;

				if (LLWinImm::isAvailable() && window_imp->mPreeditor)
				{
					window_imp->interruptLanguageTextInput();
				}

				// Because we move the cursor position in the app, we need to query
				// to find out where the cursor at the time the event is handled.
				// If we don't do this, many clicks could get buffered up, and if the
				// first click changes the cursor position, all subsequent clicks
				// will occur at the wrong location.  JC
				if (window_imp->mMousePositionModified)
				{
					LLCoordWindow cursor_coord_window;
					window_imp->getCursorPosition(&cursor_coord_window);
					gl_coord = cursor_coord_window.convert();
				}
				else
				{
					gl_coord = window_coord.convert();
				}
				MASK mask = gKeyboard->currentMask(TRUE);
				// generate move event to update mouse coordinates
				window_imp->mCallbacks->handleMouseMove(window_imp, gl_coord, mask);
				if (window_imp->mCallbacks->handleMouseDown(window_imp, gl_coord, mask))
				{
					return 0;
				}
			}
			break;

		case WM_LBUTTONDBLCLK:
		//RN: ignore right button double clicks for now
		//case WM_RBUTTONDBLCLK:
			{
				window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_LBUTTONDBLCLK");
				// Because we move the cursor position in the app, we need to query
				// to find out where the cursor at the time the event is handled.
				// If we don't do this, many clicks could get buffered up, and if the
				// first click changes the cursor position, all subsequent clicks
				// will occur at the wrong location.  JC
				if (window_imp->mMousePositionModified)
				{
					LLCoordWindow cursor_coord_window;
					window_imp->getCursorPosition(&cursor_coord_window);
					gl_coord = cursor_coord_window.convert();
				}
				else
				{
					gl_coord = window_coord.convert();
				}
				MASK mask = gKeyboard->currentMask(TRUE);
				// generate move event to update mouse coordinates
				window_imp->mCallbacks->handleMouseMove(window_imp, gl_coord, mask);
				if (window_imp->mCallbacks->handleDoubleClick(window_imp, gl_coord, mask) )
				{
					return 0;
				}
			}
			break;

		case WM_LBUTTONUP:
			{
				window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_LBUTTONUP");
				LLFastTimer t2(FTM_MOUSEHANDLER);

				if (!sHandleLeftMouseUp)
				{
					sHandleLeftMouseUp = true;
					break;
				}

				//if (gDebugClicks)
				//{
				//	LL_INFOS("Window") << "WndProc left button up" << LL_ENDL;
				//}
				// Because we move the cursor position in the app, we need to query
				// to find out where the cursor at the time the event is handled.
				// If we don't do this, many clicks could get buffered up, and if the
				// first click changes the cursor position, all subsequent clicks
				// will occur at the wrong location.  JC
				if (window_imp->mMousePositionModified)
				{
					LLCoordWindow cursor_coord_window;
					window_imp->getCursorPosition(&cursor_coord_window);
					gl_coord = cursor_coord_window.convert();
				}
				else
				{
					gl_coord = window_coord.convert();
				}
				MASK mask = gKeyboard->currentMask(TRUE);
				// generate move event to update mouse coordinates
				window_imp->mCallbacks->handleMouseMove(window_imp, gl_coord, mask);
				if (window_imp->mCallbacks->handleMouseUp(window_imp, gl_coord, mask))
				{
					return 0;
				}
			}
			break;

		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
			{
				window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_RBUTTONDOWN");
				LLFastTimer t2(FTM_MOUSEHANDLER);
				if (LLWinImm::isAvailable() && window_imp->mPreeditor)
				{
					window_imp->interruptLanguageTextInput();
				}

				// Because we move the cursor position in the llviewerapp, we need to query
				// to find out where the cursor at the time the event is handled.
				// If we don't do this, many clicks could get buffered up, and if the
				// first click changes the cursor position, all subsequent clicks
				// will occur at the wrong location.  JC
				if (window_imp->mMousePositionModified)
				{
					LLCoordWindow cursor_coord_window;
					window_imp->getCursorPosition(&cursor_coord_window);
					gl_coord = cursor_coord_window.convert();
				}
				else
				{
					gl_coord = window_coord.convert();
				}
				MASK mask = gKeyboard->currentMask(TRUE);
				// generate move event to update mouse coordinates
				window_imp->mCallbacks->handleMouseMove(window_imp, gl_coord, mask);
				if (window_imp->mCallbacks->handleRightMouseDown(window_imp, gl_coord, mask))
				{
					return 0;
				}
			}
			break;

		case WM_RBUTTONUP:
			{
				window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_RBUTTONUP");
				LLFastTimer t2(FTM_MOUSEHANDLER);
				// Because we move the cursor position in the app, we need to query
				// to find out where the cursor at the time the event is handled.
				// If we don't do this, many clicks could get buffered up, and if the
				// first click changes the cursor position, all subsequent clicks
				// will occur at the wrong location.  JC
				if (window_imp->mMousePositionModified)
				{
					LLCoordWindow cursor_coord_window;
					window_imp->getCursorPosition(&cursor_coord_window);
					gl_coord = cursor_coord_window.convert();
				}
				else
				{
					gl_coord = window_coord.convert();
				}
				MASK mask = gKeyboard->currentMask(TRUE);
				// generate move event to update mouse coordinates
				window_imp->mCallbacks->handleMouseMove(window_imp, gl_coord, mask);
				if (window_imp->mCallbacks->handleRightMouseUp(window_imp, gl_coord, mask))
				{
					return 0;
				}
			}
			break;

		case WM_MBUTTONDOWN:
//		case WM_MBUTTONDBLCLK:
			{
				window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_MBUTTONDOWN");
				LLFastTimer t2(FTM_MOUSEHANDLER);
				if (LLWinImm::isAvailable() && window_imp->mPreeditor)
				{
					window_imp->interruptLanguageTextInput();
				}

				// Because we move the cursor position in tllviewerhe app, we need to query
				// to find out where the cursor at the time the event is handled.
				// If we don't do this, many clicks could get buffered up, and if the
				// first click changes the cursor position, all subsequent clicks
				// will occur at the wrong location.  JC
				if (window_imp->mMousePositionModified)
				{
					LLCoordWindow cursor_coord_window;
					window_imp->getCursorPosition(&cursor_coord_window);
					gl_coord = cursor_coord_window.convert();
				}
				else
				{
					gl_coord = window_coord.convert();
				}
				MASK mask = gKeyboard->currentMask(TRUE);
				// generate move event to update mouse coordinates
				window_imp->mCallbacks->handleMouseMove(window_imp, gl_coord, mask);
				if (window_imp->mCallbacks->handleMiddleMouseDown(window_imp, gl_coord, mask))
				{
					return 0;
				}
			}
			break;

		case WM_MBUTTONUP:
			{
				window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_MBUTTONUP");
				LLFastTimer t2(FTM_MOUSEHANDLER);
				// Because we move the cursor position in the llviewer app, we need to query
				// to find out where the cursor at the time the event is handled.
				// If we don't do this, many clicks could get buffered up, and if the
				// first click changes the cursor position, all subsequent clicks
				// will occur at the wrong location.  JC
				if (window_imp->mMousePositionModified)
				{
					LLCoordWindow cursor_coord_window;
					window_imp->getCursorPosition(&cursor_coord_window);
					gl_coord = cursor_coord_window.convert();
				}
				else
				{
					gl_coord = window_coord.convert();
				}
				MASK mask = gKeyboard->currentMask(TRUE);
				// generate move event to update mouse coordinates
				window_imp->mCallbacks->handleMouseMove(window_imp, gl_coord, mask);
				if (window_imp->mCallbacks->handleMiddleMouseUp(window_imp, gl_coord, mask))
				{
					return 0;
				}
			}
			break;

		case WM_MOUSEWHEEL:
			{
				window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_MOUSEWHEEL");
				static short z_delta = 0;

				RECT	client_rect;

				// eat scroll events that occur outside our window, since we use mouse position to direct scroll
				// instead of keyboard focus
				// NOTE: mouse_coord is in *window* coordinates for scroll events
				POINT mouse_coord = {(S32)(S16)LOWORD(l_param), (S32)(S16)HIWORD(l_param)};

				if (ScreenToClient(window_imp->mWindowHandle, &mouse_coord)
					&& GetClientRect(window_imp->mWindowHandle, &client_rect))
				{
					// we have a valid mouse point and client rect
					if (mouse_coord.x < client_rect.left || client_rect.right < mouse_coord.x
						|| mouse_coord.y < client_rect.top || client_rect.bottom < mouse_coord.y)
					{
						// mouse is outside of client rect, so don't do anything
						return 0;
					}
				}

				S16 incoming_z_delta = HIWORD(w_param);
				z_delta += incoming_z_delta;
				// cout << "z_delta " << z_delta << endl;

				// current mouse wheels report changes in increments of zDelta (+120, -120)
				// Future, higher resolution mouse wheels may report smaller deltas.
				// So we sum the deltas and only act when we've exceeded WHEEL_DELTA
				//
				// If the user rapidly spins the wheel, we can get messages with
				// large deltas, like 480 or so.  Thus we need to scroll more quickly.
				if (z_delta <= -WHEEL_DELTA || WHEEL_DELTA <= z_delta)
				{
					window_imp->mCallbacks->handleScrollWheel(window_imp, -z_delta / WHEEL_DELTA);
					z_delta = 0;
				}
				return 0;
			}
			/*
			// TODO: add this after resolving _WIN32_WINNT issue
			case WM_MOUSELEAVE:
			{
			window_imp->mCallbacks->handleMouseLeave(window_imp);

			//				TRACKMOUSEEVENT track_mouse_event;
			//				track_mouse_event.cbSize = sizeof( TRACKMOUSEEVENT );
			//				track_mouse_event.dwFlags = TME_LEAVE;
			//				track_mouse_event.hwndTrack = h_wnd;
			//				track_mouse_event.dwHoverTime = HOVER_DEFAULT;
			//				TrackMouseEvent( &track_mouse_event ); 
			return 0;
			}
			*/
			// Handle mouse movement within the window
		case WM_MOUSEMOVE:
			{
				window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_MOUSEMOVE");
				MASK mask = gKeyboard->currentMask(TRUE);
				window_imp->mCallbacks->handleMouseMove(window_imp, window_coord.convert(), mask);
				return 0;
			}

		case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO min_max = (LPMINMAXINFO)l_param;
				min_max->ptMinTrackSize.x = window_imp->mMinWindowWidth;
				min_max->ptMinTrackSize.y = window_imp->mMinWindowHeight;
				return 0;
			}

		case WM_SIZE:
			{
				window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_SIZE");
				S32 width = S32( LOWORD(l_param) );
				S32 height = S32( HIWORD(l_param) );

				if (gDebugWindowProc)
				{
					BOOL maximized = ( w_param == SIZE_MAXIMIZED );
					BOOL restored  = ( w_param == SIZE_RESTORED );
					BOOL minimized = ( w_param == SIZE_MINIMIZED );

					LL_INFOS("Window") << "WINDOWPROC Size "
						<< width << "x" << height
						<< " max " << S32(maximized)
						<< " min " << S32(minimized)
						<< " rest " << S32(restored)
						<< LL_ENDL;
				}

				// There's an odd behavior with WM_SIZE that I would call a bug. If 
				// the window is maximized, and you call MoveWindow() with a size smaller
				// than a maximized window, it ends up sending WM_SIZE with w_param set 
				// to SIZE_MAXIMIZED -- which isn't true. So the logic below doesn't work.
				// (SL-44655). Fixed it by calling ShowWindow(SW_RESTORE) first (see 
				// LLWindowWin32::moveWindow in this file). 

				// If we are now restored, but we weren't before, this
				// means that the window was un-minimized.
				if (w_param == SIZE_RESTORED && window_imp->mLastSizeWParam != SIZE_RESTORED)
				{
					window_imp->mCallbacks->handleActivate(window_imp, TRUE);
				}

				// handle case of window being maximized from fully minimized state
				if (w_param == SIZE_MAXIMIZED && window_imp->mLastSizeWParam != SIZE_MAXIMIZED)
				{
					window_imp->mCallbacks->handleActivate(window_imp, TRUE);
				}

				// Also handle the minimization case
				if (w_param == SIZE_MINIMIZED && window_imp->mLastSizeWParam != SIZE_MINIMIZED)
				{
					window_imp->mCallbacks->handleActivate(window_imp, FALSE);
				}

				// Actually resize all of our views
				if (w_param != SIZE_MINIMIZED)
				{
					// Ignore updates for minimizing and minimized "windows"
					window_imp->mCallbacks->handleResize(	window_imp, 
						LOWORD(l_param), 
						HIWORD(l_param) );
				}

				window_imp->mLastSizeWParam = w_param;

				return 0;
			}

		case WM_SETFOCUS:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_SETFOCUS");
			if (gDebugWindowProc)
			{
				LL_INFOS("Window") << "WINDOWPROC SetFocus" << LL_ENDL;
			}
			window_imp->mCallbacks->handleFocus(window_imp);
			return 0;

		case WM_KILLFOCUS:
			window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_KILLFOCUS");
			if (gDebugWindowProc)
			{
				LL_INFOS("Window") << "WINDOWPROC KillFocus" << LL_ENDL;
			}
			window_imp->mCallbacks->handleFocusLost(window_imp);
			return 0;

		case WM_COPYDATA:
			{
				window_imp->mCallbacks->handlePingWatchdog(window_imp, "Main:WM_COPYDATA");
				// received a URL
				PCOPYDATASTRUCT myCDS = (PCOPYDATASTRUCT) l_param;
				window_imp->mCallbacks->handleDataCopy(window_imp, myCDS->dwData, myCDS->lpData);
			};
			return 0;			

			break;
		}

	window_imp->mCallbacks->handlePauseWatchdog(window_imp);	
	}


	// pass unhandled messages down to Windows
	return DefWindowProc(h_wnd, u_msg, w_param, l_param);
}

BOOL LLWindowWin32::convertCoords(LLCoordGL from, LLCoordWindow *to)
{
	S32		client_height;
	RECT	client_rect;
	LLCoordWindow window_position;

	if (!mWindowHandle ||
		!GetClientRect(mWindowHandle, &client_rect) ||
		NULL == to)
	{
		return FALSE;
	}

	to->mX = from.mX;
	client_height = client_rect.bottom - client_rect.top;
	to->mY = client_height - from.mY - 1;

	return TRUE;
}

BOOL LLWindowWin32::convertCoords(LLCoordWindow from, LLCoordGL* to)
{
	S32		client_height;
	RECT	client_rect;

	if (!mWindowHandle ||
		!GetClientRect(mWindowHandle, &client_rect) ||
		NULL == to)
	{
		return FALSE;
	}

	to->mX = from.mX;
	client_height = client_rect.bottom - client_rect.top;
	to->mY = client_height - from.mY - 1;

	return TRUE;
}

BOOL LLWindowWin32::convertCoords(LLCoordScreen from, LLCoordWindow* to)
{	
	POINT mouse_point;

	mouse_point.x = from.mX;
	mouse_point.y = from.mY;
	BOOL result = ScreenToClient(mWindowHandle, &mouse_point);

	if (result)
	{
		to->mX = mouse_point.x;
		to->mY = mouse_point.y;
	}

	return result;
}

BOOL LLWindowWin32::convertCoords(LLCoordWindow from, LLCoordScreen *to)
{
	POINT mouse_point;

	mouse_point.x = from.mX;
	mouse_point.y = from.mY;
	BOOL result = ClientToScreen(mWindowHandle, &mouse_point);

	if (result)
	{
		to->mX = mouse_point.x;
		to->mY = mouse_point.y;
	}

	return result;
}

BOOL LLWindowWin32::convertCoords(LLCoordScreen from, LLCoordGL *to)
{
	LLCoordWindow window_coord;

	if (!mWindowHandle || (NULL == to))
	{
		return FALSE;
	}

	convertCoords(from, &window_coord);
	convertCoords(window_coord, to);
	return TRUE;
}

BOOL LLWindowWin32::convertCoords(LLCoordGL from, LLCoordScreen *to)
{
	LLCoordWindow window_coord;

	if (!mWindowHandle || (NULL == to))
	{
		return FALSE;
	}

	convertCoords(from, &window_coord);
	convertCoords(window_coord, to);
	return TRUE;
}


BOOL LLWindowWin32::isClipboardTextAvailable()
{
	return IsClipboardFormatAvailable(CF_UNICODETEXT);
}


BOOL LLWindowWin32::pasteTextFromClipboard(LLWString &dst)
{
	BOOL success = FALSE;

	if (IsClipboardFormatAvailable(CF_UNICODETEXT))
	{
		if (OpenClipboard(mWindowHandle))
		{
			HGLOBAL h_data = GetClipboardData(CF_UNICODETEXT);
			if (h_data)
			{
				WCHAR *utf16str = (WCHAR*) GlobalLock(h_data);
				if (utf16str)
				{
					dst = utf16str_to_wstring(utf16str);
					LLWStringUtil::removeCRLF(dst);
					GlobalUnlock(h_data);
					success = TRUE;
				}
			}
			CloseClipboard();
		}
	}

	return success;
}


BOOL LLWindowWin32::copyTextToClipboard(const LLWString& wstr)
{
	BOOL success = FALSE;

	if (OpenClipboard(mWindowHandle))
	{
		EmptyClipboard();

		// Provide a copy of the data in Unicode format.
		LLWString sanitized_string(wstr);
		LLWStringUtil::addCRLF(sanitized_string);
		llutf16string out_utf16 = wstring_to_utf16str(sanitized_string);
		const size_t size_utf16 = (out_utf16.length() + 1) * sizeof(WCHAR);

		// Memory is allocated and then ownership of it is transfered to the system.
		HGLOBAL hglobal_copy_utf16 = GlobalAlloc(GMEM_MOVEABLE, size_utf16); 
		if (hglobal_copy_utf16)
		{
			WCHAR* copy_utf16 = (WCHAR*) GlobalLock(hglobal_copy_utf16);
			if (copy_utf16)
			{
				memcpy(copy_utf16, out_utf16.c_str(), size_utf16);	/* Flawfinder: ignore */
				GlobalUnlock(hglobal_copy_utf16);

				if (SetClipboardData(CF_UNICODETEXT, hglobal_copy_utf16))
				{
					success = TRUE;
				}
			}
		}

		CloseClipboard();
	}

	return success;
}

// Constrains the mouse to the window.
void LLWindowWin32::setMouseClipping( BOOL b )
{
	if( b != mIsMouseClipping )
	{
		BOOL success = FALSE;

		if( b )
		{
			GetClipCursor( &mOldMouseClip );

			RECT client_rect_in_screen_space;
			if( getClientRectInScreenSpace( &client_rect_in_screen_space ) )
			{
				success = ClipCursor( &client_rect_in_screen_space );
			}
		}
		else
		{
			// Must restore the old mouse clip, which may be set by another window.
			success = ClipCursor( &mOldMouseClip );
			SetRect( &mOldMouseClip, 0, 0, 0, 0 );
		}

		if( success )
		{
			mIsMouseClipping = b;
		}
	}
}

BOOL LLWindowWin32::getClientRectInScreenSpace( RECT* rectp )
{
	BOOL success = FALSE;

	RECT client_rect;
	if( mWindowHandle && GetClientRect(mWindowHandle, &client_rect) )
	{
		POINT top_left;
		top_left.x = client_rect.left;
		top_left.y = client_rect.top;
		ClientToScreen(mWindowHandle, &top_left); 

		POINT bottom_right;
		bottom_right.x = client_rect.right;
		bottom_right.y = client_rect.bottom;
		ClientToScreen(mWindowHandle, &bottom_right); 

		SetRect( rectp,
			top_left.x,
			top_left.y,
			bottom_right.x,
			bottom_right.y );

		success = TRUE;
	}

	return success;
}

void LLWindowWin32::flashIcon(F32 seconds)
{
	FLASHWINFO flash_info;

	flash_info.cbSize = sizeof(FLASHWINFO);
	flash_info.hwnd = mWindowHandle;
	flash_info.dwFlags = FLASHW_TRAY;
	flash_info.uCount = UINT(seconds / ICON_FLASH_TIME);
	flash_info.dwTimeout = DWORD(1000.f * ICON_FLASH_TIME); // milliseconds
	FlashWindowEx(&flash_info);
}

F32 LLWindowWin32::getGamma()
{
	return mCurrentGamma;
}

BOOL LLWindowWin32::restoreGamma()
{
	return SetDeviceGammaRamp(mhDC, mPrevGammaRamp);
}

BOOL LLWindowWin32::setGamma(const F32 gamma)
{
	mCurrentGamma = gamma;

	LL_DEBUGS("Window") << "Setting gamma to " << gamma << LL_ENDL;

	for ( int i = 0; i < 256; ++i )
	{
		int mult = 256 - ( int ) ( ( gamma - 1.0f ) * 128.0f );

		int value = mult * i;

		if ( value > 0xffff )
			value = 0xffff;

		mCurrentGammaRamp [ 0 * 256 + i ] = 
			mCurrentGammaRamp [ 1 * 256 + i ] = 
				mCurrentGammaRamp [ 2 * 256 + i ] = ( WORD )value;
	};

	return SetDeviceGammaRamp ( mhDC, mCurrentGammaRamp );
}

void LLWindowWin32::setFSAASamples(const U32 fsaa_samples)
{
	mFSAASamples = fsaa_samples;
}

U32 LLWindowWin32::getFSAASamples()
{
	return mFSAASamples;
}

LLWindow::LLWindowResolution* LLWindowWin32::getSupportedResolutions(S32 &num_resolutions)
{
	if (!mSupportedResolutions)
	{
		mSupportedResolutions = new LLWindowResolution[MAX_NUM_RESOLUTIONS];
		DEVMODE dev_mode;
		::ZeroMemory(&dev_mode, sizeof(DEVMODE));
		dev_mode.dmSize = sizeof(DEVMODE);

		mNumSupportedResolutions = 0;
		for (S32 mode_num = 0; mNumSupportedResolutions < MAX_NUM_RESOLUTIONS; mode_num++)
		{
			if (!EnumDisplaySettings(NULL, mode_num, &dev_mode))
			{
				break;
			}

			if (dev_mode.dmBitsPerPel == BITS_PER_PIXEL &&
				dev_mode.dmPelsWidth >= 800 &&
				dev_mode.dmPelsHeight >= 600)
			{
				BOOL resolution_exists = FALSE;
				for(S32 i = 0; i < mNumSupportedResolutions; i++)
				{
					if (mSupportedResolutions[i].mWidth == dev_mode.dmPelsWidth &&
						mSupportedResolutions[i].mHeight == dev_mode.dmPelsHeight)
					{
						resolution_exists = TRUE;
					}
				}
				if (!resolution_exists)
				{
					mSupportedResolutions[mNumSupportedResolutions].mWidth = dev_mode.dmPelsWidth;
					mSupportedResolutions[mNumSupportedResolutions].mHeight = dev_mode.dmPelsHeight;
					mNumSupportedResolutions++;
				}
			}
		}
	}

	num_resolutions = mNumSupportedResolutions;
	return mSupportedResolutions;
}


F32 LLWindowWin32::getNativeAspectRatio()
{
	if (mOverrideAspectRatio > 0.f)
	{
		return mOverrideAspectRatio;
	}
	else if (mNativeAspectRatio > 0.f)
	{
		// we grabbed this value at startup, based on the user's desktop settings
		return mNativeAspectRatio;
	}
	// RN: this hack presumes that the largest supported resolution is monitor-limited
	// and that pixels in that mode are square, therefore defining the native aspect ratio
	// of the monitor...this seems to work to a close approximation for most CRTs/LCDs
	S32 num_resolutions;
	LLWindowResolution* resolutions = getSupportedResolutions(num_resolutions);

	return ((F32)resolutions[num_resolutions - 1].mWidth / (F32)resolutions[num_resolutions - 1].mHeight);
}

F32 LLWindowWin32::getPixelAspectRatio()
{
	F32 pixel_aspect = 1.f;
	if (getFullscreen())
	{
		LLCoordScreen screen_size;
		getSize(&screen_size);
		pixel_aspect = getNativeAspectRatio() * (F32)screen_size.mY / (F32)screen_size.mX;
	}

	return pixel_aspect;
}

// Change display resolution.  Returns true if successful.
// protected
BOOL LLWindowWin32::setDisplayResolution(S32 width, S32 height, S32 bits, S32 refresh)
{
	DEVMODE dev_mode;
	::ZeroMemory(&dev_mode, sizeof(DEVMODE));
	dev_mode.dmSize = sizeof(DEVMODE);
	BOOL success = FALSE;

	// Don't change anything if we don't have to
	if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dev_mode))
	{
		if (dev_mode.dmPelsWidth        == width &&
			dev_mode.dmPelsHeight       == height &&
			dev_mode.dmBitsPerPel       == bits &&
			dev_mode.dmDisplayFrequency == refresh )
		{
			// ...display mode identical, do nothing
			return TRUE;
		}
	}

	memset(&dev_mode, 0, sizeof(dev_mode));
	dev_mode.dmSize = sizeof(dev_mode);
	dev_mode.dmPelsWidth        = width;
	dev_mode.dmPelsHeight       = height;
	dev_mode.dmBitsPerPel       = bits;
	dev_mode.dmDisplayFrequency = refresh;
	dev_mode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

	// CDS_FULLSCREEN indicates that this is a temporary change to the device mode.
	LONG cds_result = ChangeDisplaySettings(&dev_mode, CDS_FULLSCREEN);

	success = (DISP_CHANGE_SUCCESSFUL == cds_result);

	if (!success)
	{
		LL_WARNS("Window") << "setDisplayResolution failed, "
			<< width << "x" << height << "x" << bits << " @ " << refresh << LL_ENDL;
	}

	return success;
}

// protected
BOOL LLWindowWin32::setFullscreenResolution()
{
	if (mFullscreen)
	{
		return setDisplayResolution( mFullscreenWidth, mFullscreenHeight, mFullscreenBits, mFullscreenRefresh);
	}
	else
	{
		return FALSE;
	}
}

// protected
BOOL LLWindowWin32::resetDisplayResolution()
{
	LL_DEBUGS("Window") << "resetDisplayResolution START" << LL_ENDL;

	LONG cds_result = ChangeDisplaySettings(NULL, 0);

	BOOL success = (DISP_CHANGE_SUCCESSFUL == cds_result);

	if (!success)
	{
		LL_WARNS("Window") << "resetDisplayResolution failed" << LL_ENDL;
	}

	LL_DEBUGS("Window") << "resetDisplayResolution END" << LL_ENDL;

	return success;
}

void LLWindowWin32::swapBuffers()
{
	SwapBuffers(mhDC);
}


//
// LLSplashScreenImp
//
LLSplashScreenWin32::LLSplashScreenWin32()
:	mWindow(NULL)
{
}

LLSplashScreenWin32::~LLSplashScreenWin32()
{
}

void LLSplashScreenWin32::showImpl()
{
	// This appears to work.  ???
	HINSTANCE hinst = GetModuleHandle(NULL);

	mWindow = CreateDialog(hinst, 
		TEXT("SPLASHSCREEN"), 
		NULL,	// no parent
		(DLGPROC) LLSplashScreenWin32::windowProc); 
	ShowWindow(mWindow, SW_SHOW);
}


void LLSplashScreenWin32::updateImpl(const std::string& mesg)
{
	if (!mWindow) return;

	int output_str_len = MultiByteToWideChar(CP_UTF8, 0, mesg.c_str(), mesg.length(), NULL, 0);
	if( output_str_len>1024 )
		return;

	WCHAR w_mesg[1025];//big enought to keep null terminatos

	MultiByteToWideChar (CP_UTF8, 0, mesg.c_str(), mesg.length(), w_mesg, output_str_len);

	//looks like MultiByteToWideChar didn't add null terminator to converted string, see EXT-4858
	w_mesg[output_str_len] = 0;

	SendDlgItemMessage(mWindow,
		666,		// HACK: text id
		WM_SETTEXT,
		FALSE,
		(LPARAM)w_mesg);
}


void LLSplashScreenWin32::hideImpl()
{
	if (mWindow)
	{
		DestroyWindow(mWindow);
		mWindow = NULL; 
	}
}


// static
LRESULT CALLBACK LLSplashScreenWin32::windowProc(HWND h_wnd, UINT u_msg,
											WPARAM w_param, LPARAM l_param)
{
	// Just give it to windows
	return DefWindowProc(h_wnd, u_msg, w_param, l_param);
}

//
// Helper Funcs
//

S32 OSMessageBoxWin32(const std::string& text, const std::string& caption, U32 type)
{
	UINT uType;

	switch(type)
	{
	case OSMB_OK:
		uType = MB_OK;
		break;
	case OSMB_OKCANCEL:
		uType = MB_OKCANCEL;
		break;
	case OSMB_YESNO:
		uType = MB_YESNO;
		break;
	default:
		uType = MB_OK;
		break;
	}

	// HACK! Doesn't properly handle wide strings!
	int retval_win = MessageBoxA(NULL, text.c_str(), caption.c_str(), uType);
	S32 retval;

	switch(retval_win)
	{
	case IDYES:
		retval = OSBTN_YES;
		break;
	case IDNO:
		retval = OSBTN_NO;
		break;
	case IDOK:
		retval = OSBTN_OK;
		break;
	case IDCANCEL:
		retval = OSBTN_CANCEL;
		break;
	default:
		retval = OSBTN_CANCEL;
		break;
	}

	return retval;
}


void LLWindowWin32::spawnWebBrowser(const std::string& escaped_url, bool async)
{
	bool found = false;
	S32 i;
	for (i = 0; i < gURLProtocolWhitelistCount; i++)
	{
		if (escaped_url.find(gURLProtocolWhitelist[i]) == 0)
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		LL_WARNS("Window") << "spawn_web_browser() called for url with protocol not on whitelist: " << escaped_url << LL_ENDL;
		return;
	}

	LL_INFOS("Window") << "Opening URL " << escaped_url << LL_ENDL;

	// replaced ShellExecute code with ShellExecuteEx since ShellExecute doesn't work
	// reliablly on Vista.

	// this is madness.. no, this is..
	LLWString url_wstring = utf8str_to_wstring( escaped_url );
	llutf16string url_utf16 = wstring_to_utf16str( url_wstring );

	// let the OS decide what to use to open the URL
	SHELLEXECUTEINFO sei = { sizeof( sei ) };
	// NOTE: this assumes that SL will stick around long enough to complete the DDE message exchange
	// necessary for ShellExecuteEx to complete
	if (async)
	{
		sei.fMask = SEE_MASK_ASYNCOK;
	}
	sei.nShow = SW_SHOWNORMAL;
	sei.lpVerb = L"open";
	sei.lpFile = url_utf16.c_str();
	ShellExecuteEx( &sei );
}

/*
	Make the raw keyboard data available - used to poke through to LLQtWebKit so
	that Qt/Webkit has access to the virtual keycodes etc. that it needs
*/
LLSD LLWindowWin32::getNativeKeyData()
{
	LLSD result = LLSD::emptyMap();

	result["scan_code"] = (S32)mKeyScanCode;
	result["virtual_key"] = (S32)mKeyVirtualKey;

	return result;
}

BOOL LLWindowWin32::dialogColorPicker( F32 *r, F32 *g, F32 *b )
{
	BOOL retval = FALSE;

	static CHOOSECOLOR cc;
	static COLORREF crCustColors[16];
	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = mWindowHandle;
	cc.hInstance = NULL;
	cc.rgbResult = RGB ((*r * 255.f),(*g *255.f),(*b * 255.f));
	//cc.rgbResult = RGB (0x80,0x80,0x80); 
	cc.lpCustColors = crCustColors;
	cc.Flags = CC_RGBINIT | CC_FULLOPEN;
	cc.lCustData = 0;
	cc.lpfnHook = NULL;
	cc.lpTemplateName = NULL;
 
	// This call is modal, so pause agent
	//send_agent_pause();	// this is in newview and we don't want to set up a dependency
	{
		retval = ChooseColor(&cc);
	}
	//send_agent_resume();	// this is in newview and we don't want to set up a dependency

	*b = ((F32)((cc.rgbResult >> 16) & 0xff)) / 255.f;

	*g = ((F32)((cc.rgbResult >> 8) & 0xff)) / 255.f;
	
	*r = ((F32)(cc.rgbResult & 0xff)) / 255.f;

	return (retval);
}

void *LLWindowWin32::getPlatformWindow()
{
	return (void*)mWindowHandle;
}

void LLWindowWin32::bringToFront()
{
	BringWindowToTop(mWindowHandle);
}

// set (OS) window focus back to the client
void LLWindowWin32::focusClient()
{
	SetFocus ( mWindowHandle );
}

void LLWindowWin32::allowLanguageTextInput(LLPreeditor *preeditor, BOOL b)
{
	if (b == sLanguageTextInputAllowed || !LLWinImm::isAvailable())
	{
		return;
	}

	if (preeditor != mPreeditor && !b)
	{
		// This condition may occur with a call to
		// setEnabled(BOOL) from LLTextEditor or LLLineEditor
		// when the control is not focused.
		// We need to silently ignore the case so that
		// the language input status of the focused control
		// is not disturbed.
		return;
	}

	// Take care of old and new preeditors.
	if (preeditor != mPreeditor || !b)
	{
		if (sLanguageTextInputAllowed)
		{
			interruptLanguageTextInput();
		}
		mPreeditor = (b ? preeditor : NULL);
	}

	sLanguageTextInputAllowed = b;

	if ( sLanguageTextInputAllowed )
	{
		// Allowing: Restore the previous IME status, so that the user has a feeling that the previous 
		// text input continues naturally.  Be careful, however, the IME status is meaningful only during the user keeps 
		// using same Input Locale (aka Keyboard Layout).
		if (sWinIMEOpened && GetKeyboardLayout(0) == sWinInputLocale)
		{
			HIMC himc = LLWinImm::getContext(mWindowHandle);
			LLWinImm::setOpenStatus(himc, TRUE);
			LLWinImm::setConversionStatus(himc, sWinIMEConversionMode, sWinIMESentenceMode);
			LLWinImm::releaseContext(mWindowHandle, himc);
		}
	}
	else
	{
		// Disallowing: Turn off the IME so that succeeding key events bypass IME and come to us directly.
		// However, do it after saving the current IME  status.  We need to restore the status when
		//   allowing language text input again.
		sWinInputLocale = GetKeyboardLayout(0);
		sWinIMEOpened = LLWinImm::isIME(sWinInputLocale);
		if (sWinIMEOpened)
		{
			HIMC himc = LLWinImm::getContext(mWindowHandle);
			sWinIMEOpened = LLWinImm::getOpenStatus(himc);
			if (sWinIMEOpened)
			{
				LLWinImm::getConversionStatus(himc, &sWinIMEConversionMode, &sWinIMESentenceMode);

				// We need both ImmSetConversionStatus and ImmSetOpenStatus here to surely disable IME's 
				// keyboard hooking, because Some IME reacts only on the former and some other on the latter...
				LLWinImm::setConversionStatus(himc, IME_CMODE_NOCONVERSION, sWinIMESentenceMode);
				LLWinImm::setOpenStatus(himc, FALSE);
			}
			LLWinImm::releaseContext(mWindowHandle, himc);
 		}
	}
}

void LLWindowWin32::fillCandidateForm(const LLCoordGL& caret, const LLRect& bounds, 
		CANDIDATEFORM *form)
{
	LLCoordWindow caret_coord, top_left, bottom_right;
	convertCoords(caret, &caret_coord);
	convertCoords(LLCoordGL(bounds.mLeft, bounds.mTop), &top_left);
	convertCoords(LLCoordGL(bounds.mRight, bounds.mBottom), &bottom_right);

	memset(form, 0, sizeof(CANDIDATEFORM));
	form->dwStyle = CFS_EXCLUDE;
	form->ptCurrentPos.x = caret_coord.mX;
	form->ptCurrentPos.y = caret_coord.mY;
	form->rcArea.left   = top_left.mX;
	form->rcArea.top    = top_left.mY;
	form->rcArea.right  = bottom_right.mX;
	form->rcArea.bottom = bottom_right.mY;
}


// Put the IME window at the right place (near current text input).   Point coordinates should be the top of the current text line.
void LLWindowWin32::setLanguageTextInput( const LLCoordGL & position )
{
	if (sLanguageTextInputAllowed && LLWinImm::isAvailable())
	{
		HIMC himc = LLWinImm::getContext(mWindowHandle);

		LLCoordWindow win_pos;
		convertCoords( position, &win_pos );

		if ( win_pos.mX >= 0 && win_pos.mY >= 0 && 
			(win_pos.mX != sWinIMEWindowPosition.mX) || (win_pos.mY != sWinIMEWindowPosition.mY) )
		{
			COMPOSITIONFORM ime_form;
			memset( &ime_form, 0, sizeof(ime_form) );
			ime_form.dwStyle = CFS_POINT;
			ime_form.ptCurrentPos.x = win_pos.mX;
			ime_form.ptCurrentPos.y = win_pos.mY;

			LLWinImm::setCompositionWindow( himc, &ime_form );

			sWinIMEWindowPosition = win_pos;
		}

		LLWinImm::releaseContext(mWindowHandle, himc);
	}
}


void LLWindowWin32::fillCharPosition(const LLCoordGL& caret, const LLRect& bounds, const LLRect& control,
		IMECHARPOSITION *char_position)
{
	LLCoordScreen caret_coord, top_left, bottom_right;
	convertCoords(caret, &caret_coord);
	convertCoords(LLCoordGL(bounds.mLeft, bounds.mTop), &top_left);
	convertCoords(LLCoordGL(bounds.mRight, bounds.mBottom), &bottom_right);

	char_position->pt.x = caret_coord.mX;
	char_position->pt.y = top_left.mY;	// Windows wants the coordinate of upper left corner of a character...
	char_position->cLineHeight = bottom_right.mY - top_left.mY;
	char_position->rcDocument.left   = top_left.mX;
	char_position->rcDocument.top    = top_left.mY;
	char_position->rcDocument.right  = bottom_right.mX;
	char_position->rcDocument.bottom = bottom_right.mY;
}

void LLWindowWin32::fillCompositionLogfont(LOGFONT *logfont)
{
	// Our font is a list of FreeType recognized font files that may
	// not have a corresponding ones in Windows' fonts.  Hence, we
	// can't simply tell Windows which font we are using.  We will
	// notify a _standard_ font for a current input locale instead.
	// We use a hard-coded knowledge about the Windows' standard
	// configuration to do so...

	memset(logfont, 0, sizeof(LOGFONT));

	const WORD lang_id = LOWORD(GetKeyboardLayout(0));
	switch (PRIMARYLANGID(lang_id))
	{
	case LANG_CHINESE:
		// We need to identify one of two Chinese fonts.
		switch (SUBLANGID(lang_id))
		{
		case SUBLANG_CHINESE_SIMPLIFIED:
		case SUBLANG_CHINESE_SINGAPORE:
			logfont->lfCharSet = GB2312_CHARSET;
			lstrcpy(logfont->lfFaceName, TEXT("SimHei"));
			break;
		case SUBLANG_CHINESE_TRADITIONAL:
		case SUBLANG_CHINESE_HONGKONG:
		case SUBLANG_CHINESE_MACAU:
		default:
			logfont->lfCharSet = CHINESEBIG5_CHARSET;
			lstrcpy(logfont->lfFaceName, TEXT("MingLiU"));
			break;			
		}
		break;
	case LANG_JAPANESE:
		logfont->lfCharSet = SHIFTJIS_CHARSET;
		lstrcpy(logfont->lfFaceName, TEXT("MS Gothic"));
		break;		
	case LANG_KOREAN:
		logfont->lfCharSet = HANGUL_CHARSET;
		lstrcpy(logfont->lfFaceName, TEXT("Gulim"));
		break;
	default:
		logfont->lfCharSet = ANSI_CHARSET;
		lstrcpy(logfont->lfFaceName, TEXT("Tahoma"));
		break;
	}
							
	logfont->lfHeight = mPreeditor->getPreeditFontSize();
	logfont->lfWeight = FW_NORMAL;
}	

U32 LLWindowWin32::fillReconvertString(const LLWString &text,
	S32 focus, S32 focus_length, RECONVERTSTRING *reconvert_string)
{
	const llutf16string text_utf16 = wstring_to_utf16str(text);
	const DWORD required_size = sizeof(RECONVERTSTRING) + (text_utf16.length() + 1) * sizeof(WCHAR);
	if (reconvert_string && reconvert_string->dwSize >= required_size)
	{
		const DWORD focus_utf16_at = wstring_utf16_length(text, 0, focus);
		const DWORD focus_utf16_length = wstring_utf16_length(text, focus, focus_length);

		reconvert_string->dwVersion = 0;
		reconvert_string->dwStrLen = text_utf16.length();
		reconvert_string->dwStrOffset = sizeof(RECONVERTSTRING);
		reconvert_string->dwCompStrLen = focus_utf16_length;
		reconvert_string->dwCompStrOffset = focus_utf16_at * sizeof(WCHAR);
		reconvert_string->dwTargetStrLen = 0;
		reconvert_string->dwTargetStrOffset = focus_utf16_at * sizeof(WCHAR);

		const LPWSTR text = (LPWSTR)((BYTE *)reconvert_string + sizeof(RECONVERTSTRING));
		memcpy(text, text_utf16.c_str(), (text_utf16.length() + 1) * sizeof(WCHAR));
	}
	return required_size;
}

void LLWindowWin32::updateLanguageTextInputArea()
{
	if (!mPreeditor || !LLWinImm::isAvailable())
	{
		return;
	}

	LLCoordGL caret_coord;
	LLRect preedit_bounds;
	if (mPreeditor->getPreeditLocation(-1, &caret_coord, &preedit_bounds, NULL))
	{
		mLanguageTextInputPointGL = caret_coord;
		mLanguageTextInputAreaGL = preedit_bounds;

		CANDIDATEFORM candidate_form;
		fillCandidateForm(caret_coord, preedit_bounds, &candidate_form);

		HIMC himc = LLWinImm::getContext(mWindowHandle);
		// Win32 document says there may be up to 4 candidate windows.
		// This magic number 4 appears only in the document, and
		// there are no constant/macro for the value...
		for (int i = 3; i >= 0; --i)
		{
			candidate_form.dwIndex = i;
			LLWinImm::setCandidateWindow(himc, &candidate_form);
		}
		LLWinImm::releaseContext(mWindowHandle, himc);
	}
}

void LLWindowWin32::interruptLanguageTextInput()
{
	if (mPreeditor)
	{
		if (LLWinImm::isAvailable())
		{
			HIMC himc = LLWinImm::getContext(mWindowHandle);
			LLWinImm::notifyIME(himc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
			LLWinImm::releaseContext(mWindowHandle, himc);
		}

		// Win32 document says there will be no composition string
		// after NI_COMPOSITIONSTR returns.  The following call to
		// resetPreedit should be a NOP unless IME goes mad...
		mPreeditor->resetPreedit();
	}
}

void LLWindowWin32::handleStartCompositionMessage()
{
	// Let IME know the font to use in feedback UI.
	LOGFONT logfont;
	fillCompositionLogfont(&logfont);
	HIMC himc = LLWinImm::getContext(mWindowHandle);
	LLWinImm::setCompositionFont(himc, &logfont);
	LLWinImm::releaseContext(mWindowHandle, himc);
}

// Handle WM_IME_COMPOSITION message.

void LLWindowWin32::handleCompositionMessage(const U32 indexes)
{
	BOOL needs_update = FALSE;
	LLWString result_string;
	LLWString preedit_string;
	S32 preedit_string_utf16_length = 0;
	LLPreeditor::segment_lengths_t preedit_segment_lengths;
	LLPreeditor::standouts_t preedit_standouts;

	// Step I: Receive details of preedits from IME.

	HIMC himc = LLWinImm::getContext(mWindowHandle);

	if (indexes & GCS_RESULTSTR)
	{
		LONG size = LLWinImm::getCompositionString(himc, GCS_RESULTSTR, NULL, 0);
		if (size >= 0)
		{
			const LPWSTR data = new WCHAR[size / sizeof(WCHAR) + 1];
			size = LLWinImm::getCompositionString(himc, GCS_RESULTSTR, data, size);
			if (size > 0)
			{
				result_string = utf16str_to_wstring(llutf16string(data, size / sizeof(WCHAR)));
			}
			delete[] data;
			needs_update = TRUE;
		}
	}
	
	if (indexes & GCS_COMPSTR)
	{
		LONG size = LLWinImm::getCompositionString(himc, GCS_COMPSTR, NULL, 0);
		if (size >= 0)
		{
			const LPWSTR data = new WCHAR[size / sizeof(WCHAR) + 1];
			size = LLWinImm::getCompositionString(himc, GCS_COMPSTR, data, size);
			if (size > 0)
			{
				preedit_string_utf16_length = size / sizeof(WCHAR);
				preedit_string = utf16str_to_wstring(llutf16string(data, size / sizeof(WCHAR)));
			}
			delete[] data;
			needs_update = TRUE;
		}
	}

	if ((indexes & GCS_COMPCLAUSE) && preedit_string.length() > 0)
	{
		LONG size = LLWinImm::getCompositionString(himc, GCS_COMPCLAUSE, NULL, 0);
		if (size > 0)
		{
			const LPDWORD data = new DWORD[size / sizeof(DWORD)];
			size = LLWinImm::getCompositionString(himc, GCS_COMPCLAUSE, data, size);
			if (size >= sizeof(DWORD) * 2
				&& data[0] == 0 && data[size / sizeof(DWORD) - 1] == preedit_string_utf16_length)
			{
				preedit_segment_lengths.resize(size / sizeof(DWORD) - 1);
				S32 offset = 0;
				for (U32 i = 0; i < preedit_segment_lengths.size(); i++)
				{
					const S32 length = wstring_wstring_length_from_utf16_length(preedit_string, offset, data[i + 1] - data[i]);
					preedit_segment_lengths[i] = length;
					offset += length;
				}
			}
			delete[] data;
		}
	}

	if ((indexes & GCS_COMPATTR) && preedit_segment_lengths.size() > 1)
	{
		LONG size = LLWinImm::getCompositionString(himc, GCS_COMPATTR, NULL, 0);
		if (size > 0)
		{
			const LPBYTE data = new BYTE[size / sizeof(BYTE)];
			size = LLWinImm::getCompositionString(himc, GCS_COMPATTR, data, size);
			if (size == preedit_string_utf16_length)
			{
				preedit_standouts.assign(preedit_segment_lengths.size(), FALSE);
				S32 offset = 0;
				for (U32 i = 0; i < preedit_segment_lengths.size(); i++)
				{
					if (ATTR_TARGET_CONVERTED == data[offset] || ATTR_TARGET_NOTCONVERTED == data[offset])
					{
						preedit_standouts[i] = TRUE;
					}
					offset += wstring_utf16_length(preedit_string, offset, preedit_segment_lengths[i]);
				}
			}
			delete[] data;
		}
	}

	S32 caret_position = preedit_string.length();
	if (indexes & GCS_CURSORPOS)
	{
		const S32 caret_position_utf16 = LLWinImm::getCompositionString(himc, GCS_CURSORPOS, NULL, 0);
		if (caret_position_utf16 >= 0 && caret_position <= preedit_string_utf16_length)
		{
			caret_position = wstring_wstring_length_from_utf16_length(preedit_string, 0, caret_position_utf16);
		}
	}

	if (indexes == 0)
	{
		// I'm not sure this condition really happens, but
		// Windows SDK document says it is an indication
		// of "reset everything."
		needs_update = TRUE;
	}

	LLWinImm::releaseContext(mWindowHandle, himc);

	// Step II: Update the active preeditor.

	if (needs_update)
	{
		mPreeditor->resetPreedit();

		if (result_string.length() > 0)
		{
			for (LLWString::const_iterator i = result_string.begin(); i != result_string.end(); i++)
			{
				mPreeditor->handleUnicodeCharHere(*i);
			}
		}

		if (preedit_string.length() == 0)
 		{
			preedit_segment_lengths.clear();
			preedit_standouts.clear();
		}
		else
		{
			if (preedit_segment_lengths.size() == 0)
			{
				preedit_segment_lengths.assign(1, preedit_string.length());
			}
			if (preedit_standouts.size() == 0)
			{
				preedit_standouts.assign(preedit_segment_lengths.size(), FALSE);
			}
		}
		mPreeditor->updatePreedit(preedit_string, preedit_segment_lengths, preedit_standouts, caret_position);

		// Some IME doesn't query char position after WM_IME_COMPOSITION,
		// so we need to update them actively.
		updateLanguageTextInputArea();
	}
}

// Given a text and a focus range, find_context finds and returns a
// surrounding context of the focused subtext.  A variable pointed
// to by offset receives the offset in llwchars of the beginning of
// the returned context string in the given wtext.

static LLWString find_context(const LLWString & wtext, S32 focus, S32 focus_length, S32 *offset)
{
	static const S32 CONTEXT_EXCESS = 30;	// This value is by experiences.

	const S32 e = llmin((S32) wtext.length(), focus + focus_length + CONTEXT_EXCESS);
	S32 end = focus + focus_length;
	while (end < e && '\n' != wtext[end])
	{
		end++;
	}

	const S32 s = llmax(0, focus - CONTEXT_EXCESS);
	S32 start = focus;
	while (start > s && '\n' != wtext[start - 1])
	{
		--start;
	}

	*offset = start;
	return wtext.substr(start, end - start);
}

// final stage of handling drop requests - both from WM_DROPFILES message
// for files and via IDropTarget interface requests.
LLWindowCallbacks::DragNDropResult LLWindowWin32::completeDragNDropRequest( const LLCoordGL gl_coord, const MASK mask, LLWindowCallbacks::DragNDropAction action, const std::string url )
{
	return mCallbacks->handleDragNDrop( this, gl_coord, mask, action, url );
}

// Handle WM_IME_REQUEST message.
// If it handled the message, returns TRUE.  Otherwise, FALSE.
// When it handled the message, the value to be returned from
// the Window Procedure is set to *result.

BOOL LLWindowWin32::handleImeRequests(U32 request, U32 param, LRESULT *result)
{
	if ( mPreeditor )
	{
		switch (request)
		{
			case IMR_CANDIDATEWINDOW:		// http://msdn2.microsoft.com/en-us/library/ms776080.aspx
			{
				LLCoordGL caret_coord;
				LLRect preedit_bounds;
				mPreeditor->getPreeditLocation(-1, &caret_coord, &preedit_bounds, NULL);
				
				CANDIDATEFORM *const form = (CANDIDATEFORM *)param;
				DWORD const dwIndex = form->dwIndex;
				fillCandidateForm(caret_coord, preedit_bounds, form);
				form->dwIndex = dwIndex;

				*result = 1;
				return TRUE;
			}
			case IMR_QUERYCHARPOSITION:
			{
				IMECHARPOSITION *const char_position = (IMECHARPOSITION *)param;

				// char_position->dwCharPos counts in number of
				// WCHARs, i.e., UTF-16 encoding units, so we can't simply pass the
				// number to getPreeditLocation.  

				const LLWString & wtext = mPreeditor->getPreeditString();
				S32 preedit, preedit_length;
				mPreeditor->getPreeditRange(&preedit, &preedit_length);
				LLCoordGL caret_coord;
				LLRect preedit_bounds, text_control;
				const S32 position = wstring_wstring_length_from_utf16_length(wtext, preedit, char_position->dwCharPos);

				if (!mPreeditor->getPreeditLocation(position, &caret_coord, &preedit_bounds, &text_control))
				{
					LL_WARNS("Window") << "*** IMR_QUERYCHARPOSITON called but getPreeditLocation failed." << LL_ENDL;
					return FALSE;
				}
				fillCharPosition(caret_coord, preedit_bounds, text_control, char_position);

				*result = 1;
				return TRUE;
			}
			case IMR_COMPOSITIONFONT:
			{
				fillCompositionLogfont((LOGFONT *)param);

				*result = 1;
				return TRUE;
			}
			case IMR_RECONVERTSTRING:
			{
				mPreeditor->resetPreedit();
				const LLWString & wtext = mPreeditor->getPreeditString();
				S32 select, select_length;
				mPreeditor->getSelectionRange(&select, &select_length);

				S32 context_offset;
				const LLWString context = find_context(wtext, select, select_length, &context_offset);

				RECONVERTSTRING * const reconvert_string = (RECONVERTSTRING *)param;
				const U32 size = fillReconvertString(context, select - context_offset, select_length, reconvert_string);
				if (reconvert_string)
				{
					if (select_length == 0)
					{
						// Let the IME to decide the reconversion range, and
						// adjust the reconvert_string structure accordingly.
						HIMC himc = LLWinImm::getContext(mWindowHandle);
						const BOOL adjusted = LLWinImm::setCompositionString(himc,
									SCS_QUERYRECONVERTSTRING, reconvert_string, size, NULL, 0);
						LLWinImm::releaseContext(mWindowHandle, himc);
						if (adjusted)
						{
							const llutf16string & text_utf16 = wstring_to_utf16str(context);
							const S32 new_preedit_start = reconvert_string->dwCompStrOffset / sizeof(WCHAR);
							const S32 new_preedit_end = new_preedit_start + reconvert_string->dwCompStrLen;
							select = utf16str_wstring_length(text_utf16, new_preedit_start);
							select_length = utf16str_wstring_length(text_utf16, new_preedit_end) - select;
							select += context_offset;
						}
					}
					mPreeditor->markAsPreedit(select, select_length);
				}

				*result = size;
				return TRUE;
			}
			case IMR_CONFIRMRECONVERTSTRING:
			{
				*result = FALSE;
				return TRUE;
			}
			case IMR_DOCUMENTFEED:
			{
				const LLWString & wtext = mPreeditor->getPreeditString();
				S32 preedit, preedit_length;
				mPreeditor->getPreeditRange(&preedit, &preedit_length);
				
				S32 context_offset;
				LLWString context = find_context(wtext, preedit, preedit_length, &context_offset);
				preedit -= context_offset;
				if (preedit_length)
				{
					// IMR_DOCUMENTFEED may be called when we have an active preedit.
					// We should pass the context string *excluding* the preedit string.
					// Otherwise, some IME are confused.
					context.erase(preedit, preedit_length);
				}
				
				RECONVERTSTRING *reconvert_string = (RECONVERTSTRING *)param;
				*result = fillReconvertString(context, preedit, 0, reconvert_string);
				return TRUE;
			}
			default:
				return FALSE;
		}
	}

	return FALSE;
}

//static
std::vector<std::string> LLWindowWin32::getDynamicFallbackFontList()
{
	// Fonts previously in getFontListSans() have moved to fonts.xml.
	return std::vector<std::string>();
}


#endif // LL_WINDOWS
