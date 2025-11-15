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
#include "llappviewer.h"
#include "httpcommon.h"
#include "llcoros.h"
#include "lleventcoro.h"
#include "llcorehttputil.h"
#include "llexception.h"
#include "stringize.h"
#include "llworkgraphmanager.h"
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

// Work graph for sending and processing accounting cost requests.
void LLAccountingCostManager::accountingCostWorkGraph(std::string url,
    eSelectionType selectionType, const LLHandle<LLAccountingCostObserver> observerHandle)
{
    LL_DEBUGS("LLAccountingCostManager") << "Starting accounting cost work graph with url '" << url << "'" << LL_ENDL;

    LLAccountingCostManager* self = LLAccountingCostManager::getInstance();
    if (!self)
    {
        LL_WARNS("LLAccountingCostManager") << "LLAccountingCostManager instance not available" << LL_ENDL;
        return;
    }

    uuid_set_t diffSet;

    std::set_difference(self->mObjectList.begin(),
                        self->mObjectList.end(),
                        self->mPendingObjectQuota.begin(),
                        self->mPendingObjectQuota.end(),
                        std::inserter(diffSet, diffSet.begin()));

    if (diffSet.empty())
    {
        LL_WARNS("LLAccountingCostManager") << "No new objects to fetch costs for (diffSet is empty)" << LL_ENDL;
        return;
    }

    LL_WARNS("LLAccountingCostManager") << "Fetching accounting costs for " << diffSet.size() << " objects" << LL_ENDL;

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
        LL_WARNS("LLAccountingCostManager") << "Invalid selection type: " << selectionType << LL_ENDL;
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

    // Create HTTP work graph adapter
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    auto httpAdapter = std::make_shared<LLCoreHttpUtil::HttpWorkGraphAdapter>(
        "AccountingCost", httpPolicy, LLAppViewer::instance()->getMainAppGroup());

    // Make POST request and get the graph
    LL_WARNS("LLAccountingCostManager") << "Posting accounting cost request to: " << url << LL_ENDL;
    auto graphResult = httpAdapter->postRaw(url, dataToPost);

    // Add processing node that runs on main thread
    auto processNode = graphResult.graph->addNode(
        [observerHandle, sharedResult = graphResult.result]() -> LLWorkResult {
            if (LLApp::isQuitting())
            {
                LL_WARNS("LLAccountingCostManager") << "App is quitting, aborting accounting cost processing" << LL_ENDL;
                LLAccountingCostManager::getInstance()->mPendingObjectQuota.clear();
                return LLWorkResult::Complete;
            }

            if (observerHandle.isDead())
            {
                LL_WARNS("LLAccountingCostManager") << "Observer handle is dead, aborting accounting cost processing" << LL_ENDL;
                LLAccountingCostManager::getInstance()->mPendingObjectQuota.clear();
                return LLWorkResult::Complete;
            }

            if (!LLAccountingCostManager::instanceExists())
            {
                LL_WARNS("LLAccountingCostManager") << "LLAccountingCostManager instance no longer exists" << LL_ENDL;
                return LLWorkResult::Complete;
            }

            const LLSD& results = sharedResult->result;
            const LLSD& httpResults = results[LLCoreHttpUtil::HttpWorkGraphAdapter::HTTP_RESULTS];
            LLCore::HttpStatus status = LLCoreHttpUtil::HttpWorkGraphAdapter::getStatusFromLLSD(httpResults);

            LLAccountingCostObserver* observer = observerHandle.get();
            if (!observer)
            {
                LL_WARNS("LLAccountingCostManager") << "Observer is null, cannot process accounting cost results" << LL_ENDL;
                LLAccountingCostManager::getInstance()->mPendingObjectQuota.clear();
                return LLWorkResult::Complete;
            }

            // do/while(false) allows error conditions to break out of following
            // block while normal flow goes forward once.
            do
            {
                if (!status || results.has("error"))
                {
                    if (!status)
                    {
                        LL_WARNS("LLAccountingCostManager") << "HTTP request failed with status: "
                            << status.getType() << " - " << status.toString() << LL_ENDL;
                        observer->setErrorStatus(status.getType(), status.toString());
                    }
                    else
                    {
                        LL_WARNS("LLAccountingCostManager") << "Error field present in response data" << LL_ENDL;
                        observer->setErrorStatus(499, "Error on fetched data");
                    }

                    break;
                }

                if (!httpResults[LLCoreHttpUtil::HttpWorkGraphAdapter::HTTP_RESULTS_SUCCESS].asBoolean())
                {
                    LL_WARNS("LLAccountingCostManager") << "Error result from HttpWorkGraphAdapter. Code "
                        << httpResults["status"] << ": '" << httpResults["message"] << "'" << LL_ENDL;
                    observer->setErrorStatus(httpResults["status"].asInteger(), httpResults["message"].asStringRef());
                    break;
                }

                const LLSD& content = results[LLCoreHttpUtil::HttpWorkGraphAdapter::HTTP_RESULTS_CONTENT];
                if (content.has("selected"))
                {
                    LLSD selected = content["selected"];

                    F32 physicsCost = (F32)selected["physics"].asReal();
                    F32 networkCost = (F32)selected["streaming"].asReal();
                    F32 simulationCost = (F32)selected["simulation"].asReal();

                    SelectionCost selectionCost(physicsCost, networkCost, simulationCost);

                    LL_WARNS("LLAccountingCostManager") << "Successfully retrieved accounting costs - "
                        << "Physics: " << physicsCost << ", "
                        << "Network: " << networkCost << ", "
                        << "Simulation: " << simulationCost << LL_ENDL;

                    observer->onWeightsUpdate(selectionCost);
                }
                else
                {
                    LL_WARNS("LLAccountingCostManager") << "Response content missing 'selected' field" << LL_ENDL;
                }

            } while (false);

            // Clear pending object quota
            LL_WARNS("LLAccountingCostManager") << "Clearing pending object quota and completing work" << LL_ENDL;
            LLAccountingCostManager::getInstance()->mPendingObjectQuota.clear();

            return LLWorkResult::Complete;
        },
        "accounting-cost-process",
        nullptr,
        LLExecutionType::MainThread
    );

    // Set up dependency: processing depends on HTTP completing
    graphResult.graph->addDependency(graphResult.httpNode, processNode);

    // Register the graph with the manager to keep it alive while executing
    gWorkGraphManager.addGraph(graphResult.graph);

    // Execute the graph
    graphResult.graph->execute();

    LL_DEBUGS("LLAccountingCostManager") << "Work graph scheduled" << LL_ENDL;
}

//===============================================================================
void LLAccountingCostManager::fetchCosts( eSelectionType selectionType,
                                          const std::string& url,
                                          const LLHandle<LLAccountingCostObserver>& observer_handle )
{
    // Invoking system must have already determined capability availability
    if ( !url.empty() )
    {
        if (mUseWorkGraph)
        {
            // NEW: Work graph implementation
            LL_DEBUGS() << "Using work graph implementation for accounting costs" << LL_ENDL;
            accountingCostWorkGraph(url, selectionType, observer_handle);
        }
        else
        {
            // BASELINE: Original coroutine implementation
            LL_DEBUGS() << "Using coroutine baseline implementation for accounting costs" << LL_ENDL;
            std::string coroname =
                LLCoros::instance().launch("LLAccountingCostManager::accountingCostCoro",
                boost::bind(accountingCostCoro, url, selectionType, observer_handle));
            LL_DEBUGS() << coroname << " with url '" << url << LL_ENDL;
        }
    }
    else
    {
        //url was empty - warn & continue
        LL_WARNS("LLAccountingCostManager") << "Supplied url is empty, clearing object lists" << LL_ENDL;
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
