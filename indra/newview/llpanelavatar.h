/**
 * @file llpanelavatar.h
 * @brief Legacy profile panel base class
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

#ifndef LL_LLPANELAVATAR_H
#define LL_LLPANELAVATAR_H

#include "llpanel.h"
#include "llavatarpropertiesprocessor.h"
#include "llavatarnamecache.h"

class LLComboBox;
class LLLineEditor;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLProfileDropTarget
//
// This handy class is a simple way to drop something on another
// view. It handles drop events, always setting itself to the size of
// its parent.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLProfileDropTarget : public LLView
{
public:
    struct Params : public LLInitParam::Block<Params, LLView::Params>
    {
        Optional<LLUUID> agent_id;
        Params()
        :   agent_id("agent_id")
        {
            changeDefault(mouse_opaque, false);
            changeDefault(follows.flags, FOLLOWS_ALL);
        }
    };

    LLProfileDropTarget(const Params&);
    ~LLProfileDropTarget() {}

    //
    // LLView functionality
    virtual bool handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
                                   EDragAndDropType cargo_type,
                                   void* cargo_data,
                                   EAcceptance* accept,
                                   std::string& tooltip_msg);

    void setAgentID(const LLUUID &agent_id)     { mAgentID = agent_id; }

protected:
    LLUUID mAgentID;
};


/**
* Base class for any Profile View.
*/
class LLPanelProfileTab
    : public LLPanel
{
public:

    /**
     * Sets avatar ID, sets panel as observer of avatar related info replies from server.
     */
    virtual void setAvatarId(const LLUUID& avatar_id);

    /**
     * Returns avatar ID.
     */
    virtual const LLUUID& getAvatarId() const { return mAvatarId; }

    /**
     * Sends update data request to server.
     */
    virtual void updateData() {};

    /**
     * Clears panel data if viewing avatar info for first time and sends update data request.
     */
    virtual void onOpen(const LLSD& key) override;

    /**
     * Clears all data received from server.
     */
    virtual void resetData(){};

    /*virtual*/ ~LLPanelProfileTab();

protected:

    LLPanelProfileTab();

    enum ELoadingState
    {
        PROFILE_INIT,
        PROFILE_LOADING,
        PROFILE_LOADED,
    };


    // mLoading: false: Initial state, can request
    //           true:  Data requested, skip duplicate requests (happens due to LLUI's habit of repeated callbacks)
    // mLoaded:  false: Initial state, show loading indicator
    //           true:  Data recieved, which comes in a single message, hide indicator
    ELoadingState getLoadingState() { return mLoadingState; }
    virtual void setLoaded();
    void setApplyProgress(bool started);

    const bool getSelfProfile() const { return mSelfProfile; }

    bool saveAgentUserInfoCoro(std::string name, LLSD value, std::function<void(bool)> callback = nullptr) const;

public:
    void setIsLoading() { mLoadingState = PROFILE_LOADING; }
    void resetLoading() { mLoadingState = PROFILE_INIT; }

    bool getStarted() { return mLoadingState != PROFILE_INIT; }
    bool getIsLoaded() { return mLoadingState == PROFILE_LOADED; }

    virtual bool hasUnsavedChanges() { return false; }
    virtual void commitUnsavedChanges() {}

private:

    LLUUID  mAvatarId;
    ELoadingState    mLoadingState;
    bool    mSelfProfile;
};

class LLPanelProfilePropertiesProcessorTab
    : public LLPanelProfileTab
    , public LLAvatarPropertiesObserver
{
public:
    LLPanelProfilePropertiesProcessorTab();
    ~LLPanelProfilePropertiesProcessorTab();

    void setAvatarId(const LLUUID& avatar_id) override;

    void updateData() override;

    /**
     * Processes data received from server via LLAvatarPropertiesObserver.
     */
    virtual void processProperties(void* data, EAvatarProcessorType type) override = 0;
};

#endif // LL_LLPANELAVATAR_H
