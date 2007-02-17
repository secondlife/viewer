/** 
 * @file llfloaterbuy.h
 * @author James Cook
 * @brief LLFloaterBuy class definition
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/**
 * Floater that appears when buying an object, giving a preview
 * of its contents and their permissions.
 */ 

#ifndef LL_LLFLOATERBUY_H
#define LL_LLFLOATERBUY_H

#include "llfloater.h"
#include "llvoinventorylistener.h"
#include "llsaleinfo.h"
#include "llinventory.h"

class LLViewerObject;
class LLSaleInfo;
class LLObjectSelection;

class LLFloaterBuy
: public LLFloater, public LLVOInventoryListener
{
public:
	static void show(const LLSaleInfo& sale_info);

protected:
	LLFloaterBuy();
	~LLFloaterBuy();

	void reset();

	void requestObjectInventories();
	/*virtual*/ void inventoryChanged(LLViewerObject* obj,
								 InventoryObjectList* inv,
								 S32 serial_num,
								 void* data);

	static void onClickBuy(void*);
	static void onClickCancel(void*);

private:
	static LLFloaterBuy* sInstance;

	LLHandle<LLObjectSelection>	mObjectSelection;
	LLSaleInfo mSaleInfo;
};

#endif
