/** 
 * @file lltoolcomp.cpp
 * @brief Composite tools
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llviewerprecompiledheaders.h"

#include "lltoolcomp.h"

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
#include "llfloatertools.h"
#include "llviewercontrol.h"

const S32 BUTTON_HEIGHT = 16;
const S32 BUTTON_WIDTH_SMALL = 32;
const S32 BUTTON_WIDTH_BIG = 48;
const S32 HPAD = 4;

extern LLControlGroup gSavedSettings;


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

LLToolComposite::LLToolComposite(const LLString& name)
	: LLTool(name),
	  mCur(NULL), mDefault(NULL), mSelected(FALSE),
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
: LLToolComposite("Inspect")
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
	gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback);
	return TRUE;
}

void LLToolCompInspect::pickCallback(S32 x, S32 y, MASK mask)
{
	LLViewerObject* hit_obj = gViewerWindow->lastObjectHit();

	if (!LLToolCompInspect::getInstance()->mMouseDown)
	{
		// fast click on object, but mouse is already up...just do select
		LLToolCompInspect::getInstance()->mSelectRect->handleObjectSelection(hit_obj, mask, gSavedSettings.getBOOL("EditLinkedParts"), FALSE);
		return;
	}

	if( hit_obj )
	{
		if (LLSelectMgr::getInstance()->getSelection()->getObjectCount())
		{
			LLEditMenuHandler::gEditMenuHandler = LLSelectMgr::getInstance();
		}
		LLToolCompInspect::getInstance()->setCurrentTool( LLToolCompInspect::getInstance()->mSelectRect );
		LLToolCompInspect::getInstance()->mSelectRect->handleMouseDown( x, y, mask );

	}
	else
	{
		LLToolCompInspect::getInstance()->setCurrentTool( LLToolCompInspect::getInstance()->mSelectRect );
		LLToolCompInspect::getInstance()->mSelectRect->handleMouseDown( x, y, mask);
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
	: LLToolComposite("Move")
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
	gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback, TRUE);
	return TRUE;
}

void LLToolCompTranslate::pickCallback(S32 x, S32 y, MASK mask)
{
	LLViewerObject* hit_obj = gViewerWindow->lastObjectHit();

	LLToolCompTranslate::getInstance()->mManip->highlightManipulators(x, y);
	if (!LLToolCompTranslate::getInstance()->mMouseDown)
	{
		// fast click on object, but mouse is already up...just do select
		LLToolCompTranslate::getInstance()->mSelectRect->handleObjectSelection(hit_obj, mask, gSavedSettings.getBOOL("EditLinkedParts"), FALSE);
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
			LLToolCompTranslate::getInstance()->mManip->handleMouseDownOnPart( x, y, mask );
		}
		else
		{
			LLToolCompTranslate::getInstance()->setCurrentTool( LLToolCompTranslate::getInstance()->mSelectRect );
			LLToolCompTranslate::getInstance()->mSelectRect->handleMouseDown( x, y, mask );

			// *TODO: add toggle to trigger old click-drag functionality
			// LLToolCompTranslate::getInstance()->mManip->handleMouseDownOnPart( XY_part, x, y, mask);
		}
	}
	else
	{
		LLToolCompTranslate::getInstance()->setCurrentTool( LLToolCompTranslate::getInstance()->mSelectRect );
		LLToolCompTranslate::getInstance()->mSelectRect->handleMouseDown( x, y, mask);
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
		gFloaterTools->showPanel(LLFloaterTools::PANEL_CONTENTS);
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
	: LLToolComposite("Stretch")
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
	gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback);
	return TRUE;
}

void LLToolCompScale::pickCallback(S32 x, S32 y, MASK mask)
{
	LLViewerObject* hit_obj = gViewerWindow->lastObjectHit();

	LLToolCompScale::getInstance()->mManip->highlightManipulators(x, y);
	if (!LLToolCompScale::getInstance()->mMouseDown)
	{
		// fast click on object, but mouse is already up...just do select
		LLToolCompScale::getInstance()->mSelectRect->handleObjectSelection(hit_obj, mask, gSavedSettings.getBOOL("EditLinkedParts"), FALSE);

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
			LLToolCompScale::getInstance()->mManip->handleMouseDownOnPart( x, y, mask );
		}
		else
		{
			LLToolCompScale::getInstance()->setCurrentTool( LLToolCompScale::getInstance()->mSelectRect );
			LLToolCompScale::getInstance()->mSelectRect->handleMouseDown( x, y, mask );
		}
	}
	else
	{
		LLToolCompScale::getInstance()->setCurrentTool( LLToolCompScale::getInstance()->mSelectRect );
		LLToolCompScale::getInstance()->mCur->handleMouseDown( x, y, mask );
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
		gFloaterTools->showPanel(LLFloaterTools::PANEL_CONTENTS);
		//gBuildView->setPropertiesPanelOpen(TRUE);
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
	: LLToolComposite("Create")
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

	if ( !(mask == MASK_SHIFT) && !(mask == MASK_CONTROL) )
	{
		setCurrentTool( mPlacer );
		handled = mPlacer->placeObject( x, y, mask );
	}
	else
	{
		gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback);
		handled = TRUE;
	}
	
	mObjectPlacedOnMouseDown = TRUE;

	return TRUE;
}

void LLToolCompCreate::pickCallback(S32 x, S32 y, MASK mask)
{
	// *NOTE: We mask off shift and control, so you cannot
	// multi-select multiple objects with the create tool.
	mask = (mask & ~MASK_SHIFT);
	mask = (mask & ~MASK_CONTROL);

	LLToolCompCreate::getInstance()->setCurrentTool( LLToolCompCreate::getInstance()->mSelectRect );
	LLToolCompCreate::getInstance()->mSelectRect->handleMouseDown( x, y, mask);
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
	: LLToolComposite("Rotate")
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
	gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback);
	return TRUE;
}

void LLToolCompRotate::pickCallback(S32 x, S32 y, MASK mask)
{
	LLViewerObject* hit_obj = gViewerWindow->lastObjectHit();

	LLToolCompRotate::getInstance()->mManip->highlightManipulators(x, y);
	if (!LLToolCompRotate::getInstance()->mMouseDown)
	{
		// fast click on object, but mouse is already up...just do select
		LLToolCompRotate::getInstance()->mSelectRect->handleObjectSelection(hit_obj, mask, gSavedSettings.getBOOL("EditLinkedParts"), FALSE);
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
			LLToolCompRotate::getInstance()->mManip->handleMouseDownOnPart( x, y, mask );
		}
		else
		{
			LLToolCompRotate::getInstance()->setCurrentTool( LLToolCompRotate::getInstance()->mSelectRect );
			LLToolCompRotate::getInstance()->mSelectRect->handleMouseDown( x, y, mask );
		}
	}
	else
	{
		LLToolCompRotate::getInstance()->setCurrentTool( LLToolCompRotate::getInstance()->mSelectRect );
		LLToolCompRotate::getInstance()->mCur->handleMouseDown( x, y, mask );
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
		gFloaterTools->showPanel(LLFloaterTools::PANEL_CONTENTS);
		//gBuildView->setPropertiesPanelOpen(TRUE);
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
	: LLToolComposite("Mouselook")
{
	mGun = new LLToolGun(this);
	mGrab = new LLToolGrab(this);
	mNull = new LLTool("null", this);

	setCurrentTool(mGun);
	mDefault = mGun;
}


LLToolCompGun::~LLToolCompGun()
{
	delete mGun;
	mGun = NULL;

	delete mGrab;
	mGrab = NULL;

	delete mNull;
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

	// JC - I don't know if this is necessary.  Maybe we could lose capture
	// if someone ALT-Tab's out when in mouselook.
	setCurrentTool( (LLTool*) mGun );
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
		gAgent.changeCameraToDefault();

	}
	return TRUE;
}
