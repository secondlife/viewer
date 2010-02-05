/** 
 * @file llfloaternamedesc.cpp
 * @brief LLFloaterNameDesc class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llfloaternamedesc.h"

// project includes
#include "lllineeditor.h"
#include "llresmgr.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "llviewerwindow.h"
#include "llfocusmgr.h"
#include "llrootview.h"
#include "llradiogroup.h"
#include "lldbstrings.h"
#include "lldir.h"
#include "llfloaterperms.h"
#include "llviewercontrol.h"
#include "llviewermenufile.h"	// upload_new_resource()
#include "lluictrlfactory.h"
#include "llstring.h"
#include "lleconomy.h"

// linden includes
#include "llassetstorage.h"
#include "llinventorytype.h"

const S32 PREVIEW_LINE_HEIGHT = 19;
const S32 PREVIEW_CLOSE_BOX_SIZE = 16;
const S32 PREVIEW_BORDER_WIDTH = 2;
const S32 PREVIEW_RESIZE_HANDLE_SIZE = S32(RESIZE_HANDLE_WIDTH * OO_SQRT2) + PREVIEW_BORDER_WIDTH;
const S32 PREVIEW_VPAD = 2;
const S32 PREVIEW_HPAD = PREVIEW_RESIZE_HANDLE_SIZE;
const S32 PREVIEW_HEADER_SIZE = 3 * PREVIEW_LINE_HEIGHT + PREVIEW_VPAD;
const S32 PREF_BUTTON_WIDTH = 64;
const S32 PREF_BUTTON_HEIGHT = 16;

//-----------------------------------------------------------------------------
// LLFloaterNameDesc()
//-----------------------------------------------------------------------------
LLFloaterNameDesc::LLFloaterNameDesc(const LLSD& filename )
	: LLFloater(filename),
	  mIsAudio(FALSE)	  
{
	mFilenameAndPath = filename.asString();
	mFilename = gDirUtilp->getBaseFileName(mFilenameAndPath, false);
}

//-----------------------------------------------------------------------------
// postBuild()
//-----------------------------------------------------------------------------
BOOL LLFloaterNameDesc::postBuild()
{
	LLRect r;

	std::string asset_name = mFilename;
	LLStringUtil::replaceNonstandardASCII( asset_name, '?' );
	LLStringUtil::replaceChar(asset_name, '|', '?');
	LLStringUtil::stripNonprintable(asset_name);
	LLStringUtil::trim(asset_name);

	asset_name = gDirUtilp->getBaseFileName(asset_name, true); // no extsntion

	setTitle(mFilename);

	centerWithin(gViewerWindow->getRootView()->getRect());

	S32 line_width = getRect().getWidth() - 2 * PREVIEW_HPAD;
	S32 y = getRect().getHeight() - PREVIEW_LINE_HEIGHT;

	r.setLeftTopAndSize( PREVIEW_HPAD, y, line_width, PREVIEW_LINE_HEIGHT );
	y -= PREVIEW_LINE_HEIGHT;

	r.setLeftTopAndSize( PREVIEW_HPAD, y, line_width, PREVIEW_LINE_HEIGHT );    

	getChild<LLUICtrl>("name_form")->setCommitCallback(boost::bind(&LLFloaterNameDesc::doCommit, this));
	childSetValue("name_form", LLSD(asset_name));

	LLLineEditor *NameEditor = getChild<LLLineEditor>("name_form");
	if (NameEditor)
	{
		NameEditor->setMaxTextLength(DB_INV_ITEM_NAME_STR_LEN);
		NameEditor->setPrevalidate(&LLTextValidate::validateASCIIPrintableNoPipe);
	}

	y -= llfloor(PREVIEW_LINE_HEIGHT * 1.2f);
	y -= PREVIEW_LINE_HEIGHT;

	r.setLeftTopAndSize( PREVIEW_HPAD, y, line_width, PREVIEW_LINE_HEIGHT );  
	getChild<LLUICtrl>("description_form")->setCommitCallback(boost::bind(&LLFloaterNameDesc::doCommit, this));
	LLLineEditor *DescEditor = getChild<LLLineEditor>("description_form");
	if (DescEditor)
	{
		DescEditor->setMaxTextLength(DB_INV_ITEM_DESC_STR_LEN);
		DescEditor->setPrevalidate(&LLTextValidate::validateASCIIPrintableNoPipe);
	}

	y -= llfloor(PREVIEW_LINE_HEIGHT * 1.2f);

	// Cancel button
	getChild<LLUICtrl>("cancel_btn")->setCommitCallback(boost::bind(&LLFloaterNameDesc::onBtnCancel, this));

	childSetLabelArg("ok_btn", "[AMOUNT]", llformat("%d", LLGlobalEconomy::Singleton::getInstance()->getPriceUpload() ));
	
	setDefaultBtn("ok_btn");
	
	return TRUE;
}

//-----------------------------------------------------------------------------
// LLFloaterNameDesc()
//-----------------------------------------------------------------------------
LLFloaterNameDesc::~LLFloaterNameDesc()
{
	gFocusMgr.releaseFocusIfNeeded( this ); // calls onCommit()
}

// Sub-classes should override this function if they allow editing
//-----------------------------------------------------------------------------
// onCommit()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::onCommit()
{
}

//-----------------------------------------------------------------------------
// onCommit()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::doCommit()
{
	onCommit();
}

//-----------------------------------------------------------------------------
// onBtnOK()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::onBtnOK( )
{
	childDisable("ok_btn"); // don't allow inadvertent extra uploads
	
	LLAssetStorage::LLStoreAssetCallback callback = NULL;
	S32 expected_upload_cost = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload(); // kinda hack - assumes that unsubclassed LLFloaterNameDesc is only used for uploading chargeable assets, which it is right now (it's only used unsubclassed for the sound upload dialog, and THAT should be a subclass).
	void *nruserdata = NULL;
	std::string display_name = LLStringUtil::null;
	upload_new_resource(mFilenameAndPath, // file
			    childGetValue("name_form").asString(), 
			    childGetValue("description_form").asString(), 
			    0, LLFolderType::FT_NONE, LLInventoryType::IT_NONE,
			    LLFloaterPerms::getNextOwnerPerms(), LLFloaterPerms::getGroupPerms(), LLFloaterPerms::getEveryonePerms(),
			    display_name, callback, expected_upload_cost, nruserdata);
	closeFloater(false);
}

//-----------------------------------------------------------------------------
// onBtnCancel()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::onBtnCancel()
{
	closeFloater(false);
}


//-----------------------------------------------------------------------------
// LLFloaterSoundPreview()
//-----------------------------------------------------------------------------

LLFloaterSoundPreview::LLFloaterSoundPreview(const LLSD& filename )
	: LLFloaterNameDesc(filename)
{
	mIsAudio = TRUE;
}

BOOL LLFloaterSoundPreview::postBuild()
{
	if (!LLFloaterNameDesc::postBuild())
	{
		return FALSE;
	}
	getChild<LLUICtrl>("ok_btn")->setCommitCallback(boost::bind(&LLFloaterNameDesc::onBtnOK, this));
	return TRUE;
}
