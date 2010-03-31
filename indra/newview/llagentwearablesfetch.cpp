/** 
 * @file llagentwearablesfetch.cpp
 * @brief LLAgentWearblesFetch class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "llagentwearablesfetch.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llinventoryfunctions.h"
#include "llvoavatarself.h"

LLInitialWearablesFetch::LLInitialWearablesFetch()
{
}

LLInitialWearablesFetch::~LLInitialWearablesFetch()
{
}

// virtual
void LLInitialWearablesFetch::done()
{
	// Delay processing the actual results of this so it's not handled within
	// gInventory.notifyObservers.  The results will be handled in the next
	// idle tick instead.
	gInventory.removeObserver(this);
	doOnIdle(boost::bind(&LLInitialWearablesFetch::processContents,this));
}

void LLInitialWearablesFetch::add(InitialWearableData &data)

{
	mAgentInitialWearables.push_back(data);
}

void LLInitialWearablesFetch::processContents()
{
	// Fetch the wearable items from the Current Outfit Folder
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	LLFindWearables is_wearable;
	gInventory.collectDescendentsIf(mCompleteFolders.front(), cat_array, wearable_array, 
									LLInventoryModel::EXCLUDE_TRASH, is_wearable);

	LLAppearanceMgr::instance().setAttachmentInvLinkEnable(true);
	if (wearable_array.count() > 0)
	{
		LLAppearanceMgr::instance().updateAppearanceFromCOF();
	}
	else
	{
		// if we're constructing the COF from the wearables message, we don't have a proper outfit link
		LLAppearanceMgr::instance().setOutfitDirty(true);
		processWearablesMessage();
	}
	delete this;
}

class LLFetchAndLinkObserver: public LLInventoryFetchObserver
{
public:
	LLFetchAndLinkObserver(LLInventoryFetchObserver::item_ref_t& ids):
		m_ids(ids),
		LLInventoryFetchObserver(true) // retry for missing items
	{
	}
	~LLFetchAndLinkObserver()
	{
	}
	virtual void done()
	{
		gInventory.removeObserver(this);

		// Link to all fetched items in COF.
		LLPointer<LLInventoryCallback> link_waiter = new LLUpdateAppearanceOnDestroy;
		for (LLInventoryFetchObserver::item_ref_t::iterator it = m_ids.begin();
			 it != m_ids.end();
			 ++it)
		{
			LLUUID id = *it;
			LLViewerInventoryItem *item = gInventory.getItem(*it);
			if (!item)
			{
				llwarns << "fetch failed!" << llendl;
				continue;
			}

			link_inventory_item(gAgent.getID(),
								item->getLinkedUUID(),
								LLAppearanceMgr::instance().getCOF(),
								item->getName(),
								LLAssetType::AT_LINK,
								link_waiter);
		}
	}
private:
	LLInventoryFetchObserver::item_ref_t m_ids;
};

void LLInitialWearablesFetch::processWearablesMessage()
{
	if (!mAgentInitialWearables.empty()) // We have an empty current outfit folder, use the message data instead.
	{
		const LLUUID current_outfit_id = LLAppearanceMgr::instance().getCOF();
		LLInventoryFetchObserver::item_ref_t ids;
		for (U8 i = 0; i < mAgentInitialWearables.size(); ++i)
		{
			// Populate the current outfit folder with links to the wearables passed in the message
			InitialWearableData *wearable_data = new InitialWearableData(mAgentInitialWearables[i]); // This will be deleted in the callback.
			
			if (wearable_data->mAssetID.notNull())
			{
				ids.push_back(wearable_data->mItemID);
			}
			else
			{
				llinfos << "Invalid wearable, type " << wearable_data->mType << " itemID "
				<< wearable_data->mItemID << " assetID " << wearable_data->mAssetID << llendl;
				delete wearable_data;
			}
		}

		// Add all current attachments to the requested items as well.
		if (isAgentAvatarValid())
		{
			for (LLVOAvatar::attachment_map_t::const_iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
				 iter != gAgentAvatarp->mAttachmentPoints.end(); ++iter)
			{
				LLViewerJointAttachment* attachment = iter->second;
				if (!attachment) continue;
				for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
					 attachment_iter != attachment->mAttachedObjects.end();
					 ++attachment_iter)
				{
					LLViewerObject* attached_object = (*attachment_iter);
					if (!attached_object) continue;
					const LLUUID& item_id = attached_object->getItemID();
					if (item_id.isNull()) continue;
					ids.push_back(item_id);
				}
			}
		}

		// Need to fetch the inventory items for ids, then create links to them after they arrive.
		LLFetchAndLinkObserver *fetcher = new LLFetchAndLinkObserver(ids);
		fetcher->fetchItems(ids);
		// If no items to be fetched, done will never be triggered.
		// TODO: Change LLInventoryFetchObserver::fetchItems to trigger done() on this condition.
		if (fetcher->isEverythingComplete())
		{
			fetcher->done();
		}
		else
		{
			gInventory.addObserver(fetcher);
		}
	}
	else
	{
		LL_WARNS("Wearables") << "No current outfit folder items found and no initial wearables fallback message received." << LL_ENDL;
	}
}

LLLibraryOutfitsFetch::LLLibraryOutfitsFetch() : 
	mCurrFetchStep(LOFS_FOLDER), 
	mOutfitsPopulated(false) 
{
	mMyOutfitsID = LLUUID::null;
	mClothingID = LLUUID::null;
	mLibraryClothingID = LLUUID::null;
	mImportedClothingID = LLUUID::null;
	mImportedClothingName = "Imported Library Clothing";
}

LLLibraryOutfitsFetch::~LLLibraryOutfitsFetch()
{
}

void LLLibraryOutfitsFetch::done()
{
	// Delay this until idle() routine, since it's a heavy operation and
	// we also can't have it run within notifyObservers.
	doOnIdle(boost::bind(&LLLibraryOutfitsFetch::doneIdle,this));
	gInventory.removeObserver(this); // Prevent doOnIdle from being added twice.
}

void LLLibraryOutfitsFetch::doneIdle()
{
	gInventory.addObserver(this); // Add this back in since it was taken out during ::done()
	
	switch (mCurrFetchStep)
	{
		case LOFS_FOLDER:
			folderDone();
			mCurrFetchStep = LOFS_OUTFITS;
			break;
		case LOFS_OUTFITS:
			outfitsDone();
			mCurrFetchStep = LOFS_LIBRARY;
			break;
		case LOFS_LIBRARY:
			libraryDone();
			mCurrFetchStep = LOFS_IMPORTED;
			break;
		case LOFS_IMPORTED:
			importedFolderDone();
			mCurrFetchStep = LOFS_CONTENTS;
			break;
		case LOFS_CONTENTS:
			contentsDone();
			break;
		default:
			llwarns << "Got invalid state for outfit fetch: " << mCurrFetchStep << llendl;
			mOutfitsPopulated = TRUE;
			break;
	}

	// We're completely done.  Cleanup.
	if (mOutfitsPopulated)
	{
		gInventory.removeObserver(this);
		delete this;
		return;
	}
}

void LLLibraryOutfitsFetch::folderDone(void)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	gInventory.collectDescendents(mMyOutfitsID, cat_array, wearable_array, 
								  LLInventoryModel::EXCLUDE_TRASH);
	// Early out if we already have items in My Outfits.
	if (cat_array.count() > 0 || wearable_array.count() > 0)
	{
		mOutfitsPopulated = true;
		return;
	}

	mClothingID = gInventory.findCategoryUUIDForType(LLFolderType::FT_CLOTHING);
	mLibraryClothingID = gInventory.findCategoryUUIDForType(LLFolderType::FT_CLOTHING, false, true);

	// If Library->Clothing->Initial Outfits exists, use that.
	LLNameCategoryCollector matchFolderFunctor("Initial Outfits");
	gInventory.collectDescendentsIf(mLibraryClothingID,
									cat_array, wearable_array, 
									LLInventoryModel::EXCLUDE_TRASH,
									matchFolderFunctor);
	if (cat_array.count() > 0)
	{
		const LLViewerInventoryCategory *cat = cat_array.get(0);
		mLibraryClothingID = cat->getUUID();
	}

	mCompleteFolders.clear();
	
	// Get the complete information on the items in the inventory.
	uuid_vec_t folders;
	folders.push_back(mClothingID);
	folders.push_back(mLibraryClothingID);
	fetchDescendents(folders);
	if (isEverythingComplete())
	{
		done();
	}
}

void LLLibraryOutfitsFetch::outfitsDone(void)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	uuid_vec_t folders;
	
	// Collect the contents of the Library's Clothing folder
	gInventory.collectDescendents(mLibraryClothingID, cat_array, wearable_array, 
								  LLInventoryModel::EXCLUDE_TRASH);
	
	llassert(cat_array.count() > 0);
	for (LLInventoryModel::cat_array_t::const_iterator iter = cat_array.begin();
		 iter != cat_array.end();
		 ++iter)
	{
		const LLViewerInventoryCategory *cat = iter->get();
		
		// Get the names and id's of every outfit in the library, skip "Ruth"
		// because it's a low quality legacy outfit
		if (cat->getName() != "Ruth")
		{
			// Get the name of every outfit in the library 
			folders.push_back(cat->getUUID());
			mLibraryClothingFolders.push_back(cat->getUUID());
		}
	}
	cat_array.clear();
	wearable_array.clear();

	// Check if you already have an "Imported Library Clothing" folder
	LLNameCategoryCollector matchFolderFunctor(mImportedClothingName);
	gInventory.collectDescendentsIf(mClothingID, 
									cat_array, wearable_array, 
									LLInventoryModel::EXCLUDE_TRASH,
									matchFolderFunctor);
	if (cat_array.size() > 0)
	{
		const LLViewerInventoryCategory *cat = cat_array.get(0);
		mImportedClothingID = cat->getUUID();
	}
	
	mCompleteFolders.clear();
	
	fetchDescendents(folders);
	if (isEverythingComplete())
	{
		done();
	}
}

class LLLibraryOutfitsCopyDone: public LLInventoryCallback
{
public:
	LLLibraryOutfitsCopyDone(LLLibraryOutfitsFetch * fetcher):
	mFireCount(0), mLibraryOutfitsFetcher(fetcher)
	{
	}
	
	virtual ~LLLibraryOutfitsCopyDone()
	{
		if (!LLApp::isExiting() && mLibraryOutfitsFetcher)
		{
			gInventory.addObserver(mLibraryOutfitsFetcher);
			mLibraryOutfitsFetcher->done();
		}
	}
	
	/* virtual */ void fire(const LLUUID& inv_item)
	{
		mFireCount++;
	}
