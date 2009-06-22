/** 
 * @file llpanelplaces.h
 * @brief Side Bar "Places" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLPANELPLACES_H
#define LL_LLPANELPLACES_H

#include "llpanel.h"

#include "llinventory.h"

#include "llinventorymodel.h"
#include "llpanelplaceinfo.h"

class LLPanelPlacesTab;
class LLSearchEditor;
class LLTabContainer;

class LLPanelPlaces : public LLPanel, LLInventoryObserver
{
public:
	enum PLACE_INFO_TYPE
	{
		AGENT,
		LANDMARK,
		TELEPORT_HISTORY
	};

	LLPanelPlaces();
	virtual ~LLPanelPlaces();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();
	/*virtual*/ void changed(U32 mask);
	/*virtual*/ void onOpen(const LLSD& key);

	void onSearchEdit(const std::string& search_string);
	void onTabSelected();
	//void onAddLandmarkButtonClicked();
	//void onCopySLURLButtonClicked();
	void onShareButtonClicked();
	void onTeleportButtonClicked();
	void onShowOnMapButtonClicked();
	void onBackButtonClicked();
	void togglePlaceInfoPanel(BOOL visible);

private:
	LLSearchEditor*			mSearchEditor;
	LLPanelPlacesTab*		mActivePanel;
	LLTabContainer*			mTabContainer;
	LLPanelPlaceInfo*		mPlaceInfo;
	std::string				mFilterSubString;

	// Place information type currently shown in Information panel
	S32						mPlaceInfoType;
};

#endif //LL_LLPANELPLACES_H
