/**
 * @file LLAccountingQuotaManager.cpp
 * @ Handles the setting and accessing for costs associated with mesh
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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
#include "llaccountingcostmanager.h"
#include "llagent.h"
#include "httpcommon.h"
#include "llcoros.h"
#include "lleventcoro.h"
#include "llcorehttputil.h"
#include "llexception.h"
#include "stringize.h"
#include <algorithm>
#include <iterator>

//===============================================================================
LLAccountingCostManager::LLAccountingCostManager()
{

}

// Coroutine for sending and processing avatar name cache requests.
// Do not call directly.  See documentation in lleventcoro.h and llcoro.h for
// further explanation.
void LLAccountingCostManager::accountingCostCoro(std::string url,
    eSelectionType selectionType, const LLHandle<LLAccountingCostObserver> observerHandle)
{
    LL_DEBUGS("LLAccountingCostManager") << "Entering coroutine " << LLCoros::getName()
        << " with url '" << url << LL_ENDL;

    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("AccountingCost", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    try
    {
        LLAccountingCostManager* self = LLAccountingCostManager::getInstance();
        uuid_set_t diffSet;

        std::set_difference(self->mObjectList.begin(),
                            self->mObjectList.end(),
                            self->mPendingObjectQuota.begin(),
                            self->mPendingObjectQuota.end(),
                            std::inserter(diffSet, diffSet.begin()));

        if (diffSet.empty())
            return;

        self->mObjectList.clear();

        std::string keystr;
        if (selectionType == Roots)
        {
            keystr = "selected_roots";
        }
        else if (selectionType == Prims)
        {
            keystr = "selected_prims";
        }
        else
        {
            LL_INFOS() << "Invalid selection type " << LL_ENDL;
            return;
        }

        LLSD objectList(LLSD::emptyMap());

        for (uuid_set_t::iterator it = diffSet.begin(); it != diffSet.end(); ++it)
        {
            objectList.append(*it);
        }

        self->mPendingObjectQuota.insert(diffSet.begin(), diffSet.end());

        LLSD dataToPost = LLSD::emptyMap();
        dataToPost[keystr.c_str()] = objectList;

        LLSD results = httpAdapter->postAndSuspend(httpRequest, url, dataToPost);

        LLSD httpResults = results["http_result"];
        LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

        if (LLApp::isQuitting()
            || observerHandle.isDead()
            || !LLAccountingCostManager::instanceExists())
        {
            return;
        }

        LLAccountingCostObserver* observer = NULL;

        // do/while(false) allows error conditions to break out of following
        // block while normal flow goes forward once.
        do
        {
            observer = observerHandle.get();

            if (!status || results.has("error"))
            {
                LL_WARNS() << "Error on fetched data" << LL_ENDL;
                if (!status)
                    observer->setErrorStatus(status.getType(), status.toString());
                else
                    observer->setErrorStatus(499, "Error on fetched data");

                break;
            }

            if (!httpResults["success"].asBoolean())
            {
                LL_WARNS() << "Error result from LLCoreHttpUtil::HttpCoroHandler. Code "
                    << httpResults["status"] << ": '" << httpResults["message"] << "'" << LL_ENDL;
                if (observer)
                {
                    observer->setErrorStatus(httpResults["status"].asInteger(), httpResults["message"].asStringRef());
                }
                break;
            }


            if (results.has("selected"))
            {
                LLSD selected = results["selected"];

                F32 physicsCost = 0.0f;
                F32 networkCost = 0.0f;
                F32 simulationCost = 0.0f;

                physicsCost = (F32)selected["physics"].asReal();
                networkCost = (F32)selected["streaming"].asReal();
                simulationCost = (F32)selected["simulation"].asReal();

                SelectionCost selectionCost( physicsCost, networkCost, simulationCost);

                observer->onWeightsUpdate(selectionCost);
            }

        } while (false);

    }
    catch (...)
    {
        LOG_UNHANDLED_EXCEPTION(STRINGIZE("coroutine " << LLCoros::getName()
                                          << "('" << url << "')"));
        throw;
    }

    // self can be obsolete by this point
    LLAccountingCostManager::getInstance()->mPendingObjectQuota.clear();
}

//===============================================================================
void LLAccountingCostManager::fetchCosts( eSelectionType selectionType,
                                          const std::string& url,
                                          const LLHandle<LLAccountingCostObserver>& observer_handle )
{
    // Invoking system must have already determined capability availability
    if ( !url.empty() )
    {
        std::string coroname =
            LLCoros::instance().launch("LLAccountingCostManager::accountingCostCoro",
            boost::bind(accountingCostCoro, url, selectionType, observer_handle));
        LL_DEBUGS() << coroname << " with  url '" << url << LL_ENDL;

    }
    else
    {
        //url was empty - warn & continue
        LL_WARNS()<<"Supplied url is empty "<<LL_ENDL;
        mObjectList.clear();
        mPendingObjectQuota.clear();
    }
}
//===============================================================================
void LLAccountingCostManager::addObject( const LLUUID& objectID )
{
    mObjectList.insert( objectID );
}
//===============================================================================
void LLAccountingCostManager::removePendingObject( const LLUUID& objectID )
{
    mPendingObjectQuota.erase( objectID );
}
//===============================================================================