private:
	U32 mFireCount;
	LLLibraryOutfitsFetch * mLibraryOutfitsFetcher;
};

// Copy the clothing folders from the library into the imported clothing folder
void LLLibraryOutfitsFetch::libraryDone(void)
{
	if (mImportedClothingID != LLUUID::null)
	{
		// Skip straight to fetching the contents of the imported folder
		importedFolderFetch();
		return;
	}

	// Remove observer; next autopopulation step will be triggered externally by LLLibraryOutfitsCopyDone.
	gInventory.removeObserver(this);
	
	LLPointer<LLInventoryCallback> copy_waiter = new LLLibraryOutfitsCopyDone(this);
	mImportedClothingID = gInventory.createNewCategory(mClothingID,
													   LLFolderType::FT_NONE,
													   mImportedClothingName);
	// Copy each folder from library into clothing unless it already exists.
	for (uuid_vec_t::const_iterator iter = mLibraryClothingFolders.begin();
		 iter != mLibraryClothingFolders.end();
		 ++iter)
	{
		const LLUUID& src_folder_id = (*iter); // Library clothing folder ID
		const LLViewerInventoryCategory *cat = gInventory.getCategory(src_folder_id);
		if (!cat)
		{
			llwarns << "Library folder import for uuid:" << src_folder_id << " failed to find folder." << llendl;
			continue;
		}
		
		if (!LLAppearanceMgr::getInstance()->getCanMakeFolderIntoOutfit(src_folder_id))
		{
			llinfos << "Skipping non-outfit folder name:" << cat->getName() << llendl;
			continue;
		}
		
		// Don't copy the category if it already exists.
		LLNameCategoryCollector matchFolderFunctor(cat->getName());
		LLInventoryModel::cat_array_t cat_array;
		LLInventoryModel::item_array_t wearable_array;
		gInventory.collectDescendentsIf(mImportedClothingID, 
										cat_array, wearable_array, 
										LLInventoryModel::EXCLUDE_TRASH,
										matchFolderFunctor);
		if (cat_array.size() > 0)
		{
			continue;
		}

		LLUUID dst_folder_id = gInventory.createNewCategory(mImportedClothingID,
															LLFolderType::FT_NONE,
															cat->getName());
		LLAppearanceMgr::getInstance()->shallowCopyCategoryContents(src_folder_id, dst_folder_id, copy_waiter);
	}
}

