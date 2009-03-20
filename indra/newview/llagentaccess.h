/** 
 * @file llagentaccess.h
 * @brief LLAgentAccess class implementation - manages maturity and godmode info
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#ifndef LL_LLAGENTACCESS_H
#define LL_LLAGENTACCESS_H

#include "stdtypes.h"

// forward declaration so that we don't have to include the whole class
class LLControlGroupReader;

class LLAgentAccess
{
public:
	LLAgentAccess(LLControlGroupReader& savedSettings);
	
	bool getAdminOverride() const;
	void setAdminOverride(bool b);

	void setGodLevel(U8 god_level);
	bool isGodlike() const;
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
	
	// we want this to be const but the getters for it aren't, so we're 
	// overriding it for now
	/* const */ LLControlGroupReader& mSavedSettings;
};

#endif // LL_LLAGENTACCESS_H
