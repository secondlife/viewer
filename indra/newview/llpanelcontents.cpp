/** 
 * @file llpanelcontents.cpp
 * @brief Object contents panel in the tools floater.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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

#include "llagent.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llresmgr.h"
#include "lltexturetable.h"
#include "llselectmgr.h"
#include "llpreviewscript.h"
#include "lltool.h"
#include "lltoolmgr.h"
#include "lltoolcomp.h"
#include "llpanelinventory.h"
#include "viewer.h"

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
		//mBtnNewScript->setEnabled( FALSE );
		return;
	}

	// BUG? Check for all objects being editable?
	BOOL editable = gAgent.isGodlike() 
					|| (objectp->permModify() && objectp->permYouOwner());
	BOOL all_volume = gSelectMgr->selectionAllPCode( LL_PCODE_VOLUME );

	// Edit script button - ok if object is editable and there's an
	// unambiguous destination for the object.
	if(	editable && 
		all_volume && 
		((gSelectMgr->getRootObjectCount() == 1)
					|| (gSelectMgr->getObjectCount() == 1)))
	{
		//mBtnNewScript->setEnabled(TRUE);
		childSetEnabled("button new script",TRUE);
	}
	else
	{
		//mBtnNewScript->setEnabled(FALSE);
		childSetEnabled("button new script",FALSE);
	}
}


void LLPanelContents::refresh()
{
	LLViewerObject* object = gSelectMgr->getFirstRootObject();
	if(!object)
	{
		object = gSelectMgr->getFirstObject();
	}

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
	LLViewerObject* object = gSelectMgr->getFirstRootObject();
	if(!object)
	{
		object = gSelectMgr->getFirstObject();
	}
	if(object)
	{
		// *HACK: In order to resolve SL-22177, we need to create the
		// script first, and then you have to click it in inventory to
		// edit it. Bring this back when the functionality is secure.
		LLPermissions perm;
		perm.init(gAgent.getID(), gAgent.getID(), LLUUID::null, LLUUID::null);
		perm.initMasks(
			PERM_ALL,
			PERM_ALL,
			PERM_NONE,
			PERM_NONE,
			PERM_MOVE | PERM_TRANSFER);
		LLString desc;
		LLAssetType::generateDescriptionFor(LLAssetType::AT_LSL_TEXT, desc);
		LLPointer<LLViewerInventoryItem> new_item =
			new LLViewerInventoryItem(
				LLUUID::null,
				LLUUID::null,
				perm,
				LLUUID::null,
				LLAssetType::AT_LSL_TEXT,
				LLInventoryType::IT_LSL,
				LLString("New Script"),
				desc,
				LLSaleInfo::DEFAULT,
				LLViewerInventoryItem::II_FLAGS_NONE,
				time_corrected());
		object->saveScript(new_item, TRUE, true);
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
		editor->open();

		// keep onscreen
		gFloaterView->adjustToFitScreen(editor, FALSE);
#endif
	}
}
