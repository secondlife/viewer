/** 
 * @file llagentwearablesinitialfetch.h
 * @brief LLAgentWearablesInitialFetch class header file
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLAGENTWEARABLESINITIALFETCH_H
#define LL_LLAGENTWEARABLESINITIALFETCH_H

#include "llinventoryobserver.h"
#include "llwearabletype.h"
#include "lluuid.h"

//--------------------------------------------------------------------
// InitialWearablesFetch
// 
// This grabs contents from the COF and processes them.
// The processing is handled in idle(), i.e. outside of done(),
// to avoid gInventory.notifyObservers recursion.
//--------------------------------------------------------------------
class LLInitialWearablesFetch : public LLInventoryFetchDescendentsObserver
{
	LOG_CLASS(LLInitialWearablesFetch);

public:
	LLInitialWearablesFetch(const LLUUID& cof_id);
	~LLInitialWearablesFetch();
	virtual void done();

	struct InitialWearableData
	{
		LLWearableType::EType mType;
		LLUUID mItemID;
		LLUUID mAssetID;
		InitialWearableData(LLWearableType::EType type, LLUUID& itemID, LLUUID& assetID) :
			mType(type),
			mItemID(itemID),
			mAssetID(assetID)
		{}
	};

	void add(InitialWearableData &data);

protected:
	void processWearablesMessage();
	void processContents();

private:
	typedef std::vector<InitialWearableData> initial_wearable_data_vec_t;
	initial_wearable_data_vec_t mAgentInitialWearables; // Wearables from the old agent wearables msg
};

//--------------------------------------------------------------------
// InitialWearablesFetch
// 
// This grabs outfits from the Library and copies those over to the user's
// outfits folder, typically during first-ever login.
//--------------------------------------------------------------------
class LLLibraryOutfitsFetch : public LLInventoryFetchDescendentsObserver
{
public:
	enum ELibraryOutfitFetchStep
	{
		LOFS_FOLDER = 0,
		LOFS_OUTFITS,
		LOFS_LIBRARY,
		LOFS_IMPORTED,
		LOFS_CONTENTS
	};

	LLLibraryOutfitsFetch(const LLUUID& my_outfits_id);
	~LLLibraryOutfitsFetch();

	virtual void done();
	void doneIdle();
	LLUUID mMyOutfitsID;
	void importedFolderFetch();
protected:
	void folderDone();
	void outfitsDone();
	void libraryDone();
	void importedFolderDone();
	void contentsDone();
	enum ELibraryOutfitFetchStep mCurrFetchStep;
	uuid_vec_t mLibraryClothingFolders;
	uuid_vec_t mImportedClothingFolders;
	bool mOutfitsPopulated;
	LLUUID mClothingID;
	LLUUID mLibraryClothingID;
	LLUUID mImportedClothingID;
	std::string mImportedClothingName;
};

#endif // LL_AGENTWEARABLESINITIALFETCH_H
