/** 
 * @file lluseroperation.h
 * @brief LLUserOperation class header file - used for message based
 * transaction. For example, money transactions.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	virtual BOOL isExpired();

	// Send request to the dataserver
	virtual void sendRequest() = 0;

	// Run the operation. This will only be called in the case of an
	// actual success or failure of the operation.
	virtual BOOL execute(BOOL transaction_success) = 0;	

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
};


class LLUserOperationMgr
{
public:
	LLUserOperationMgr();
	~LLUserOperationMgr();

	void addOperation(LLUserOperation* op);
	LLUserOperation* findOperation(const LLUUID& transaction_id);
	BOOL deleteOperation(LLUserOperation* op);

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
