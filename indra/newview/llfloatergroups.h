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
#include "llmap.h"
#include "llevent.h"
#include "llfloater.h"

class LLUICtrl;
class LLTextBox;
class LLScrollListCtrl;
class LLButton;

class LLFloaterGroups : public LLFloater
{
public:
	//LLEventListener
	/*virtual*/ bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata);
	
	enum EGroupDialog
	{
		AGENT_GROUPS,
		CHOOSE_ONE
	};
	// Call this with an agent id and AGENT_GROUPS for an agent's
	// groups, otherwise, call with an object id and SET_OBJECT_GROUP
	// when modifying an object.
	static LLFloaterGroups* show(const LLUUID& id, EGroupDialog type);

	// Return the instance requested if it already exists. Otherwise,
	// return NULL.
	static LLFloaterGroups* getInstance(const LLUUID& id);

	// clear the group list, and get a fresh set of info.
	void reset();

	void setOkCallback( void (*callback)(LLUUID, void*), 
						void* userdata);

	EGroupDialog getType() const { return mType; }

protected:
	// initialize based on the type
	BOOL postBuild();

	// highlight_id is a group id to highlight
	void initAgentGroups(const LLUUID& highlight_id);
	void enableButtons();

	static void onBtnCreate(void* userdata);
	static void onBtnActivate(void* userdata);
	static void onBtnInfo(void* userdata);
	static void onBtnLeave(void* userdata);
	static void onBtnSearch(void* userdata);
	static void onBtnVote(void* userdata);
	static void onBtnOK(void* userdata);
	static void onBtnCancel(void* userdata);
	static void onGroupList(LLUICtrl* ctrl, void* userdata);
	static void onDoubleClickGroup(void* userdata);

	void create();
	void activate();
	void info();
	void leave();
	void search();
	void callVote();
	void ok();
	void highlightGroupList(LLUICtrl*);

	static void callbackLeaveGroup(S32 option, void* userdata);

protected:
	LLUUID mID;

	EGroupDialog mType;

	void (*mOKCallback)(LLUUID id, void* userdata);
	void* mCallbackUserdata;

protected:
	static LLMap<const LLUUID, LLFloaterGroups*> sInstances;

public:
	// do not call these directly
	LLFloaterGroups(const std::string& name, const LLRect& rect, const std::string& title,
					const LLUUID& id);
	virtual ~LLFloaterGroups();
};


#endif // LL_LLFLOATERGROUPS_H
