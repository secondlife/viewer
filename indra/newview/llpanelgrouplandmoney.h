/** 
 * @file llpanelgrouplandmoney.h
 * @brief Panel for group land and L$.
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

#ifndef LL_PANEL_GROUP_LAND_MONEY_H
#define LL_PANEL_GROUP_LAND_MONEY_H

#include "llpanelgroup.h"
#include <map>
#include "lluuid.h"

class LLPanelGroupLandMoney : public LLPanelGroupTab
{
public:
    LLPanelGroupLandMoney();
    virtual ~LLPanelGroupLandMoney();
    virtual BOOL postBuild();
    virtual BOOL isVisibleByAgent(LLAgent* agentp);

    virtual void activate();
    virtual bool needsApply(std::string& mesg);
    virtual bool apply(std::string& mesg);
    virtual void cancel();
    virtual void update(LLGroupChange gc);

    static void processPlacesReply(LLMessageSystem* msg, void**);

    typedef std::map<LLUUID, LLPanelGroupLandMoney*> group_id_map_t;
    static group_id_map_t sGroupIDs;

    static void processGroupAccountDetailsReply(LLMessageSystem* msg,  void** data);
    static void processGroupAccountTransactionsReply(LLMessageSystem* msg, void** data);
    static void processGroupAccountSummaryReply(LLMessageSystem* msg, void** data);

    virtual void setGroupID(const LLUUID& id);

    virtual void onLandSelectionChanged();
    
protected:
    class impl;
    impl* mImplementationp;
};

#endif // LL_PANEL_GROUP_LAND_MONEY_H
