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

	void clear();
	void update();

	void setCloseCallback(void (*close_callback)(void*), void* data);

	virtual BOOL postBuild();
protected:
	class impl;
	impl* mImplementation;
};

#endif
