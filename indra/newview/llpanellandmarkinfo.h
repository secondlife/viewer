/**
 * @file llpanellandmarkinfo.h
 * @brief Displays landmark info in Side Tray.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 *
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_LLPANELLANDMARKINFO_H
#define LL_LLPANELLANDMARKINFO_H

#include "llpanelplaceinfo.h"

class LLComboBox;
class LLLineEditor;
class LLTextEditor;

class LLPanelLandmarkInfo : public LLPanelPlaceInfo
{
public:
	LLPanelLandmarkInfo();
	/*virtual*/ ~LLPanelLandmarkInfo();

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void resetLocation();

	/*virtual*/ void setInfoType(INFO_TYPE type);

	/*virtual*/ void processParcelInfo(const LLParcelData& parcel_data);

	// Displays landmark owner, creator and creation date info.
	void displayItemInfo(const LLInventoryItem* pItem);

	void toggleLandmarkEditMode(BOOL enabled);

	const std::string& getLandmarkTitle() const;
	const std::string getLandmarkNotes() const;
	const LLUUID getLandmarkFolder() const;

	// Select current landmark folder in combobox.
	BOOL setLandmarkFolder(const LLUUID& id);

	// Create a landmark for the current location
	// in a folder specified by folder_id.
	void createLandmark(const LLUUID& folder_id);

	static std::string getFullFolderName(const LLViewerInventoryCategory* cat);

private:
	void populateFoldersList();

	LLTextBox*			mOwner;
	LLTextBox*			mCreator;
	LLTextBox*			mCreated;
	LLTextBox*			mLandmarkTitle;
	LLLineEditor*		mLandmarkTitleEditor;
	LLTextEditor*		mNotesEditor;
	LLComboBox*			mFolderCombo;
};

#endif // LL_LLPANELLANDMARKINFO_H
