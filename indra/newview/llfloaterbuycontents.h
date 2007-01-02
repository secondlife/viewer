/** 
 * @file llfloaterbuycontents.h
 * @author James Cook
 * @brief LLFloaterBuyContents class header file
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/**
 * Shows the contents of an object and their permissions when you
 * click "Buy..." on an object with "Sell Contents" checked.
 */

#ifndef LL_LLFLOATERBUYCONTENTS_H
#define LL_LLFLOATERBUYCONTENTS_H

#include "llfloater.h"
#include "llvoinventorylistener.h"
#include "llinventory.h"

class LLViewerObject;

class LLFloaterBuyContents
: public LLFloater, public LLVOInventoryListener
{
public:
	static void show(const LLSaleInfo& sale_info);

protected:
	LLFloaterBuyContents();
	~LLFloaterBuyContents();

	void requestObjectInventories();
	/*virtual*/ void inventoryChanged(LLViewerObject* obj,
								 InventoryObjectList* inv,
								 S32 serial_num,
								 void* data);

	static void onClickBuy(void*);
	static void onClickCancel(void*);

protected:
	static LLFloaterBuyContents* sInstance;

	LLSaleInfo mSaleInfo;
};

#endif
