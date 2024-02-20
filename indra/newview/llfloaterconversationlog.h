/**
 * @file llfloaterconversationlog.h
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_LLFLOATERCONVERSATIONLOG_H_
#define LL_LLFLOATERCONVERSATIONLOG_H_

#include "llfloater.h"

class LLConversationLogList;

class LLFloaterConversationLog : public LLFloater
{
public:

	LLFloaterConversationLog(const LLSD& key);
	virtual ~LLFloaterConversationLog(){};

	bool postBuild() override;

	void draw() override;

	void onFilterEdit(const std::string& search_string);

private:

	void onCustomAction (const LLSD& userdata);
	bool isActionEnabled(const LLSD& userdata);
	bool isActionChecked(const LLSD& userdata);

	LLConversationLogList* mConversationLogList;
};


#endif /* LLFLOATERCONVERSATIONLOG_H_ */
