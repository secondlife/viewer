/** 
 * @file llpanelgroupinvite.h
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPANELGROUPINVITE_H
#define LL_LLPANELGROUPINVITE_H

#include "llpanel.h"
#include "lluuid.h"

class LLPanelGroupInvite
: public LLPanel
{
public:
	LLPanelGroupInvite(const std::string& name, const LLUUID& group_id);
	~LLPanelGroupInvite();
	
	void addUsers(std::vector<LLUUID>& agent_ids);
	void clear();
	void update();

	void setCloseCallback(void (*close_callback)(void*), void* data);

	virtual void draw();
	virtual BOOL postBuild();
protected:
	class impl;
	impl* mImplementation;

	BOOL mPendingUpdate;
	LLUUID mStoreSelected;
	void updateLists();
};

#endif
