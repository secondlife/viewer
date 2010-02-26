/** 
 * @file llsidepanelinventorysubpanel.cpp
 * @brief A floater which shows an inventory item's properties.
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
#include "llsidepanelinventorysubpanel.h"

#include "roles_constants.h"

#include "llagent.h"
#include "llavataractions.h"
#include "llbutton.h"
#include "llfloaterreg.h"
#include "llgroupactions.h"
#include "llinventorymodel.h"
#include "lllineeditor.h"
#include "llradiogroup.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"


///----------------------------------------------------------------------------
/// Class LLSidepanelInventorySubpanel
///----------------------------------------------------------------------------

// Default constructor
LLSidepanelInventorySubpanel::LLSidepanelInventorySubpanel()
  : LLPanel(),
	mIsDirty(TRUE),
	mIsEditing(FALSE),
	mCancelBtn(NULL),
	mSaveBtn(NULL)
{
}

// Destroys the object
LLSidepanelInventorySubpanel::~LLSidepanelInventorySubpanel()
{
}

// virtual
BOOL LLSidepanelInventorySubpanel::postBuild()
{
	mSaveBtn = getChild<LLButton>("save_btn");
	mSaveBtn->setClickedCallback(boost::bind(&LLSidepanelInventorySubpanel::onSaveButtonClicked, this));

	mCancelBtn = getChild<LLButton>("cancel_btn");
	mCancelBtn->setClickedCallback(boost::bind(&LLSidepanelInventorySubpanel::onCancelButtonClicked, this));
	return TRUE;
}

void LLSidepanelInventorySubpanel::setVisible(BOOL visible)
{
	if (visible)
	{
		dirty();
	}
	LLPanel::setVisible(visible);
}

void LLSidepanelInventorySubpanel::setIsEditing(BOOL edit)
{
	mIsEditing = edit;
	mIsDirty = TRUE;
}

BOOL LLSidepanelInventorySubpanel::getIsEditing() const
{

	return TRUE; // Default everything to edit mode since we're not using an edit button anymore.
	// return mIsEditing;
}

void LLSidepanelInventorySubpanel::reset()
{
	mIsDirty = TRUE;
}

void LLSidepanelInventorySubpanel::draw()
{
	if (mIsDirty)
	{
		refresh();
		updateVerbs();
		mIsDirty = FALSE;
	}

	LLPanel::draw();
}

void LLSidepanelInventorySubpanel::dirty()
{
	mIsDirty = TRUE;
	setIsEditing(FALSE);
}

void LLSidepanelInventorySubpanel::updateVerbs()
{
	mSaveBtn->setVisible(mIsEditing);
	mCancelBtn->setVisible(mIsEditing);
}

void LLSidepanelInventorySubpanel::onEditButtonClicked()
{
	setIsEditing(TRUE);
	refresh();
	updateVerbs();
}

void LLSidepanelInventorySubpanel::onSaveButtonClicked()
{
	save();
	setIsEditing(FALSE);
	refresh();
	updateVerbs();
}

void LLSidepanelInventorySubpanel::onCancelButtonClicked()
{
	setIsEditing(FALSE);
	refresh();
	updateVerbs();
}
