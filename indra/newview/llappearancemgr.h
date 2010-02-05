/** 
 * @file llappearancemgr.h
 * @brief Manager for initiating appearance changes on the viewer
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_LLAPPEARANCEMGR_H
#define LL_LLAPPEARANCEMGR_H

#include "llsingleton.h"
#include "llinventorymodel.h"
#include "llcallbacklist.h"

class LLWearable;
class LLWearableHoldingPattern;
class LLInventoryCallback;

class LLAppearanceManager: public LLSingleton<LLAppearanceManager>
{
	friend class LLSingleton<LLAppearanceManager>;
	
public:
	void updateAppearanceFromCOF();
	bool needToSaveCOF();
	void updateCOF(const LLUUID& category, bool append = false);
	void wearInventoryCategory(LLInventoryCategory* category, bool copy, bool append);
	void wearInventoryCategoryOnAvatar(LLInventoryCategory* category, bool append);
	void wearOutfitByName(const std::string& name);
	void changeOutfit(bool proceed, const LLUUID& category, bool append);

	// Copy all items and the src category itself.
	void shallowCopyCategory(const LLUUID& src_id, const LLUUID& dst_id,
							 LLPointer<LLInventoryCallback> cb);

	// Copy all items in a category.
	void shallowCopyCategoryContents(const LLUUID& src_id, const LLUUID& dst_id,
									 LLPointer<LLInventoryCallback> cb);

	// Find the Current Outfit folder.
	const LLUUID getCOF() const;

	// Finds the folder link to the currently worn outfit
	const LLViewerInventoryItem *getBaseOutfitLink();
	bool getBaseOutfitName(std::string &name);

	// Update the displayed outfit name in UI.
	void updatePanelOutfitName(const std::string& name);

	void createBaseOutfitLink(const LLUUID& category, LLPointer<LLInventoryCallback> link_waiter);

	void updateAgentWearables(LLWearableHoldingPattern* holder, bool append);

	// For debugging - could be moved elsewhere.
	void dumpCat(const LLUUID& cat_id, const std::string& msg);
	void dumpItemArray(const LLInventoryModel::item_array_t& items, const std::string& msg);

	// Attachment link management
	void unregisterAttachment(const LLUUID& item_id);
	void registerAttachment(const LLUUID& item_id);
	void setAttachmentInvLinkEnable(bool val);
	void linkRegisteredAttachments();

	// utility function for bulk linking.
	void linkAll(const LLUUID& category,
				 LLInventoryModel::item_array_t& items,
				 LLPointer<LLInventoryCallback> cb);

	// Add COF link to individual item.
	void addCOFItemLink(const LLUUID& item_id, bool do_update = true);
	void addCOFItemLink(const LLInventoryItem *item, bool do_update = true);

	// Remove COF entries
	void removeCOFItemLinks(const LLUUID& item_id, bool do_update = true);

	// Add COF link to ensemble folder.
	void addEnsembleLink(LLInventoryCategory* item, bool do_update = true);

	//has the current outfit changed since it was loaded?
	bool isOutfitDirty() { return mOutfitIsDirty; }

	// set false if you just loaded the outfit, true otherwise
	void setOutfitDirty(bool isDirty) { mOutfitIsDirty = isDirty; }
	
	// manually compare ouftit folder link to COF to see if outfit has changed.
	// should only be necessary to do on initial login.
	void updateIsDirty();

protected:
	LLAppearanceManager();
	~LLAppearanceManager();

private:

	void filterWearableItems(LLInventoryModel::item_array_t& items, S32 max_per_type);
	
	void getDescendentsOfAssetType(const LLUUID& category, 
										  LLInventoryModel::item_array_t& items,
										  LLAssetType::EType type,
										  bool follow_folder_links);

	void getUserDescendents(const LLUUID& category, 
								   LLInventoryModel::item_array_t& wear_items,
								   LLInventoryModel::item_array_t& obj_items,
								   LLInventoryModel::item_array_t& gest_items,
								   bool follow_folder_links);

	void purgeCategory(const LLUUID& category, bool keep_outfit_links);
	void purgeBaseOutfitLink(const LLUUID& category);

	std::set<LLUUID> mRegisteredAttachments;
	bool mAttachmentInvLinkEnabled;
	bool mOutfitIsDirty;

	//////////////////////////////////////////////////////////////////////////////////
	// Item-specific convenience functions 
public:
	// Is this in the COF?
	BOOL getIsInCOF(const LLUUID& obj_id) const;
	// Is this in the COF and can the user delete it from the COF?
	BOOL getIsProtectedCOFItem(const LLUUID& obj_id) const;
};

#define SUPPORT_ENSEMBLES 0

// Shim class and template function to allow arbitrary boost::bind
// expressions to be run as one-time idle callbacks.
template <typename T>
class OnIdleCallback
{
public:
	OnIdleCallback(T callable):
		mCallable(callable)
	{
	}
	static void onIdle(void *data)
	{
		gIdleCallbacks.deleteFunction(onIdle, data);
		OnIdleCallback<T>* self = reinterpret_cast<OnIdleCallback<T>*>(data);
		self->call();
		delete self;
	}
	void call()
	{
		mCallable();
	}
private:
	T mCallable;
};

template <typename T>
void doOnIdle(T callable)
{
	OnIdleCallback<T>* cb_functor = new OnIdleCallback<T>(callable);
	gIdleCallbacks.addFunction(&OnIdleCallback<T>::onIdle,cb_functor);
}

// Shim class and template function to allow arbitrary boost::bind
// expressions to be run as recurring idle callbacks.
template <typename T>
class OnIdleCallbackRepeating
{
public:
	OnIdleCallbackRepeating(T callable):
		mCallable(callable)
	{
	}
	// Will keep getting called until the callable returns false.
	static void onIdle(void *data)
	{
		OnIdleCallbackRepeating<T>* self = reinterpret_cast<OnIdleCallbackRepeating<T>*>(data);
		bool done = self->call();
		if (done)
		{
			gIdleCallbacks.deleteFunction(onIdle, data);
			delete self;
		}
	}
	bool call()
	{
		return mCallable();
	}
private:
	T mCallable;
};

template <typename T>
void doOnIdleRepeating(T callable)
{
	OnIdleCallbackRepeating<T>* cb_functor = new OnIdleCallbackRepeating<T>(callable);
	gIdleCallbacks.addFunction(&OnIdleCallbackRepeating<T>::onIdle,cb_functor);
}

#endif
