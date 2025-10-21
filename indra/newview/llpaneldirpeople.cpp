/**
 * @file llpaneldirpeople.cpp
 * @brief People panel in the legacy Search directory.
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

#include "llpaneldirpeople.h"
#include "llviewerwindow.h"
#include "llsearcheditor.h"

// viewer project includes
#include "llqueryflags.h"
#include "llnotificationsutil.h"

static LLPanelInjector<LLPanelDirPeople> t_panel_dir_people("panel_dir_people");

LLPanelDirPeople::LLPanelDirPeople()
:   LLPanelDirBrowser()
{
    mMinSearchChars = 3;
}

bool LLPanelDirPeople::postBuild()
{
    LLPanelDirBrowser::postBuild();

    //getChild<LLLineEditor>("name")->setKeystrokeCallback(boost::bind(&LLPanelDirBrowser::onKeystrokeName, _1, _2), NULL);

    childSetAction("Search", &LLPanelDirBrowser::onClickSearchCore, this);
    //childDisable("Search");
    setDefaultBtn( "Search" );

    return true;
}

LLPanelDirPeople::~LLPanelDirPeople()
{
}

// virtual
void LLPanelDirPeople::performQuery()
{
    if (childGetValue("name").asString().length() < mMinSearchChars)
    {
        return;
    }

    // filter short words out of the query string
    // and indidate if we did have to filter it
    // The shortest username is 2 characters long.
    const S32 SHORTEST_WORD_LEN = 2;
    bool query_was_filtered = false;
    std::string query_string = LLPanelDirBrowser::filterShortWords( 
            childGetValue("name").asString(), 
            SHORTEST_WORD_LEN, 
            query_was_filtered );

    // possible we threw away all the short words in the query so check length
    if ( query_string.length() < mMinSearchChars )
    {
        LLNotificationsUtil::add("SeachFilteredOnShortWordsEmpty");
        return;
    };

    // if we filtered something out, display a popup
    if ( query_was_filtered )
    {
        LLSD args;
        args["FINALQUERY"] = query_string;
        LLNotificationsUtil::add("SeachFilteredOnShortWords", args);
    };

    setupNewSearch();

    U32 scope = DFQ_PEOPLE;

    // send the message
    sendDirFindQuery(
        gMessageSystem,
        mSearchID,
        query_string,
        scope,
        mSearchStart);
}
