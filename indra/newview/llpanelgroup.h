/**
 * @file llpanelgroup.h
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

#ifndef LL_LLPANELGROUP_H
#define LL_LLPANELGROUP_H

#include "llgroupmgr.h"
#include "llpanel.h"
#include "lltimer.h"
#include "llvoiceclient.h"

class LLOfferInfo;

const F32 UPDATE_MEMBERS_SECONDS_PER_FRAME = 0.005f; // 5ms

// Forward declares
class LLPanelGroupTab;
class LLTabContainer;
class LLAccordionCtrl;
class LLAgent;


class LLPanelGroup : public LLPanel,
                     public LLGroupMgrObserver,
                     public LLVoiceClientStatusObserver
{
public:
    LLPanelGroup();
    virtual ~LLPanelGroup();

    virtual bool postBuild();

    void setGroupID(const LLUUID& group_id);

    void draw();

    void onOpen(const LLSD& key);

    // Group manager observer trigger.
    virtual void changed(LLGroupChange gc);

    // Implements LLVoiceClientStatusObserver::onChange() to enable the call
    // button when voice is available
    /*virtual*/ void onChange(EStatusType status, const LLSD& channelInfo, bool proximal);

    void showNotice(const std::string& subject,
                    const std::string& message,
                    const bool& has_inventory,
                    const std::string& inventory_name,
                    LLOfferInfo* inventory_offer);

    void notifyObservers();

    bool apply();
    void refreshData();
    void callGroup();
    void chatGroup();

    virtual void reshape(S32 width, S32 height, bool called_from_parent = true);

    static void showNotice(const std::string& subject,
                           const std::string& message,
                           const LLUUID& group_id,
                           const bool& has_inventory,
                           const std::string& inventory_name,
                           LLOfferInfo* inventory_offer);


protected:
    virtual void update(LLGroupChange gc);

    void onBackBtnClick();
    void onBtnJoin();

    static void onBtnApply(void*);
    static void onBtnRefresh(void*);
    static void onBtnGroupCallClicked(void*);
    static void onBtnGroupChatClicked(void*);

    void reposButton(LLButton* button);
    void reposButtons();


protected:
    bool    apply(LLPanelGroupTab* tab);

    LLTimer mRefreshTimer;

    bool mSkipRefresh;

    std::string mDefaultNeedsApplyMesg;
    std::string mWantApplyMesg;

    std::vector<LLPanelGroupTab* > mTabs;

    LLAccordionCtrl* mGroupsAccordion = nullptr;

    LLUICtrl*       mGroupNameCtrl = nullptr;
    LLButton*       mButtonJoin = nullptr;
    LLButton*       mButtonApply = nullptr;
    LLButton*       mButtonCall = nullptr;
    LLButton*       mButtonChat = nullptr;
    LLButton*       mButtonRefresh = nullptr;
    LLUICtrl*       mJoinText;
};

class LLPanelGroupTab : public LLPanel
{
public:
    LLPanelGroupTab();
    virtual ~LLPanelGroupTab();

    // Triggered when the tab becomes active.
    virtual void activate() { }

    // Triggered when the tab becomes inactive.
    virtual void deactivate() { }

    // Asks if something needs to be applied.
    // If returning true, this function should modify the message to the user.
    virtual bool needsApply(std::string& mesg) { return false; }

    // Asks if there is currently a modal dialog being shown.
    virtual bool hasModal() { return mHasModal; }

    // Request to apply current data.
    // If returning fail, this function should modify the message to the user.
    virtual bool apply(std::string& mesg) { return true; }

    // Request a cancel of changes
    virtual void cancel() { }

    // Triggered when group information changes in the group manager.
    virtual void update(LLGroupChange gc) { }

    // This just connects the help button callback.
    virtual bool postBuild();

    virtual bool isVisibleByAgent(LLAgent* agentp);

    virtual void setGroupID(const LLUUID& id) {mGroupID = id;};

    void notifyObservers() {};

    const LLUUID& getGroupID() const { return mGroupID;}

    virtual void setupCtrls (LLPanel* parent) {};

    virtual void onFilterChanged() { }

protected:
    LLUUID  mGroupID;
    bool mAllowEdit;
    bool mHasModal;
};

#endif // LL_LLPANELGROUP_H
