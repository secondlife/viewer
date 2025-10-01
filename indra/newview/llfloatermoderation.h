/**
 * @file llfloatermoderation.h
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#pragma once
#include "llfloater.h"
#include "llavatarpropertiesprocessor.h"

#include <vector>

class LLScrollListCtrl;
class LLUUID;

extern LLControlGroup gSavedSettings;

class LLFloaterModeration :
    public LLAvatarPropertiesObserver,
    public LLFloater {
        friend class LLFloaterReg;

    public:
        virtual void refresh() override;

    private:
        LLFloaterModeration(const LLSD& key);
        ~LLFloaterModeration();
        bool postBuild() override;
        void draw() override;

        // Sort the list of residents based on loudness (high to low)
        void sortListByLoudness();

        // Trim the list of residents (in case we just want a subset of residents)
        void trimList(size_t final_size);

        // Refresh the list with resident data
        void refreshList();

        // Using the list of residents we collected, refresh the UI
        void refreshUI();

        typedef struct list_element
        {
            LLUUID id;
            double distance;
            std::string name;
            bool is_linden;
            bool is_voice_muted;
            LLDate born_on;
            int recent_loudness;

        } list_elem_t;

        std::vector < struct list_element* > mResidentList;
        LLScrollListCtrl* mResidentListScroller;
        LLUICtrl* mShowProfileBtn;
        LLUICtrl* mTrackResidentBtn;
        LLUICtrl* mCloseBtn;
        LLUICtrl* mRefreshListBtn;
        LLUICtrl* mSelectAllBtn;
        LLUICtrl* mSelectNoneBtn;
        LLUICtrl* mMuteResidentsBtn;
        LLUICtrl* mUnmuteResidentsBtn;

        void onRefreshList();

        // Utility function to get the avatar ID from the selected item
        // in the list. A list item has the concept of userdata but the
        // crazy casts needed to convert a block of LLSD to void* and
        // back meant I figured it's easier to add them to the list and
        // not display them.
        LLUUID getSelectedAvatarId();

        // Triggered when a user double-clicks on an item in the list
        static void onDoubleClickListItem(void* data);

        // Triggered when a profile is selected to view
        void openSelectedProfile();

        // Triggered when the tracking mechanism to show the target
        // resident/avatar on the world map
        void trackResidentPosition();

        // Mute and unmute the selected resident(s)
        void muteResidents();
        void unmuteResidents();

        // Find an (VO)Avatar from a specified ID
        LLVOAvatar* getAvatarFromId(const LLUUID& id);

        // Inelegant way to check if a user is a Linden but it's all we have
        bool isLinden(const LLUUID& av_id);

        // Determine how loud this person has been in the past
        // (If we can figure it out, loudness score will appear in UI
        // and can be sorted so moderators can find noisy, disruptive people
        int getRecentLoudness(const LLUUID& av_id);

        // Column ids/values for the primary scrolling list
        enum EListColumnNum
        {
            ID = 0,
            ROWNUM = 1,
            NAME = 2,
            ACCOUNT_AGE = 3,
            DISTANCE = 4,
            LINDEN = 5,
            VOICE_MUTED = 6,
            RECENT_LOUDNESS = 7,
        };

        // strings we use in several places
        const std::string mScrollListFontFace = "OCRA";

        // For testing without having to find busy regions
        void addDummyResident(const std::string name);

        // virtual override for asynchronous avatar information callback
        void processProperties(void* data, EAvatarProcessorType type) override;

        // apply an action to the selected residents
        enum EResidentAction
        {
            MUTE = 0,
            UNMUTE = 1
        };
        void applyActionSelectedResidents(EResidentAction action);
};

// Simple helper class to perform the mute/unmute actions - broken out
// into a separate class even though it's accessed from the moderation
// UI floater (should maybe be a "Simpleton" vs "Singleton" so that access
// from other parts of the code is possible without polluting 
class LLNearbyVoiceMuteHelper : public LLSingleton < LLNearbyVoiceMuteHelper > {
        LLSINGLETON(LLNearbyVoiceMuteHelper) {};
        ~LLNearbyVoiceMuteHelper() {};

    public:
        // make a CAP request to mute or unmute
        void requestMuteChange(LLVOAvatar* avatar, bool mute);
};
