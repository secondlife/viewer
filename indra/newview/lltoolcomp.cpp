/** 
 * @file lltoolcomp.cpp
 * @brief Composite tools
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

#include "lltoolcomp.h"

#include "llfloaterreg.h"
#include "llgl.h"
#include "indra_constants.h"

#include "llmanip.h"
#include "llmaniprotate.h"
#include "llmanipscale.h"
#include "llmaniptranslate.h"
#include "llmenugl.h"			// for right-click menu hack
#include "llselectmgr.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolgun.h"
#include "lltoolmgr.h"
#include "lltoolselectrect.h"
#include "lltoolplacer.h"
#include "llviewermenu.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llfloatertools.h"
#include "llviewercontrol.h"

const S32 BUTTON_HEIGHT = 16;
const S32 BUTTON_WIDTH_SMALL = 32;
const S32 BUTTON_WIDTH_BIG = 48;
const S32 HPAD = 4;

extern LLControlGroup gSavedSettings;


// we use this in various places instead of NULL
static LLTool* sNullTool = new LLTool(std::string("null"), NULL); 

//-----------------------------------------------------------------------
// LLToolComposite

//static
void LLToolComposite::setCurrentTool( LLTool* new_tool )
{
	if( mCur != new_tool )
	{
		if( mSelected )
		{
			mCur->handleDeselect();
			mCur = new_tool;
			mCur->handleSelect();
		}
		else
		{
			mCur = new_tool;
		}
	}
}

LLToolComposite::LLToolComposite(const std::string& name)
	: LLTool(name),
	  mCur(sNullTool), 
	  mDefault(sNullTool), 
	  mSelected(FALSE),
	  mMouseDown(FALSE), mManip(NULL), mSelectRect(NULL)
{
}

// Returns to the default tool
BOOL LLToolComposite::handleMouseUp(S32 x, S32 y, MASK mask)
{ 
	BOOL handled = mCur->handleMouseUp( x, y, mask );
	if( handled )
	{
		setCurrentTool( mDefault );
	}
 return handled;
}

void LLToolComposite::onMouseCaptureLost()
{
	mCur->onMouseCaptureLost();
	setCurrentTool( mDefault );
}

BOOL LLToolComposite::isSelecting()
{ 
	return mCur == mSelectRect; 
}

void LLToolComposite::handleSelect()
{
	if (!gSavedSettings.getBOOL("EditLinkedParts"))
	{
		LLSelectMgr::getInstance()->promoteSelectionToRoot();
	}
	mCur = mDefault; 
	mCur->handleSelect(); 
	mSelected = TRUE; 
}

//----------------------------------------------------------------------------
// LLToolCompInspect
//----------------------------------------------------------------------------

LLToolCompInspect::LLToolCompInspect()
: LLToolComposite(std::string("Inspect"))
{
	mSelectRect		= new LLToolSelectRect(this);
	mDefault = mSelectRect;
}


LLToolCompInspect::~LLToolCompInspect()
{
	delete mSelectRect;
	mSelectRect = NULL;
}

BOOL LLToolCompInspect::handleMouseDown(S32 x, S32 y, MASK mask)
{
	mMouseDown = TRUE;
	gViewerWindow->pickAsync(x, y, mask, pickCallback);
	return TRUE;
}

void LLToolCompInspect::pickCallback(const LLPickInfo& pick_info)
{
	LLViewerObject* hit_obj = pick_info.getObject();

	if (!LLToolCompInspect::getInstance()->mMouseDown)
	{
		// fast click on object, but mouse is already up...just do select
		LLToolCompInspect::getInstance()->mSelectRect->handleObjectSelection(pick_info, gSavedSettings.getBOOL("EditLinkedParts"), FALSE);
		return;
	}

	if( hit_obj )
	{
		if (LLSelectMgr::getInstance()->getSelection()->getObjectCount())
		{
			LLEditMenuHandler::gEditMenuHandler = LLSelectMgr::getInstance();
		}
		LLToolCompInspect::getInstance()->setCurrentTool( LLToolCompInspect::getInstance()->mSelectRect );
		LLToolCompInspect::getInstance()->mSelectRect->handlePick( pick_info );

	}
	else
	{
		LLToolCompInspect::getInstance()->setCurrentTool( LLToolCompInspect::getInstance()->mSelectRect );
		LLToolCompInspect::getInstance()->mSelectRect->handlePick( pick_info );
	}
}

BOOL LLToolCompInspect::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	return TRUE;
}

//----------------------------------------------------------------------------
// LLToolCompTranslate
//----------------------------------------------------------------------------

LLToolCompTranslate::LLToolCompTranslate()
	: LLToolComposite(std::string("Move"))
{
	mManip		= new LLManipTranslate(this);
	mSelectRect		= new LLToolSelectRect(this);

	mCur			= mManip;
	mDefault		= mManip;
}

LLToolCompTranslate::~LLToolCompTranslate()
{
	delete mManip;
	mManip = NULL;

	delete mSelectRect;
	mSelectRect = NULL;
}

BOOL LLToolCompTranslate::handleHover(S32 x, S32 y, MASK mask)
{
	if( !mCur->hasMouseCapture() )
	{
		setCurrentTool( mManip );
	}
	return mCur->handleHover( x, y, mask );
}


BOOL LLToolCompTranslate::handleMouseDown(S32 x, S32 y, MASK mask)
{
	mMouseDown = TRUE;
	gViewerWindow->pickAsync(x, y, mask, pickCallback, TRUE);
	return TRUE;
}

void LLToolCompTranslate::pickCallback(const LLPickInfo& pick_info)
{
	LLViewerObject* hit_obj = pick_info.getObject();

	LLToolCompTranslate::getInstance()->mManip->highlightManipulators(pick_info.mMousePt.mX, pick_info.mMousePt.mY);
	if (!LLToolCompTranslate::getInstance()->mMouseDown)
	{
		// fast click on object, but mouse is already up...just do select
		LLToolCompTranslate::getInstance()->mSelectRect->handleObjectSelection(pick_info, gSavedSettings.getBOOL("EditLinkedParts"), FALSE);
		return;
	}

	if( hit_obj || LLToolCompTranslate::getInstance()->mManip->getHighlightedPart() != LLManip::LL_NO_PART )
	{
		if (LLToolCompTranslate::getInstance()->mManip->getSelection()->getObjectCount())
		{
			LLEditMenuHandler::gEditMenuHandler = LLSelectMgr::getInstance();
		}

		BOOL can_move = LLToolCompTranslate::getInstance()->mManip->canAffectSelection();

		if(	LLManip::LL_NO_PART != LLToolCompTranslate::getInstance()->mManip->getHighlightedPart() && can_move)
		{
			LLToolCompTranslate::getInstance()->setCurrentTool( LLToolCompTranslate::getInstance()->mManip );
			LLToolCompTranslate::getInstance()->mManip->handleMouseDownOnPart( pick_info.mMousePt.mX, pick_info.mMousePt.mY, pick_info.mKeyMask );
		}
		else
		{
			LLToolCompTranslate::getInstance()->setCurrentTool( LLToolCompTranslate::getInstance()->mSelectRect );
			LLToolCompTranslate::getInstance()->mSelectRect->handlePick( pick_info );

			// *TODO: add toggle to trigger old click-drag functionality
			// LLToolCompTranslate::getInstance()->mManip->handleMouseDownOnPart( XY_part, x, y, mask);
		}
	}
	else
	{
		LLToolCompTranslate::getInstance()->setCurrentTool( LLToolCompTranslate::getInstance()->mSelectRect );
		LLToolCompTranslate::getInstance()->mSelectRect->handlePick( pick_info );
	}
}

BOOL LLToolCompTranslate::handleMouseUp(S32 x, S32 y, MASK mask)
{
	mMouseDown = FALSE;
	return LLToolComposite::handleMouseUp(x, y, mask);
}

LLTool* LLToolCompTranslate::getOverrideTool(MASK mask)
{
	if (mask == MASK_CONTROL)
	{
		return LLToolCompRotate::getInstance();
	}
	else if (mask == (MASK_CONTROL | MASK_SHIFT))
	{
		return LLToolCompScale::getInstance();
	}
	return LLToolComposite::getOverrideTool(mask);
}

BOOL LLToolCompTranslate::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (mManip->getSelection()->isEmpty() && mManip->getHighlightedPart() == LLManip::LL_NO_PART)
	{
		// You should already have an object selected from the mousedown.
		// If so, show its properties
		LLFloaterReg::showInstance("build", "Content");
		return TRUE;
	}
	// Nothing selected means the first mouse click was probably
	// bad, so try again.
	return FALSE;
}


void LLToolCompTranslate::render()
{
	mCur->render(); // removing this will not draw the RGB arrows and guidelines

	if( mCur != mManip )
	{
		LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
		mManip->renderGuidelines();
	}
}


//-----------------------------------------------------------------------
// LLToolCompScale

LLToolCompScale::LLToolCompScale()
	: LLToolComposite(std::string("Stretch"))
{
	mManip = new LLManipScale(this);
	mSelectRect = new LLToolSelectRect(this);

	mCur = mManip;
	mDefault = mManip;
}

LLToolCompScale::~LLToolCompScale()
{
	delete mManip;
	delete mSelectRect;
}

BOOL LLToolCompScale::handleHover(S32 x, S32 y, MASK mask)
{
	if( !mCur->hasMouseCapture() )
	{
		setCurrentTool(mManip );
	}
	return mCur->handleHover( x, y, mask );
}


BOOL LLToolCompScale::handleMouseDown(S32 x, S32 y, MASK mask)
{
	mMouseDown = TRUE;
	gViewerWindow->pickAsync(x, y, mask, pickCallback);
	return TRUE;
}

void LLToolCompScale::pickCallback(const LLPickInfo& pick_info)
{
	LLViewerObject* hit_obj = pick_info.getObject();

	LLToolCompScale::getInstance()->mManip->highlightManipulators(pick_info.mMousePt.mX, pick_info.mMousePt.mY);
	if (!LLToolCompScale::getInstance()->mMouseDown)
	{
		// fast click on object, but mouse is already up...just do select
		LLToolCompScale::getInstance()->mSelectRect->handleObjectSelection(pick_info, gSavedSettings.getBOOL("EditLinkedParts"), FALSE);

		return;
	}

	if( hit_obj || LLToolCompScale::getInstance()->mManip->getHighlightedPart() != LLManip::LL_NO_PART)
	{
		if (LLToolCompScale::getInstance()->mManip->getSelection()->getObjectCount())
		{
			LLEditMenuHandler::gEditMenuHandler = LLSelectMgr::getInstance();
		}
		if(	LLManip::LL_NO_PART != LLToolCompScale::getInstance()->mManip->getHighlightedPart() )
		{
			LLToolCompScale::getInstance()->setCurrentTool( LLToolCompScale::getInstance()->mManip );
			LLToolCompScale::getInstance()->mManip->handleMouseDownOnPart( pick_info.mMousePt.mX, pick_info.mMousePt.mY, pick_info.mKeyMask );
		}
		else
		{
			LLToolCompScale::getInstance()->setCurrentTool( LLToolCompScale::getInstance()->mSelectRect );
			LLToolCompScale::getInstance()->mSelectRect->handlePick( pick_info );
		}
	}
	else
	{
		LLToolCompScale::getInstance()->setCurrentTool( LLToolCompScale::getInstance()->mSelectRect );
		LLToolCompScale::getInstance()->mSelectRect->handlePick( pick_info );
	}
}

BOOL LLToolCompScale::handleMouseUp(S32 x, S32 y, MASK mask)
{
	mMouseDown = FALSE;
	return LLToolComposite::handleMouseUp(x, y, mask);
}

LLTool* LLToolCompScale::getOverrideTool(MASK mask)
{
	if (mask == MASK_CONTROL)
	{
		return LLToolCompRotate::getInstance();
	}

	return LLToolComposite::getOverrideTool(mask);
}


BOOL LLToolCompScale::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (!mManip->getSelection()->isEmpty() && mManip->getHighlightedPart() == LLManip::LL_NO_PART)
	{
		// You should already have an object selected from the mousedown.
		// If so, show its properties
		LLFloaterReg::showInstance("build", "Content");
		return TRUE;
	}
	else
	{
		// Nothing selected means the first mouse click was probably
		// bad, so try again.
		return handleMouseDown(x, y, mask);
	}
}


void LLToolCompScale::render()
{
	mCur->render();

	if( mCur != mManip )
	{
		LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
		mManip->renderGuidelines();
	}
}

//-----------------------------------------------------------------------
// LLToolCompCreate

LLToolCompCreate::LLToolCompCreate()
	: LLToolComposite(std::string("Create"))
{
	mPlacer = new LLToolPlacer();
	mSelectRect = new LLToolSelectRect(this);

	mCur = mPlacer;
	mDefault = mPlacer;
	mObjectPlacedOnMouseDown = FALSE;
}


LLToolCompCreate::~LLToolCompCreate()
{
	delete mPlacer;
	delete mSelectRect;
}


BOOL LLToolCompCreate::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	mMouseDown = TRUE;

	if ( (mask == MASK_SHIFT) || (mask == MASK_CONTROL) )
	{
		gViewerWindow->pickAsync(x, y, mask, pickCallback);
		handled = TRUE;
	}
	else
	{
		setCurrentTool( mPlacer );
		handled = mPlacer->placeObject( x, y, mask );
	}
	
	mObjectPlacedOnMouseDown = TRUE;

	return handled;
}

void LLToolCompCreate::pickCallback(const LLPickInfo& pick_info)
{
	// *NOTE: We mask off shift and control, so you cannot
	// multi-select multiple objects with the create tool.
	MASK mask = (pick_info.mKeyMask & ~MASK_SHIFT);
	mask = (mask & ~MASK_CONTROL);

	LLToolCompCreate::getInstance()->setCurrentTool( LLToolCompCreate::getInstance()->mSelectRect );
	LLToolCompCreate::getInstance()->mSelectRect->handlePick( pick_info );
}

BOOL LLToolCompCreate::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	return handleMouseDown(x, y, mask);
}

BOOL LLToolCompCreate::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;

	if ( mMouseDown && !mObjectPlacedOnMouseDown && !(mask == MASK_SHIFT) && !(mask == MASK_CONTROL) )
	{
		setCurrentTool( mPlacer );
		handled = mPlacer->placeObject( x, y, mask );
	}

	mObjectPlacedOnMouseDown = FALSE;
	mMouseDown = FALSE;

	if (!handled)
	{
		handled = LLToolComposite::handleMouseUp(x, y, mask);
	}

	return handled;
}

//-----------------------------------------------------------------------
// LLToolCompRotate

LLToolCompRotate::LLToolCompRotate()
	: LLToolComposite(std::string("Rotate"))
{
	mManip = new LLManipRotate(this);
	mSelectRect = new LLToolSelectRect(this);

	mCur = mManip;
	mDefault = mManip;
}


LLToolCompRotate::~LLToolCompRotate()
{
	delete mManip;
	delete mSelectRect;
}

BOOL LLToolCompRotate::handleHover(S32 x, S32 y, MASK mask)
{
	if( !mCur->hasMouseCapture() )
	{
		setCurrentTool( mManip );
	}
	return mCur->handleHover( x, y, mask );
}


BOOL LLToolCompRotate::handleMouseDown(S32 x, S32 y, MASK mask)
{
	mMouseDown = TRUE;
	gViewerWindow->pickAsync(x, y, mask, pickCallback);
	return TRUE;
}

void LLToolCompRotate::pickCallback(const LLPickInfo& pick_info)
{
	LLViewerObject* hit_obj = pick_info.getObject();

	LLToolCompRotate::getInstance()->mManip->highlightManipulators(pick_info.mMousePt.mX, pick_info.mMousePt.mY);
	if (!LLToolCompRotate::getInstance()->mMouseDown)
	{
		// fast click on object, but mouse is already up...just do select
		LLToolCompRotate::getInstance()->mSelectRect->handleObjectSelection(pick_info, gSavedSettings.getBOOL("EditLinkedParts"), FALSE);
		return;
	}
	
	if( hit_obj || LLToolCompRotate::getInstance()->mManip->getHighlightedPart() != LLManip::LL_NO_PART)
	{
		if (LLToolCompRotate::getInstance()->mManip->getSelection()->getObjectCount())
		{
			LLEditMenuHandler::gEditMenuHandler = LLSelectMgr::getInstance();
		}
		if(	LLManip::LL_NO_PART != LLToolCompRotate::getInstance()->mManip->getHighlightedPart() )
		{
			LLToolCompRotate::getInstance()->setCurrentTool( LLToolCompRotate::getInstance()->mManip );
			LLToolCompRotate::getInstance()->mManip->handleMouseDownOnPart( pick_info.mMousePt.mX, pick_info.mMousePt.mY, pick_info.mKeyMask );
		}
		else
		{
			LLToolCompRotate::getInstance()->setCurrentTool( LLToolCompRotate::getInstance()->mSelectRect );
			LLToolCompRotate::getInstance()->mSelectRect->handlePick( pick_info );
		}
	}
	else
	{
		LLToolCompRotate::getInstance()->setCurrentTool( LLToolCompRotate::getInstance()->mSelectRect );
		LLToolCompRotate::getInstance()->mSelectRect->handlePick( pick_info );
	}
}

BOOL LLToolCompRotate::handleMouseUp(S32 x, S32 y, MASK mask)
{
	mMouseDown = FALSE;
	return LLToolComposite::handleMouseUp(x, y, mask);
}

LLTool* LLToolCompRotate::getOverrideTool(MASK mask)
{
	if (mask == (MASK_CONTROL | MASK_SHIFT))
	{
		return LLToolCompScale::getInstance();
	}
	return LLToolComposite::getOverrideTool(mask);
}

BOOL LLToolCompRotate::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (!mManip->getSelection()->isEmpty() && mManip->getHighlightedPart() == LLManip::LL_NO_PART)
	{
		// You should already have an object selected from the mousedown.
		// If so, show its properties
		LLFloaterReg::showInstance("build", "Content");
		return TRUE;
	}
	else
	{
		// Nothing selected means the first mouse click was probably
		// bad, so try again.
		return handleMouseDown(x, y, mask);
	}
}


void LLToolCompRotate::render()
{
	mCur->render();

	if( mCur != mManip )
	{
		LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
		mManip->renderGuidelines();
	}
}


//-----------------------------------------------------------------------
// LLToolCompGun

LLToolCompGun::LLToolCompGun()
	: LLToolComposite(std::string("Mouselook"))
{
	mGun = new LLToolGun(this);
	mGrab = new LLToolGrab(this);
	mNull = sNullTool;

	setCurrentTool(mGun);
	mDefault = mGun;
}


LLToolCompGun::~LLToolCompGun()
{
	delete mGun;
	mGun = NULL;

	delete mGrab;
	mGrab = NULL;

	// don't delete a static object
	// delete mNull;
	mNull = NULL;
}

BOOL LLToolCompGun::handleHover(S32 x, S32 y, MASK mask)
{
	// *NOTE: This hack is here to make mouselook kick in again after
	// item selected from context menu.
	if ( mCur == mNull && !gPopupMenuView->getVisible() )
	{
		LLSelectMgr::getInstance()->deselectAll();
		setCurrentTool( (LLTool*) mGrab );
	}

	// Note: if the tool changed, we can't delegate the current mouse event
	// after the change because tools can modify the mouse during selection and deselection.
	// Instead we let the current tool handle the event and then make the change.
	// The new tool will take effect on the next frame.

	mCur->handleHover( x, y, mask );

	// If mouse button not down...
	if( !gViewerWindow->getLeftMouseDown())
	{
		// let ALT switch from gun to grab
		if ( mCur == mGun && (mask & MASK_ALT) )
		{
			setCurrentTool( (LLTool*) mGrab );
		}
		else if ( mCur == mGrab && !(mask & MASK_ALT) )
		{
			setCurrentTool( (LLTool*) mGun );
			setMouseCapture(TRUE);
		}
	}

	return TRUE; 
}


BOOL LLToolCompGun::handleMouseDown(S32 x, S32 y, MASK mask)
{ 
	// if the left button is grabbed, don't put up the pie menu
	if (gAgent.leftButtonGrabbed())
	{
		gAgent.setControlFlags(AGENT_CONTROL_ML_LBUTTON_DOWN);
		return FALSE;
	}

	// On mousedown, start grabbing
	gGrabTransientTool = this;
	LLToolMgr::getInstance()->getCurrentToolset()->selectTool( (LLTool*) mGrab );

	return LLToolGrab::getInstance()->handleMouseDown(x, y, mask);
}


BOOL LLToolCompGun::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	// if the left button is grabbed, don't put up the pie menu
	if (gAgent.leftButtonGrabbed())
	{
		gAgent.setControlFlags(AGENT_CONTROL_ML_LBUTTON_DOWN);
		return FALSE;
	}

	// On mousedown, start grabbing
	gGrabTransientTool = this;
	LLToolMgr::getInstance()->getCurrentToolset()->selectTool( (LLTool*) mGrab );

	return LLToolGrab::getInstance()->handleDoubleClick(x, y, mask);
}


BOOL LLToolCompGun::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	/* JC - suppress context menu 8/29/2002

	// On right mouse, go through some convoluted steps to
	// make the build menu appear.
	setCurrentTool( (LLTool*) mNull );

	// This should return FALSE, meaning the context menu will
	// be shown.
	return FALSE;
	*/

	// Returning true will suppress the context menu
	return TRUE;
}


BOOL LLToolCompGun::handleMouseUp(S32 x, S32 y, MASK mask)
{
	gAgent.setControlFlags(AGENT_CONTROL_ML_LBUTTON_UP);
	setCurrentTool( (LLTool*) mGun );
	return TRUE;
}

void LLToolCompGun::onMouseCaptureLost()
{
	if (mComposite)
	{
		mComposite->onMouseCaptureLost();
		return;
	}
	mCur->onMouseCaptureLost();
}

void	LLToolCompGun::handleSelect()
{
	LLToolComposite::handleSelect();
	setMouseCapture(TRUE);
}

void	LLToolCompGun::handleDeselect()
{
	LLToolComposite::handleDeselect();
	setMouseCapture(FALSE);
}


BOOL LLToolCompGun::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	if (clicks > 0)
	{
		gAgentCamera.changeCameraToDefault();

	}
	return TRUE;
}
