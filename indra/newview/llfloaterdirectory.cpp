/**
 * @file llfloaterdirectory.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterdirectory.h"

#include "llpaneldirevents.h"
#include "llpaneleventinfo.h"
#include "llpaneldirland.h"
#include "llpaneldirpeople.h"
#include "llpaneldirgroups.h"
#include "llpaneldirplaces.h"
#include "llpaneldirclassified.h"
#include "llpaneldirweb.h"
#include "llscrollbar.h"
#include "llviewercontrol.h"
#include "llpanelavatar.h"
#include "llpanelclassified.h"
#include "llpanelgroup.h"
#include "llpanelplaces.h"
#include "llpanelprofile.h"

LLFloaterDirectory::LLFloaterDirectory(const std::string& name)
:   LLFloater(name),
    mMinimizing(false),
    mPanelAvatarp(nullptr),
    mPanelGroupp(nullptr),
    mPanelPlacep(nullptr),
    mPanelClassifiedp(nullptr),
    mPanelEventp(nullptr)
{
}

LLFloaterDirectory::~LLFloaterDirectory()
{
}

bool LLFloaterDirectory::postBuild()
{
    const std::vector<std::string> panel_names = {
        "panel_dir_classified",
        "panel_dir_events",
        "panel_dir_places",
        "panel_dir_land",
        "panel_dir_people",
        "panel_dir_groups" };

    for (const std::string& panel_name : panel_names)
    {
        if (LLPanelDirBrowser* panel_tab = findChild<LLPanelDirBrowser>(panel_name))
        {
            panel_tab->setFloaterDirectory(this);
        }
    }
    findChild<LLPanelDirWeb>("panel_dir_web")->setFloaterDirectory(this);

    mPanelAvatarp = findChild<LLPanelProfileSecondLife>("panel_profile_secondlife");
    mPanelAvatarp->setAllowEdit(false);
    mPanelGroupp = findChild<LLPanelGroup>("panel_group_info_sidetray");
    mPanelGroupp->hideBackBtn();
    mPanelPlacep = findChild<LLPanelPlaces>("panel_places");
    mPanelPlacep->hideBackBtn();
    mPanelClassifiedp = findChild<LLPanelClassifiedInfo>("panel_classified_info");
    mPanelClassifiedp->setBackgroundVisible(false);
    mPanelEventp = findChild<LLPanelEventInfo>("panel_event_info");

    return true;
}

void LLFloaterDirectory::hideAllDetailPanels()
{
    if (mPanelAvatarp) mPanelAvatarp->setVisible(false);
    if (mPanelGroupp) mPanelGroupp->setVisible(false);
    if (mPanelPlacep) mPanelPlacep->setVisible(false);
    if (mPanelClassifiedp) mPanelClassifiedp->setVisible(false);
    if (mPanelEventp) mPanelEventp->setVisible(false);
}
