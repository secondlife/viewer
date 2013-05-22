/**
 * @file llpanellandmarkinfo.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llpanellandmarkinfo.h"

#include "llcombobox.h"
#include "lliconctrl.h"
#include "llinventoryfunctions.h"
#include "lllineeditor.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "lltrans.h"

#include "llagent.h"
#include "llagentui.h"
#include "lllandmarkactions.h"
#include "llslurl.h"
#include "llviewerinventory.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"

//----------------------------------------------------------------------------
// Aux types and methods
//----------------------------------------------------------------------------

typedef std::pair<LLUUID, std::string> folder_pair_t;

static bool cmp_folders(const folder_pair_t& left, const folder_pair_t& right);
static void collectLandmarkFolders(LLInventoryModel::cat_array_t& cats);

static LLRegisterPanelClassWrapper<LLPanelLandmarkInfo> t_landmark_info("panel_landmark_info");

// Statics for textures filenames
static std::string icon_pg;
static std::string icon_m;
static std::string icon_r;

LLPanelLandmarkInfo::LLPanelLandmarkInfo()
:	LLPanelPlaceInfo()
{}

// virtual
LLPanelLandmarkInfo::~LLPanelLandmarkInfo()
{}

// virtual
BOOL LLPanelLandmarkInfo::postBuild()
{
	LLPanelPlaceInfo::postBuild();

	mOwner = getChild<LLTextBox>("owner");
	mCreator = getChild<LLTextBox>("creator");
	mCreated = getChild<LLTextBox>("created");

	mLandmarkTitle = getChild<LLTextBox>("title_value");
	mLandmarkTitleEditor = getChild<LLLineEditor>("title_editor");
	mNotesEditor = getChild<LLTextEditor>("notes_editor");
	mFolderCombo = getChild<LLComboBox>("folder_combo");

	icon_pg = getString("icon_PG");
	icon_m = getString("icon_M");
	icon_r = getString("icon_R");

	return TRUE;
}

// virtual
void LLPanelLandmarkInfo::resetLocation()
{
	LLPanelPlaceInfo::resetLocation();

	std::string loading = LLTrans::getString("LoadingData");
	mCreator->setText(loading);
	mOwner->setText(loading);
	mCreated->setText(loading);
	mLandmarkTitle->setText(LLStringUtil::null);
	mLandmarkTitleEditor->setText(LLStringUtil::null);
	mNotesEditor->setText(LLStringUtil::null);
}

// virtual
void LLPanelLandmarkInfo::setInfoType(EInfoType type)
{
	LLPanel* landmark_info_panel = getChild<LLPanel>("landmark_info_panel");

	bool is_info_type_create_landmark = type == CREATE_LANDMARK;

	landmark_info_panel->setVisible(type == LANDMARK);

	getChild<LLTextBox>("folder_label")->setVisible(is_info_type_create_landmark);
	mFolderCombo->setVisible(is_info_type_create_landmark);

	switch(type)
	{
		case CREATE_LANDMARK:
		{
			mCurrentTitle = getString("title_create_landmark");

			mLandmarkTitle->setVisible(FALSE);
			mLandmarkTitleEditor->setVisible(TRUE);
			mNotesEditor->setEnabled(TRUE);

			LLViewerParcelMgr* parcel_mgr = LLViewerParcelMgr::getInstance();
			std::string name = parcel_mgr->getAgentParcelName();
			LLVector3 agent_pos = gAgent.getPositionAgent();

			if (name.empty())
			{
				S32 region_x = llround(agent_pos.mV[VX]);
				S32 region_y = llround(agent_pos.mV[VY]);
				S32 region_z = llround(agent_pos.mV[VZ]);

				std::string region_name;
				LLViewerRegion* region = parcel_mgr->getSelectionRegion();
				if (region)
				{
					region_name = region->getName();
				}
				else
				{
					region_name = getString("unknown");
				}

				mLandmarkTitleEditor->setText(llformat("%s (%d, %d, %d)",
									  region_name.c_str(), region_x, region_y, region_z));
			}
			else
			{
				mLandmarkTitleEditor->setText(name);
			}

			std::string desc;
			LLAgentUI::buildLocationString(desc, LLAgentUI::LOCATION_FORMAT_FULL, agent_pos);
			mNotesEditor->setText(desc);

			// Moved landmark creation here from LLPanelLandmarkInfo::processParcelInfo()
			// because we use only agent's current coordinates instead of waiting for
			// remote parcel request to complete.
			if (!LLLandmarkActions::landmarkAlreadyExists())
			{
				createLandmark(LLUUID());
			}
		}
		break;

		case LANDMARK:
		default:
			mCurrentTitle = getString("title_landmark");

			mLandmarkTitle->setVisible(TRUE);
			mLandmarkTitleEditor->setVisible(FALSE);
			mNotesEditor->setEnabled(FALSE);
		break;
	}

	populateFoldersList();

	// Prevent the floater from losing focus (if the sidepanel is undocked).
	setFocus(TRUE);

	LLPanelPlaceInfo::setInfoType(type);
}

// virtual
void LLPanelLandmarkInfo::processParcelInfo(const LLParcelData& parcel_data)
{
	LLPanelPlaceInfo::processParcelInfo(parcel_data);

	// HACK: Flag 0x2 == adult region,
	// Flag 0x1 == mature region, otherwise assume PG
	if (parcel_data.flags & 0x2)
	{
		mMaturityRatingIcon->setValue(icon_r);
		mMaturityRatingText->setText(LLViewerRegion::accessToString(SIM_ACCESS_ADULT));
	}
	else if (parcel_data.flags & 0x1)
	{
		mMaturityRatingIcon->setValue(icon_m);
		mMaturityRatingText->setText(LLViewerRegion::accessToString(SIM_ACCESS_MATURE));
	}
	else
	{
		mMaturityRatingIcon->setValue(icon_pg);
		mMaturityRatingText->setText(LLViewerRegion::accessToString(SIM_ACCESS_PG));
	}

	LLSD info;
	info["update_verbs"] = true;
	info["global_x"] = parcel_data.global_x;
	info["global_y"] = parcel_data.global_y;
	info["global_z"] = parcel_data.global_z;
	notifyParent(info);
}

void LLPanelLandmarkInfo::displayItemInfo(const LLInventoryItem* pItem)
{
	if (!pItem)
		return;

	if(!gCacheName)
		return;

	const LLPermissions& perm = pItem->getPermissions();

	//////////////////
	// CREATOR NAME //
	//////////////////
	if (pItem->getCreatorUUID().notNull())
	{
		// IDEVO
		LLUUID creator_id = pItem->getCreatorUUID();
		std::string name =
			LLSLURL("agent", creator_id, "inspect").getSLURLString();
		mCreator->setText(name);
	}
	else
	{
		mCreator->setText(getString("unknown"));
	}

	////////////////
	// OWNER NAME //
	////////////////
	if(perm.isOwned())
	{
		std::string name;
		if (perm.isGroupOwned())
		{
			LLUUID group_id = perm.getGroup();
			name = LLSLURL("group", group_id, "inspect").getSLURLString();
		}
		else
		{
			LLUUID owner_id = perm.getOwner();
			name = LLSLURL("agent", owner_id, "inspect").getSLURLString();
		}
		mOwner->setText(name);
	}
	else
	{
		mOwner->setText(getString("public"));
	}

	//////////////////
	// ACQUIRE DATE //
	//////////////////
	time_t time_utc = pItem->getCreationDate();
	if (0 == time_utc)
	{
		mCreated->setText(getString("unknown"));
	}
	else
	{
		std::string timeStr = getString("acquired_date");
		LLSD substitution;
		substitution["datetime"] = (S32) time_utc;
		LLStringUtil::format (timeStr, substitution);
		mCreated->setText(timeStr);
	}

	mLandmarkTitle->setText(pItem->getName());
	mLandmarkTitleEditor->setText(pItem->getName());
	mNotesEditor->setText(pItem->getDescription());
}

void LLPanelLandmarkInfo::toggleLandmarkEditMode(BOOL enabled)
{
	// If switching to edit mode while creating landmark
	// the "Create Landmark" title remains.
	if (enabled && mInfoType != CREATE_LANDMARK)
	{
		mTitle->setText(getString("title_edit_landmark"));
	}
	else
	{
		mTitle->setText(mCurrentTitle);

		mLandmarkTitle->setText(mLandmarkTitleEditor->getText());
	}

	if (mNotesEditor->getReadOnly() ==  (enabled == TRUE))
	{
		mLandmarkTitle->setVisible(!enabled);
		mLandmarkTitleEditor->setVisible(enabled);
		mNotesEditor->setReadOnly(!enabled);
		mFolderCombo->setVisible(enabled);
		getChild<LLTextBox>("folder_label")->setVisible(enabled);

		// HACK: To change the text color in a text editor
		// when it was enabled/disabled we set the text once again.
		mNotesEditor->setText(mNotesEditor->getText());
	}

	// Prevent the floater from losing focus (if the sidepanel is undocked).
	setFocus(TRUE);
}

const std::string& LLPanelLandmarkInfo::getLandmarkTitle() const
{
	return mLandmarkTitleEditor->getText();
}

const std::string LLPanelLandmarkInfo::getLandmarkNotes() const
{
	return mNotesEditor->getText();
}

const LLUUID LLPanelLandmarkInfo::getLandmarkFolder() const
{
	return mFolderCombo->getValue().asUUID();
}

BOOL LLPanelLandmarkInfo::setLandmarkFolder(const LLUUID& id)
{
	return mFolderCombo->setCurrentByID(id);
}

void LLPanelLandmarkInfo::createLandmark(const LLUUID& folder_id)
{
	std::string name = mLandmarkTitleEditor->getText();
	std::string desc = mNotesEditor->getText();

	LLStringUtil::trim(name);
	LLStringUtil::trim(desc);

	// If typed name is empty use the parcel name instead.
	if (name.empty())
	{
		name = mParcelName->getText();

		// If no parcel exists use the region name instead.
		if (name.empty())
		{
			name = mRegionName->getText();
		}
	}

	LLStringUtil::replaceChar(desc, '\n', ' ');

	// If no folder chosen use the "Landmarks" folder.
	LLLandmarkActions::createLandmarkHere(name, desc,
		folder_id.notNull() ? folder_id : gInventory.findCategoryUUIDForType(LLFolderType::FT_LANDMARK));
}

// static
std::string LLPanelLandmarkInfo::getFullFolderName(const LLViewerInventoryCategory* cat)
{
	std::string name;
	LLUUID parent_id;

	llassert(cat);
	if (cat)
	{
		name = cat->getName();
		parent_id = cat->getParentUUID();
		bool is_under_root_category = parent_id == gInventory.getRootFolderID();

		// we don't want "My Inventory" to appear in the name
		while ((parent_id = cat->getParentUUID()).notNull())
		{
			cat = gInventory.getCategory(parent_id);
			llassert(cat);
			if (cat)
			{
				if (is_under_root_category || cat->getParentUUID() == gInventory.getRootFolderID())
				{
					std::string localized_name;

					// Looking for translation only for protected type categories
					// to avoid warnings about non existent string in strings.xml.
					bool is_protected_type = LLFolderType::lookupIsProtectedType(cat->getPreferredType());

					if (is_under_root_category)
					{
						// translate category name, if it's right below the root
						bool is_found = is_protected_type && LLTrans::findString(localized_name, "InvFolder " + name);
						name = is_found ? localized_name : name;
					}
					else
					{
						bool is_found = is_protected_type && LLTrans::findString(localized_name, "InvFolder " + cat->getName());

						// add translated category name to folder's full name
						name = (is_found ? localized_name : cat->getName()) + "/" + name;
					}

					break;
				}
				else
				{
					name = cat->getName() + "/" + name;
				}
			}
		}
	}

	return name;
}

void LLPanelLandmarkInfo::populateFoldersList()
{
	// Collect all folders that can contain landmarks.
	LLInventoryModel::cat_array_t cats;
	collectLandmarkFolders(cats);

	mFolderCombo->removeall();

	// Put the "Landmarks" folder first in list.
	LLUUID landmarks_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_LANDMARK);
	const LLViewerInventoryCategory* lmcat = gInventory.getCategory(landmarks_id);
	if (!lmcat)
	{
		llwarns << "Cannot find the landmarks folder" << llendl;
	}
	else
	{
		std::string cat_full_name = getFullFolderName(lmcat);
		mFolderCombo->add(cat_full_name, lmcat->getUUID());
	}

	typedef std::vector<folder_pair_t> folder_vec_t;
	folder_vec_t folders;
	// Sort the folders by their full name.
	for (S32 i = 0; i < cats.count(); i++)
	{
		const LLViewerInventoryCategory* cat = cats.get(i);
		std::string cat_full_name = getFullFolderName(cat);
		folders.push_back(folder_pair_t(cat->getUUID(), cat_full_name));
	}
	sort(folders.begin(), folders.end(), cmp_folders);

	// Finally, populate the combobox.
	for (folder_vec_t::const_iterator it = folders.begin(); it != folders.end(); it++)
		mFolderCombo->add(it->second, LLSD(it->first));
}

static bool cmp_folders(const folder_pair_t& left, const folder_pair_t& right)
{
	return left.second < right.second;
}

static void collectLandmarkFolders(LLInventoryModel::cat_array_t& cats)
{
	LLUUID landmarks_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_LANDMARK);

	// Add descendent folders of the "Landmarks" category.
	LLInventoryModel::item_array_t items; // unused
	LLIsType is_category(LLAssetType::AT_CATEGORY);
	gInventory.collectDescendentsIf(
		landmarks_id,
		cats,
		items,
		LLInventoryModel::EXCLUDE_TRASH,
		is_category);

	// Add the "My Favorites" category.
	LLUUID favorites_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE);
	LLViewerInventoryCategory* favorites_cat = gInventory.getCategory(favorites_id);
	if (!favorites_cat)
	{
		llwarns << "Cannot find the favorites folder" << llendl;
	}
	else
	{
		cats.put(favorites_cat);
	}
}
