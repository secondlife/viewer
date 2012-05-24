/** 
 * @file llagentaccess.h
 * @brief LLAgentAccess class implementation - manages maturity and godmode info
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

#ifndef LL_LLAGENTACCESS_H
#define LL_LLAGENTACCESS_H

#include "stdtypes.h"

// forward declaration so that we don't have to include the whole class
class LLControlGroup;

class LLAgentAccess
{
public:
	LLAgentAccess(LLControlGroup& savedSettings);
	
	bool getAdminOverride() const;
	void setAdminOverride(bool b);

	void setGodLevel(U8 god_level);
	bool isGodlike() const;
	bool isGodlikeWithoutAdminMenuFakery() const;
	U8 getGodLevel() const;
	
	
	// rather than just expose the preference setting, we're going to actually
	// expose what the client code cares about -- what the user should see
	// based on a combination of the is* and prefers* flags, combined with God bit.
	bool wantsPGOnly() const;
	bool canAccessMature() const;
	bool canAccessAdult() const;
	bool prefersPG() const;
	bool prefersMature() const;
	bool prefersAdult() const;
	bool isTeen() const;
	bool isMature() const;
	bool isAdult() const;
	
	void setMaturity(char text);
	
	static int convertTextToMaturity(char text);
	
	bool canSetMaturity(S32 maturity);
	
private:
	U8 mAccess;	// SIM_ACCESS_MATURE etc
	U8 mGodLevel;
	bool mAdminOverride;
	
	LLControlGroup& mSavedSettings;
};

#endif // LL_LLAGENTACCESS_H
