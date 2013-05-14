/** 
 * @file llpanelplaces.h
 * @brief Side Bar "Places" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLPANELPLACES_H
#define LL_LLPANELPLACES_H

#include "lltimer.h"

#include "llpanel.h"

class LLInventoryItem;
class LLFilterEditor;
class LLLandmark;

class LLPanelLandmarkInfo;
class LLPanelPlaceProfile;

class LLPanelPickEdit;
class LLPanelPlaceInfo;
class LLPanelPlacesTab;
class LLParcelSelection;
class LLPlacesInventoryObserver;
class LLPlacesParcelObserver;
class LLRemoteParcelInfoObserver;
class LLTabContainer;
class LLToggleableMenu;
class LLMenuButton;

typedef std::pair<LLUUID, std::string>	folder_pair_t;

class LLPanelPlaces : public LLPanel
{
public:
	LLPanelPlaces();
	virtual ~LLPanelPlaces();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

	// Called on parcel selection change to update place information.
	void changedParcelSelection();
	// Called once on agent inventory first change to find out when inventory gets usable
	// and to create "My Landmarks" and "Teleport History" tabs.
	void createTabs();
	// Called when we receive the global 3D position of a parcel.
	void changedGlobalPos(const LLVector3d &global_pos);

	// Opens landmark info panel when agent creates or receives landmark.
	void showAddedLandmarkInfo(const uuid_vec_t& items);

	void setItem(LLInventoryItem* item);

	LLInventoryItem* getItem() { return mItem; }

	std::string getPlaceInfoType() { return mPlaceInfoType; }

	bool tabsCreated() { return mTabsCreated;}

	/*virtual*/ S32 notifyParent(const LLSD& info);

private:
	void onLandmarkLoaded(LLLandmark* landmark);
	void onFilterEdit(const std::string& search_string, bool force_filter);
	void onTabSelected();

	void onTeleportButtonClicked();
	void onShowOnMapButtonClicked();
	void onEditButtonClicked();
	void onSaveButtonClicked();
	void onCancelButtonClicked();
	void onOverflowButtonClicked();
	void onOverflowMenuItemClicked(const LLSD& param);
	bool onOverflowMenuItemEnable(const LLSD& param);
	void onCreateLandmarkButtonClicked(const LLUUID& folder_id);
	void onBackButtonClicked();
	void onProfileButtonClicked();

	void toggleMediaPanel();
	void togglePickPanel(BOOL visible);
	void togglePlaceInfoPanel(BOOL visible);

	/*virtual*/ void handleVisibilityChange(BOOL new_visibility);

	void updateVerbs();

	LLPanelPlaceInfo* getCurrentInfoPanel();

	LLFilterEditor*				mFilterEditor;
	LLPanelPlacesTab*			mActivePanel;
	LLTabContainer*				mTabContainer;
	LLPanelPlaceProfile*		mPlaceProfile;
	LLPanelLandmarkInfo*		mLandmarkInfo;

	LLPanelPickEdit*			mPickPanel;
	LLToggleableMenu*			mPlaceMenu;
	LLToggleableMenu*			mLandmarkMenu;

	LLButton*					mPlaceProfileBackBtn;
	LLButton*					mTeleportBtn;
	LLButton*					mShowOnMapBtn;
	LLButton*					mEditBtn;
	LLButton*					mSaveBtn;
	LLButton*					mCancelBtn;
	LLButton*					mCloseBtn;
	LLMenuButton*				mOverflowBtn;
	LLButton*					mPlaceInfoBtn;

	LLPlacesInventoryObserver*	mInventoryObserver;
	LLPlacesParcelObserver*		mParcelObserver;
	LLRemoteParcelInfoObserver* mRemoteParcelObserver;

	// Pointer to a landmark item or to a linked landmark
	LLPointer<LLInventoryItem>	mItem;

	// Absolute position of the location for teleport, may not
	// be available (hence zero)
	LLVector3d					mPosGlobal;

	// Sets a period of time during which the requested place information
	// is expected to be updated and doesn't need to be reset.
	LLTimer						mResetInfoTimer;

	// Information type currently shown in Place Information panel
	std::string					mPlaceInfoType;

	bool						isLandmarkEditModeOn;

	// Holds info whether "My Landmarks" and "Teleport History" tabs have been created.
	bool						mTabsCreated;

	LLSafeHandle<LLParcelSelection>	mParcel;

	boost::signals2::connection mAgentParcelChangedConnection;
};

#endif //LL_LLPANELPLACES_H
