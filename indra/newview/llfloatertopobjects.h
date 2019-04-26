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

// Bits for simulator performance query flags
enum LAND_STAT_FLAGS
{
	STAT_FILTER_BY_PARCEL	= 0x00000001,
	STAT_FILTER_BY_OWNER	= 0x00000002,
	STAT_FILTER_BY_OBJECT	= 0x00000004,
	STAT_FILTER_BY_PARCEL_NAME	= 0x00000008,
	STAT_REQUEST_LAST_ENTRY	= 0x80000000,
};

enum LAND_STAT_REPORT_TYPE
{
	STAT_REPORT_TOP_SCRIPTS = 0,
	STAT_REPORT_TOP_COLLIDERS
};

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
	void disableRefreshBtn();

private:
	LLFloaterTopObjects(const LLSD& key);
	~LLFloaterTopObjects();

	void initColumns(LLCtrlListInterface *list);

	void onCommitObjectsList();
	static void onDoubleClickObjectsList(void* data);
	void onClickShowBeacon();

	void returnObjects(bool all);

	void onReturnAll();
	void onReturnSelected();

	static bool callbackReturnAll(const LLSD& notification, const LLSD& response);

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

	static LLFloaterTopObjects* sInstance;
};

#endif
