/**
* @file llpanelexperiencelisteditor.cpp
* @brief Editor for building a list of experiences
*
* $LicenseInfo:firstyear=2014&license=viewerlgpl$
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

#ifndef LL_LLPANELEXPERIENCELISTEDITOR_H
#define LL_LLPANELEXPERIENCELISTEDITOR_H

#include "llpanel.h"
#include "lluuid.h"
#include <set>

class LLNameListCtrl;
class LLScrollListCtrl;
class LLButton;
class LLFloaterExperiencePicker;

class LLPanelExperienceListEditor : public LLPanel
{
public:

    typedef boost::signals2::signal<void (const LLUUID&) > list_changed_signal_t;
    // filter function for experiences, return true if the experience should be hidden.
    typedef boost::function<bool (const LLSD&)> experience_function;
    typedef std::vector<experience_function> filter_list;
    typedef LLHandle<LLFloaterExperiencePicker> PickerHandle;
    LLPanelExperienceListEditor();
    ~LLPanelExperienceListEditor();
    BOOL postBuild();

    void loading();

    const uuid_list_t& getExperienceIds()const;
    void setExperienceIds(const LLSD& experience_ids);
    void addExperienceIds(const uuid_vec_t& experience_ids);

    void addExperience(const LLUUID& id);

    boost::signals2::connection setAddedCallback(list_changed_signal_t::slot_type cb );
    boost::signals2::connection setRemovedCallback(list_changed_signal_t::slot_type cb );

    bool getReadonly() const { return mReadonly; }
    void setReadonly(bool val);

    void refreshExperienceCounter();

    void addFilter(experience_function func){mFilters.push_back(func);}
    void setStickyFunction(experience_function func){mSticky = func;}
    U32 getMaxExperienceIDs() const { return mMaxExperienceIDs; }
    void setMaxExperienceIDs(U32 val) { mMaxExperienceIDs = val; }
private:

    void onItems();
    void onRemove();
    void onAdd();
    void onProfile();

    void checkButtonsEnabled();
    static void experienceDetailsCallback( LLHandle<LLPanelExperienceListEditor> panel, const LLSD& experience );
    void onExperienceDetails( const LLSD& experience );
    void processResponse( const LLSD& content );
    uuid_list_t mExperienceIds;


    LLNameListCtrl*             mItems;
    filter_list                 mFilters;
    LLButton*                   mAdd;
    LLButton*                   mRemove;
    LLButton*                   mProfile;
    PickerHandle                mPicker;
    list_changed_signal_t       mAddedCallback;
    list_changed_signal_t       mRemovedCallback;
    LLUUID                      mKey;
    bool                        mReadonly;
    experience_function         mSticky;
    U32                         mMaxExperienceIDs;

};

#endif //LL_LLPANELEXPERIENCELISTEDITOR_H
