/** 
 * @file llwindowwin32.cpp
 * @brief Platform-dependent implementation of llwindow
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#if LL_WINDOWS && !LL_MESA_HEADLESS

#include "llwindowwin32.h"

#include <commdlg.h>
#include <WinUser.h>
#include <mapi.h>
#include <process.h>	// for _spawn
#include <shellapi.h>

// Require DirectInput version 8
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>


#include "llkeyboardwin32.h"
#include "llerror.h"
#include "llgl.h"
#include "llstring.h"
#include "lldir.h"

#include "llglheaders.h"

#include "indra_constants.h"

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

void show_window_creation_error(const char* title)
{
	llwarns << title << llendl;
	shell_open( "help/window_creation_error.html");
}

//static
BOOL LLWindowWin32::sIsClassRegistered = FALSE;



LPDIRECTINPUT8       g_pDI              = NULL;         
LPDIRECTINPUTDEVICE8 g_pJoystick        = NULL;     
BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance,
									VOID* pContext );
BOOL CALLBACK EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi,
								  VOID* pContext );


LLWindowWin32::LLWindowWin32(char *title, char *name, S32 x, S32 y, S32 width,
							 S32 height, U32 flags, 
							 BOOL fullscreen, BOOL clearBg,
							 BOOL disable_vsync, BOOL use_gl,
							 BOOL ignore_pixel_depth)
	: LLWindow(fullscreen, flags)
{
	S32 i = 0;
	mIconResource = gIconResource;
	mOverrideAspectRatio = 0.f;
	mNativeAspectRatio = 0.f;
	mMousePositionModified = FALSE;
	mInputProcessingPaused = FALSE;

	// Initialize the keyboard
	gKeyboard = new LLKeyboardWin32();

	GLuint			pixel_format;
	WNDCLASS		wc;
	DWORD			dw_ex_style;
	DWORD			dw_style;
	RECT			window_rect;

	// Set the window title
	if (!title)
	{
		mWindowTitle = new WCHAR[50];
		wsprintf(mWindowTitle, L"OpenGL Window");
	}
	else
	{
		mWindowTitle = new WCHAR[256]; // Assume title length < 255 chars.
		mbstowcs(mWindowTitle, title, 255);
		mWindowTitle[255] = 0;
	}

	// Set the window class name
	if (!name)
	{
		mWindowClassName = new WCHAR[50];
		wsprintf(mWindowClassName, L"OpenGL Window");
	}
	else
	{
		mWindowClassName = new WCHAR[256]; // Assume title length < 255 chars.
		mbstowcs(mWindowClassName, name, 255);
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
			OSMessageBox("RegisterClass failed", "Error", OSMB_OK);
			return;
		}
		sIsClassRegistered = TRUE;
	}

	//-----------------------------------------------------------------------
	// Get the current refresh rate
	//-----------------------------------------------------------------------

	DEVMODE dev_mode;
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
			llwarns << "Couldn't find display mode " << width << " by " << height << " at " << BITS_PER_PIXEL << " bits per pixel" << llendl;
			success = FALSE;
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

			llinfos << "Running at " << dev_mode.dmPelsWidth
				<< "x"   << dev_mode.dmPelsHeight
				<< "x"   << dev_mode.dmBitsPerPel
				<< " @ " << dev_mode.dmDisplayFrequency
				<< llendl;
		}
		else
		{
			mFullscreen = FALSE;
			mFullscreenWidth   = -1;
			mFullscreenHeight  = -1;
			mFullscreenBits    = -1;
			mFullscreenRefresh = -1;

			char error[256];	/* Flawfinder: ignore */
			snprintf(error, sizeof(error), "Unable to run fullscreen at %d x %d.\nRunning in window.", width, height);	/* Flawfinder: ignore */
			OSMessageBox(error, "Error", OSMB_OK);
		}
	}

	//-----------------------------------------------------------------------
	// Resize window to account for borders
	//-----------------------------------------------------------------------
	if (mFullscreen)
	{
		dw_ex_style = WS_EX_APPWINDOW;
		dw_style = WS_POPUP;

		// Move window borders out not to cover window contents
		AdjustWindowRectEx(&window_rect, dw_style, FALSE, dw_ex_style);
	}
	else
	{
		// Window with an edge
		dw_ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dw_style = WS_OVERLAPPEDWINDOW;
	}

	//-----------------------------------------------------------------------
	// Create the window
	// Microsoft help indicates that GL windows must be created with
	// WS_CLIPSIBLINGS and WS_CLIPCHILDREN, but not CS_PARENTDC
	//-----------------------------------------------------------------------
	mWindowHandle = CreateWindowEx(dw_ex_style,
		mWindowClassName,
		mWindowTitle,
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dw_style,
		x,												// x pos
		y,												// y pos
		window_rect.right - window_rect.left,			// width
		window_rect.bottom - window_rect.top,			// height
		NULL,
		NULL,
		mhInstance,
		NULL);

	if (!mWindowHandle)
	{
		DestroyWindow(mWindowHandle);
		OSMessageBox("Window creation error", "Error", OSMB_OK);
		return;
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



	S32 pfdflags = PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	if (use_gl)
	{
		pfdflags |= PFD_SUPPORT_OPENGL;
	}

	//-----------------------------------------------------------------------
	// Create GL drawing context
	//-----------------------------------------------------------------------
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR), 
			1,
			pfdflags, 
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
		OSMessageBox("Can't make GL device context", "Error", OSMB_OK);
		return;
	}

	if (!(pixel_format = ChoosePixelFormat(mhDC, &pfd)))
	{
		close();
		OSMessageBox("Can't find suitable pixel format", "Error", OSMB_OK);
		return;
	}

	// Verify what pixel format we actually received.
	if (!DescribePixelFormat(mhDC, pixel_format, sizeof(PIXELFORMATDESCRIPTOR),
		&pfd))
	{
		close();
		OSMessageBox("Can't get pixel format description", "Error", OSMB_OK);
		return;
	}

	// sanity check pfd returned by Windows
	if (!ignore_pixel_depth && (pfd.cColorBits < 32))
	{
		close();
		OSMessageBox(
			"Second Life requires True Color (32-bit) to run in a window.\n"
			"Please go to Control Panels -> Display -> Settings and\n"
			"set the screen to 32-bit color.\n"
			"Alternately, if you choose to run fullscreen, Second Life\n"
			"will automatically adjust the screen each time it runs.",
			"Error",
			OSMB_OK);
		return;
	}

	if (!ignore_pixel_depth && (pfd.cAlphaBits < 8))
	{
		close();
		OSMessageBox(
			"Second Life is unable to run because it can't get an 8 bit alpha\n"
			"channel.  Usually this is due to video card driver issues.\n"
			"Please make sure you have the latest video card drivers installed.\n"
			"Also be sure your monitor is set to True Color (32-bit) in\n"
			"Control Panels -> Display -> Settings.\n"
			"If you continue to receive this message, contact customer service.",
			"Error",
			OSMB_OK);
		return;
	}

	if (!SetPixelFormat(mhDC, pixel_format, &pfd))
	{
		close();
		OSMessageBox("Can't set pixel format", "Error", OSMB_OK);
		return;
	}

	if (use_gl)
	{
		if (!(mhRC = wglCreateContext(mhDC)))
		{
			close();
			OSMessageBox("Can't create GL rendering context", "Error", OSMB_OK);
			return;
		}

		if (!wglMakeCurrent(mhDC, mhRC))
		{
			close();
			OSMessageBox("Can't activate GL rendering context", "Error", OSMB_OK);
			return;
		}

		gGLManager.initWGL();

		if (gGLManager.mHasWGLARBPixelFormat && (wglChoosePixelFormatARB != NULL))
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

			attrib_list[cur_attrib++] = WGL_RED_BITS_ARB;
			attrib_list[cur_attrib++] = 8;

			attrib_list[cur_attrib++] = WGL_GREEN_BITS_ARB;
			attrib_list[cur_attrib++] = 8;

			attrib_list[cur_attrib++] = WGL_BLUE_BITS_ARB;
			attrib_list[cur_attrib++] = 8;

			attrib_list[cur_attrib++] = WGL_ALPHA_BITS_ARB;
			attrib_list[cur_attrib++] = 8;

			// End the list
			attrib_list[cur_attrib++] = 0;

			GLint pixel_formats[256];
			U32 num_formats = 0;

			// First we try and get a 32 bit depth pixel format
			BOOL result = wglChoosePixelFormatARB(mhDC, attrib_list, NULL, 256, pixel_formats, &num_formats);
			if (!result)
			{
				close();
				show_window_creation_error("Error after wglChoosePixelFormatARB 32-bit");
				return;
			}

			if (!num_formats)
			{
				llinfos << "No 32 bit z-buffer, trying 24 bits instead" << llendl;
				// Try 24-bit format
				attrib_list[1] = 24;
				BOOL result = wglChoosePixelFormatARB(mhDC, attrib_list, NULL, 256, pixel_formats, &num_formats);
				if (!result)
				{
					close();
					show_window_creation_error("Error after wglChoosePixelFormatARB 24-bit");
					return;
				}

				if (!num_formats)
				{
					llwarns << "Couldn't get 24 bit z-buffer,trying 16 bits instead!" << llendl;
					attrib_list[1] = 16;
					BOOL result = wglChoosePixelFormatARB(mhDC, attrib_list, NULL, 256, pixel_formats, &num_formats);
					if (!result || !num_formats)
					{
						close();
						show_window_creation_error("Error after wglChoosePixelFormatARB 16-bit");
						return;
					}
				}

				llinfos << "Choosing pixel formats: " << num_formats << " pixel formats returned" << llendl;

				pixel_format = pixel_formats[0];
			}

			DestroyWindow(mWindowHandle);

			mWindowHandle = CreateWindowEx(dw_ex_style,
				mWindowClassName,
				mWindowTitle,
				WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dw_style,
				x,								// x pos
				y,								// y pos
				window_rect.right - window_rect.left,			// width
				window_rect.bottom - window_rect.top,			// height
				NULL,
				NULL,
				mhInstance,
				NULL);

			if (!(mhDC = GetDC(mWindowHandle)))
			{
				close();
				OSMessageBox("Can't make GL device context", "Error", OSMB_OK);
				return;
			}

			if (!SetPixelFormat(mhDC, pixel_format, &pfd))
			{
				close();
				OSMessageBox("Can't set pixel format", "Error", OSMB_OK);
				return;
			}

			int swap_method = 0;
			GLint swap_query = WGL_SWAP_METHOD_ARB;

			if (wglGetPixelFormatAttribivARB(mhDC, pixel_format, 0, 1, &swap_query, &swap_method))
			{
				switch (swap_method)
				{
				case WGL_SWAP_EXCHANGE_ARB:
					mSwapMethod = SWAP_METHOD_EXCHANGE;
					llinfos << "Swap Method: Exchange" << llendl;
					break;
				case WGL_SWAP_COPY_ARB:
					mSwapMethod = SWAP_METHOD_COPY;
					llinfos << "Swap Method: Copy" << llendl;
					break;
				case WGL_SWAP_UNDEFINED_ARB:
					mSwapMethod = SWAP_METHOD_UNDEFINED;
					llinfos << "Swap Method: Undefined" << llendl;
					break;
				default:
					mSwapMethod = SWAP_METHOD_UNDEFINED;
					llinfos << "Swap Method: Unknown" << llendl;
					break;
				}
			}		
		}
		else
		{
			llwarns << "No wgl_ARB_pixel_format extension, using default ChoosePixelFormat!" << llendl;
		}

		// Verify what pixel format we actually received.
		if (!DescribePixelFormat(mhDC, pixel_format, sizeof(PIXELFORMATDESCRIPTOR),
			&pfd))
		{
			close();
			OSMessageBox("Can't get pixel format description", "Error", OSMB_OK);
			return;
		}
		llinfos << "GL buffer: Color Bits " << S32(pfd.cColorBits) 
			<< " Alpha Bits " << S32(pfd.cAlphaBits)
			<< " Depth Bits " << S32(pfd.cDepthBits) 
			<< llendl;

		if (pfd.cColorBits < 32)
		{
			close();
			OSMessageBox(
				"Second Life requires True Color (32-bit) to run in a window.\n"
				"Please go to Control Panels -> Display -> Settings and\n"
				"set the screen to 32-bit color.\n"
				"Alternately, if you choose to run fullscreen, Second Life\n"
				"will automatically adjust the screen each time it runs.",
				"Error",
				OSMB_OK);
			return;
		}

		if (pfd.cAlphaBits < 8)
		{
			close();
			OSMessageBox(
				"Second Life is unable to run because it can't get an 8 bit alpha\n"
				"channel.  Usually this is due to video card driver issues.\n"
				"Please make sure you have the latest video card drivers installed.\n"
				"Also be sure your monitor is set to True Color (32-bit) in\n"
				"Control Panels -> Display -> Settings.\n"
				"If you continue to receive this message, contact customer service.",
				"Error",
				OSMB_OK);
			return;
		}

		if (!(mhRC = wglCreateContext(mhDC)))
		{
			close();
			OSMessageBox("Can't create GL rendering context", "Error", OSMB_OK);
			return;
		}

		if (!wglMakeCurrent(mhDC, mhRC))
		{
			close();
			OSMessageBox("Can't activate GL rendering context", "Error", OSMB_OK);
			return;
		}

		if (!gGLManager.initGL())
		{
			close();
			OSMessageBox(
				"Second Life is unable to run because your video card drivers\n"
				"are out of date or unsupported. Please make sure you have\n"
				"the latest video card drivers installed.\n\n"
				"If you continue to receive this message, contact customer service.",
				"Error",
				OSMB_OK);
			return;
		}

		// Disable vertical sync for swap
		if (disable_vsync && wglSwapIntervalEXT)
		{
			llinfos << "Disabling vertical sync" << llendl;
			wglSwapIntervalEXT(0);
		}
		else
		{
			llinfos << "Keeping vertical sync" << llendl;
		}


		// OK, let's get the current gamma information and store it off.
		mCurrentGamma = 0.f; // Not set, default;
		if (!GetDeviceGammaRamp(mhDC, mPrevGammaRamp))
		{
			llwarns << "Unable to get device gamma ramp" << llendl;
		}

		// Calculate what the current gamma is.  From a posting by Garrett T. Bass, Get/SetDeviceGammaRamp Demystified
		// http://apollo.iwt.uni-bielefeld.de/~ml_robot/OpenGL-04-2000/0058.html

		// We're going to assume that gamma's the same for all 3 channels, because I don't feel like doing it otherwise.
		// Using the red channel.

		F32 Csum = 0.0; 
		S32 Ccount = 0; 
		for (i = 0; i < 256; i++) 
		{ 
			if (i != 0 && mPrevGammaRamp[i] != 0 && mPrevGammaRamp[i] != 65536) 
			{ 
				F64 B = (i % 256) / 256.0; 
				F64 A = mPrevGammaRamp[i] / 65536.0; 
				F32 C = (F32) ( log(A) / log(B) ); 
				Csum += C; 
				Ccount++; 
			} 
		} 
		mCurrentGamma = Csum / Ccount; 

		llinfos << "Previous gamma: " << mCurrentGamma << llendl;
	}


	//store this pointer for wndProc callback
	SetWindowLong(mWindowHandle, GWL_USERDATA, (U32)this);

	//start with arrow cursor
	initCursors();
	setCursor( UI_CURSOR_ARROW );

	// Direct Input
	HRESULT hr;

	if( FAILED( hr = DirectInput8Create( GetModuleHandle(NULL), DIRECTINPUT_VERSION, 
		IID_IDirectInput8, (VOID**)&g_pDI, NULL ) ) )
	{
		llwarns << "Direct8InputCreate failed!" << llendl;
	}
	else
	{
		while(1)
		{
			// Look for a simple joystick we can use for this sample program.
			if (FAILED( hr = g_pDI->EnumDevices( DI8DEVCLASS_GAMECTRL, 
				EnumJoysticksCallback,
				NULL, DIEDFL_ATTACHEDONLY ) ) )
				break;
			if (!g_pJoystick)
				break;
			if( FAILED( hr = g_pJoystick->SetDataFormat( &c_dfDIJoystick ) ) )
				break;
			if( FAILED( hr = g_pJoystick->EnumObjects( EnumObjectsCallback, 
				(VOID*)mWindowHandle, DIDFT_ALL ) ) )
				break;
			g_pJoystick->Acquire();
			break;
		}
	}

	SetTimer( mWindowHandle, 0, 1000 / 30, NULL ); // 30 fps timer
}


