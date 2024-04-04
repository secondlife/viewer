/** 
 * @file llfloaternamedesc.cpp
 * @brief LLFloaterNameDesc class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#include "llstatusbar.h"	// can_afford_transaction()
#include "llnotificationsutil.h"
#include "lluictrlfactory.h"
#include "llstring.h"
#include "llpermissions.h"
#include "lltrans.h"

// linden includes
#include "llassetstorage.h"
#include "llinventorytype.h"
#include "llagentbenefits.h"

const S32 PREVIEW_LINE_HEIGHT = 19;
const S32 PREVIEW_BORDER_WIDTH = 2;
const S32 PREVIEW_RESIZE_HANDLE_SIZE = S32(RESIZE_HANDLE_WIDTH * OO_SQRT2) + PREVIEW_BORDER_WIDTH;
const S32 PREVIEW_HPAD = PREVIEW_RESIZE_HANDLE_SIZE;

//-----------------------------------------------------------------------------
// LLFloaterNameDesc()
//-----------------------------------------------------------------------------
LLFloaterNameDesc::LLFloaterNameDesc(const LLSD& filename )
	: LLFloater(filename),
	  mIsAudio(false)	  
{
	mFilenameAndPath = filename.asString();
	mFilename = gDirUtilp->getBaseFileName(mFilenameAndPath, false);
}

//-----------------------------------------------------------------------------
// postBuild()
//-----------------------------------------------------------------------------
bool LLFloaterNameDesc::postBuild()
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
	getChild<LLUICtrl>("name_form")->setValue(LLSD(asset_name));

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

	S32 expected_upload_cost = getExpectedUploadCost();
	getChild<LLUICtrl>("ok_btn")->setLabelArg("[AMOUNT]", llformat("%d", expected_upload_cost));

	LLTextBox* info_text = getChild<LLTextBox>("info_text");
	if (info_text)
	{
		info_text->setValue(LLTrans::getString("UploadFeeInfo"));
	}
	
	setDefaultBtn("ok_btn");
	
	return true;
}

S32 LLFloaterNameDesc::getExpectedUploadCost() const
{
    std::string exten = gDirUtilp->getExtension(mFilename);
	LLAssetType::EType asset_type;
	S32 upload_cost = -1;
	if (LLResourceUploadInfo::findAssetTypeOfExtension(exten, asset_type))
	{
		if (!LLAgentBenefitsMgr::current().findUploadCost(asset_type, upload_cost))
		{
			LL_WARNS() << "Unable to find upload cost for asset type " << asset_type << LL_ENDL;
		}
	}
	else
	{
		LL_WARNS() << "Unable to find upload cost for " << mFilename << LL_ENDL;
	}
	return upload_cost;
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
	getChildView("ok_btn")->setEnabled(false); // don't allow inadvertent extra uploads
	
	LLAssetStorage::LLStoreAssetCallback callback;
	S32 expected_upload_cost = getExpectedUploadCost();
    if (can_afford_transaction(expected_upload_cost))
    {
        void *nruserdata = NULL;
        std::string display_name = LLStringUtil::null;

        LLResourceUploadInfo::ptr_t uploadInfo(std::make_shared<LLNewFileResourceUploadInfo>(
            mFilenameAndPath,
            getChild<LLUICtrl>("name_form")->getValue().asString(),
            getChild<LLUICtrl>("description_form")->getValue().asString(), 0,
            LLFolderType::FT_NONE, LLInventoryType::IT_NONE,
            LLFloaterPerms::getNextOwnerPerms("Uploads"),
            LLFloaterPerms::getGroupPerms("Uploads"),
            LLFloaterPerms::getEveryonePerms("Uploads"),
            expected_upload_cost));

        upload_new_resource(uploadInfo, callback, nruserdata);
    }
    else
    {
        LLSD args;
        args["COST"] = llformat("%d", expected_upload_cost);
        LLNotificationsUtil::add("ErrorCannotAffordUpload", args);
    }

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
	mIsAudio = true;
}

bool LLFloaterSoundPreview::postBuild()
{
	if (!LLFloaterNameDesc::postBuild())
	{
		return false;
	}
	getChild<LLUICtrl>("ok_btn")->setCommitCallback(boost::bind(&LLFloaterNameDesc::onBtnOK, this));
	return true;
}


//-----------------------------------------------------------------------------
// LLFloaterAnimPreview()
//-----------------------------------------------------------------------------

LLFloaterAnimPreview::LLFloaterAnimPreview(const LLSD& filename )
	: LLFloaterNameDesc(filename)
{
}

bool LLFloaterAnimPreview::postBuild()
{
	if (!LLFloaterNameDesc::postBuild())
	{
		return false;
	}
	getChild<LLUICtrl>("ok_btn")->setCommitCallback(boost::bind(&LLFloaterNameDesc::onBtnOK, this));
	return true;
}

//-----------------------------------------------------------------------------
// LLFloaterScriptPreview()
//-----------------------------------------------------------------------------

LLFloaterScriptPreview::LLFloaterScriptPreview(const LLSD& filename )
	: LLFloaterNameDesc(filename)
{
	mIsText = true;
}

bool LLFloaterScriptPreview::postBuild()
{
	if (!LLFloaterNameDesc::postBuild())
	{
		return false;
	}
	getChild<LLUICtrl>("ok_btn")->setCommitCallback(boost::bind(&LLFloaterNameDesc::onBtnOK, this));
	return true;
}
