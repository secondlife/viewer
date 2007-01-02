/** 
 * @file llfloaterlandholdings.h
 * @brief "My Land" floater showing all your land parcels.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERLANDHOLDINGS_H
#define LL_LLFLOATERLANDHOLDINGS_H

#include "llfloater.h"

class LLMessageSystem;
class LLTextBox;
class LLScrollListCtrl;
class LLButton;

class LLFloaterLandHoldings
:	public LLFloater
{
public:
	BOOL postBuild();

	static void show(void*);

	virtual void draw();

	void refresh();

	void buttonCore(S32 which);

	static void processPlacesReply(LLMessageSystem* msg, void**);

	static void onClickTeleport(void*);
	static void onClickMap(void*);
	static void onClickLandmark(void*);

	static void onGrantList(void* data);

protected:
	LLFloaterLandHoldings();
	virtual ~LLFloaterLandHoldings();

	void refreshAggregates();

protected:
	static LLFloaterLandHoldings* sInstance;

	// Sum up as packets arrive the total holdings
	S32 mActualArea;
	S32 mBillableArea;

	// Has a packet of data been received?
	// Used to clear out the mParcelList's "Loading..." indicator
	BOOL mFirstPacketReceived;

	LLString mSortColumn;
	BOOL mSortAscending;
};

#endif