LLWindowWin32::~LLWindowWin32()
{
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

void LLWindowWin32::minimize()
{
	setMouseClipping(FALSE);
	showCursor();
	ShowWindow(mWindowHandle, SW_MINIMIZE);
}


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
	llinfos << "Closing LLWindowWin32" << llendl;
	// Is window is already closed?
	if (!mWindowHandle)
	{
		return;
	}

	// Make sure cursor is visible and we haven't mangled the clipping state.
	setMouseClipping(FALSE);
	showCursor();

	// Go back to screen mode written in the registry.
	if (mFullscreen)
	{
		resetDisplayResolution();
	}

	// Clean up remaining GL state
	llinfos << "Shutting down GL" << llendl;
	gGLManager.shutdownGL();

	llinfos << "Releasing Context" << llendl;
	if (mhRC)
	{
		if (!wglMakeCurrent(NULL, NULL))
		{
			llwarns << "Release of DC and RC failed" << llendl;
		}

		if (!wglDeleteContext(mhRC))
		{
			llwarns << "Release of rendering context failed" << llendl;
		}

		mhRC = NULL;
	}

	// Restore gamma to the system values.
	restoreGamma();

	if (mhDC && !ReleaseDC(mWindowHandle, mhDC))
	{
		llwarns << "Release of ghDC failed" << llendl;
		mhDC = NULL;
	}

	llinfos << "Destroying Window" << llendl;
	
	// Don't process events in our mainWindowProc any longer.
	SetWindowLong(mWindowHandle, GWL_USERDATA, NULL);

	// Make sure we don't leave a blank toolbar button.
	ShowWindow(mWindowHandle, SW_HIDE);

	// This causes WM_DESTROY to be sent *immediately*
	if (!DestroyWindow(mWindowHandle))
	{
		OSMessageBox("DestroyWindow(mWindowHandle) failed", "Shutdown Error", OSMB_OK);
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

BOOL LLWindowWin32::setSize(const LLCoordScreen size)
{
	LLCoordScreen position;

	getPosition(&position);
	if (!mWindowHandle)
	{
		return FALSE;
	}

	moveWindow(position, size);
	return TRUE;
}

// changing fullscreen resolution
BOOL LLWindowWin32::switchContext(BOOL fullscreen, LLCoordScreen size, BOOL disable_vsync)
{
	GLuint	pixel_format;
	DEVMODE dev_mode;
	DWORD	current_refresh;
	DWORD	dw_ex_style;
	DWORD	dw_style;
	RECT	window_rect;
	S32 width = size.mX;
	S32 height = size.mY;

	resetDisplayResolution();

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
			llwarns << "Release of DC and RC failed" << llendl;
		}

		if (!wglDeleteContext(mhRC))
		{
			llwarns << "Release of rendering context failed" << llendl;
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
			llwarns << "Couldn't find display mode " << width << " by " << height << " at " << BITS_PER_PIXEL << " bits per pixel" << llendl;
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

			llinfos << "Running at " << dev_mode.dmPelsWidth
				<< "x"   << dev_mode.dmPelsHeight
				<< "x"   << dev_mode.dmBitsPerPel
				<< " @ " << dev_mode.dmDisplayFrequency
				<< llendl;

			window_rect.left = (long) 0;
			window_rect.right = (long) width;			// Windows GDI rects don't include rightmost pixel
			window_rect.top = (long) 0;
			window_rect.bottom = (long) height;
			dw_ex_style = WS_EX_APPWINDOW;
			dw_style = WS_POPUP;

			// Move window borders out not to cover window contents
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

			llinfos << "Unable to run fullscreen at " << width << "x" << height << llendl;
			llinfos << "Running in window." << llendl;
			return FALSE;
		}
	}
	else
	{
		mFullscreen = FALSE;
		window_rect.left = (long) 0;
		window_rect.right = (long) width;			// Windows GDI rects don't include rightmost pixel
		window_rect.top = (long) 0;
		window_rect.bottom = (long) height;
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
		OSMessageBox("Can't make GL device context", "Error", OSMB_OK);
		return FALSE;
	}

	if (!(pixel_format = ChoosePixelFormat(mhDC, &pfd)))
	{
		close();
		OSMessageBox("Can't find suitable pixel format", "Error", OSMB_OK);
		return FALSE;
	}

	// Verify what pixel format we actually received.
	if (!DescribePixelFormat(mhDC, pixel_format, sizeof(PIXELFORMATDESCRIPTOR),
		&pfd))
	{
		close();
		OSMessageBox("Can't get pixel format description", "Error", OSMB_OK);
		return FALSE;
	}

	if (pfd.cColorBits < 32)
	{
		close();
		OSMessageBox(
			"Second Life requires True Color (32-bit) to run in a window.\n"
			"Please go to Control Panels -> Display -> Settings and\n"
			"set the screen to 32-bit color.\n"
			"Alternately, if you choose to run fullscreen, Second Life\n"
			"will automatically adjust the screen each time it runs.",
			"Error",
			OSMB_OK);
		return FALSE;
	}

	if (pfd.cAlphaBits < 8)
	{
		close();
		OSMessageBox(
			"Second Life is unable to run because it can't get an 8 bit alpha\n"
			"channel.  Usually this is due to video card driver issues.\n"
			"Please make sure you have the latest video card drivers installed.\n"
			"Also be sure your monitor is set to True Color (32-bit) in\n"
			"Control Panels -> Display -> Settings.\n"
			"If you continue to receive this message, contact customer service.",
			"Error",
			OSMB_OK);
		return FALSE;
	}

	if (!SetPixelFormat(mhDC, pixel_format, &pfd))
	{
		close();
		OSMessageBox("Can't set pixel format", "Error", OSMB_OK);
		return FALSE;
	}

	if (!(mhRC = wglCreateContext(mhDC)))
	{
		close();
		OSMessageBox("Can't create GL rendering context", "Error", OSMB_OK);
		return FALSE;
	}

	if (!wglMakeCurrent(mhDC, mhRC))
	{
		close();
		OSMessageBox("Can't activate GL rendering context", "Error", OSMB_OK);
		return FALSE;
	}

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

		attrib_list[cur_attrib++] = WGL_RED_BITS_ARB;
		attrib_list[cur_attrib++] = 8;

		attrib_list[cur_attrib++] = WGL_GREEN_BITS_ARB;
		attrib_list[cur_attrib++] = 8;

		attrib_list[cur_attrib++] = WGL_BLUE_BITS_ARB;
		attrib_list[cur_attrib++] = 8;

		attrib_list[cur_attrib++] = WGL_ALPHA_BITS_ARB;
		attrib_list[cur_attrib++] = 8;

		// End the list
		attrib_list[cur_attrib++] = 0;

		GLint pixel_formats[256];
		U32 num_formats = 0;

		// First we try and get a 32 bit depth pixel format
		BOOL result = wglChoosePixelFormatARB(mhDC, attrib_list, NULL, 256, pixel_formats, &num_formats);
		if (!result)
		{
			close();
			show_window_creation_error("Error after wglChoosePixelFormatARB 32-bit");
			return FALSE;
		}

		if (!num_formats)
		{
			llinfos << "No 32 bit z-buffer, trying 24 bits instead" << llendl;
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
				llwarns << "Couldn't get 24 bit z-buffer,trying 16 bits instead!" << llendl;
				attrib_list[1] = 16;
				BOOL result = wglChoosePixelFormatARB(mhDC, attrib_list, NULL, 256, pixel_formats, &num_formats);
				if (!result || !num_formats)
				{
					close();
					show_window_creation_error("Error after wglChoosePixelFormatARB 16-bit");
					return FALSE;
				}
			}

			llinfos << "Choosing pixel formats: " << num_formats << " pixel formats returned" << llendl;

			pixel_format = pixel_formats[0];
		}

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

		if (!(mhDC = GetDC(mWindowHandle)))
		{
			close();
			OSMessageBox("Can't make GL device context", "Error", OSMB_OK);
			return FALSE;
		}

		if (!SetPixelFormat(mhDC, pixel_format, &pfd))
		{
			close();
			OSMessageBox("Can't set pixel format", "Error", OSMB_OK);
			return FALSE;
		}

		int swap_method = 0;
		GLint swap_query = WGL_SWAP_METHOD_ARB;

		if (wglGetPixelFormatAttribivARB(mhDC, pixel_format, 0, 1, &swap_query, &swap_method))
		{
			switch (swap_method)
			{
			case WGL_SWAP_EXCHANGE_ARB:
				mSwapMethod = SWAP_METHOD_EXCHANGE;
				llinfos << "Swap Method: Exchange" << llendl;
				break;
			case WGL_SWAP_COPY_ARB:
				mSwapMethod = SWAP_METHOD_COPY;
				llinfos << "Swap Method: Copy" << llendl;
				break;
			case WGL_SWAP_UNDEFINED_ARB:
				mSwapMethod = SWAP_METHOD_UNDEFINED;
				llinfos << "Swap Method: Undefined" << llendl;
				break;
			default:
				mSwapMethod = SWAP_METHOD_UNDEFINED;
				llinfos << "Swap Method: Unknown" << llendl;
				break;
			}
		}		
	}
	else
	{
		llwarns << "No wgl_ARB_pixel_format extension, using default ChoosePixelFormat!" << llendl;
	}

	// Verify what pixel format we actually received.
	if (!DescribePixelFormat(mhDC, pixel_format, sizeof(PIXELFORMATDESCRIPTOR),
		&pfd))
	{
		close();
		OSMessageBox("Can't get pixel format description", "Error", OSMB_OK);
		return FALSE;
	}

	llinfos << "GL buffer: Color Bits " << S32(pfd.cColorBits) 
		<< " Alpha Bits " << S32(pfd.cAlphaBits)
		<< " Depth Bits " << S32(pfd.cDepthBits) 
		<< llendl;

	if (pfd.cColorBits < 32)
	{
		close();
		OSMessageBox(
			"Second Life requires True Color (32-bit) to run in a window.\n"
			"Please go to Control Panels -> Display -> Settings and\n"
			"set the screen to 32-bit color.\n"
			"Alternately, if you choose to run fullscreen, Second Life\n"
			"will automatically adjust the screen each time it runs.",
			"Error",
			OSMB_OK);
		return FALSE;
	}

	if (pfd.cAlphaBits < 8)
	{
		close();
		OSMessageBox(
			"Second Life is unable to run because it can't get an 8 bit alpha\n"
			"channel.  Usually this is due to video card driver issues.\n"
			"Please make sure you have the latest video card drivers installed.\n"
			"Also be sure your monitor is set to True Color (32-bit) in\n"
			"Control Panels -> Display -> Settings.\n"
			"If you continue to receive this message, contact customer service.",
			"Error",
			OSMB_OK);
		return FALSE;
	}

	if (!(mhRC = wglCreateContext(mhDC)))
	{
		close();
		OSMessageBox("Can't create GL rendering context", "Error", OSMB_OK);
		return FALSE;
	}

	if (!wglMakeCurrent(mhDC, mhRC))
	{
		close();
		OSMessageBox("Can't activate GL rendering context", "Error", OSMB_OK);
		return FALSE;
	}

	if (!gGLManager.initGL())
	{
		close();
		OSMessageBox(
					 "Second Life is unable to run because your video card drivers\n"
					 "are out of date or unsupported. Please make sure you have\n"
					 "the latest video card drivers installed.\n\n"
					 "If you continue to receive this message, contact customer service.",
					 "Error",
					 OSMB_OK);
		return FALSE;
	}

	// Disable vertical sync for swap
	if (disable_vsync && wglSwapIntervalEXT)
	{
		llinfos << "Disabling vertical sync" << llendl;
		wglSwapIntervalEXT(0);
	}
	else
	{
		llinfos << "Keeping vertical sync" << llendl;
	}

	SetWindowLong(mWindowHandle, GWL_USERDATA, (U32)this);
	show();

	// ok to post quit messages now
	mPostQuit = TRUE;
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

	MoveWindow(mWindowHandle, position.mX, position.mY, size.mX, size.mY, TRUE);
}

