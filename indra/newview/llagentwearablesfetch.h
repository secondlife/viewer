/** 
 * @file llagentwearablesinitialfetch.h
 * @brief LLAgentWearablesInitialFetch class header file
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#ifndef LL_LLAGENTWEARABLESINITIALFETCH_H
#define LL_LLAGENTWEARABLESINITIALFETCH_H

#include "llinventoryobserver.h"
#include "llwearabledictionary.h"
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
