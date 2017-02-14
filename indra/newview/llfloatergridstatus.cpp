/** 
 * @file llfloatergridstatus.cpp
 * @brief Grid status floater - uses an embedded web browser to show Grid status info
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2017, Linden Research, Inc.
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

#include "llfloatergridstatus.h"

#include "llcallbacklist.h"
#include "llcorehttputil.h"
#include "llfloaterreg.h"
#include "llhttpconstants.h"
#include "llmediactrl.h"
#include "llsdserialize.h"
#include "lltoolbarview.h"
#include "llviewercontrol.h"
#include "llxmltree.h"

std::map<std::string, std::string> LLFloaterGridStatus::sItemsMap;
const std::string DEFAULT_GRID_STATUS_URL = "http://secondlife-status.statuspage.io/";

LLFloaterGridStatus::LLFloaterGridStatus(const Params& key) :
    LLFloaterWebContent(key),
    mIsFirstUpdate(TRUE)
{
}

BOOL LLFloaterGridStatus::postBuild()
{
    LLFloaterWebContent::postBuild();
    mWebBrowser->addObserver(this);

    return TRUE;
}

void LLFloaterGridStatus::onOpen(const LLSD& key)
{
    Params p(key);
    p.trusted_content = true;
    p.allow_address_entry = false;

    LLFloaterWebContent::onOpen(p);
    applyPreferredRect();
    if (mWebBrowser)
    {
        mWebBrowser->navigateTo(DEFAULT_GRID_STATUS_URL, HTTP_CONTENT_TEXT_HTML);
    }
}

void LLFloaterGridStatus::startGridStatusTimer()
{
    checkGridStatusRSS();
    doPeriodically(boost::bind(&LLFloaterGridStatus::checkGridStatusRSS), gSavedSettings.getF32("GridStatusUpdateDelay"));
}

bool LLFloaterGridStatus::checkGridStatusRSS()
{
    if(gToolBarView->hasCommand(LLCommandId("gridstatus")))
    {
        LLCoros::instance().launch("LLFloaterGridStatus::getGridStatusRSSCoro",
                boost::bind(&LLFloaterGridStatus::getGridStatusRSSCoro));
    }
    return false;
}

void LLFloaterGridStatus::getGridStatusRSSCoro()
{

    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
    httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("getGridStatusRSSCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    LLCore::HttpHeaders::ptr_t httpHeaders(new LLCore::HttpHeaders);

    httpHeaders->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_TEXT_XML);
    std::string url = gSavedSettings.getString("GridStatusRSS");

    LLSD result = httpAdapter->getRawAndSuspend(httpRequest, url, httpOpts, httpHeaders);
    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
    if (!status)
    {
        return;
    }

    const LLSD::Binary &rawBody = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_RAW].asBinary();
    std::string body(rawBody.begin(), rawBody.end());

    std::string fullpath = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"grid_status_rss.xml");
    if(!gSavedSettings.getBOOL("TestGridStatusRSSFromFile"))
    {
        llofstream custom_file_out(fullpath.c_str(), std::ios::trunc);
        if (custom_file_out.is_open())
        {
            custom_file_out << body;
            custom_file_out.close();
        }
    }
    LLXmlTree grid_status_xml;
    if (!grid_status_xml.parseFile(fullpath))
    {
        return ;
    }
    bool new_entries = false;
    LLXmlTreeNode* rootp = grid_status_xml.getRoot();
    for (LLXmlTreeNode* item = rootp->getChildByName( "entry" ); item; item = rootp->getNextNamedChild())
    {
        LLXmlTreeNode* id_node = item->getChildByName("id");
        LLXmlTreeNode* updated_node = item->getChildByName("updated");
        if (!id_node || !updated_node)
        {
            continue;
        }
        std::string guid = id_node->getContents();
        std::string date = updated_node->getContents();
        if(sItemsMap.find( guid ) == sItemsMap.end())
        {
            new_entries = true;
        }
        else
        {
            if(sItemsMap[guid] != date)
            {
                new_entries = true;
            }
        }
        sItemsMap[guid] = date;
    }
    if(new_entries && !getInstance()->isFirstUpdate())
    {
        gToolBarView->flashCommand(LLCommandId("gridstatus"), true);
    }
    getInstance()->setFirstUpdate(FALSE);
}

// virtual
void LLFloaterGridStatus::handleReshape(const LLRect& new_rect, bool by_user)
{
    if (by_user && !isMinimized())
    {
        gSavedSettings.setRect("GridStatusFloaterRect", new_rect);
    }

    LLFloaterWebContent::handleReshape(new_rect, by_user);
}

void LLFloaterGridStatus::applyPreferredRect()
{
    const LLRect preferred_rect = gSavedSettings.getRect("GridStatusFloaterRect");

    LLRect new_rect = getRect();
    new_rect.setLeftTopAndSize(
        new_rect.mLeft, new_rect.mTop,
        preferred_rect.getWidth(), preferred_rect.getHeight());
    setShape(new_rect);
}

LLFloaterGridStatus* LLFloaterGridStatus::getInstance()
{
    return LLFloaterReg::getTypedInstance<LLFloaterGridStatus>("grid_status");
}
