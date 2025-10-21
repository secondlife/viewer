/** 
 * @file llpaneldirland.cpp
 * @brief Land For Sale and Auction in the Find directory.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llpaneldirland.h"

// linden library includes
#include "llfontgl.h"
#include "llparcel.h"
#include "llqueryflags.h"
#include "message.h"

// viewer project includes
#include "llagent.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llscrolllistctrl.h"
#include "llstatusbar.h"
#include "lluiconstants.h"
#include "lltextbox.h"
#include "llviewercontrol.h"
#include "llviewermessage.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

static const char FIND_ALL[] = "All Types";
static const char FIND_AUCTION[] = "Auction";
static const char FIND_MAINLANDSALES[] = "Mainland Sales";
static const char FIND_ESTATESALES[] = "Estate Sales";

static LLPanelInjector<LLPanelDirLand> t_panel_dir_land("panel_dir_land");

LLPanelDirLand::LLPanelDirLand()
    : LLPanelDirBrowser()
{
}

bool LLPanelDirLand::postBuild()
{
    LLPanelDirBrowser::postBuild();

    childSetValue("type", gSavedSettings.getString("FindLandType"));

    bool adult_enabled = gAgent.canAccessAdult();
    bool mature_enabled = gAgent.canAccessMature();
    childSetVisible("incpg", true);
    if (!mature_enabled)
    {
        childSetValue("incmature", FALSE);
        childDisable("incmature");
    }
    if (!adult_enabled)
    {
        childSetValue("incadult", FALSE);
        childDisable("incadult");
    }

    childSetCommitCallback("pricecheck", onCommitPrice, this);
    childSetCommitCallback("areacheck", onCommitArea, this);

    if (gStatusBar)
    {
        childSetValue("priceedit", gStatusBar->getBalance());
    }
    childSetEnabled("priceedit", gSavedSettings.getBOOL("FindLandPrice"));
    LLLineEditor* priceedit = getChild<LLLineEditor>("priceedit");
    priceedit->setPrevalidateInput(LLTextValidate::validateNonNegativeS32);

    childSetEnabled("areaedit", gSavedSettings.getBOOL("FindLandArea"));
    LLLineEditor* areaedit = getChild<LLLineEditor>("areaedit");
    areaedit->setPrevalidateInput(LLTextValidate::validateNonNegativeS32);

    childSetAction("Search", onClickSearchCore, this);
    setDefaultBtn("Search");

    mCurrentSortColumn = "per_meter";

    LLScrollListCtrl* results = getChild<LLScrollListCtrl>("results");
    if (results)
    {
        results->setSortChangedCallback(boost::bind(&LLPanelDirLand::onClickSort, this));
        results->sortByColumn(mCurrentSortColumn,mCurrentSortAscending);
    }

    return TRUE;
}

LLPanelDirLand::~LLPanelDirLand()
{
    // Children all cleaned up by default view destructor.
}

// virtual
void LLPanelDirLand::draw()
{
    updateMaturityCheckbox();

    LLPanelDirBrowser::draw();
}

void LLPanelDirLand::onClickSort()
{
    performQuery();
}

// static 
void LLPanelDirLand::onCommitPrice(LLUICtrl* ctrl, void* data)
{
    LLPanelDirLand* self = (LLPanelDirLand*)data;
    LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;

    if (!self || !check) return;
    self->childSetEnabled("priceedit", check->get());
}

// static 
void LLPanelDirLand::onCommitArea(LLUICtrl* ctrl, void* data)
{
    LLPanelDirLand* self = (LLPanelDirLand*)data;
    LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;

    if (!self || !check) return;
    self->childSetEnabled("areaedit", check->get());
}

void LLPanelDirLand::performQuery()
{
    BOOL inc_pg = childGetValue("incpg").asBoolean();
    BOOL inc_mature = childGetValue("incmature").asBoolean();
    BOOL inc_adult = childGetValue("incadult").asBoolean();
    if (!(inc_pg || inc_mature || inc_adult))
    {
        LLNotificationsUtil::add("NoContentToSearch");
        return;
    }

    LLMessageSystem* msg = gMessageSystem;

    setupNewSearch();

    // We could change the UI to allow arbitrary combinations of these options
    U32 search_type = ST_ALL;
    const std::string& type = childGetValue("type").asString();
    if(!type.empty())
    {
        if (FIND_AUCTION == type) search_type = ST_AUCTION;
        else if(FIND_MAINLANDSALES == type) search_type = ST_MAINLAND;
        else if(FIND_ESTATESALES == type) search_type = ST_ESTATE;
    }

    U32 query_flags = 0x0;
    if (gAgent.wantsPGOnly()) query_flags |= DFQ_PG_SIMS_ONLY;

    bool adult_enabled = gAgent.canAccessAdult();
    bool mature_enabled = gAgent.canAccessMature();

    if (inc_pg)
    {
        query_flags |= DFQ_INC_PG;
    }

    if (inc_mature && mature_enabled)
    {
        query_flags |= DFQ_INC_MATURE;
    }

    if (inc_adult && adult_enabled)
    {
        query_flags |= DFQ_INC_ADULT;
    }
	
    // Add old flags in case we are talking to an old dataserver
    if (inc_pg && !inc_mature)
    {
        query_flags |= DFQ_PG_SIMS_ONLY;
    }

    if (!inc_pg && inc_mature)
    {
        query_flags |= DFQ_MATURE_SIMS_ONLY; 
    }

    LLScrollListCtrl* list = getChild<LLScrollListCtrl>("results");
    if (list)
    {
        std::string sort_name = list->getSortColumnName();
        BOOL sort_asc = list->getSortAscending();

        if (sort_name == "name")
        {
            query_flags |= DFQ_NAME_SORT;
        }
        else if (sort_name == "price")
        {
            query_flags |= DFQ_PRICE_SORT;
        }
        else if (sort_name == "per_meter")
        {
            query_flags |= DFQ_PER_METER_SORT;
        }
        else if (sort_name == "area")
        {
            query_flags |= DFQ_AREA_SORT;
        }

        if (sort_asc)
        {
            query_flags |= DFQ_SORT_ASC;
        }
    }

    if (childGetValue("pricecheck").asBoolean())
    {
        query_flags |= DFQ_LIMIT_BY_PRICE;
    }

    if (childGetValue("areacheck").asBoolean())
    {
        query_flags |= DFQ_LIMIT_BY_AREA;
    }

    msg->newMessage("DirLandQuery");
    msg->nextBlock("AgentData");
    msg->addUUID("AgentID", gAgent.getID());
    msg->addUUID("SessionID", gAgent.getSessionID());
    msg->nextBlock("QueryData");
    msg->addUUID("QueryID", getSearchID());
    msg->addU32("QueryFlags", query_flags);
    msg->addU32("SearchType", search_type);
    msg->addS32("Price", childGetValue("priceedit").asInteger());
    msg->addS32("Area", childGetValue("areaedit").asInteger());
    msg->addS32Fast(_PREHASH_QueryStart,mSearchStart);
    gAgent.sendReliableMessage();
}
