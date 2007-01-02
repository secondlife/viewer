/** 
 * @file llfloatergroupinvite.h
 * @brief This floater is just a wrapper for LLPanelGroupInvite, which
 * is used to invite members to a specific group
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERGROUPINVITE_H
#define LL_LLFLOATERGROUPINVITE_H

#include "llfloater.h"
#include "lluuid.h"

class LLFloaterGroupInvite
: public LLFloater
{
public:
	virtual ~LLFloaterGroupInvite();

	static void showForGroup(const LLUUID &group_id);

protected:
	LLFloaterGroupInvite(const std::string& name,
						 const LLRect &rect,
						 const std::string& title,
						 const LLUUID& group_id = LLUUID::null);

	class impl;
	impl* mImpl;
};

#endif
