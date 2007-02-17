/** 
 * @file lltoolface.cpp
 * @brief A tool to manipulate faces
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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

// Globals
LLToolFace *gToolFace = NULL;

//
// Member functions
//

LLToolFace::LLToolFace()
:	LLTool("Texture")
{ }


LLToolFace::~LLToolFace()
{ }


BOOL LLToolFace::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (!gSelectMgr->getSelection()->isEmpty())
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
	gPickFaces = TRUE;
	gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback);
	return TRUE;
}

void LLToolFace::pickCallback(S32 x, S32 y, MASK mask)
{
	LLViewerObject* hit_obj	= gViewerWindow->lastObjectHit();
	if (hit_obj)
	{
		S32 hit_face = gLastHitObjectFace;
		
		if (hit_obj->isAvatar())
		{
			// ...clicked on an avatar, so don't do anything
			return;
		}

		// ...clicked on a world object, try to pick the appropriate face

		if (mask & MASK_SHIFT)
		{
			// If object not selected, need to inform sim
			if ( !hit_obj->isSelected() )
			{
				// object wasn't selected so add the object and face
				gSelectMgr->selectObjectOnly(hit_obj, hit_face);
			}
			else if (!gSelectMgr->getSelection()->contains(hit_obj, hit_face) )
			{
				// object is selected, but not this face, so add it.
				gSelectMgr->addAsIndividual(hit_obj, hit_face);
			}
			else
			{
				// object is selected, as is this face, so remove the face.
				gSelectMgr->remove(hit_obj, hit_face);

				// BUG: If you remove the last face, the simulator won't know about it.
			}
		}
		else
		{
			// clicked without modifiers, select only
			// this face
			gSelectMgr->deselectAll();
			gSelectMgr->selectObjectOnly(hit_obj, hit_face);
		}
	}
	else
	{
		if (!(mask == MASK_SHIFT))
		{
			gSelectMgr->deselectAll();
		}
	}
}


void LLToolFace::handleSelect()
{
	// From now on, draw faces
	gSelectMgr->setTEMode(TRUE);
}


void LLToolFace::handleDeselect()
{
	// Stop drawing faces
	gSelectMgr->setTEMode(FALSE);
}


void LLToolFace::render()
{
	// for now, do nothing
}
