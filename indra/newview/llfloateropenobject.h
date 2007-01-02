/** 
 * @file llfloateropenobject.h
 * @brief LLFloaterOpenObject class definition
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/*
 * Shows the contents of an object and their permissions when you
 * click "Buy..." on an object with "Sell Contents" checked.
 */

#ifndef LL_LLFLOATEROPENOBJECT_H
#define LL_LLFLOATEROPENOBJECT_H

#include "llfloater.h"



class LLPanelInventory;

class LLFloaterOpenObject
: public LLFloater
{
public:
	static void show();
	static void dirty();
	
	struct LLCatAndWear
	{
		LLUUID mCatID;
		bool mWear;
	};

protected:
	LLFloaterOpenObject();
	~LLFloaterOpenObject();

	void refresh();
	void draw();

	static void onClickMoveToInventory(void* data);
	static void onClickMoveAndWear(void* data);
	static void moveToInventory(bool wear);
	static void callbackMoveInventory(S32 result, void* data);
	static void* createPanelInventory(void* data);

protected:
	static LLFloaterOpenObject* sInstance;

	LLPanelInventory*	mPanelInventory;
	BOOL mDirty;
};

#endif
