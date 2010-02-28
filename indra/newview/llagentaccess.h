/** 
 * @file llagentaccess.h
 * @brief LLAgentAccess class implementation - manages maturity and godmode info
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
	
	void setTeen(bool teen);
	void setMaturity(char text);
	
	static int convertTextToMaturity(char text);
	
	void setTransition();	// sets the transition bit, which defaults to false
	bool isInTransition() const;
	bool canSetMaturity(S32 maturity);
	
private:
	U8 mAccess;	// SIM_ACCESS_MATURE etc
	U8 mGodLevel;
	bool mAdminOverride;
	
	// this should be deleted after the 60-day AO transition.
	// It should be safe to remove it in Viewer 2009
	// It's set by a special short-term flag in login.cgi 
	// called ao_transition. When that's gone, this can go, along with
	// all of the code that depends on it.
	bool mAOTransition;
	
	LLControlGroup& mSavedSettings;
};

#endif // LL_LLAGENTACCESS_H
