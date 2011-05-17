/** 
 * @file llfloatersounddevices.h
 * @author Leyla Farazha
 * @brief Sound Preferences used for minimal skin
 *
* $LicenseInfo:firstyear=2011&license=viewerlgpl$
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

#ifndef LL_LLFLOATERSOUNDDEVICES_H
#define LL_LLFLOATERSOUNDDEVICES_H

#include "lltransientdockablefloater.h"

class LLFloaterSoundDevices : public LLTransientDockableFloater
{
public:

	LOG_CLASS(LLFloaterSoundDevices);

	LLFloaterSoundDevices(const LLSD& key);
	~LLFloaterSoundDevices();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void setDocked(bool docked, bool pop_on_undock = true);
	/*virtual*/ void setFocus( BOOL b );
};


#endif //LL_LLFLOATERSOUNDDEVICES_H

