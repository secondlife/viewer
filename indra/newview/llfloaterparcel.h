/** 
 * @file llfloaterparcel.h
 * @brief Parcel information as shown in a floating window from a sl-url.
 * Just a wrapper for LLPanelPlace, shared with the Find directory.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
#ifndef LL_FLOATERPARCELINFO_H
#define LL_FLOATERPARCELINFO_H

#include "llfloater.h"

class LLPanelPlace;

class LLFloaterParcelInfo
:	public LLFloater
{
public:
	static	void*	createPanelPlace(void*	data);

	LLFloaterParcelInfo( const LLSD& parcel_id );
	/*virtual*/ ~LLFloaterParcelInfo();
	
	/*virtual*/ BOOL postBuild();
	
	void displayParcelInfo(const LLUUID& parcel_id);

private:
	LLUUID			mParcelID;			// for which parcel is this window?
	LLPanelPlace*	mPanelParcelp;
};


#endif
