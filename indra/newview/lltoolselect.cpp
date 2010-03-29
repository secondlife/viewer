/** 
 * @file lltoolselect.cpp
 * @brief LLToolSelect class implementation
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

#include "lltoolselect.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llviewercontrol.h"
#include "lldrawable.h"
#include "llmanip.h"
#include "llmenugl.h"
#include "llselectmgr.h"
#include "lltoolmgr.h"
#include "llfloaterscriptdebug.h"
#include "llviewercamera.h"
#include "llviewermenu.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h" 
#include "llviewerregion.h" 
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llworld.h"

// Globals
//extern BOOL gAllowSelectAvatar;

const F32 SELECTION_ROTATION_TRESHOLD = 0.1f;

LLToolSelect::LLToolSelect( LLToolComposite* composite )
:	LLTool( std::string("Select"), composite ),
	mIgnoreGroup( FALSE )
{
 }

// True if you selected an object.
BOOL LLToolSelect::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// do immediate pick query
	mPick = gViewerWindow->pickImmediate(x, y, TRUE);

	// Pass mousedown to agent
	LLTool::handleMouseDown(x, y, mask);

	return mPick.getObject().notNull();
}


// static
LLObjectSelectionHandle LLToolSelect::handleObjectSelection(const LLPickInfo& pick, BOOL ignore_group, BOOL temp_select, BOOL select_root)
{
	LLViewerObject* object = pick.getObject();
	if (select_root)
	{
		object = object->getRootEdit();
	}
	BOOL select_owned = gSavedSettings.getBOOL("SelectOwnedOnly");
	BOOL select_movable = gSavedSettings.getBOOL("SelectMovableOnly");
	
	// *NOTE: These settings must be cleaned up at bottom of function.
	if (temp_select || LLSelectMgr::getInstance()->mAllowSelectAvatar)
	{
		gSavedSettings.setBOOL("SelectOwnedOnly", FALSE);
		gSavedSettings.setBOOL("SelectMovableOnly", FALSE);
		LLSelectMgr::getInstance()->setForceSelection(TRUE);
	}

	BOOL extend_select = (pick.mKeyMask == MASK_SHIFT) || (pick.mKeyMask == MASK_CONTROL);

	// If no object, check for icon, then just deselect
	if (!object)
	{
		LLHUDIcon* last_hit_hud_icon = pick.mHUDIcon;

		if (last_hit_hud_icon && last_hit_hud_icon->getSourceObject())
		{
			LLFloaterScriptDebug::show(last_hit_hud_icon->getSourceObject()->getID());
		}
		else if (!extend_select)
		{
			LLSelectMgr::getInstance()->deselectAll();
		}
	}
	else
	{
		BOOL already_selected = object->isSelected();

		if ( extend_select )
		{
			if ( already_selected )
			{
				if ( ignore_group )
				{
					LLSelectMgr::getInstance()->deselectObjectOnly(object);
				}
				else
				{
					LLSelectMgr::getInstance()->deselectObjectAndFamily(object, TRUE, TRUE);
				}
			}
			else
			{
				if ( ignore_group )
				{
					LLSelectMgr::getInstance()->selectObjectOnly(object, SELECT_ALL_TES);
				}
				else
				{
					LLSelectMgr::getInstance()->selectObjectAndFamily(object);
				}
			}
		}
		else
		{
			// Save the current zoom values because deselect resets them.
			F32 target_zoom;
			F32 current_zoom;
			LLSelectMgr::getInstance()->getAgentHUDZoom(target_zoom, current_zoom);

			// JC - Change behavior to make it easier to select children
			// of linked sets. 9/3/2002
			if( !already_selected || ignore_group)
			{
				// ...lose current selection in favor of just this object
				LLSelectMgr::getInstance()->deselectAll();
			}

			if ( ignore_group )
			{
				LLSelectMgr::getInstance()->selectObjectOnly(object, SELECT_ALL_TES);
			}
			else
			{
				LLSelectMgr::getInstance()->selectObjectAndFamily(object);
			}

			// restore the zoom to the previously stored values.
			LLSelectMgr::getInstance()->setAgentHUDZoom(target_zoom, current_zoom);
		}

		if (!gAgentCamera.getFocusOnAvatar() &&										// if camera not glued to avatar
			LLVOAvatar::findAvatarFromAttachment(object) != gAgentAvatar &&	// and it's not one of your attachments
			object != gAgentAvatar)									// and it's not you
		{
			// have avatar turn to face the selected object(s)
			LLVector3d selection_center = LLSelectMgr::getInstance()->getSelectionCenterGlobal();
			selection_center = selection_center - gAgent.getPositionGlobal();
			LLVector3 selection_dir;
			selection_dir.setVec(selection_center);
			selection_dir.mV[VZ] = 0.f;
			selection_dir.normVec();
			if (!object->isAvatar() && gAgent.getAtAxis() * selection_dir < 0.6f)
			{
				LLQuaternion target_rot;
				target_rot.shortestArc(LLVector3::x_axis, selection_dir);
				gAgent.startAutoPilotGlobal(gAgent.getPositionGlobal(), "", &target_rot, NULL, NULL, 1.f, SELECTION_ROTATION_TRESHOLD);
			}
		}

		if (temp_select)
		{
			if (!already_selected)
			{
				LLViewerObject* root_object = (LLViewerObject*)object->getRootEdit();
				LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();

				// this is just a temporary selection
				LLSelectNode* select_node = selection->findNode(root_object);
				if (select_node)
				{
					select_node->setTransient(TRUE);
				}

				LLViewerObject::const_child_list_t& child_list = root_object->getChildren();
				for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
					 iter != child_list.end(); iter++)
				{
					LLViewerObject* child = *iter;
					select_node = selection->findNode(child);
					if (select_node)
					{
						select_node->setTransient(TRUE);
					}
				}

			}
		} //if(temp_select)
	} //if(!object)

	// Cleanup temp select settings above.
	if (temp_select ||LLSelectMgr::getInstance()->mAllowSelectAvatar)
	{
		gSavedSettings.setBOOL("SelectOwnedOnly", select_owned);
		gSavedSettings.setBOOL("SelectMovableOnly", select_movable);
		LLSelectMgr::getInstance()->setForceSelection(FALSE);
	}

	return LLSelectMgr::getInstance()->getSelection();
}

BOOL LLToolSelect::handleMouseUp(S32 x, S32 y, MASK mask)
{
	mIgnoreGroup = gSavedSettings.getBOOL("EditLinkedParts");

	handleObjectSelection(mPick, mIgnoreGroup, FALSE);

	return LLTool::handleMouseUp(x, y, mask);
}

void LLToolSelect::handleDeselect()
{
	if(	hasMouseCapture() )
	{
		setMouseCapture( FALSE );  // Calls onMouseCaptureLost() indirectly
	}
}


void LLToolSelect::stopEditing()
{
	if(	hasMouseCapture() )
	{
		setMouseCapture( FALSE );  // Calls onMouseCaptureLost() indirectly
	}
}

void LLToolSelect::onMouseCaptureLost()
{
	// Finish drag

	LLSelectMgr::getInstance()->enableSilhouette(TRUE);

	// Clean up drag-specific variables
	mIgnoreGroup = FALSE;
}




