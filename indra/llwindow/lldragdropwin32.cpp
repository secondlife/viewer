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

#if LL_OS_DRAGDROP_ENABLED

#include "linden_common.h"

#include "llwindowwin32.h"
#include "llkeyboardwin32.h"
#include "llwindowcallbacks.h"
#include "lldragdropwin32.h"

class LLDragDropWin32Target: 
	public IDropTarget
{
	public:
		////////////////////////////////////////////////////////////////////////////////
		//
		LLDragDropWin32Target( HWND  hWnd ) :
			mRefCount( 1 ),
			mAppWindowHandle( hWnd ),
			mAllowDrop( false)
		{
		};

		virtual ~LLDragDropWin32Target()
		{
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		ULONG __stdcall AddRef( void )
		{
			return InterlockedIncrement( &mRefCount );
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		ULONG __stdcall Release( void )
		{
			LONG count = InterlockedDecrement( &mRefCount );
				
			if ( count == 0 )
			{
				delete this;
				return 0;
			}
			else
			{
				return count;
			};
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		HRESULT __stdcall QueryInterface( REFIID iid, void** ppvObject )
		{
			if ( iid == IID_IUnknown || iid == IID_IDropTarget )
			{
				AddRef();
				*ppvObject = this;
				return S_OK;
			}
			else
			{
				*ppvObject = 0;
				return E_NOINTERFACE;
			};
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		HRESULT __stdcall DragEnter( IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
		{
			FORMATETC fmtetc = { CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

			// support CF_TEXT using a HGLOBAL?
			if ( S_OK == pDataObject->QueryGetData( &fmtetc ) )
			{
				mAllowDrop = true;
				mDropUrl = std::string();
				mIsSlurl = false;

				STGMEDIUM stgmed;
				if( S_OK == pDataObject->GetData( &fmtetc, &stgmed ) )
				{
					PVOID data = GlobalLock( stgmed.hGlobal );
					mDropUrl = std::string( (char*)data );
					// XXX MAJOR MAJOR HACK!
					LLWindowWin32 *window_imp = (LLWindowWin32 *)GetWindowLong(mAppWindowHandle, GWL_USERDATA);
					if (NULL != window_imp)
					{
						LLCoordGL gl_coord( 0, 0 );

						POINT pt2;
						pt2.x = pt.x;
						pt2.y = pt.y;
						ScreenToClient( mAppWindowHandle, &pt2 );

						LLCoordWindow cursor_coord_window( pt2.x, pt2.y );
						window_imp->convertCoords(cursor_coord_window, &gl_coord);
						MASK mask = gKeyboard->currentMask(TRUE);

						LLWindowCallbacks::DragNDropResult result = window_imp->completeDragNDropRequest( gl_coord, mask, 
							LLWindowCallbacks::DNDA_START_TRACKING, mDropUrl );

						switch (result)
						{
						case LLWindowCallbacks::DND_COPY:
							*pdwEffect = DROPEFFECT_COPY;
							break;
						case LLWindowCallbacks::DND_LINK:
							*pdwEffect = DROPEFFECT_LINK;
							break;
						case LLWindowCallbacks::DND_MOVE:
							*pdwEffect = DROPEFFECT_MOVE;
							break;
						case LLWindowCallbacks::DND_NONE:
						default:
							*pdwEffect = DROPEFFECT_NONE;
							break;
						}
					};

					GlobalUnlock( stgmed.hGlobal );
					ReleaseStgMedium( &stgmed );
				};
				SetFocus( mAppWindowHandle );
			}
			else
			{
				mAllowDrop = false;
				*pdwEffect = DROPEFFECT_NONE;
			};

			return S_OK;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		HRESULT __stdcall DragOver( DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
		{
			if ( mAllowDrop )
			{
				// XXX MAJOR MAJOR HACK!
				LLWindowWin32 *window_imp = (LLWindowWin32 *)GetWindowLong(mAppWindowHandle, GWL_USERDATA);
				if (NULL != window_imp)
				{
					LLCoordGL gl_coord( 0, 0 );

					POINT pt2;
					pt2.x = pt.x;
					pt2.y = pt.y;
					ScreenToClient( mAppWindowHandle, &pt2 );

					LLCoordWindow cursor_coord_window( pt2.x, pt2.y );
					window_imp->convertCoords(cursor_coord_window, &gl_coord);
					MASK mask = gKeyboard->currentMask(TRUE);

					LLWindowCallbacks::DragNDropResult result = window_imp->completeDragNDropRequest( gl_coord, mask, 
						LLWindowCallbacks::DNDA_TRACK, std::string( "" ) );
					
					switch (result)
					{
					case LLWindowCallbacks::DND_COPY:
						*pdwEffect = DROPEFFECT_COPY;
						break;
					case LLWindowCallbacks::DND_LINK:
						*pdwEffect = DROPEFFECT_LINK;
						break;
					case LLWindowCallbacks::DND_MOVE:
						*pdwEffect = DROPEFFECT_MOVE;
						break;
					case LLWindowCallbacks::DND_NONE:
					default:
						*pdwEffect = DROPEFFECT_NONE;
						break;
					}
				};
			}
			else
			{
				*pdwEffect = DROPEFFECT_NONE;
			};

			return S_OK;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		HRESULT __stdcall DragLeave( void )
		{
			// XXX MAJOR MAJOR HACK!
			LLWindowWin32 *window_imp = (LLWindowWin32 *)GetWindowLong(mAppWindowHandle, GWL_USERDATA);
			if (NULL != window_imp)
			{
				LLCoordGL gl_coord( 0, 0 );
				MASK mask = gKeyboard->currentMask(TRUE);
				window_imp->completeDragNDropRequest( gl_coord, mask, LLWindowCallbacks::DNDA_STOP_TRACKING, mDropUrl );
			};
			return S_OK;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		HRESULT __stdcall Drop( IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
		{
			if ( mAllowDrop )
			{
				// window impl stored in Window data (neat!)
				LLWindowWin32 *window_imp = (LLWindowWin32 *)GetWindowLong( mAppWindowHandle, GWL_USERDATA );
				if ( NULL != window_imp )
				{
					LLCoordGL gl_coord( 0, 0 );

					POINT pt_client;
					pt_client.x = pt.x;
					pt_client.y = pt.y;
					ScreenToClient( mAppWindowHandle, &pt_client );

					LLCoordWindow cursor_coord_window( pt_client.x, pt_client.y );
					window_imp->convertCoords(cursor_coord_window, &gl_coord);
					llinfos << "### (Drop) URL is: " << mDropUrl << llendl;
					llinfos << "###        raw coords are: " << pt.x << " x " << pt.y << llendl;
					llinfos << "###	    client coords are: " << pt_client.x << " x " << pt_client.y << llendl;
					llinfos << "###         GL coords are: " << gl_coord.mX << " x " << gl_coord.mY << llendl;
					llinfos << llendl;

					// no keyboard modifier option yet but we could one day
					MASK mask = gKeyboard->currentMask( TRUE );

					// actually do the drop
					LLWindowCallbacks::DragNDropResult result = window_imp->completeDragNDropRequest( gl_coord, mask, 
						LLWindowCallbacks::DNDA_DROPPED, mDropUrl );

					switch (result)
					{
					case LLWindowCallbacks::DND_COPY:
						*pdwEffect = DROPEFFECT_COPY;
						break;
					case LLWindowCallbacks::DND_LINK:
						*pdwEffect = DROPEFFECT_LINK;
						break;
					case LLWindowCallbacks::DND_MOVE:
						*pdwEffect = DROPEFFECT_MOVE;
						break;
					case LLWindowCallbacks::DND_NONE:
					default:
						*pdwEffect = DROPEFFECT_NONE;
						break;
					}
				};
			}
			else
			{
				*pdwEffect = DROPEFFECT_NONE;
			};

			return S_OK;
		};

	////////////////////////////////////////////////////////////////////////////////
	//
	private:
		LONG mRefCount;
		HWND mAppWindowHandle;
		bool mAllowDrop;
		std::string mDropUrl;
		bool mIsSlurl;
		friend class LLWindowWin32;
};

////////////////////////////////////////////////////////////////////////////////
//
LLDragDropWin32::LLDragDropWin32() :
	mDropTarget( NULL ),
	mDropWindowHandle( NULL )

{
}

////////////////////////////////////////////////////////////////////////////////
//
LLDragDropWin32::~LLDragDropWin32()
{
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLDragDropWin32::init( HWND hWnd )
{
	if ( NOERROR != OleInitialize( NULL ) )
		return FALSE; 

	mDropTarget = new LLDragDropWin32Target( hWnd );
	if ( mDropTarget )
	{
		HRESULT result = CoLockObjectExternal( mDropTarget, TRUE, FALSE );
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

////////////////////////////////////////////////////////////////////////////////
//
void LLDragDropWin32::reset()
{
	if ( mDropTarget )
	{
		RevokeDragDrop( mDropWindowHandle );
		CoLockObjectExternal( mDropTarget, FALSE, TRUE );
		mDropTarget->Release();  
	};
	
	OleUninitialize();
}

#endif // LL_OS_DRAGDROP_ENABLED

#endif // LL_WINDOWS

