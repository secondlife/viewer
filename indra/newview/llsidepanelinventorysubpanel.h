/** 
 * @file llsidepanelinventorysubpanel.h
 * @brief A panel which shows an inventory item's properties.
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

#ifndef LL_LLSIDEPANELINVENTORYSUBPANEL_H
#define LL_LLSIDEPANELINVENTORYSUBPANEL_H

#include "llpanel.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLSidepanelInventorySubpanel
// Base class for inventory sidepanel panels (e.g. item info, task info).
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLButton;
class LLInventoryItem;

class LLSidepanelInventorySubpanel : public LLPanel
{
public:
	LLSidepanelInventorySubpanel(const LLPanel::Params& p = getDefaultParams());
	virtual ~LLSidepanelInventorySubpanel();

	/*virtual*/ void setVisible(BOOL visible);
	virtual BOOL postBuild();
	virtual void draw();
	virtual void reset();

	void dirty();
	void setIsEditing(BOOL edit);
protected:
	virtual void refresh() = 0;
	virtual void save() = 0;
	virtual void updateVerbs();
	
	BOOL getIsEditing() const;
	
	//
	// UI Elements
	// 
protected:
	void 						onEditButtonClicked();
	void 						onCancelButtonClicked();
	LLButton*					mCancelBtn;

private:
	BOOL mIsDirty; 		// item properties need to be updated
	BOOL mIsEditing; 	// if we're in edit mode
};

#endif // LL_LLSIDEPANELINVENTORYSUBPANEL_H
