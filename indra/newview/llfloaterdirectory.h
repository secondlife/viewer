/**
 * @file llfloaterdirectory.h
 * @brief The legacy "Search" floater
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

#ifndef LL_LLFLOATERDIRECTORY_H
#define LL_LLFLOATERDIRECTORY_H

#include "llfloater.h"
#include "lltabcontainer.h"

#include "llpaneldirevents.h"
#include "llpaneldirland.h"
#include "llpaneldirpeople.h"
#include "llpaneldirgroups.h"
#include "llpaneldirplaces.h"
#include "llpaneldirclassified.h"

class LLDirectoryCore;
class LLPanelDirBrowser;

class LLPanelDirAdvanced;
class LLPanelDirClassified;
class LLPanelDirEvents;
class LLPanelDirGroups;
class LLPanelDirLand;
class LLPanelDirPeople;
class LLPanelDirPlaces;

class LLPanelProfileSecondLife;
class LLPanelEventInfo;
class LLPanelGroup;
class LLPanelPlaces;
class LLPanelClassifiedInfo;

// Floater to find people, places, things
class LLFloaterDirectory : public LLFloater
{
public: 
    LLFloaterDirectory(const std::string& name);
    /*virtual*/ ~LLFloaterDirectory();

    void hideAllDetailPanels();

    bool postBuild() override;

public:
    LLPanelProfileSecondLife* mPanelAvatarp;
    LLPanelEventInfo* mPanelEventp;
    LLPanelGroup* mPanelGroupp;
    LLPanelPlaces* mPanelPlacep;
    LLPanelClassifiedInfo* mPanelClassifiedp;

private:
    bool mMinimizing; // HACK: see reshape() for details
    static LLFloaterDirectory *sInstance;
};

//extern BOOL gDisplayEventHack;

#endif  // LL_LLDIRECTORYFLOATER_H
