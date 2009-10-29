/** 
 * @file lllandmarklist.h
 * @brief Landmark asset list class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLLANDMARKLIST_H
#define LL_LLLANDMARKLIST_H

#include <boost/function.hpp>
#include <map>
#include "lllandmark.h"
#include "lluuid.h"
#include "llassetstorage.h"

class LLMessageSystem;
class LLLineEditor;
class LLInventoryItem;

class LLLandmarkList
{
public:
	typedef boost::function<void(LLLandmark*)> loaded_callback_t;

	LLLandmarkList() {}
	~LLLandmarkList();

	//S32					getLength() { return mList.getLength(); }
	//const LLLandmark*	getFirst()	{ return mList.getFirstData(); }
	//const LLLandmark*	getNext()	{ return mList.getNextData(); }

	BOOL assetExists(const LLUUID& asset_uuid);
	LLLandmark* getAsset(const LLUUID& asset_uuid, loaded_callback_t cb = NULL);
	static void processGetAssetReply(
		LLVFS *vfs,
		const LLUUID& uuid,
		LLAssetType::EType type,
		void* user_data,
		S32 status,
		LLExtStat ext_status );

protected:
	void onRegionHandle(const LLUUID& landmark_id);
	void makeCallbacks(const LLUUID& landmark_id);

	typedef std::map<LLUUID, LLLandmark*> landmark_list_t;
	landmark_list_t mList;

	typedef std::set<LLUUID> landmark_bad_list_t;
	landmark_bad_list_t mBadList;
	
	typedef std::map<LLUUID,F32> landmark_requested_list_t;
	landmark_requested_list_t mRequestedList;
	
	// *TODO: make the callback multimap a template class and make use of it
	// here and in LLLandmark.
	typedef std::multimap<LLUUID, loaded_callback_t> loaded_callback_map_t;
	loaded_callback_map_t mLoadedCallbackMap;
};


extern LLLandmarkList gLandmarkList;

#endif  // LL_LLLANDMARKLIST_H
