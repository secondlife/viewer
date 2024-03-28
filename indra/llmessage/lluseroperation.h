/** 
 * @file lluseroperation.h
 * @brief LLUserOperation class header file - used for message based
 * transaction. For example, L$ transactions.
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

#ifndef LL_LLUSEROPERATION_H
#define LL_LLUSEROPERATION_H

#include "lluuid.h"
#include "llframetimer.h"

#include <map>

class LLUserOperation
{
public:
	LLUserOperation(const LLUUID& agent_id);
	LLUserOperation(const LLUUID& agent_id, const LLUUID& transaction_id);
	virtual ~LLUserOperation();

	const LLUUID& getTransactionID() const { return mTransactionID; }
	const LLUUID& getAgentID() const { return mAgentID; }

	// Operation never got necessary data, so expired	
	virtual bool isExpired();

	// ability to mark this operation as never expiring.
	void SetNoExpireFlag(const bool flag);

	// Send request to the dataserver
	virtual void sendRequest() = 0;

	// Run the operation. This will only be called in the case of an
	// actual success or failure of the operation.
	virtual bool execute(bool transaction_success) = 0;

	// This method is called when the user op has expired, and is
	// about to be deleted by the manager. This gives the user op the
	// ability to nack someone when the user op is never evaluated
	virtual void expire();

protected:
	LLUserOperation();
	
protected:
	LLUUID mAgentID;
	LLUUID mTransactionID;
	LLFrameTimer mTimer;
	bool   mNoExpire;			// this is used for operations that expect an answer and will wait till it gets one.
};


class LLUserOperationMgr
{
public:
	LLUserOperationMgr();
	~LLUserOperationMgr();

	void addOperation(LLUserOperation* op);
	LLUserOperation* findOperation(const LLUUID& transaction_id);
	bool deleteOperation(LLUserOperation* op);

	// Call this method every once in a while to clean up old
	// transactions.
	void deleteExpiredOperations();
	
private:
	typedef std::map<LLUUID, LLUserOperation*> user_operation_list_t;
	user_operation_list_t mUserOperationList;
	LLUUID mLastOperationConsidered;
};

extern LLUserOperationMgr* gUserOperationMgr;

#endif // LL_LLUSEROPERATION_H
