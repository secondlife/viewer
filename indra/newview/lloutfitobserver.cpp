/**
 * @file lloutfitobserver.cpp
 * @brief Outfit observer facade.
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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

#include "llappearancemgr.h"
#include "lloutfitobserver.h"
#include "llinventorymodel.h"
#include "llviewerinventory.h"

LLOutfitObserver::LLOutfitObserver() :
	mCOFLastVersion(LLViewerInventoryCategory::VERSION_UNKNOWN)
{
	mItemNameHash.finalize();
	gInventory.addObserver(this);
}

LLOutfitObserver::~LLOutfitObserver()
{
	if (gInventory.containsObserver(this))
	{
		gInventory.removeObserver(this);
	}
}

void LLOutfitObserver::changed(U32 mask)
{
	if (!gInventory.isInventoryUsable())
		return;

	checkCOF();

	checkBaseOutfit();
}

// static
S32 LLOutfitObserver::getCategoryVersion(const LLUUID& cat_id)
{
	LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
	if (!cat)
		return LLViewerInventoryCategory::VERSION_UNKNOWN;

	return cat->getVersion();
}

// static
const std::string& LLOutfitObserver::getCategoryName(const LLUUID& cat_id)
{
	LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
	if (!cat)
		return LLStringUtil::null;

	return cat->getName();
}

bool LLOutfitObserver::checkCOF()
{
	LLUUID cof = LLAppearanceMgr::getInstance()->getCOF();
	if (cof.isNull())
		return false;

	bool cof_changed = false;
	LLMD5 item_name_hash = gInventory.hashDirectDescendentNames(cof);
	if (item_name_hash != mItemNameHash)
	{
		cof_changed = true;
		mItemNameHash = item_name_hash;
	}

	S32 cof_version = getCategoryVersion(cof);
	if (cof_version != mCOFLastVersion)
	{
		cof_changed = true;
		mCOFLastVersion = cof_version;
	}

	if (!cof_changed)
		return false;
	
	// dirtiness state should be updated before sending signal
	LLAppearanceMgr::getInstance()->updateIsDirty();
	mCOFChanged();

	return true;
}

void LLOutfitObserver::checkBaseOutfit()
{
	LLUUID baseoutfit_id =
			LLAppearanceMgr::getInstance()->getBaseOutfitUUID();

	if (baseoutfit_id == mBaseOutfitId)
	{
		if (baseoutfit_id.isNull())
			return;

		const S32 baseoutfit_ver = getCategoryVersion(baseoutfit_id);
		const std::string& baseoutfit_name = getCategoryName(baseoutfit_id);

		if (baseoutfit_ver == mBaseOutfitLastVersion
				// renaming category doesn't change version, so it's need to check it
				&& baseoutfit_name == mLastBaseOutfitName)
			return;
	}
	else
	{
		mBaseOutfitId = baseoutfit_id;
		mBOFReplaced();

		if (baseoutfit_id.isNull())
			return;
	}

	mBaseOutfitLastVersion = getCategoryVersion(mBaseOutfitId);
	mLastBaseOutfitName = getCategoryName(baseoutfit_id);

	LLAppearanceMgr& app_mgr = LLAppearanceMgr::instance();
	// dirtiness state should be updated before sending signal
	app_mgr.updateIsDirty();
	mBOFChanged();

	if (mLastOutfitDirtiness != app_mgr.isOutfitDirty())
	{
		if(!app_mgr.isOutfitDirty())
		{
			mCOFSaved();
		}
		mLastOutfitDirtiness = app_mgr.isOutfitDirty();
	}
}
