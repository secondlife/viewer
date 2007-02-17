/** 
 * @file llfloatersellland.h
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERSELLLAND_H
#define LL_LLFLOATERSELLLAND_H
#include "llmemory.h"

class LLParcel;
class LLViewerRegion;
class LLParcelSelection;

class LLFloaterSellLand
{
public:
	static void sellLand(LLViewerRegion* region,
						LLHandle<LLParcelSelection> parcel);
};

#endif // LL_LLFLOATERSELLLAND_H
