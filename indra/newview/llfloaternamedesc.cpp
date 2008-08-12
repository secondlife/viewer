/** 
 * @file llfloaternamedesc.cpp
 * @brief LLFloaterNameDesc class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#include "llfloaternamedesc.h"

// project includes
#include "lllineeditor.h"
#include "llresmgr.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "llviewerwindow.h"
#include "llfocusmgr.h"
#include "llradiogroup.h"
#include "lldbstrings.h"
#include "lldir.h"
#include "llviewercontrol.h"
#include "llviewermenufile.h"	// upload_new_resource()
#include "lluictrlfactory.h"
#include "llstring.h"

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
LLFloaterNameDesc::LLFloaterNameDesc(const std::string& filename )
	: LLFloater(std::string("Name/Description Floater"))
{
	mFilenameAndPath = filename;
	mFilename = gDirUtilp->getBaseFileName(filename, false);
	// SL-5521 Maintain capitalization of filename when making the inventory item. JC
	//LLStringUtil::toLower(mFilename);
	mIsAudio = FALSE;
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

	std::string exten = gDirUtilp->getExtension(asset_name);
	if (exten == "wav")
	{
		mIsAudio = TRUE;
	}
	asset_name = gDirUtilp->getBaseFileName(asset_name, true); // no extsntion

	setTitle(mFilename);

	centerWithin(gViewerWindow->getRootView()->getRect());

	S32 line_width = getRect().getWidth() - 2 * PREVIEW_HPAD;
	S32 y = getRect().getHeight() - PREVIEW_LINE_HEIGHT;

	r.setLeftTopAndSize( PREVIEW_HPAD, y, line_width, PREVIEW_LINE_HEIGHT );
	y -= PREVIEW_LINE_HEIGHT;

	r.setLeftTopAndSize( PREVIEW_HPAD, y, line_width, PREVIEW_LINE_HEIGHT );    

	childSetCommitCallback("name_form", doCommit, this);
	childSetValue("name_form", LLSD(asset_name));

	LLLineEditor *NameEditor = getChild<LLLineEditor>("name_form");
	if (NameEditor)
	{
		NameEditor->setMaxTextLength(DB_INV_ITEM_NAME_STR_LEN);
		NameEditor->setPrevalidate(&LLLineEditor::prevalidatePrintableNotPipe);
	}

	y -= llfloor(PREVIEW_LINE_HEIGHT * 1.2f);
	y -= PREVIEW_LINE_HEIGHT;

	r.setLeftTopAndSize( PREVIEW_HPAD, y, line_width, PREVIEW_LINE_HEIGHT );  
	childSetCommitCallback("description_form", doCommit, this);
	LLLineEditor *DescEditor = getChild<LLLineEditor>("description_form");
	if (DescEditor)
	{
		DescEditor->setMaxTextLength(DB_INV_ITEM_DESC_STR_LEN);
		DescEditor->setPrevalidate(&LLLineEditor::prevalidatePrintableNotPipe);
	}

	y -= llfloor(PREVIEW_LINE_HEIGHT * 1.2f);

	// Cancel button
	childSetAction("cancel_btn", onBtnCancel, this);

	// OK button
	childSetAction("ok_btn", onBtnOK, this);
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

// static 
//-----------------------------------------------------------------------------
// onCommit()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::doCommit( class LLUICtrl *, void* userdata )
{
	LLFloaterNameDesc* self = (LLFloaterNameDesc*) userdata;
	self->onCommit();
}

// static 
//-----------------------------------------------------------------------------
// onBtnOK()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::onBtnOK( void* userdata )
{
	LLFloaterNameDesc *fp =(LLFloaterNameDesc *)userdata;

	fp->childDisable("ok_btn"); // don't allow inadvertent extra uploads
	
	upload_new_resource(fp->mFilenameAndPath, // file
		fp->childGetValue("name_form").asString(), 
		fp->childGetValue("description_form").asString(), 
		0, LLAssetType::AT_NONE, LLInventoryType::IT_NONE);
	fp->close(false);
}

// static 
//-----------------------------------------------------------------------------
// onBtnCancel()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::onBtnCancel( void* userdata )
{
	LLFloaterNameDesc *fp =(LLFloaterNameDesc *)userdata;
	fp->close(false);
}
