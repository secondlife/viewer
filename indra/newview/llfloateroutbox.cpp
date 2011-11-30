/** 
 * @file llfloateroutbox.cpp
 * @brief Implementation of the merchant outbox window
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llfloateroutbox.h"

#include "llfloaterreg.h"
#include "lltransientfloatermgr.h"


///----------------------------------------------------------------------------
/// LLFloaterOutbox
///----------------------------------------------------------------------------

LLFloaterOutbox::LLFloaterOutbox(const LLSD& key)
	: LLFloater(key)
	, mPanelOutboxInventory(NULL)
{
	LLTransientFloaterMgr::getInstance()->addControlView(this);
}

LLFloaterOutbox::~LLFloaterOutbox()
{
	LLTransientFloaterMgr::getInstance()->removeControlView(this);
}

BOOL LLFloaterOutbox::postBuild()
{
	return TRUE;
}

void LLFloaterOutbox::onOpen(const LLSD& key)
{
	//LLFirstUse::useInventory();
}

void LLFloaterOutbox::onClose(bool app_quitting)
{
	LLFloater::onClose(app_quitting);
	if (mKey.asInteger() > 1)
	{
		destroy();
	}
}
