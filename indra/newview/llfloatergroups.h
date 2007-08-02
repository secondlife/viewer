/** 
 * @file llfloatergroups.h
 * @brief LLFloaterGroups class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/*
 * Shown from Edit -> Groups...
 * Shows the agent's groups and allows the edit window to be invoked.
 * Also overloaded to allow picking of a single group for assigning 
 * objects and land to groups.
 */

#ifndef LL_LLFLOATERGROUPS_H
#define LL_LLFLOATERGROUPS_H

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class llfloatergroups
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "lluuid.h"
#include "llfloater.h"
#include <map>

class LLUICtrl;
class LLTextBox;
class LLScrollListCtrl;
class LLButton;
class LLFloaterGroupPicker;

class LLFloaterGroupPicker : public LLFloater, public LLUIInstanceMgr<LLFloaterGroupPicker>
{
	friend class LLUIInstanceMgr<LLFloaterGroupPicker>;
public:
	~LLFloaterGroupPicker();
	void setSelectCallback( void (*callback)(LLUUID, void*), 
							void* userdata);
	BOOL postBuild();

protected:
	LLFloaterGroupPicker(const LLSD& seed);
	void ok();
	static LLFloaterGroupPicker* findInstance(const LLSD& seed);
	static LLFloaterGroupPicker* createInstance(const LLSD& seed);
	static void onBtnOK(void* userdata);
	static void onBtnCancel(void* userdata);

protected:
	LLUUID mID;
	void (*mSelectCallback)(LLUUID id, void* userdata);
	void* mCallbackUserdata;

	typedef std::map<const LLUUID, LLFloaterGroupPicker*> instance_map_t;
	static instance_map_t sInstances;
};

class LLPanelGroups : public LLPanel
{
public:
	LLPanelGroups();
	virtual ~LLPanelGroups();

	//LLEventListener
	/*virtual*/ bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata);
	
	// clear the group list, and get a fresh set of info.
	void reset();

protected:
	// initialize based on the type
	BOOL postBuild();

	// highlight_id is a group id to highlight
	void enableButtons();

	static void onGroupList(LLUICtrl* ctrl, void* userdata);
	static void onBtnCreate(void* userdata);
	static void onBtnActivate(void* userdata);
	static void onBtnInfo(void* userdata);
	static void onBtnIM(void* userdata);
	static void onBtnLeave(void* userdata);
	static void onBtnSearch(void* userdata);
	static void onBtnVote(void* userdata);
	static void onDoubleClickGroup(void* userdata);

	void create();
	void activate();
	void info();
	void startIM();
	void leave();
	void search();
	void callVote();

	static void callbackLeaveGroup(S32 option, void* userdata);

};


#endif // LL_LLFLOATERGROUPS_H
