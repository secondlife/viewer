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

				*pdwEffect = DROPEFFECT_COPY;

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
				*pdwEffect = DROPEFFECT_COPY;
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
			return S_OK;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		HRESULT __stdcall Drop( IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
		{
			if ( mAllowDrop )
			{
				// construct a FORMATETC object
				FORMATETC fmtetc = { CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

				// do we have text?
				if( S_OK == pDataObject->QueryGetData( &fmtetc ) )
				{
					STGMEDIUM stgmed;
					if( S_OK == pDataObject->GetData( &fmtetc, &stgmed ) )
					{
						// note: data is in an HGLOBAL - not 'regular' memory
						PVOID data = GlobalLock( stgmed.hGlobal );

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
							llinfos << "### (Drop) URL is: " << data << llendl;
							llinfos << "###        raw coords are: " << pt.x << " x " << pt.y << llendl;
							llinfos << "###	    client coords are: " << pt_client.x << " x " << pt_client.y << llendl;
							llinfos << "###         GL coords are: " << gl_coord.mX << " x " << gl_coord.mY << llendl;
							llinfos << llendl;

							// no keyboard modifier option yet but we could one day
							MASK mask = gKeyboard->currentMask( TRUE );

							// actually do the drop
							window_imp->completeDropRequest( gl_coord, mask, std::string( (char*)data ) );
						};

						GlobalUnlock( stgmed.hGlobal );

						ReleaseStgMedium( &stgmed );
					};
				};

				*pdwEffect = DROPEFFECT_COPY;
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

#endif
