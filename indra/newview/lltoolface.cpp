/** 
 * @file lltoolface.cpp
 * @brief A tool to manipulate faces
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

#include "llviewerprecompiledheaders.h"

// File includes
#include "lltoolface.h" 

// Library includes
#include "v3math.h"

// Viewer includes
#include "llagent.h"
//#include "llbuildview.h"
#include "llviewercontrol.h"
#include "llselectmgr.h"
#include "lltoolview.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llfloatertools.h"

//
// Member functions
//

LLToolFace::LLToolFace()
:	LLTool(std::string("Texture"))
{ }


LLToolFace::~LLToolFace()
{ }


BOOL LLToolFace::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (!LLSelectMgr::getInstance()->getSelection()->isEmpty())
	{
		// You should already have an object selected from the mousedown.
		// If so, show its properties
		//gBuildView->showFacePanel();
		gFloaterTools->showPanel( LLFloaterTools::PANEL_FACE );
		//gBuildView->showMore(LLBuildView::PANEL_FACE);
		return TRUE;
	}
	else
	{
		// Nothing selected means the first mouse click was probably
		// bad, so try again.
		return FALSE;
	}
}


BOOL LLToolFace::handleMouseDown(S32 x, S32 y, MASK mask)
{
	gViewerWindow->pickAsync(x, y, mask, pickCallback);
	return TRUE;
}

void LLToolFace::pickCallback(const LLPickInfo& pick_info)
{
	LLViewerObject* hit_obj	= pick_info.getObject();
	if (hit_obj)
	{
		S32 hit_face = pick_info.mObjectFace;
		
		if (hit_obj->isAvatar())
		{
			// ...clicked on an avatar, so don't do anything
			return;
		}

		// ...clicked on a world object, try to pick the appropriate face

		if (pick_info.mKeyMask & MASK_SHIFT)
		{
			// If object not selected, need to inform sim
			if ( !hit_obj->isSelected() )
			{
				// object wasn't selected so add the object and face
				LLSelectMgr::getInstance()->selectObjectOnly(hit_obj, hit_face);
			}
			else if (!LLSelectMgr::getInstance()->getSelection()->contains(hit_obj, hit_face) )
			{
				// object is selected, but not this face, so add it.
				LLSelectMgr::getInstance()->addAsIndividual(hit_obj, hit_face);
			}
			else
			{
				// object is selected, as is this face, so remove the face.
				LLSelectMgr::getInstance()->remove(hit_obj, hit_face);

				// BUG: If you remove the last face, the simulator won't know about it.
			}
		}
		else
		{
			// clicked without modifiers, select only
			// this face
			LLSelectMgr::getInstance()->deselectAll();
			LLSelectMgr::getInstance()->selectObjectOnly(hit_obj, hit_face);
		}
	}
	else
	{
		if (!(pick_info.mKeyMask == MASK_SHIFT))
		{
			LLSelectMgr::getInstance()->deselectAll();
		}
	}
}


void LLToolFace::handleSelect()
{
	// From now on, draw faces
	LLSelectMgr::getInstance()->setTEMode(TRUE);
}


void LLToolFace::handleDeselect()
{
	// Stop drawing faces
	LLSelectMgr::getInstance()->setTEMode(FALSE);
}


void LLToolFace::render()
{
	// for now, do nothing
}
