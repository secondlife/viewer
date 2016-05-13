/** 
 * @file llfloateroutfitsnapshot.h
 * @brief Snapshot preview window for saving as an outfit thumbnail in visual outfit gallery
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2016, Linden Research, Inc.
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

#ifndef LL_LLFLOATEROUTFITSNAPSHOT_H
#define LL_LLFLOATEROUTFITSNAPSHOT_H

#include "llfloater.h"
#include "llfloatersnapshot.h"
#include "lloutfitgallery.h"

class LLSpinCtrl;

class LLFloaterOutfitSnapshot : public LLFloaterSnapshotBase
{
	LOG_CLASS(LLFloaterOutfitSnapshot);

public:

	LLFloaterOutfitSnapshot(const LLSD& key);
	virtual ~LLFloaterOutfitSnapshot();
    
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ S32 notify(const LLSD& info);
	
	static void update();

	// TODO: create a snapshot model instead
	static LLFloaterOutfitSnapshot* getInstance();
	static LLFloaterOutfitSnapshot* findInstance();
	static void saveTexture();
	static void postSave();
	static void postPanelSwitch();
	static LLPointer<LLImageFormatted> getImageData();
	static const LLVector3d& getPosTakenGlobal();

	static const LLRect& getThumbnailPlaceholderRect() { return sThumbnailPlaceholder->getRect(); }

    void setOutfitID(LLUUID id) { mOutfitID = id; }
    LLUUID getOutfitID() { return mOutfitID; }
    void setGallery(LLOutfitGallery* gallery) { mOutfitGallery = gallery; }
private:
	static LLUICtrl* sThumbnailPlaceholder;
	LLUICtrl *mRefreshBtn, *mRefreshLabel;
	LLUICtrl *mSucceessLblPanel, *mFailureLblPanel;

	class Impl;
	Impl& impl;

    LLUUID mOutfitID;
    LLOutfitGallery* mOutfitGallery;
};

class LLOutfitSnapshotFloaterView : public LLFloaterView
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLFloaterView::Params>
	{
	};

protected:
	LLOutfitSnapshotFloaterView (const Params& p);
	friend class LLUICtrlFactory;

public:
	virtual ~LLOutfitSnapshotFloaterView();

	/*virtual*/	BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);
	/*virtual*/	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/	BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/	BOOL handleHover(S32 x, S32 y, MASK mask);
};

extern LLOutfitSnapshotFloaterView* gOutfitSnapshotFloaterView;

#endif // LL_LLFLOATEROUTFITSNAPSHOT_H
