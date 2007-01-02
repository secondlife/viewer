/** 
 * @file llfloatertopobjects.h
 * @brief Shows top colliders or top scripts
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERTOPOBJECTS_H
#define LL_LLFLOATERTOPOBJECTS_H

#include "llfloater.h"

class LLUICtrl;

class LLFloaterTopObjects : public LLFloater
{
public:
	// Opens the floater on screen.
	static void show();

	// Opens the floater if it's not on-screen.
	// Juggles the UI based on method = "scripts" or "colliders"
	static void handle_land_reply(LLMessageSystem* msg, void** data);
	void handleReply(LLMessageSystem* msg, void** data);
	
	void clearList();
	void updateSelectionInfo();
	virtual BOOL postBuild();

	static void onRefresh(void* data);

	static void setMode(U32 mode)		{ if (sInstance) sInstance->mCurrentMode = mode; }

private:
	LLFloaterTopObjects();
	~LLFloaterTopObjects();

	void initColumns(LLCtrlListInterface *list);

	static void onCommitObjectsList(LLUICtrl* ctrl, void* data);
	static void onDoubleClickObjectsList(void* data);
	static void onClickShowBeacon(void* data);

	void doToObjects(int action, bool all);

	static void onReturnAll(void* data);
	static void onReturnSelected(void* data);
	static void onDisableAll(void* data);
	static void onDisableSelected(void* data);

	static void callbackReturnAll(S32 option, void* userdata);
	static void callbackDisableAll(S32 option, void* userdata);

	static void onGetByOwnerName(LLUICtrl* ctrl, void* data);
	static void onGetByObjectName(LLUICtrl* ctrl, void* data);

	static void onGetByOwnerNameClicked(void* data)  { onGetByOwnerName(NULL, data); };
	static void onGetByObjectNameClicked(void* data) { onGetByObjectName(NULL, data); };

	void showBeacon();

private:
	LLString mMethod;

	LLSD mObjectListData;
	std::vector<LLUUID> mObjectListIDs;

	U32 mCurrentMode;
	U32 mFlags;
	LLString mFilter;

	BOOL mInitialized;

	F32 mtotalScore;

	enum
	{
		ACTION_RETURN = 0,
		ACTION_DISABLE
	};

	static LLFloaterTopObjects* sInstance;
};

#endif
