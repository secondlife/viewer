/** 
* @file llfloaterinspect.h
* @author Cube
* @date 2006-12-16
* @brief Declaration of class for displaying object attributes
*
* Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
* $License$
*/

#ifndef LL_LLFLOATERINSPECT_H
#define LL_LLFLOATERINSPECT_H

#include "llfloater.h"

//class LLTool;
class LLObjectSelection;
class LLScrollListCtrl;
class LLUICtrl;

class LLFloaterInspect : public LLFloater
{
public:
	virtual ~LLFloaterInspect(void);
	static void show(void* ignored = NULL);
	virtual BOOL postBuild();
	static void dirty();
	static LLUUID getSelectedUUID();
	virtual void draw();
	virtual void refresh();
	static BOOL isVisible();
	virtual void onFocusReceived();
	static void onClickCreatorProfile(void* ctrl);
	static void onClickOwnerProfile(void* ctrl);
	static void onSelectObject(LLUICtrl* ctrl, void* user_data);
	LLScrollListCtrl* mObjectList;
protected:
	// protected members
	LLFloaterInspect();
	void setDirty() { mDirty = TRUE; }
	bool mDirty;

private:
	// static data
	static LLFloaterInspect* sInstance;

	LLHandle<LLObjectSelection> mObjectSelection;
};

#endif //LL_LLFLOATERINSPECT_H
