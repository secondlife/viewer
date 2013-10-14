/**
 * @file llpanellandmarkinfo.h
 * @brief Displays landmark info in Side Tray.
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

	/*virtual*/ void setInfoType(EInfoType type);

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
