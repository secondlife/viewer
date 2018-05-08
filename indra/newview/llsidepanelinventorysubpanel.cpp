/** 
 * @file llsidepanelinventorysubpanel.cpp
 * @brief A floater which shows an inventory item's properties.
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
LLSidepanelInventorySubpanel::LLSidepanelInventorySubpanel(const LLPanel::Params& p)
  : LLPanel(p),
	mIsDirty(TRUE),
	mIsEditing(FALSE),
	mCancelBtn(NULL)
{
}

// Destroys the object
LLSidepanelInventorySubpanel::~LLSidepanelInventorySubpanel()
{
}

// virtual
BOOL LLSidepanelInventorySubpanel::postBuild()
{

	mCancelBtn = findChild<LLButton>("cancel_btn");
	if (mCancelBtn)
	{
		mCancelBtn->setClickedCallback(boost::bind(&LLSidepanelInventorySubpanel::onCancelButtonClicked, this));
	}
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
	if (mCancelBtn)
	{
		mCancelBtn->setVisible(mIsEditing);
	}
}

void LLSidepanelInventorySubpanel::onEditButtonClicked()
{
	setIsEditing(TRUE);
	refresh();
	updateVerbs();
}

void LLSidepanelInventorySubpanel::onCancelButtonClicked()
{
	setIsEditing(FALSE);
	refresh();
	updateVerbs();
}
