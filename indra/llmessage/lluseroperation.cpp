/** 
 * @file lluseroperation.cpp
 * @brief LLUserOperation class definition.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	mTimer()
{
	mTransactionID.generate();
}

LLUserOperation::LLUserOperation(const LLUUID& agent_id,
								 const LLUUID& transaction_id) :
	mAgentID(agent_id),
	mTransactionID(transaction_id),
	mTimer()
{
}

// protected constructor which is used by base classes that determine
// transaction, agent, et. after construction.
LLUserOperation::LLUserOperation() :
	mTimer()
{
}

LLUserOperation::~LLUserOperation()
{
}


BOOL LLUserOperation::isExpired()
{
	const F32 EXPIRE_TIME_SECS = 10.f;
	return mTimer.getElapsedTimeF32() > EXPIRE_TIME_SECS;
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
		llwarns << "Exiting with user operations pending." << llendl;
	}
}


void LLUserOperationMgr::addOperation(LLUserOperation* op)
{
	if(!op)
	{
		llwarns << "Tried to add null op" << llendl;
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


BOOL LLUserOperationMgr::deleteOperation(LLUserOperation* op)
{
	size_t rv = 0;
	if(op)
	{
		LLUUID id = op->getTransactionID();
		rv = mUserOperationList.erase(id);
		delete op;
		op = NULL;
	}
	return rv ? TRUE : FALSE;
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
			lldebugs << "expiring: " << (*it).first << llendl;
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
