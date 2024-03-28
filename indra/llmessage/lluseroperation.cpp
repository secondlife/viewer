/** 
 * @file lluseroperation.cpp
 * @brief LLUserOperation class definition.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "linden_common.h"

#include "lluseroperation.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

LLUserOperationMgr* gUserOperationMgr = NULL;

///----------------------------------------------------------------------------
/// Class LLUserOperation
///----------------------------------------------------------------------------

LLUserOperation::LLUserOperation(const LLUUID& agent_id)
:	mAgentID(agent_id),
	mTimer(),
	mNoExpire(false)
{
	mTransactionID.generate();
}

LLUserOperation::LLUserOperation(const LLUUID& agent_id,
								 const LLUUID& transaction_id) :
	mAgentID(agent_id),
	mTransactionID(transaction_id),
	mTimer(),
	mNoExpire(false)
{
}

// protected constructor which is used by base classes that determine
// transaction, agent, et. after construction.
LLUserOperation::LLUserOperation() :
	mTimer(),
	mNoExpire(false)
{
}

LLUserOperation::~LLUserOperation()
{
}

void LLUserOperation::SetNoExpireFlag(const bool flag)
{
	mNoExpire = flag;
}

bool LLUserOperation::isExpired()
{
	if (!mNoExpire)
	{
		const F32 EXPIRE_TIME_SECS = 10.f;
		return mTimer.getElapsedTimeF32() > EXPIRE_TIME_SECS;
	}
	return false;
}

void LLUserOperation::expire()
{
	// by default, do do anything.
}

///----------------------------------------------------------------------------
/// Class LLUserOperationMgr
///----------------------------------------------------------------------------

LLUserOperationMgr::LLUserOperationMgr()
{
}


LLUserOperationMgr::~LLUserOperationMgr()
{
	if (mUserOperationList.size() > 0)
	{
		LL_WARNS() << "Exiting with user operations pending." << LL_ENDL;
	}
}


void LLUserOperationMgr::addOperation(LLUserOperation* op)
{
	if(!op)
	{
		LL_WARNS() << "Tried to add null op" << LL_ENDL;
		return;
	}
	LLUUID id = op->getTransactionID();
	llassert(mUserOperationList.count(id) == 0);
	mUserOperationList[id] = op;
}


LLUserOperation* LLUserOperationMgr::findOperation(const LLUUID& tid)
{
	user_operation_list_t::iterator iter = mUserOperationList.find(tid);
	if (iter != mUserOperationList.end())
		return iter->second;
	else
		return NULL;
}


bool LLUserOperationMgr::deleteOperation(LLUserOperation* op)
{
	size_t rv = 0;
	if(op)
	{
		LLUUID id = op->getTransactionID();
		rv = mUserOperationList.erase(id);
		delete op;
		op = NULL;
	}
	return rv ? true : false;
}

void LLUserOperationMgr::deleteExpiredOperations()
{
	const S32 MAX_OPS_CONSIDERED = 2000;
	S32 ops_left = MAX_OPS_CONSIDERED;
	LLUserOperation* op = NULL;
	user_operation_list_t::iterator it;
	if(mLastOperationConsidered.isNull())
	{
		it = mUserOperationList.begin();
	}
	else
	{
		it = mUserOperationList.lower_bound(mLastOperationConsidered);
	}
	while((ops_left--) && (it != mUserOperationList.end()))
	{
		op = (*it).second;
		if(op && op->isExpired())
		{
			LL_DEBUGS() << "expiring: " << (*it).first << LL_ENDL;
			op->expire();
			mUserOperationList.erase(it++);
			delete op;
		}
		else if(op)
		{
			++it;
		}
		else
		{
			mUserOperationList.erase(it++);
		}
	}
	if(it != mUserOperationList.end())
	{
		mLastOperationConsidered = (*it).first;
	}
	else
	{
		mLastOperationConsidered.setNull();
	}
}


///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------
