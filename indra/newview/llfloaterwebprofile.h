/** 
 * @file llfloaterwebprofile.h
 * @brief Avatar profile floater.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLFLOATERWEBPROFILE_H
#define LL_LLFLOATERWEBPROFILE_H

#include "llfloaterwebcontent.h"
#include "llviewermediaobserver.h"

#include <string>

class LLMediaCtrl;

/**
 * Displays avatar profile web page.
 */
class LLFloaterWebProfile
:	public LLFloaterWebContent
{
	LOG_CLASS(LLFloaterWebProfile);
public:
	typedef LLFloaterWebContent::Params Params;

	LLFloaterWebProfile(const Params& key);

	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void handleReshape(const LLRect& new_rect, bool by_user = false);

	static LLFloater* create(const LLSD& key);

private:
	void applyPreferredRect();
};

#endif  // LL_LLFLOATERWEBPROFILE_H

