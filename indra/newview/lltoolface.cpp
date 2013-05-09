/** 
 * @file lltoolface.cpp
 * @brief A tool to manipulate faces
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

// File includes
#include "lltoolface.h" 

// Library includes
#include "llfloaterreg.h"
#include "v3math.h"

// Viewer includes
#include "llviewercontrol.h"
#include "llselectmgr.h"
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
		LLFloaterReg::showInstance("build", "Texture");
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
