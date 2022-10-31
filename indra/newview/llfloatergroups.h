/** 
 * @file llfloatergroups.h
 * @brief LLFloaterGroups class definition
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

/*
 * Shown from Edit -> Groups...
 * Shows the agent's groups and allows the edit window to be invoked.
 * Also overloaded to allow picking of a single group for assigning 
 * objects and land to groups.
 */

#ifndef LL_LLFLOATERGROUPS_H
#define LL_LLFLOATERGROUPS_H

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class llfloatergroups
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "lluuid.h"
#include "llfloater.h"
#include <map>
#include <boost/function.hpp>
#include <boost/signals2.hpp>

class LLUICtrl;
class LLTextBox;
class LLScrollListCtrl;
class LLButton;
class LLFloaterGroupPicker;

class LLFloaterGroupPicker : public LLFloater
{
public:
    LLFloaterGroupPicker(const LLSD& seed);
    ~LLFloaterGroupPicker();
    
    // Note: Don't return connection; use boost::bind + boost::signals2::trackable to disconnect slots
    typedef boost::signals2::signal<void (LLUUID id)> signal_t; 
    void setSelectGroupCallback(const signal_t::slot_type& cb) { mGroupSelectSignal.connect(cb); }
    void setPowersMask(U64 powers_mask);
    BOOL postBuild();

    // implementation of factory policy
    static LLFloaterGroupPicker* findInstance(const LLSD& seed);
    static LLFloaterGroupPicker* createInstance(const LLSD& seed);

    // for cases like inviting avatar to group we don't want the none option
    void removeNoneOption();

protected:
    void ok();
    static void onBtnOK(void* userdata);
    static void onBtnCancel(void* userdata);

protected:
    LLUUID mID;
    U64 mPowersMask;
    signal_t mGroupSelectSignal;

    typedef std::map<const LLUUID, LLFloaterGroupPicker*> instance_map_t;
    static instance_map_t sInstances;
};

class LLPanelGroups : public LLPanel, public LLOldEvents::LLSimpleListener
{
public:
    LLPanelGroups();
    virtual ~LLPanelGroups();

    //LLEventListener
    /*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
    
    // clear the group list, and get a fresh set of info.
    void reset();

protected:
    // initialize based on the type
    BOOL postBuild();

    // highlight_id is a group id to highlight
    void enableButtons();

    static void onGroupList(LLUICtrl* ctrl, void* userdata);
    static void onBtnCreate(void* userdata);
    static void onBtnActivate(void* userdata);
    static void onBtnInfo(void* userdata);
    static void onBtnIM(void* userdata);
    static void onBtnLeave(void* userdata);
    static void onBtnSearch(void* userdata);
    static void onBtnVote(void* userdata);
    static void onDoubleClickGroup(void* userdata);

    void create();
    void activate();
    void info();
    void startIM();
    void leave();
    void search();
    void callVote();

    static bool callbackLeaveGroup(const LLSD& notification, const LLSD& response);

};


#endif // LL_LLFLOATERGROUPS_H
