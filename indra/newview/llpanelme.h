/** 
 * @file llpanelme.h
 * @brief Side tray "Me" (My Profile) panel
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

#ifndef LL_LLPANELMEPROFILE_H
#define LL_LLPANELMEPROFILE_H

#include "llpanel.h"
#include "llpanelprofile.h"

/**
* Panel for displaying Agent's Picks and Classifieds panel.
* LLPanelMe allows user to edit his picks and classifieds.
*/
class LLPanelMe : public LLPanelProfile
{
	LOG_CLASS(LLPanelMe);

public:

	LLPanelMe();

	/*virtual*/ void onOpen(const LLSD& key);

	/*virtual*/ BOOL postBuild();
};

#endif // LL_LLPANELMEPROFILE_H
