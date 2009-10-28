/**
 * @file lldragdrop32.cpp
 * @brief Handler for Windows specific drag and drop (OS to client) code
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#if LL_WINDOWS

#include "linden_common.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "llwindowwin32.h"
#include "llkeyboardwin32.h"
#include "lldragdropwin32.h"

#include "llwindowcallbacks.h"

#include <windows.h>
#include <ole2.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>

// FIXME: this should be done in CMake
#pragma comment( lib, "shlwapi.lib" )

class LLDragDropWin32Target: 
	public IDropTarget
{
	public:
		LLDragDropWin32Target( HWND  hWnd ) :
		  mWindowHandle( hWnd ),
		  mRefCount( 0 )
		{
			strcpy(szFileDropped,"");
			bDropTargetValid = false;
			bTextDropped = false;		
		};

		/* IUnknown methods */
		STDMETHOD_( ULONG, AddRef )( void )
		{
			return ++mRefCount;
		};

		STDMETHOD_( ULONG, Release )( void )
		{
			if ( --mRefCount == 0 )
			{
				delete this;
				return 0;
			}
			return mRefCount;
		};

		STDMETHOD ( QueryInterface )( REFIID iid, void ** ppvObject )
		{
			if ( iid == IID_IUnknown || iid == IID_IDropTarget )
			{
				*ppvObject = this;
				AddRef();
				return S_OK;
			};

			*ppvObject = NULL;
			return E_NOINTERFACE;
		};
		
		/* IDropTarget methods */
		STDMETHOD (DragEnter)(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
		{
			HRESULT hr = E_INVALIDARG;
			bDropTargetValid = false;
			bTextDropped = false;
			*pdwEffect=DROPEFFECT_NONE;
			
			FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
			STGMEDIUM medium;
				
			if (pDataObj && SUCCEEDED (pDataObj->GetData (&fmte, &medium)))
			{
				// We can Handle Only one File At a time !!!
				if (1 == DragQueryFile ((HDROP)medium.hGlobal,0xFFFFFFFF,NULL,0 ))
				{
					// Get the File Name
					if (DragQueryFileA((HDROP)medium.hGlobal, 0, szFileDropped,MAX_PATH))
					{
						if (!PathIsDirectoryA(szFileDropped))
						{
							char szTempFile[MAX_PATH];
							_splitpath(szFileDropped,NULL,NULL,NULL,szTempFile);

//							if (!stricmp(szTempFile,".lnk"))
//							{
//								if (ResolveLink(szFileDropped,szTempFile))
//								{
//									strcpy(szFileDropped,szTempFile);
//									*pdwEffect=DROPEFFECT_COPY;
//									We Want to Create a Copy
//									bDropTargetValid = true;
//									hr = S_OK;
//								}
//							}
//							else
//							{
								*pdwEffect=DROPEFFECT_COPY;
								//We Want to Create a Copy
								bDropTargetValid = true;
								hr = S_OK;
//							}
						}
					}
				}

				if (medium.pUnkForRelease)
					medium.pUnkForRelease->Release ();
				else
					GlobalFree (medium.hGlobal);
			}
			else 
			{
				fmte.cfFormat = CF_TEXT;
				fmte.ptd = NULL;
				fmte.dwAspect = DVASPECT_CONTENT;
				fmte.lindex = -1;
				fmte.tymed = TYMED_HGLOBAL; 

				// Does the drag source provide CF_TEXT ?    
				if (NOERROR == pDataObj->QueryGetData(&fmte))
				{
					bDropTargetValid = true;
					bTextDropped = true;
					*pdwEffect=DROPEFFECT_COPY;
					hr = S_OK;
				}
			}
				return hr;



		};

		STDMETHOD (DragOver)(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
		{
			HRESULT hr = S_OK;
			if (bDropTargetValid) 
				*pdwEffect=DROPEFFECT_COPY;

			return hr;
		};

		STDMETHOD (DragLeave)(void)
		{
			HRESULT hr = S_OK;
			strcpy(szFileDropped,"");
			return hr;
		};

		STDMETHOD (Drop)(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
		{
			HRESULT hr = S_OK;
			if (bDropTargetValid) 
			{
				*pdwEffect=DROPEFFECT_COPY;
			
				FORMATETC fmte;
				STGMEDIUM medium;

				if (bTextDropped)
				{
					fmte.cfFormat = CF_TEXT;
					fmte.ptd = NULL;
					fmte.dwAspect = DVASPECT_CONTENT;  
					fmte.lindex = -1;
					fmte.tymed = TYMED_HGLOBAL;       

					hr = pDataObj->GetData(&fmte, &medium);
					HGLOBAL hText = medium.hGlobal;
					LPSTR lpszText = (LPSTR)GlobalLock(hText);

					LLWindowWin32 *window_imp = (LLWindowWin32 *)GetWindowLong(mWindowHandle, GWL_USERDATA);
					if (NULL != window_imp)
					{
						LLCoordGL gl_coord( pt.x, pt.y);
						LLCoordWindow cursor_coord_window( pt.x, pt.y );
						window_imp->convertCoords(cursor_coord_window, &gl_coord);
						llinfos << "### (Drop) URL is: " << lpszText << llendl;
						llinfos << "###        raw coords are: " << pt.x << " x " << pt.y << llendl;
						llinfos << "###         GL coords are: " << gl_coord.mX << " x " << gl_coord.mY << llendl;
						llinfos << llendl;

						MASK mask = gKeyboard->currentMask(TRUE);
						window_imp->completeDropRequest( gl_coord, mask, std::string( lpszText ) );
					};

					GlobalUnlock(hText);
					ReleaseStgMedium(&medium);
				}
			}
			return hr;
		};
	   
	private:
		ULONG mRefCount;
		HWND mWindowHandle;
		char szFileDropped[1024];
		bool bDropTargetValid;
		bool bTextDropped;
		friend class LLWindowWin32;
};

LLDragDropWin32::LLDragDropWin32() :
	mDropTarget( NULL ),
	mDropWindowHandle( NULL )

{
}

LLDragDropWin32::~LLDragDropWin32()
{
}

bool LLDragDropWin32::init( HWND hWnd )
{
	if (NOERROR != OleInitialize(NULL))
		return FALSE; 

	mDropTarget = new LLDragDropWin32Target( hWnd );
	if ( mDropTarget )
	{
		HRESULT result = CoLockObjectExternal( mDropTarget, TRUE, TRUE );
		if ( S_OK == result )
		{
			result = RegisterDragDrop( hWnd, mDropTarget );
			if ( S_OK != result )
			{
				// RegisterDragDrop failed
				return false;
			};

			// all ok
			mDropWindowHandle = hWnd;
		}
		else
		{
			// Unable to lock OLE object
			return false;
		};
	};

	// success
	return true;
}

void LLDragDropWin32::reset()
{
	if ( mDropTarget )
	{
		CoLockObjectExternal( mDropTarget, FALSE, TRUE );
		RevokeDragDrop( mDropWindowHandle );
		mDropTarget->Release();  
	};
	
	OleUninitialize();
}

#endif
