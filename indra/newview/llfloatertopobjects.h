/** 
 * @file llfloatertopobjects.h
 * @brief Shows top colliders or top scripts
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLFLOATERTOPOBJECTS_H
#define LL_LLFLOATERTOPOBJECTS_H

#include "llfloater.h"

class LLUICtrl;

class LLFloaterTopObjects : public LLFloater
{
	friend class LLFloaterReg;
public:
	// Opens the floater on screen.
//	static void show();

	// Opens the floater if it's not on-screen.
	// Juggles the UI based on method = "scripts" or "colliders"
	static void handle_land_reply(LLMessageSystem* msg, void** data);
	void handleReply(LLMessageSystem* msg, void** data);
	
	void clearList();
	void updateSelectionInfo();
	virtual BOOL postBuild();

	void onRefresh();

	static void setMode(U32 mode);

private:
	LLFloaterTopObjects(const LLSD& key);
	~LLFloaterTopObjects();

	void initColumns(LLCtrlListInterface *list);

	void onCommitObjectsList();
	static void onDoubleClickObjectsList(void* data);
	void onClickShowBeacon();

	void doToObjects(int action, bool all);

	void onReturnAll();
	void onReturnSelected();
	void onDisableAll();
	void onDisableSelected();

	static bool callbackReturnAll(const LLSD& notification, const LLSD& response);
	static bool callbackDisableAll(const LLSD& notification, const LLSD& response);

	void onGetByOwnerName();
	void onGetByObjectName();
	void onGetByParcelName();

	void showBeacon();

private:
	std::string mMethod;

	LLSD mObjectListData;
	uuid_vec_t mObjectListIDs;

	U32 mCurrentMode;
	U32 mFlags;
	std::string mFilter;

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
