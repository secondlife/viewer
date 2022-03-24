/** 
 * @file llpanelgroupcreate.h
 *
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2019, Linden Research, Inc.
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

#ifndef LL_LLPANELGROUPCREATE_H
#define LL_LLPANELGROUPCREATE_H

#include "llpanel.h"


// Forward declares
class LLButton;
class LLCheckBoxCtrl;
class LLComboBox;
class LLLineEditor;
class LLTextEditor;
class LLTextureCtrl;
class LLScrollListCtrl;
class LLSpinCtrl;


class LLPanelGroupCreate : public LLPanel
{
public:
    LLPanelGroupCreate();
    virtual ~LLPanelGroupCreate();

    virtual BOOL postBuild();

    void onOpen(const LLSD& key);

    static void refreshCreatedGroup(const LLUUID& group_id);

private:
    void addMembershipRow(const std::string &name);
    bool confirmMatureApply(const LLSD& notification, const LLSD& response);
    void onBtnCreate();
    void onBackBtnClick();
    void createGroup();

    LLComboBox       *mComboMature;
    LLButton         *mCreateButton;
    LLCheckBoxCtrl   *mCtrlOpenEnrollment;
    LLCheckBoxCtrl   *mCtrlEnrollmentFee;
    LLTextEditor     *mEditCharter;
    LLTextureCtrl    *mInsignia;
    LLLineEditor     *mGroupNameEditor;
    LLScrollListCtrl *mMembershipList;
    LLSpinCtrl       *mSpinEnrollmentFee;
};

#endif // LL_LLPANELGROUPCREATE_H
