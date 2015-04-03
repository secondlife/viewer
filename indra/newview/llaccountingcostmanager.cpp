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

//===============================================================================
LLAccountingCostManager::LLAccountingCostManager():
    mHttpRequest(),
    mHttpHeaders(),
    mHttpOptions(),
    mHttpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID),
    mHttpPriority(0)
{	
    mHttpRequest = LLCore::HttpRequest::ptr_t(new LLCore::HttpRequest());
    mHttpHeaders = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders(), false);
    mHttpOptions = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions(), false);
    //mHttpPolicy = LLCore::HttpRequest::DEFAULT_POLICY_ID;

}

// Coroutine for sending and processing avatar name cache requests.  
// Do not call directly.  See documentation in lleventcoro.h and llcoro.h for
// further explanation.
void LLAccountingCostManager::accountingCostCoro(LLCoros::self& self, std::string url,
    eSelectionType selectionType, const LLHandle<LLAccountingCostObserver> observerHandle)
{
    LLEventStream  replyPump("AccountingCostReply", true);
    LLCoreHttpUtil::HttpCoroHandler::ptr_t httpHandler = 
        LLCoreHttpUtil::HttpCoroHandler::ptr_t(new LLCoreHttpUtil::HttpCoroHandler(replyPump));

    LL_DEBUGS("LLAccountingCostManager") << "Entering coroutine " << LLCoros::instance().getName(self)
        << " with url '" << url << LL_ENDL;

    try
    {
        LLSD objectList;
        U32  objectIndex = 0;

        IDIt IDIter = mObjectList.begin();
        IDIt IDIterEnd = mObjectList.end();

        for (; IDIter != IDIterEnd; ++IDIter)
        {
            // Check to see if a request for this object has already been made.
            if (mPendingObjectQuota.find(*IDIter) == mPendingObjectQuota.end())
            {
                mPendingObjectQuota.insert(*IDIter);
                objectList[objectIndex++] = *IDIter;
            }
        }

        mObjectList.clear();

        //Post results
        if (objectList.size() == 0)
            return;

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
            mObjectList.clear();
            mPendingObjectQuota.clear();
            return;
        }

        LLSD dataToPost = LLSD::emptyMap();
        dataToPost[keystr.c_str()] = objectList;

        LLAccountingCostObserver* observer = observerHandle.get();
        LLUUID transactionId = observer->getTransactionID();
        observer = NULL;

        LLSD results;
        {   // Scoping block for pumper object
            //LL_INFOS() << "Requesting transaction " << transactionId << LL_ENDL;
            LLCoreHttpUtil::HttpRequestPumper pumper(mHttpRequest);
            LLCore::HttpHandle hhandle = LLCoreHttpUtil::requestPostWithLLSD(mHttpRequest,
                mHttpPolicy, mHttpPriority, url, dataToPost, mHttpOptions, mHttpHeaders,
                httpHandler.get());

            if (hhandle == LLCORE_HTTP_HANDLE_INVALID)
            {
                LLCore::HttpStatus status = mHttpRequest->getStatus();
                LL_WARNS() << "Error posting to " << url << " Status=" << status.getStatus() <<
                    " message = " << status.getMessage() << LL_ENDL;
                mPendingObjectQuota.clear();
                return;
            }

            results = waitForEventOn(self, replyPump);
            //LL_INFOS() << "Results for transaction " << transactionId << LL_ENDL;
        }
        LLSD httpResults;
        httpResults = results["http_result"];

        do 
        {
            observer = observerHandle.get();
            if ((!observer) || (observer->getTransactionID() != transactionId))
            {   // *TODO: Rider: I've noticed that getTransactionID() does not 
                // always match transactionId (the new transaction Id does not show a 
                // corresponding request.)
                if (!observer)
                    break;
                LL_WARNS() << "Request transaction Id(" << transactionId
                    << ") does not match observer's transaction Id("
                    << observer->getTransactionID() << ")." << LL_ENDL;
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

            if (!results.isMap() || results.has("error"))
            {
                LL_WARNS() << "Error on fetched data" << LL_ENDL;
                observer->setErrorStatus(499, "Error on fetched data");
                break;
            }

            if (results.has("selected"))
            {
                F32 physicsCost = 0.0f;
                F32 networkCost = 0.0f;
                F32 simulationCost = 0.0f;

                physicsCost = results["selected"]["physics"].asReal();
                networkCost = results["selected"]["streaming"].asReal();
                simulationCost = results["selected"]["simulation"].asReal();

                SelectionCost selectionCost( physicsCost, networkCost, simulationCost);

                observer->onWeightsUpdate(selectionCost);
            }

        } while (false);

    }
    catch (std::exception e)
    {
        LL_WARNS() << "Caught exception '" << e.what() << "'" << LL_ENDL;
    }
    catch (...)
    {
        LL_WARNS() << "Caught unknown exception." << LL_ENDL;
    }

    mPendingObjectQuota.clear();
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
            boost::bind(&LLAccountingCostManager::accountingCostCoro, this, _1, url, selectionType, observer_handle));
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
