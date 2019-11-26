/** 
 * @file llpanelgroupcreate.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llpanelgroupcreate.h"

// UI includes
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfloatersidepanelcontainer.h"
#include "llsidetraypanelcontainer.h"
#include "llscrolllistctrl.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lluictrlfactory.h"

// Viewer includes
#include "llagentbenefits.h"
#include "llfloaterreg.h"
#include "llfloater.h"
#include "llgroupmgr.h"
#include "lltrans.h"
#include "llnotificationsutil.h"
#include "lluicolortable.h"


const S32 MATURE_CONTENT = 1;
const S32 NON_MATURE_CONTENT = 2;
const S32 DECLINE_TO_STATE = 0;

static LLPanelInjector<LLPanelGroupCreate> t_panel_group_creation("panel_group_creation_sidetray");

LLPanelGroupCreate::LLPanelGroupCreate()
:    LLPanel()
{
}

LLPanelGroupCreate::~LLPanelGroupCreate()
{
}

BOOL LLPanelGroupCreate::postBuild()
{
    childSetCommitCallback("back", boost::bind(&LLPanelGroupCreate::onBackBtnClick, this), NULL);

    mComboMature = getChild<LLComboBox>("group_mature_check", TRUE);
    mCtrlOpenEnrollment = getChild<LLCheckBoxCtrl>("open_enrollement", TRUE);
    mCtrlEnrollmentFee = getChild<LLCheckBoxCtrl>("check_enrollment_fee", TRUE);
    mEditCharter = getChild<LLTextEditor>("charter", TRUE);
    mSpinEnrollmentFee = getChild<LLSpinCtrl>("spin_enrollment_fee", TRUE);
    mMembershipList = getChild<LLScrollListCtrl>("membership_list", TRUE);

    mCreateButton = getChild<LLButton>("btn_create", TRUE);
    mCreateButton->setCommitCallback(boost::bind(&LLPanelGroupCreate::onBtnCreate, this));

    mGroupNameEditor = getChild<LLLineEditor>("group_name_editor", TRUE);
    mGroupNameEditor->setPrevalidate(LLTextValidate::validateASCIINoLeadingSpace);

    mInsignia = getChild<LLTextureCtrl>("insignia", TRUE);
    mInsignia->setAllowLocalTexture(FALSE);
    mInsignia->setCanApplyImmediately(FALSE);

    return TRUE;
}

void LLPanelGroupCreate::onOpen(const LLSD& key)
{
    mInsignia->setImageAssetID(LLUUID::null);
    mInsignia->setImageAssetName(mInsignia->getDefaultImageName());
    mGroupNameEditor->clear();
    mEditCharter->clear();
    mSpinEnrollmentFee->set(0.f);
    mCtrlEnrollmentFee->set(FALSE);
    mCtrlOpenEnrollment->set(FALSE);
    mMembershipList->clearRows();

    // populate list
    addMembershipRow("Base");
    addMembershipRow("Premium");
    addMembershipRow("Premium Plus");
    addMembershipRow("Internal");// Present only if you are already in one, needed for testing

    S32 cost = LLAgentBenefitsMgr::current().getCreateGroupCost();
    mCreateButton->setLabelArg("[COST]", llformat("%d", cost));
}

//static
void LLPanelGroupCreate::refreshCreatedGroup(const LLUUID& group_id)
{
    LLSD params;
    params["group_id"] = group_id;
    params["open_tab_name"] = "panel_group_info_sidetray";
    LLFloaterSidePanelContainer::showPanel("people", "panel_group_info_sidetray", params);
}

void LLPanelGroupCreate::addMembershipRow(const std::string &name)
{
    if (LLAgentBenefitsMgr::has(name))
    {
        bool is_current = LLAgentBenefitsMgr::isCurrent(name);

        LLScrollListItem::Params item_params;
        LLScrollListCell::Params cell_params;
        cell_params.font = LLFontGL::getFontSansSerif();
        // Start out right justifying numeric displays
        cell_params.font_halign = LLFontGL::LEFT;
        if (is_current)
        {
            cell_params.color = LLUIColorTable::instance().getColor("DrYellow");
        }

        cell_params.column = "clmn_name";
        std::string mem_str = name + "Membership";
        if (is_current)
        {
            cell_params.value = LLTrans::getString(mem_str) + " " + getString("current_membership");
        }
        else
        {
            cell_params.value = LLTrans::getString(mem_str);
        }
        item_params.columns.add(cell_params);
        cell_params.column = "clmn_price";
        cell_params.value = llformat("L$ %d",LLAgentBenefitsMgr::get(name).getCreateGroupCost());
        item_params.columns.add(cell_params);
        mMembershipList->addRow(item_params);
    }
}

void LLPanelGroupCreate::onBackBtnClick()
{
    LLSideTrayPanelContainer* parent = dynamic_cast<LLSideTrayPanelContainer*>(getParent());
    if(parent)
    {
        parent->openPreviousPanel();
    }
}

bool LLPanelGroupCreate::confirmMatureApply(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    // 0 == Yes
    // 1 == No
    // 2 == Cancel
    switch (option)
    {
    case 0:
        mComboMature->setCurrentByIndex(MATURE_CONTENT);
        createGroup();
        break;
    case 1:
        mComboMature->setCurrentByIndex(NON_MATURE_CONTENT);
        createGroup();
        break;
    default:
        break;
    }

    return true;
}

void LLPanelGroupCreate::onBtnCreate()
{
    LL_INFOS() << "Validating group creation" << LL_ENDL;

    // Validate the group name length.
    std::string gr_name = mGroupNameEditor->getText();
    LLStringUtil::trim(gr_name);
    S32 group_name_len = gr_name.size();
    if (group_name_len < DB_GROUP_NAME_MIN_LEN
        || group_name_len > DB_GROUP_NAME_STR_LEN)
    {
        LLSD args;
        args["MIN_LEN"] = DB_GROUP_NAME_MIN_LEN;
        args["MAX_LEN"] = DB_GROUP_NAME_STR_LEN;
        LLNotificationsUtil::add("GroupNameLengthWarning", args);
    }
    else
    // Check to make sure mature has been set
    if (mComboMature &&
        mComboMature->getCurrentIndex() == DECLINE_TO_STATE)
    {
        LLNotificationsUtil::add("SetGroupMature", LLSD(), LLSD(),
            boost::bind(&LLPanelGroupCreate::confirmMatureApply, this, _1, _2));
    }
    else
    {
        createGroup();
    }
}

void LLPanelGroupCreate::createGroup()
{
    LL_INFOS() << "Creating group" << LL_ENDL;

    U32 enrollment_fee = (mCtrlEnrollmentFee->get() ?
        (U32)mSpinEnrollmentFee->get() : 0);
    LLUUID insignia_id = mInsignia->getImageItemID().isNull() ? LLUUID::null : mInsignia->getImageAssetID();

    std::string gr_name = mGroupNameEditor->getText();
    LLStringUtil::trim(gr_name);
    LLGroupMgr::getInstance()->sendCreateGroupRequest(gr_name,
        mEditCharter->getText(),
        true,
        insignia_id,
        enrollment_fee,
        mCtrlOpenEnrollment->get(),
        false,
        mComboMature->getCurrentIndex() == MATURE_CONTENT);
}

