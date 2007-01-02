/** 
 * @file llfloatersellland.h
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERSELLLAND_H
#define LL_LLFLOATERSELLLAND_H

class LLParcel;
class LLViewerRegion;

class LLFloaterSellLand
{
public:
	static void sellLand(LLViewerRegion* region,
						LLParcel* parcel);
};

#endif // LL_LLFLOATERSELLLAND_H
