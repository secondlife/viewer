/**
 * @file llpanelgroup.cpp
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llpanelgroup.h"

// Library includes
#include "llbutton.h"
#include "llfloatersidepanelcontainer.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"

// Viewer includes
#include "llviewermessage.h"
#include "llviewerwindow.h"
#include "llappviewer.h"
#include "llnotificationsutil.h"
#include "llfloaterreg.h"
#include "llfloater.h"
#include "llgroupactions.h"

#include "llagent.h"

#include "llsidetraypanelcontainer.h"

#include "llpanelgroupnotices.h"
#include "llpanelgroupgeneral.h"
#include "llpanelgrouproles.h"

#include "llaccordionctrltab.h"
#include "llaccordionctrl.h"

#include "lltrans.h"

static LLPanelInjector<LLPanelGroup> t_panel_group("panel_group_info_sidetray");



LLPanelGroupTab::LLPanelGroupTab()
    : LLPanel(),
      mAllowEdit(true),
      mHasModal(false)
{
    mGroupID = LLUUID::null;
}

LLPanelGroupTab::~LLPanelGroupTab()
{
}

bool LLPanelGroupTab::isVisibleByAgent(LLAgent* agentp)
{
    //default to being visible
    return true;
}

bool LLPanelGroupTab::postBuild()
{
    return true;
}

LLPanelGroup::LLPanelGroup()
:   LLPanel(),
    LLGroupMgrObserver( LLUUID() ),
    mSkipRefresh(false),
    mButtonJoin(NULL)
{
    // Set up the factory callbacks.
    // Roles sub tabs
    LLGroupMgr::getInstance()->addObserver(this);
}


LLPanelGroup::~LLPanelGroup()
{
    LLGroupMgr::getInstance()->removeObserver(this);
    LLVoiceClient::removeObserver(this);
}

void LLPanelGroup::onOpen(const LLSD& key)
{
    if(!key.has("group_id"))
        return;

    LLUUID group_id = key["group_id"];
    if(!key.has("action"))
    {
        setGroupID(group_id);
        mGroupsAccordion->expandDefaultTab();
        return;
    }

    std::string str_action = key["action"];

    if(str_action == "refresh")
    {
        if(mID == group_id || group_id == LLUUID::null)
            refreshData();
    }
    else if(str_action == "close")
    {
        onBackBtnClick();
    }
    else if(str_action == "refresh_notices")
    {
        LLPanelGroupNotices* panel_notices = findChild<LLPanelGroupNotices>("group_notices_tab_panel");
        if(panel_notices)
            panel_notices->refreshNotices();
    }
    if (str_action == "show_notices")
    {
        setGroupID(group_id);

        LLAccordionCtrl *tab_ctrl = getChild<LLAccordionCtrl>("groups_accordion");
        tab_ctrl->collapseAllTabs();
        getChild<LLAccordionCtrlTab>("group_notices_tab")->setDisplayChildren(true);
        tab_ctrl->arrange();
    }

}

bool LLPanelGroup::postBuild()
{
    mGroupsAccordion = getChild<LLAccordionCtrl>("groups_accordion");

    mDefaultNeedsApplyMesg = getString("default_needs_apply_text");
    mWantApplyMesg = getString("want_apply_text");

    mButtonApply = getChild<LLButton>("btn_apply");
    mButtonApply->setClickedCallback(onBtnApply, this);
    mButtonApply->setVisible(true);
    mButtonApply->setEnabled(false);

    mButtonCall = getChild<LLButton>("btn_call");
    mButtonCall->setClickedCallback(onBtnGroupCallClicked, this);

    mButtonChat = getChild<LLButton>("btn_chat");
    mButtonChat->setClickedCallback(onBtnGroupChatClicked, this);

    mButtonRefresh = getChild<LLButton>("btn_refresh");
    mButtonRefresh->setClickedCallback(onBtnRefresh, this);

    mGroupNameCtrl = getChild<LLUICtrl>("group_name");

    childSetCommitCallback("back",boost::bind(&LLPanelGroup::onBackBtnClick,this),NULL);

    LLPanelGroupTab* panel_general = findChild<LLPanelGroupTab>("group_general_tab_panel");
    LLPanelGroupTab* panel_roles = findChild<LLPanelGroupTab>("group_roles_tab_panel");
    LLPanelGroupTab* panel_notices = findChild<LLPanelGroupTab>("group_notices_tab_panel");
    LLPanelGroupTab* panel_land = findChild<LLPanelGroupTab>("group_land_tab_panel");
    LLPanelGroupTab* panel_experiences = findChild<LLPanelGroupTab>("group_experiences_tab_panel");

    if(panel_general)   mTabs.push_back(panel_general);
    if(panel_roles)     mTabs.push_back(panel_roles);
    if(panel_notices)   mTabs.push_back(panel_notices);
    if(panel_land)      mTabs.push_back(panel_land);
    if(panel_experiences)       mTabs.push_back(panel_experiences);

    if(panel_general)
    {
        panel_general->setupCtrls(this);
        LLButton* button = panel_general->getChild<LLButton>("btn_join");
        button->setVisible(false);
        button->setEnabled(true);

        mButtonJoin = button;
        mButtonJoin->setCommitCallback(boost::bind(&LLPanelGroup::onBtnJoin,this));

        mJoinText = panel_general->getChild<LLUICtrl>("join_cost_text");
    }

    LLVoiceClient::addObserver(this);

    return true;
}

void LLPanelGroup::reposButton(LLButton* button)
{
    if(!button)
        return;
    LLRect btn_rect = button->getRect();
    btn_rect.setLeftTopAndSize( btn_rect.mLeft, btn_rect.getHeight() + 2, btn_rect.getWidth(), btn_rect.getHeight());
    button->setRect(btn_rect);
}

void LLPanelGroup::reposButtons()
{
    reposButton(mButtonApply);
    reposButton(mButtonRefresh);
    reposButton(mButtonChat);
    reposButton(mButtonCall);
}

void LLPanelGroup::reshape(S32 width, S32 height, bool called_from_parent )
{
    LLPanel::reshape(width, height, called_from_parent );

    reposButtons();
}

void LLPanelGroup::onBackBtnClick()
{
    LLSideTrayPanelContainer* parent = dynamic_cast<LLSideTrayPanelContainer*>(getParent());
    if(parent)
    {
        parent->openPreviousPanel();
    }
}

void LLPanelGroup::onBtnRefresh(void* user_data)
{
    LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
    self->refreshData();
}

void LLPanelGroup::onBtnApply(void* user_data)
{
    LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
    self->apply();
    self->refreshData();
}

void LLPanelGroup::onBtnGroupCallClicked(void* user_data)
{
    LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
    self->callGroup();
}

void LLPanelGroup::onBtnGroupChatClicked(void* user_data)
{
    LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
    self->chatGroup();
}

void LLPanelGroup::onBtnJoin()
{
    if (LLGroupActions::isInGroup(mID))
    {
        LLGroupActions::leave(mID);
    }
    else
    {
        LL_DEBUGS() << "joining group: " << mID << LL_ENDL;
        LLGroupActions::join(mID);
    }
}

void LLPanelGroup::changed(LLGroupChange gc)
{
    for(std::vector<LLPanelGroupTab* >::iterator it = mTabs.begin();it!=mTabs.end();++it)
        (*it)->update(gc);
    update(gc);
}

// virtual
void LLPanelGroup::onChange(EStatusType status, const LLSD& channelInfo, bool proximal)
{
    if(status == STATUS_JOINING || status == STATUS_LEFT_CHANNEL)
    {
        return;
    }

    mButtonCall->setEnabled(LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking());
}

void LLPanelGroup::notifyObservers()
{
    changed(GC_ALL);
}

void LLPanelGroup::update(LLGroupChange gc)
{
    LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mID);
    if(gdatap)
    {
        std::string group_name =  gdatap->mName.empty() ? LLTrans::getString("LoadingData") : gdatap->mName;
        mGroupNameCtrl->setValue(group_name);
        mGroupNameCtrl->setToolTip(group_name);

        LLGroupData agent_gdatap;
        bool is_member = gAgent.getGroupData(mID,agent_gdatap) || gAgent.isGodlikeWithoutAdminMenuFakery();
        bool join_btn_visible = is_member || gdatap->mOpenEnrollment;

        mButtonJoin->setVisible(join_btn_visible);
        mJoinText->setVisible(join_btn_visible);

        if (is_member)
        {
            mJoinText->setValue(getString("group_member"));
            mButtonJoin->setLabel(getString("leave_txt"));
        }
        else if(join_btn_visible)
        {
            LLStringUtil::format_map_t string_args;
            std::string fee_buff;
            if(gdatap->mMembershipFee)
            {
                string_args["[AMOUNT]"] = llformat("%d", gdatap->mMembershipFee);
                fee_buff = getString("group_join_btn", string_args);

            }
            else
            {
                fee_buff = getString("group_join_free", string_args);
            }
            mJoinText->setValue(fee_buff);
            mButtonJoin->setLabel(getString("join_txt"));
        }
    }
}

void LLPanelGroup::setGroupID(const LLUUID& group_id)
{
    std::string str_group_id;
    group_id.toString(str_group_id);

    bool is_same_id = group_id == mID;

    LLGroupMgr::getInstance()->removeObserver(this);
    mID = group_id;
    LLGroupMgr::getInstance()->addObserver(this);

    for(std::vector<LLPanelGroupTab* >::iterator it = mTabs.begin();it!=mTabs.end();++it)
        (*it)->setGroupID(group_id);

    LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mID);
    if(gdatap)
    {
        std::string group_name =  gdatap->mName.empty() ? LLTrans::getString("LoadingData") : gdatap->mName;
        mGroupNameCtrl->setValue(group_name);
        mGroupNameCtrl->setToolTip(group_name);
    }

    bool is_null_group_id = group_id == LLUUID::null;
    if(mButtonApply)
        mButtonApply->setVisible(!is_null_group_id);
    if(mButtonRefresh)
        mButtonRefresh->setVisible(!is_null_group_id);

    if(mButtonCall)
            mButtonCall->setVisible(!is_null_group_id);
    if(mButtonChat)
            mButtonChat->setVisible(!is_null_group_id);

    getChild<LLUICtrl>("prepend_founded_by")->setVisible(!is_null_group_id);

    mGroupsAccordion->reset();

    LLAccordionCtrlTab* tab_general = getChild<LLAccordionCtrlTab>("group_general_tab");
    LLAccordionCtrlTab* tab_roles = getChild<LLAccordionCtrlTab>("group_roles_tab");
    LLAccordionCtrlTab* tab_notices = getChild<LLAccordionCtrlTab>("group_notices_tab");
    LLAccordionCtrlTab* tab_land = getChild<LLAccordionCtrlTab>("group_land_tab");
    LLAccordionCtrlTab* tab_experiences = getChild<LLAccordionCtrlTab>("group_experiences_tab");

    if(mButtonJoin)
        mButtonJoin->setVisible(false);


    if(is_null_group_id)//creating new group
    {
        if(!tab_general->getDisplayChildren())
            tab_general->changeOpenClose(tab_general->getDisplayChildren());

        if(tab_roles->getDisplayChildren())
            tab_roles->changeOpenClose(tab_roles->getDisplayChildren());
        if(tab_notices->getDisplayChildren())
            tab_notices->changeOpenClose(tab_notices->getDisplayChildren());
        if(tab_land->getDisplayChildren())
            tab_land->changeOpenClose(tab_land->getDisplayChildren());
        if(tab_experiences->getDisplayChildren())
            tab_experiences->changeOpenClose(tab_land->getDisplayChildren());

        tab_roles->setVisible(false);
        tab_notices->setVisible(false);
        tab_land->setVisible(false);
        tab_experiences->setVisible(false);

        mGroupNameCtrl->setVisible(false);
        getChild<LLUICtrl>("group_name_editor")->setVisible(true);

        if(mButtonCall)
            mButtonCall->setVisible(false);
        if(mButtonChat)
            mButtonChat->setVisible(false);
    }
    else
    {
        if(!is_same_id)
        {
            if(!tab_general->getDisplayChildren())
                tab_general->changeOpenClose(tab_general->getDisplayChildren());
            if(tab_roles->getDisplayChildren())
                tab_roles->changeOpenClose(tab_roles->getDisplayChildren());
            if(tab_notices->getDisplayChildren())
                tab_notices->changeOpenClose(tab_notices->getDisplayChildren());
            if(tab_land->getDisplayChildren())
                tab_land->changeOpenClose(tab_land->getDisplayChildren());
            if(tab_experiences->getDisplayChildren())
                tab_experiences->changeOpenClose(tab_land->getDisplayChildren());
        }

        LLGroupData agent_gdatap;
        bool is_member = gAgent.getGroupData(mID,agent_gdatap) || gAgent.isGodlikeWithoutAdminMenuFakery();

        tab_roles->setVisible(is_member);
        tab_notices->setVisible(is_member);
        tab_land->setVisible(is_member);
        tab_experiences->setVisible(is_member);

        mGroupNameCtrl->setVisible(true);
        getChild<LLUICtrl>("group_name_editor")->setVisible(false);

        if(mButtonApply)
            mButtonApply->setVisible(is_member);
        if(mButtonCall)
            mButtonCall->setVisible(is_member);
        if(mButtonChat)
            mButtonChat->setVisible(is_member);
    }

    mGroupsAccordion->arrange();

    reposButtons();
    update(GC_ALL);//show/hide "join" button if data is already ready
}

bool LLPanelGroup::apply(LLPanelGroupTab* tab)
{
    if(!tab)
        return false;

    std::string mesg;
    if ( !tab->needsApply(mesg) )
        return true;

    std::string apply_mesg;
    if(tab->apply( apply_mesg ) )
    {
        //we skip refreshing group after ew manually apply changes since its very annoying
        //for those who are editing group

        LLPanelGroupRoles * roles_tab = dynamic_cast<LLPanelGroupRoles*>(tab);
        if (roles_tab)
        {
            LLGroupMgr* gmgrp = LLGroupMgr::getInstance();
            LLGroupMgrGroupData* gdatap = gmgrp->getGroupData(roles_tab->getGroupID());

            // allow refresh only for one specific case:
            // there is only one member in group and it is not owner
            // it's a wrong situation and need refresh panels from server
            if (gdatap && gdatap->isSingleMemberNotOwner())
            {
                return true;
            }
        }

        mSkipRefresh = true;
        return true;
    }

    if ( !apply_mesg.empty() )
    {
        LLSD args;
        args["MESSAGE"] = apply_mesg;
        LLNotificationsUtil::add("GenericAlert", args);
    }
    return false;
}

bool LLPanelGroup::apply()
{
    return apply(findChild<LLPanelGroupTab>("group_general_tab_panel"))
        && apply(findChild<LLPanelGroupTab>("group_roles_tab_panel"))
        && apply(findChild<LLPanelGroupTab>("group_notices_tab_panel"))
        && apply(findChild<LLPanelGroupTab>("group_land_tab_panel"))
        && apply(findChild<LLPanelGroupTab>("group_experiences_tab_panel"))
        ;
}


// virtual
void LLPanelGroup::draw()
{
    LLPanel::draw();

    if (mRefreshTimer.hasExpired())
    {
        mRefreshTimer.stop();
        if(mButtonRefresh) mButtonRefresh->setEnabled(true);
        mGroupsAccordion->setEnabled(true);
    }

    if(mButtonApply && mButtonApply->getVisible())
    {
        bool enable = false;
        std::string mesg;
        for(std::vector<LLPanelGroupTab* >::iterator it = mTabs.begin();it!=mTabs.end();++it)
            enable = enable || (*it)->needsApply(mesg);

        mButtonApply->setEnabled(enable);
    }
}

void LLPanelGroup::refreshData()
{
    if(mSkipRefresh)
    {
        mSkipRefresh = false;
        return;
    }
    LLGroupMgr::getInstance()->clearGroupData(getID());

    setGroupID(getID());

    // 5 second timeout
    if(mButtonRefresh) mButtonRefresh->setEnabled(false);
    mGroupsAccordion->setEnabled(false);

    mRefreshTimer.start();
    mRefreshTimer.setTimerExpirySec(5);
}

void LLPanelGroup::callGroup()
{
    LLGroupActions::startCall(getID());
}

void LLPanelGroup::chatGroup()
{
    LLGroupActions::startIM(getID());
}

void LLPanelGroup::showNotice(const std::string& subject,
                  const std::string& message,
                  const bool& has_inventory,
                  const std::string& inventory_name,
                  LLOfferInfo* inventory_offer)
{
    LLPanelGroupNotices* panel_notices = findChild<LLPanelGroupNotices>("group_notices_tab_panel");
    if(!panel_notices)
    {
        // We need to clean up that inventory offer.
        if (inventory_offer)
        {
            inventory_offer->forceResponse(IOR_DECLINE);
        }
        return;
    }
    panel_notices->showNotice(subject,message,has_inventory,inventory_name,inventory_offer);
}

//static

void LLPanelGroup::showNotice(const std::string& subject,
                       const std::string& message,
                       const LLUUID& group_id,
                       const bool& has_inventory,
                       const std::string& inventory_name,
                       LLOfferInfo* inventory_offer)
{
    LLPanelGroup* panel = LLFloaterSidePanelContainer::getPanel<LLPanelGroup>("people", "panel_group_info_sidetray");
    if(!panel)
        return;

    if(panel->getID() != group_id)//???? only for current group_id or switch panels? FIXME
        return;
    panel->showNotice(subject,message,has_inventory,inventory_name,inventory_offer);

}


