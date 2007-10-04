/** 
 * @file llfloaterlandholdings.h
 * @brief "My Land" floater showing all your land parcels.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
