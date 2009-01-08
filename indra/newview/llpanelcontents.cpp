/**
 * @file llpanelcontents.cpp
 * @brief Object contents panel in the tools floater.
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

// file include
#include "llpanelcontents.h"

// linden library includes
#include "llerror.h"
#include "llrect.h"
#include "llstring.h"
#include "llmaterialtable.h"
#include "llfontgl.h"
#include "m3math.h"
#include "llpermissionsflags.h"
#include "lleconomy.h"
#include "material_codes.h"

// project includes
#include "llui.h"
#include "llspinctrl.h"
#include "llcheckboxctrl.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llfloaterbulkpermission.h"

#include "llagent.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llpreviewscript.h"
#include "lltool.h"
#include "lltoolmgr.h"
#include "lltoolcomp.h"
#include "llpanelinventory.h"

//
// Imported globals
//


//
// Globals
//

BOOL LLPanelContents::postBuild()
{
	LLRect rect = this->getRect();

	setMouseOpaque(FALSE);

	childSetAction("button new script",&LLPanelContents::onClickNewScript, this);
	childSetAction("button permissions",&LLPanelContents::onClickPermissions, this);

	return TRUE;
}

LLPanelContents::LLPanelContents(const std::string& name)
	:	LLPanel(name),
		mPanelInventory(NULL)
{
}


LLPanelContents::~LLPanelContents()
{
	// Children all cleaned up by default view destructor.
}


void LLPanelContents::getState(LLViewerObject *objectp )
{
	if( !objectp )
	{
		childSetEnabled("button new script",FALSE);
		return;
	}

	LLUUID group_id;			// used for SL-23488
	LLSelectMgr::getInstance()->selectGetGroup(group_id);  // sets group_id as a side effect SL-23488

	// BUG? Check for all objects being editable?
	BOOL editable = gAgent.isGodlike()
					|| (objectp->permModify()
					       && ( objectp->permYouOwner() || ( !group_id.isNull() && gAgent.isInGroup(group_id) )));  // solves SL-23488
	BOOL all_volume = LLSelectMgr::getInstance()->selectionAllPCode( LL_PCODE_VOLUME );

	// Edit script button - ok if object is editable and there's an unambiguous destination for the object.
	childSetEnabled("button new script",
		editable &&
		all_volume &&
		((LLSelectMgr::getInstance()->getSelection()->getRootObjectCount() == 1)
			|| (LLSelectMgr::getInstance()->getSelection()->getObjectCount() == 1)));
}


void LLPanelContents::refresh()
{
	const BOOL children_ok = TRUE;
	LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject(children_ok);

	getState(object);
	if (mPanelInventory)
	{
		mPanelInventory->refresh();
	}
}



//
// Static functions
//

// static
void LLPanelContents::onClickNewScript(void *userdata)
{
	const BOOL children_ok = TRUE;
	LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject(children_ok);
	if(object)
	{
		LLPermissions perm;
		perm.init(gAgent.getID(), gAgent.getID(), LLUUID::null, LLUUID::null);
		perm.initMasks(
			PERM_ALL,
			PERM_ALL,
			PERM_NONE,
			PERM_NONE,
			PERM_MOVE | PERM_TRANSFER);
		std::string desc;
		LLAssetType::generateDescriptionFor(LLAssetType::AT_LSL_TEXT, desc);
		LLPointer<LLViewerInventoryItem> new_item =
			new LLViewerInventoryItem(
				LLUUID::null,
				LLUUID::null,
				perm,
				LLUUID::null,
				LLAssetType::AT_LSL_TEXT,
				LLInventoryType::IT_LSL,
				std::string("New Script"),
				desc,
				LLSaleInfo::DEFAULT,
				LLViewerInventoryItem::II_FLAGS_NONE,
				time_corrected());
		object->saveScript(new_item, TRUE, true);

		// *NOTE: In order to resolve SL-22177, we needed to create
		// the script first, and then you have to click it in
		// inventory to edit it.
		// *TODO: The script creation should round-trip back to the
		// viewer so the viewer can auto-open the script and start
		// editing ASAP.
#if 0
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("PreviewScriptRect");
		rect.translate( left - rect.mLeft, top - rect.mTop );

		LLLiveLSLEditor* editor;
		editor = new LLLiveLSLEditor("script ed",
									   rect,
									   "Script: New Script",
									   object->mID,
									   LLUUID::null);
		editor->open();	/*Flawfinder: ignore*/

		// keep onscreen
		gFloaterView->adjustToFitScreen(editor, FALSE);
#endif
	}
}


// static
void LLPanelContents::onClickPermissions(void *userdata)
{
	LLPanelContents* self = (LLPanelContents*)userdata;
	gFloaterView->getParentFloater(self)->addDependentFloater(LLFloaterBulkPermission::showInstance());
}
