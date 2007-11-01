/** 
 * @file llfloaterparcelinfo.h
 * @brief LLFloaterParcelInfo class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/**
 * Parcel information as shown in a floating window from a sl-url.
 * Just a wrapper for LLPanelPlace, shared with the Find directory.
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

	LLFloaterParcelInfo(const std::string& name, const LLUUID &parcel_id );
	/*virtual*/ ~LLFloaterParcelInfo();

	void displayParcelInfo(const LLUUID& parcel_id);

	static LLFloaterParcelInfo* show(const LLUUID& parcel_id);

private:
	LLUUID			mParcelID;			// for which parcel is this window?
	LLPanelPlace*	mPanelParcelp;
};


#endif
