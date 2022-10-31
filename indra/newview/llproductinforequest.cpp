/** 
 * @file llproductinforequest.cpp
 * @author Kent Quirk
 * @brief Get region type descriptions (translation from SKU to description)
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llproductinforequest.h"

#include "llagent.h"  // for gAgent
#include "lltrans.h"
#include "llviewerregion.h"
#include "llcorehttputil.h"

LLProductInfoRequestManager::LLProductInfoRequestManager(): 
    mSkuDescriptions()
{
}

void LLProductInfoRequestManager::initSingleton()
{
    std::string url = gAgent.getRegionCapability("ProductInfoRequest");
    if (!url.empty())
    {
        LLCoros::instance().launch("LLProductInfoRequestManager::getLandDescriptionsCoro",
            boost::bind(&LLProductInfoRequestManager::getLandDescriptionsCoro, this, url));
    }
}

std::string LLProductInfoRequestManager::getDescriptionForSku(const std::string& sku)
{
    // The description LLSD is an array of maps; each array entry
    // has a map with 3 fields -- description, name, and sku
    for (LLSD::array_const_iterator it = mSkuDescriptions.beginArray();
         it != mSkuDescriptions.endArray();
         ++it)
    {
        //  LL_WARNS() <<  (*it)["sku"].asString() << " = " << (*it)["description"].asString() << LL_ENDL;
        if ((*it)["sku"].asString() == sku)
        {
            return (*it)["description"].asString();
        }
    }
    return LLTrans::getString("land_type_unknown");
}

void LLProductInfoRequestManager::getLandDescriptionsCoro(std::string url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericPostCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        return;
    }

    if (result.has(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT) &&
        result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT].isArray())
    {
        mSkuDescriptions = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT];
    }
    else
    {
        LL_WARNS() << "Land SKU description response is malformed" << LL_ENDL;
    }
}