void LLLibraryOutfitsFetch::importedFolderFetch(void)
{
	// Fetch the contents of the Imported Clothing Folder
	uuid_vec_t folders;
	folders.push_back(mImportedClothingID);
	
	mCompleteFolders.clear();
	
	fetchDescendents(folders);
	if (isEverythingComplete())
	{
		done();
	}
}

void LLLibraryOutfitsFetch::importedFolderDone(void)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	uuid_vec_t folders;
	
	// Collect the contents of the Imported Clothing folder
	gInventory.collectDescendents(mImportedClothingID, cat_array, wearable_array, 
								  LLInventoryModel::EXCLUDE_TRASH);
	
	for (LLInventoryModel::cat_array_t::const_iterator iter = cat_array.begin();
		 iter != cat_array.end();
		 ++iter)
	{
		const LLViewerInventoryCategory *cat = iter->get();
		
		// Get the name of every imported outfit
		folders.push_back(cat->getUUID());
		mImportedClothingFolders.push_back(cat->getUUID());
	}
	
	mCompleteFolders.clear();
	fetchDescendents(folders);
	if (isEverythingComplete())
	{
		done();
	}
}

void LLLibraryOutfitsFetch::contentsDone(void)
{		
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	
	for (uuid_vec_t::const_iterator folder_iter = mImportedClothingFolders.begin();
		 folder_iter != mImportedClothingFolders.end();
		 ++folder_iter)
	{
		const LLUUID &folder_id = (*folder_iter);
		const LLViewerInventoryCategory *cat = gInventory.getCategory(folder_id);
		if (!cat)
		{
			llwarns << "Library folder import for uuid:" << folder_id << " failed to find folder." << llendl;
			continue;
		}
		
		// First, make a folder in the My Outfits directory.
		LLUUID new_outfit_folder_id = gInventory.createNewCategory(mMyOutfitsID, LLFolderType::FT_OUTFIT, cat->getName());
		
		cat_array.clear();
		wearable_array.clear();
		// Collect the contents of each imported clothing folder, so we can create new outfit links for it
		gInventory.collectDescendents(folder_id, cat_array, wearable_array, 
									  LLInventoryModel::EXCLUDE_TRASH);
		
		for (LLInventoryModel::item_array_t::const_iterator wearable_iter = wearable_array.begin();
			 wearable_iter != wearable_array.end();
			 ++wearable_iter)
		{
			const LLViewerInventoryItem *item = wearable_iter->get();
			link_inventory_item(gAgent.getID(),
								item->getLinkedUUID(),
								new_outfit_folder_id,
								item->getName(),
								LLAssetType::AT_LINK,
								NULL);
		}
	}

	mOutfitsPopulated = true;
}

