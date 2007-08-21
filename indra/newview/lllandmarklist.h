/** 
 * @file lllandmarklist.h
 * @brief Landmark asset list class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLLANDMARKLIST_H
#define LL_LLLANDMARKLIST_H

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
	LLLandmarkList() {}
	~LLLandmarkList();

	//S32					getLength() { return mList.getLength(); }
	//const LLLandmark*	getFirst()	{ return mList.getFirstData(); }
	//const LLLandmark*	getNext()	{ return mList.getNextData(); }

	BOOL assetExists(const LLUUID& asset_uuid);
	LLLandmark* getAsset(const LLUUID& asset_uuid);
	static void processGetAssetReply(
		LLVFS *vfs,
		const LLUUID& uuid,
		LLAssetType::EType type,
		void* user_data,
		S32 status,
		LLExtStat ext_status );

protected:
	typedef std::map<LLUUID, LLLandmark*> landmark_list_t;
	landmark_list_t mList;

	typedef std::set<LLUUID> landmark_bad_list_t;
	landmark_bad_list_t mBadList;
};


extern LLLandmarkList gLandmarkList;

#endif  // LL_LLLANDMARKLIST_H
