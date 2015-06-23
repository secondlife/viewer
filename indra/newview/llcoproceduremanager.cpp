/**
* @file llcoproceduremanager.cpp
* @author Rider Linden
* @brief Singleton class for managing asset uploads to the sim.
*
* $LicenseInfo:firstyear=2015&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2015, Linden Research, Inc.
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
#include "linden_common.h" 

#include "llviewercontrol.h"

#include "llcoproceduremanager.h"

//=========================================================================
#define COROCOUNT 1

//=========================================================================
LLCoprocedureManager::LLCoprocedureManager():
    LLSingleton<LLCoprocedureManager>(),
    mPendingCoprocs(),
    mShutdown(false),
    mWakeupTrigger("CoprocedureManager", true),
    mCoroMapping(),
    mHTTPPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID)
{

    // *TODO: Retrieve the actual number of concurrent coroutines fro gSavedSettings and
    // clamp to a "reasonable" number.
    for (int count = 0; count < COROCOUNT; ++count)
    {
        LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter =
            LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t(
            new LLCoreHttpUtil::HttpCoroutineAdapter("uploadPostAdapter", mHTTPPolicy));

        std::string uploadCoro = LLCoros::instance().launch("LLCoprocedureManager::coprocedureInvokerCoro",
            boost::bind(&LLCoprocedureManager::coprocedureInvokerCoro, this, _1, httpAdapter));

        mCoroMapping.insert(CoroAdapterMap_t::value_type(uploadCoro, httpAdapter));
    }

    mWakeupTrigger.post(LLSD());
}

LLCoprocedureManager::~LLCoprocedureManager() 
{
    shutdown();
}

//=========================================================================

void LLCoprocedureManager::shutdown(bool hardShutdown)
{
    CoroAdapterMap_t::iterator it;

    for (it = mCoroMapping.begin(); it != mCoroMapping.end(); ++it)
    {
        if (!(*it).first.empty())
        {
            if (hardShutdown)
            {
                LLCoros::instance().kill((*it).first);
            }
        }
        if ((*it).second)
        {
            (*it).second->cancelYieldingOperation();
        }
    }

    mShutdown = true;
    mCoroMapping.clear();
    mPendingCoprocs.clear();
}

//=========================================================================
LLUUID LLCoprocedureManager::enqueueCoprocedure(const std::string &name, LLCoprocedureManager::CoProcedure_t proc)
{
    LLUUID id(LLUUID::generateNewID());

    mPendingCoprocs.push_back(QueuedCoproc::ptr_t(new QueuedCoproc(name, id, proc)));
    LL_INFOS() << "Coprocedure(" << name << ") enqueued with id=" << id.asString() << LL_ENDL;

    mWakeupTrigger.post(LLSD());

    return id;
}

void LLCoprocedureManager::cancelCoprocedure(const LLUUID &id)
{
    // first check the active coroutines.  If there, remove it and return.
    ActiveCoproc_t::iterator itActive = mActiveCoprocs.find(id);
    if (itActive != mActiveCoprocs.end())
    {
        LL_INFOS() << "Found and canceling active coprocedure with id=" << id.asString() << LL_ENDL;
        (*itActive).second->cancelYieldingOperation();
        mActiveCoprocs.erase(itActive);
        return;
    }

    for (CoprocQueue_t::iterator it = mPendingCoprocs.begin(); it != mPendingCoprocs.end(); ++it)
    {
        if ((*it)->mId == id)
        {
            LL_INFOS() << "Found and removing queued coroutine(" << (*it)->mName << ") with Id=" << id.asString() << LL_ENDL;
            mPendingCoprocs.erase(it);
            return;
        }
    }

    LL_INFOS() << "Coprocedure with Id=" << id.asString() << " was not found." << LL_ENDL;
}

//=========================================================================
void LLCoprocedureManager::coprocedureInvokerCoro(LLCoros::self& self, LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter)
{
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    while (!mShutdown)
    {
        waitForEventOn(self, mWakeupTrigger);
        if (mShutdown)
            break;
        
        while (!mPendingCoprocs.empty())
        {
            QueuedCoproc::ptr_t coproc = mPendingCoprocs.front();
            mPendingCoprocs.pop_front();
            mActiveCoprocs.insert(ActiveCoproc_t::value_type(coproc->mId, httpAdapter));

            LL_INFOS() << "Dequeued and invoking coprocedure(" << coproc->mName << ") with id=" << coproc->mId.asString() << LL_ENDL;

            try
            {
                coproc->mProc(self, httpAdapter, coproc->mId);
            }
            catch (std::exception &e)
            {
                LL_WARNS() << "Coprocedure(" << coproc->mName << ") id=" << coproc->mId.asString() <<
                    " threw an exception! Message=\"" << e.what() << "\"" << LL_ENDL;
            }
            catch (...)
            {
                LL_WARNS() << "A non std::exception was thrown from " << coproc->mName << " with id=" << coproc->mId << "." << LL_ENDL;
            }

            LL_INFOS() << "Finished coprocedure(" << coproc->mName << ")" << LL_ENDL;

            ActiveCoproc_t::iterator itActive = mActiveCoprocs.find(coproc->mId);
            if (itActive != mActiveCoprocs.end())
            {
                mActiveCoprocs.erase(itActive);
            }
        }
    }
}
