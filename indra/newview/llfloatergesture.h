/** 
 * @file llfloatergesture.h
 * @brief Read-only list of gestures from your inventory.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/**
 * (Also has legacy gesture editor for testing.)
 */

#ifndef LL_LLFLOATERGESTURE_H
#define LL_LLFLOATERGESTURE_H

#include "llfloater.h"

#include "lldarray.h"

class LLScrollableContainerView;
class LLView;
class LLButton;
class LLLineEditor;
class LLComboBox;
class LLViewerGesture;
class LLGestureOptions;
class LLScrollListCtrl;
class LLFloaterGestureObserver;
class LLFloaterGestureInventoryObserver;

class LLFloaterGesture
:	public LLFloater
{
public:
	LLFloaterGesture();
	virtual ~LLFloaterGesture();

	virtual BOOL postBuild();

	static void show();
	static void toggleVisibility();
	static void refreshAll();

protected:
	// Reads from the gesture manager's list of active gestures
	// and puts them in this list.
	void buildGestureList();

	static void onClickInventory(void* data);
	static void onClickEdit(void* data);
	static void onClickPlay(void* data);
	static void onClickNew(void* data);
	static void onCommitList(LLUICtrl* ctrl, void* data);

protected:
	LLUUID mSelectedID;

	static LLFloaterGesture* sInstance;
	static LLFloaterGestureObserver* sObserver;
	static LLFloaterGestureInventoryObserver* sInventoryObserver;
};


#endif