BOOL LLWindowWin32::setCursorPosition(const LLCoordWindow position)
{
	LLCoordScreen screen_pos;

	mMousePositionModified = TRUE;
	if (!mWindowHandle)
	{
		return FALSE;
	}

	if (!convertCoords(position, &screen_pos))
	{
		return FALSE;
	}

	return SetCursorPos(screen_pos.mX, screen_pos.mY);
}

BOOL LLWindowWin32::getCursorPosition(LLCoordWindow *position)
{
	POINT cursor_point;
	LLCoordScreen screen_pos;

	if (!mWindowHandle ||
		!GetCursorPos(&cursor_point))
	{
		return FALSE;
	}

	screen_pos.mX = cursor_point.x;
	screen_pos.mY = cursor_point.y;

	return convertCoords(screen_pos, position);
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

	// Color cursors
	mCursor[UI_CURSOR_TOOLSIT] = loadColorCursor(TEXT("TOOLSIT"));
	mCursor[UI_CURSOR_TOOLBUY] = loadColorCursor(TEXT("TOOLBUY"));
	mCursor[UI_CURSOR_TOOLPAY] = loadColorCursor(TEXT("TOOLPAY"));
	mCursor[UI_CURSOR_TOOLOPEN] = loadColorCursor(TEXT("TOOLOPEN"));

	// Note: custom cursors that are not found make LoadCursor() return NULL.
	for( S32 i = 0; i < UI_CURSOR_COUNT; i++ )
	{
		if( !mCursor[i] )
		{
			mCursor[i] = LoadCursor(NULL, IDC_ARROW);
		}
	}
}



void LLWindowWin32::setCursor(ECursorType cursor)
{
	if (cursor == UI_CURSOR_ARROW
		&& mBusyCount > 0)
	{
		cursor = UI_CURSOR_WORKING;
	}

	if( mCurrentCursor != cursor )
	{
		mCurrentCursor = cursor;
		SetCursor( mCursor[cursor] );
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
	ReleaseCapture();
}


void LLWindowWin32::delayInputProcessing()
{
	mInputProcessingPaused = TRUE;
}

void LLWindowWin32::gatherInput()
{
	MSG		msg;
	int		msg_count = 0;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msg_count < MAX_MESSAGE_PER_UPDATE)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
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

		// For async host by name support.  Really hacky.
		if (gAsyncMsgCallback && (LL_WM_HOST_RESOLVED == msg.message))
		{
			gAsyncMsgCallback(msg);
		}
	}

	mInputProcessingPaused = FALSE;

	// clear this once we've processed all mouse messages that might have occurred after
	// we slammed the mouse position
	mMousePositionModified = FALSE;
}

LRESULT CALLBACK LLWindowWin32::mainWindowProc(HWND h_wnd, UINT u_msg, WPARAM w_param, LPARAM l_param)
{
	LLWindowWin32 *window_imp = (LLWindowWin32 *)GetWindowLong(h_wnd, GWL_USERDATA);

	if (NULL != window_imp)
	{
		// Has user provided their own window callback?
		if (NULL != window_imp->mWndProc)
		{
			if (!window_imp->mWndProc(h_wnd, u_msg, w_param, l_param))
			{
				// user has handled window message
				return 0;
			}
		}

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
			window_imp->updateJoystick( );
			break;

		case WM_PAINT:
			GetUpdateRect(window_imp->mWindowHandle, &update_rect, FALSE);
			update_width = update_rect.right - update_rect.left + 1;
			update_height = update_rect.bottom - update_rect.top + 1;
			window_imp->mCallbacks->handlePaint(window_imp, update_rect.left, update_rect.top,
				update_width, update_height);
			break;
		case WM_PARENTNOTIFY:
			u_msg = u_msg;
			break;

		case WM_SETCURSOR:
			// This message is sent whenever the cursor is moved in a window.
			// You need to set the appropriate cursor appearance.

			// Only take control of cursor over client region of window
			// This allows Windows(tm) to handle resize cursors, etc.
			if (LOWORD(l_param) == HTCLIENT)
			{
				SetCursor(window_imp->mCursor[ window_imp->mCurrentCursor] );
				return 0;
			}
			break;

		case WM_ENTERMENULOOP:
			window_imp->mCallbacks->handleWindowBlock(window_imp);
			break;

		case WM_EXITMENULOOP:
			window_imp->mCallbacks->handleWindowUnblock(window_imp);
			break;

		case WM_ACTIVATEAPP:
			{
				// This message should be sent whenever the app gains or loses focus.
				BOOL activating = (BOOL) w_param;
				BOOL minimized = window_imp->getMinimized();

				if (gDebugWindowProc)
				{
					llinfos << "WINDOWPROC ActivateApp "
						<< " activating " << S32(activating)
						<< " minimized " << S32(minimized)
						<< " fullscreen " << S32(window_imp->mFullscreen)
						<< llendl;
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
				break;
			}

		case WM_ACTIVATE:
			{
				// Can be one of WA_ACTIVE, WA_CLICKACTIVE, or WA_INACTIVE
				BOOL activating = (LOWORD(w_param) != WA_INACTIVE);

				BOOL minimized = BOOL(HIWORD(w_param));

				// JC - I'm not sure why, but if we don't report that we handled the 
				// WM_ACTIVATE message, the WM_ACTIVATEAPP messages don't work 
				// properly when we run fullscreen.
				if (gDebugWindowProc)
				{
					llinfos << "WINDOWPROC Activate "
						<< " activating " << S32(activating) 
						<< " minimized " << S32(minimized)
						<< llendl;
				}

				// Don't handle this.
				break;
			}

		case WM_QUERYOPEN:
			// TODO: use this to return a nice icon
			break;

		case WM_SYSCOMMAND:
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
			// Will the app allow the window to close?
			if (window_imp->mCallbacks->handleCloseRequest(window_imp))
			{
				// Get the app to initiate cleanup.
				window_imp->mCallbacks->handleQuit(window_imp);
				// The app is responsible for calling destroyWindow when done with GL
			}
			return 0;

		case WM_DESTROY:
			if (window_imp->shouldPostQuit())
			{
				PostQuitMessage(0);  // Posts WM_QUIT with an exit code of 0
			}
			return 0;

		case WM_COMMAND:
			if (!HIWORD(w_param)) // this message is from a menu
			{
				window_imp->mCallbacks->handleMenuSelect(window_imp, LOWORD(w_param));
			}
			break;

		case WM_SYSKEYDOWN:
			// allow system keys, such as ALT-F4 to be processed by Windows
			eat_keystroke = FALSE;
		case WM_KEYDOWN:
			{
				if (gDebugWindowProc)
				{
					llinfos << "Debug WindowProc WM_KEYDOWN "
						<< " key " << S32(w_param) 
						<< llendl;
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
			if (gDebugWindowProc)
			{
				llinfos << "Debug WindowProc WM_KEYUP "
					<< " key " << S32(w_param) 
					<< llendl;
			}
			if (gKeyboard->handleKeyUp(w_param, mask) && eat_keystroke)
			{
				return 0;
			}

			// pass on to windows
			break;


		case WM_CHAR:
			// Should really use WM_UNICHAR eventually, but it requires a specific Windows version and I need
			// to figure out how that works. - Doug
			// llinfos << "WM_CHAR: " << w_param << llendl;
			if (gDebugWindowProc)
			{
				llinfos << "Debug WindowProc WM_CHAR "
					<< " key " << S32(w_param) 
					<< llendl;
			}
			if (window_imp->mCallbacks->handleUnicodeChar(w_param, gKeyboard->currentMask(FALSE)))
			{
				return 0;
			}
			break;

		case WM_LBUTTONDOWN:
			{
				// Because we move the cursor position in the app, we need to query
				// to find out where the cursor at the time the event is handled.
				// If we don't do this, many clicks could get buffered up, and if the
				// first click changes the cursor position, all subsequent clicks
				// will occur at the wrong location.  JC
				LLCoordWindow cursor_coord_window;
				if (window_imp->mMousePositionModified)
				{
					window_imp->getCursorPosition(&cursor_coord_window);
					window_imp->convertCoords(cursor_coord_window, &gl_coord);
				}
				else
				{
					window_imp->convertCoords(window_coord, &gl_coord);
				}
				MASK mask = gKeyboard->currentMask(TRUE);
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
				// Because we move the cursor position in the app, we need to query
				// to find out where the cursor at the time the event is handled.
				// If we don't do this, many clicks could get buffered up, and if the
				// first click changes the cursor position, all subsequent clicks
				// will occur at the wrong location.  JC
				LLCoordWindow cursor_coord_window;
				if (window_imp->mMousePositionModified)
				{
					window_imp->getCursorPosition(&cursor_coord_window);
					window_imp->convertCoords(cursor_coord_window, &gl_coord);
				}
				else
				{
					window_imp->convertCoords(window_coord, &gl_coord);
				}
				MASK mask = gKeyboard->currentMask(TRUE);
				if (window_imp->mCallbacks->handleDoubleClick(window_imp, gl_coord, mask) )
				{
					return 0;
				}
			}
			break;

		case WM_LBUTTONUP:
			{
				//if (gDebugClicks)
				//{
				//	llinfos << "WndProc left button up" << llendl;
				//}
				// Because we move the cursor position in the app, we need to query
				// to find out where the cursor at the time the event is handled.
				// If we don't do this, many clicks could get buffered up, and if the
				// first click changes the cursor position, all subsequent clicks
				// will occur at the wrong location.  JC
				LLCoordWindow cursor_coord_window;
				if (window_imp->mMousePositionModified)
				{
					window_imp->getCursorPosition(&cursor_coord_window);
					window_imp->convertCoords(cursor_coord_window, &gl_coord);
				}
				else
				{
					window_imp->convertCoords(window_coord, &gl_coord);
				}
				MASK mask = gKeyboard->currentMask(TRUE);
				if (window_imp->mCallbacks->handleMouseUp(window_imp, gl_coord, mask))
				{
					return 0;
				}
			}
			break;

		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
			{
				// Because we move the cursor position in tllviewerhe app, we need to query
				// to find out where the cursor at the time the event is handled.
				// If we don't do this, many clicks could get buffered up, and if the
				// first click changes the cursor position, all subsequent clicks
				// will occur at the wrong location.  JC
				LLCoordWindow cursor_coord_window;
				if (window_imp->mMousePositionModified)
				{
					window_imp->getCursorPosition(&cursor_coord_window);
					window_imp->convertCoords(cursor_coord_window, &gl_coord);
				}
				else
				{
					window_imp->convertCoords(window_coord, &gl_coord);
				}
				MASK mask = gKeyboard->currentMask(TRUE);
				if (window_imp->mCallbacks->handleRightMouseDown(window_imp, gl_coord, mask))
				{
					return 0;
				}
			}
			break;

		case WM_RBUTTONUP:
			{
				// Because we move the cursor position in the app, we need to query
				// to find out where the cursor at the time the event is handled.
				// If we don't do this, many clicks could get buffered up, and if the
				// first click changes the cursor position, all subsequent clicks
				// will occur at the wrong location.  JC
				LLCoordWindow cursor_coord_window;
				if (window_imp->mMousePositionModified)
				{
					window_imp->getCursorPosition(&cursor_coord_window);
					window_imp->convertCoords(cursor_coord_window, &gl_coord);
				}
				else
				{
					window_imp->convertCoords(window_coord, &gl_coord);
				}
				MASK mask = gKeyboard->currentMask(TRUE);
				if (window_imp->mCallbacks->handleRightMouseUp(window_imp, gl_coord, mask))
				{
					return 0;
				}
			}
			break;

		case WM_MBUTTONDOWN:
			// Handle middle button click
			break;

		case WM_MOUSEWHEEL:
			{
				static short z_delta = 0;

				z_delta += HIWORD(w_param);
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
				window_imp->convertCoords(window_coord, &gl_coord);
				MASK mask = gKeyboard->currentMask(TRUE);
				window_imp->mCallbacks->handleMouseMove(window_imp, gl_coord, mask);
				return 0;
			}

		case WM_SIZE:
			{
				S32 width = S32( LOWORD(l_param) );
				S32 height = S32( HIWORD(l_param) );

				if (gDebugWindowProc)
				{
					BOOL maximized = ( w_param == SIZE_MAXIMIZED );
					BOOL restored  = ( w_param == SIZE_RESTORED );
					BOOL minimized = ( w_param == SIZE_MINIMIZED );

					llinfos << "WINDOWPROC Size "
						<< width << "x" << height
						<< " max " << S32(maximized)
						<< " min " << S32(minimized)
						<< " rest " << S32(restored)
						<< llendl;
				}

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
			if (gDebugWindowProc)
			{
				llinfos << "WINDOWPROC SetFocus" << llendl;
			}
			window_imp->mCallbacks->handleFocus(window_imp);
			return 0;

		case WM_KILLFOCUS:
			if (gDebugWindowProc)
			{
				llinfos << "WINDOWPROC KillFocus" << llendl;
			}
			window_imp->mCallbacks->handleFocusLost(window_imp);
			return 0;

		case WM_COPYDATA:
			// received a URL
			PCOPYDATASTRUCT myCDS = (PCOPYDATASTRUCT) l_param;
			window_imp->mCallbacks->handleDataCopy(window_imp, myCDS->dwData, myCDS->lpData);
			return 0;			
		}
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
					LLWString::removeCRLF(dst);
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
		LLWString::addCRLF(sanitized_string);
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


BOOL LLWindowWin32::sendEmail(const char* address, const char* subject, const char* body_text,
									   const char* attachment, const char* attachment_displayed_name )
{
	// Based on "A SendMail() DLL" by Greg Turner, Windows Developer Magazine, Nov. 1997.
	// See article for use of GetProcAddress
	// No restrictions on use.

	enum SendResult
	{
		LL_EMAIL_SUCCESS,
		LL_EMAIL_MAPI_NOT_INSTALLED,	// No MAPI Server (eg Microsoft Exchange) installed
		LL_EMAIL_MAPILOAD_FAILED,		// Load of MAPI32.DLL failed
		LL_EMAIL_SEND_FAILED			// The message send itself failed
	};

	SendResult  result = LL_EMAIL_SUCCESS;

	U32 mapi_installed = GetProfileInt(L"Mail", L"MAPI", 0);
	if( !mapi_installed)
	{
		result = LL_EMAIL_MAPI_NOT_INSTALLED;
	}
	else
	{
		HINSTANCE hMAPIInst = LoadLibrary(L"MAPI32.DLL");	/* Flawfinder: ignore */
		if(!hMAPIInst)
		{
			result =  LL_EMAIL_MAPILOAD_FAILED;
		}
		else
		{
			LPMAPISENDMAIL	pMAPISendMail   = (LPMAPISENDMAIL)	GetProcAddress(hMAPIInst, "MAPISendMail");

			// Send the message
			MapiRecipDesc recipients[1];
			recipients[0].ulReserved = 0;
			recipients[0].ulRecipClass = MAPI_TO;
			recipients[0].lpszName = (char*)address;
			recipients[0].lpszAddress = (char*)address;
			recipients[0].ulEIDSize = 0;
			recipients[0].lpEntryID = 0;

			MapiFileDesc files[1];
			files[0].ulReserved = 0;
			files[0].flFlags = 0;				// non-OLE file
			files[0].nPosition = -1;			// Leave file location in email unspecified.
			files[0].lpszPathName = (char*)attachment; // Must be fully qualified name, including drive letter.
			files[0].lpszFileName = (char*)attachment_displayed_name;		// If NULL, uses attachment as displayed name.
			files[0].lpFileType = NULL;			// Recipient will have to figure out what kind of file this is.

			MapiMessage msg;
			memset(&msg, 0, sizeof(msg));
			msg.lpszSubject         = (char*)subject;		// may be NULL
			msg.lpszNoteText        = (char*)body_text;
			msg.nRecipCount         = address ? 1 : 0;
			msg.lpRecips            = address ? recipients : NULL;
			msg.nFileCount			= attachment ? 1 : 0;
			msg.lpFiles				= attachment ? files : NULL;

			U32 success = pMAPISendMail(0, (U32) mWindowHandle, &msg, MAPI_DIALOG|MAPI_LOGON_UI|MAPI_NEW_SESSION, 0);
			if(success != SUCCESS_SUCCESS)
			{
				result = LL_EMAIL_SEND_FAILED;
			}

			FreeLibrary(hMAPIInst);
		}
	}

	return result == LL_EMAIL_SUCCESS;
}


S32 LLWindowWin32::stat(const char* file_name, struct stat* stat_info)
{
	llassert( sizeof(struct stat) == sizeof(struct _stat) );  // They are defined identically in sys/stat.h, but I'm paranoid.
	return LLFile::stat( file_name, (struct _stat*) stat_info );
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

	llinfos << "Setting gamma to " << gamma << llendl;

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

LLWindow::LLWindowResolution* LLWindowWin32::getSupportedResolutions(S32 &num_resolutions)
{
	if (!mSupportedResolutions)
	{
		mSupportedResolutions = new LLWindowResolution[MAX_NUM_RESOLUTIONS];
		DEVMODE dev_mode;

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
	dev_mode.dmSize = sizeof(dev_mode);
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
		llwarns << "setDisplayResolution failed, "
			<< width << "x" << height << "x" << bits << " @ " << refresh << llendl;
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
	llinfos << "resetDisplayResolution START" << llendl;

	LONG cds_result = ChangeDisplaySettings(NULL, 0);

	BOOL success = (DISP_CHANGE_SUCCESSFUL == cds_result);

	if (!success)
	{
		llwarns << "resetDisplayResolution failed" << llendl;
	}

	llinfos << "resetDisplayResolution END" << llendl;

	return success;
}

void LLWindowWin32::swapBuffers()
{
	SwapBuffers(mhDC);
}


BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance,
									VOID* pContext )
{
	HRESULT hr;

	// Obtain an interface to the enumerated joystick.
	hr = g_pDI->CreateDevice( pdidInstance->guidInstance, &g_pJoystick, NULL );

	// If it failed, then we can't use this joystick. (Maybe the user unplugged
	// it while we were in the middle of enumerating it.)
	if( FAILED(hr) ) 
		return DIENUM_CONTINUE;

	// Stop enumeration. Note: we're just taking the first joystick we get. You
	// could store all the enumerated joysticks and let the user pick.
	return DIENUM_STOP;
}

BOOL CALLBACK EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi,
								  VOID* pContext )
{
	if( pdidoi->dwType & DIDFT_AXIS )
	{
		DIPROPRANGE diprg; 
		diprg.diph.dwSize       = sizeof(DIPROPRANGE); 
		diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
		diprg.diph.dwHow        = DIPH_BYID; 
		diprg.diph.dwObj        = pdidoi->dwType; // Specify the enumerated axis
		diprg.lMin              = -1000; 
		diprg.lMax              = +1000; 

		// Set the range for the axis
		if( FAILED( g_pJoystick->SetProperty( DIPROP_RANGE, &diprg.diph ) ) ) 
			return DIENUM_STOP;

	}
	return DIENUM_CONTINUE;
}

void LLWindowWin32::updateJoystick( )
{
	HRESULT hr;
	DIJOYSTATE js;           // DInput joystick state

	if (!g_pJoystick)
		return;
	hr = g_pJoystick->Poll();
	if ( hr == DIERR_INPUTLOST )
	{
		hr = g_pJoystick->Acquire();
		return;
	}
	else if ( FAILED(hr) )
		return;

	// Get the input's device state
	if( FAILED( hr = g_pJoystick->GetDeviceState( sizeof(DIJOYSTATE), &js ) ) )
		return; // The device should have been acquired during the Poll()

	mJoyAxis[0] = js.lX/1000.f;
	mJoyAxis[1] = js.lY/1000.f;
	mJoyAxis[2] = js.lZ/1000.f;
	mJoyAxis[3] = js.lRx/1000.f;
	mJoyAxis[4] = js.lRy/1000.f;
	mJoyAxis[5] = js.lRz/1000.f;

	for (U32 i = 0; i < 16; i++)
	{
		mJoyButtonState[i] = js.rgbButtons[i];
	}
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


void LLSplashScreenWin32::updateImpl(const char *mesg)
{
	if (!mWindow) return;

	WCHAR w_mesg[1024];
	mbstowcs(w_mesg, mesg, 1024);

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

S32 OSMessageBoxWin32(const char* text, const char* caption, U32 type)
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
	int retval_win = MessageBoxA(NULL, text, caption, uType);
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


void spawn_web_browser(const char* escaped_url )
{
	bool found = false;
	S32 i;
	for (i = 0; i < gURLProtocolWhitelistCount; i++)
	{
		S32 len = strlen(gURLProtocolWhitelist[i]);	/* Flawfinder: ignore */
		if (!strncmp(escaped_url, gURLProtocolWhitelist[i], len)
			&& escaped_url[len] == ':')
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		llwarns << "spawn_web_browser() called for url with protocol not on whitelist: " << escaped_url << llendl;
		return;
	}

	llinfos << "Opening URL " << escaped_url << llendl;

	// Figure out the user's default web browser
	// HKEY_CLASSES_ROOT\http\shell\open\command
	char reg_path_str[256];	/* Flawfinder: ignore */
	snprintf(reg_path_str, sizeof(reg_path_str), "%s\\shell\\open\\command", gURLProtocolWhitelistHandler[i]);	/* Flawfinder: ignore */
	WCHAR reg_path_wstr[256];
	mbstowcs(reg_path_wstr, reg_path_str, 1024);

	HKEY key;
	WCHAR browser_open_wstr[1024];
	DWORD buffer_length = 1024;
	RegOpenKeyEx(HKEY_CLASSES_ROOT, reg_path_wstr, 0, KEY_QUERY_VALUE, &key);
	RegQueryValueEx(key, NULL, NULL, NULL, (LPBYTE)browser_open_wstr, &buffer_length);
	RegCloseKey(key);

	// Convert to STL string
	LLWString browser_open_wstring = utf16str_to_wstring(browser_open_wstr);

	if (browser_open_wstring.length() < 2)
	{
		llwarns << "Invalid browser executable in registry " << browser_open_wstring << llendl;
		return;
	}

	// Extract the process that's supposed to be launched
	LLWString browser_executable;
	if (browser_open_wstring[0] == '"')
	{
		// executable is quoted, find the matching quote
		size_t quote_pos = browser_open_wstring.find('"', 1);
		// copy out the string including both quotes
		browser_executable = browser_open_wstring.substr(0, quote_pos+1);
	}
	else
	{
		// executable not quoted, find a space
		size_t space_pos = browser_open_wstring.find(' ', 1);
		browser_executable = browser_open_wstring.substr(0, space_pos);
	}

	llinfos << "Browser reg key: " << wstring_to_utf8str(browser_open_wstring) << llendl;
	llinfos << "Browser executable: " << wstring_to_utf8str(browser_executable) << llendl;

	// Convert URL to wide string for Windows API
	// Assume URL is UTF8, as can come from scripts
	LLWString url_wstring = utf8str_to_wstring(escaped_url);
	llutf16string url_utf16 = wstring_to_utf16str(url_wstring);

	// Convert executable and path to wide string for Windows API
	llutf16string browser_exec_utf16 = wstring_to_utf16str(browser_executable);

	// ShellExecute returns HINSTANCE for backwards compatiblity.
	// MS docs say to cast to int and compare to 32.
	HWND our_window = NULL;
	LPCWSTR directory_wstr = NULL;
	int retval = (int) ShellExecute(our_window, 	/* Flawfinder: ignore */
									L"open", 
									browser_exec_utf16.c_str(), 
									url_utf16.c_str(), 
									directory_wstr,
									SW_SHOWNORMAL);
	if (retval > 32)
	{
		llinfos << "load_url success with " << retval << llendl;
	}
	else
	{
		llinfos << "load_url failure with " << retval << llendl;
	}
}

void shell_open( const char* file_path )
{
	llinfos << "Opening " << file_path << llendl;

	WCHAR wstr[1024];
	mbstowcs(wstr, file_path, 1024);

	HWND our_window = NULL;
	int retval = (int) ShellExecute(our_window, L"open", wstr, NULL, NULL, SW_SHOWNORMAL);	/* Flawfinder: ignore */
	if (retval > 32)
	{
		llinfos << "ShellExecute success with " << retval << llendl;
	}
	else
	{
		llinfos << "ShellExecute failure with " << retval << llendl;
	}
}

BOOL LLWindowWin32::dialog_color_picker ( F32 *r, F32 *g, F32 *b )
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
};

#endif // LL_WINDOWS
