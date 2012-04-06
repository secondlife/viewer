/** 
 * @file llfloaterpathfindingcharacters.cpp
 * @author William Todd Stinson
 * @brief "Pathfinding characters" floater, allowing for identification of pathfinding characters and their cpu usage.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#include "llfloater.h"
#include "llfloaterpathfindingcharacters.h"
#include "llpathfindingcharacterlist.h"
#include "llsd.h"
#include "llagent.h"
#include "llhandle.h"
#include "llfloaterreg.h"
#include "lltextbase.h"
#include "llscrolllistitem.h"
#include "llscrolllistctrl.h"
#include "llcheckboxctrl.h"
#include "llradiogroup.h"
#include "llbutton.h"
#include "llresmgr.h"
#include "llviewerregion.h"
#include "llhttpclient.h"
#include "lluuid.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewermenu.h"
#include "llselectmgr.h"
#include "llenvmanager.h"

//---------------------------------------------------------------------------
// LLFloaterPathfindingCharacters
//---------------------------------------------------------------------------

BOOL LLFloaterPathfindingCharacters::postBuild()
{
	mCharactersScrollList = findChild<LLScrollListCtrl>("pathfinding_characters");
	llassert(mCharactersScrollList != NULL);
	mCharactersScrollList->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onCharactersSelectionChange, this));
	mCharactersScrollList->sortByColumnIndex(0, true);

	mCharactersStatus = findChild<LLTextBase>("characters_status");
	llassert(mCharactersStatus != NULL);

	mRefreshListButton = findChild<LLButton>("refresh_characters_list");
	llassert(mRefreshListButton != NULL);
	mRefreshListButton->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onRefreshCharactersClicked, this));

	mSelectAllButton = findChild<LLButton>("select_all_characters");
	llassert(mSelectAllButton != NULL);
	mSelectAllButton->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onSelectAllCharactersClicked, this));

	mSelectNoneButton = findChild<LLButton>("select_none_characters");
	llassert(mSelectNoneButton != NULL);
	mSelectNoneButton->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onSelectNoneCharactersClicked, this));

	mShowBeaconCheckBox = findChild<LLCheckBoxCtrl>("show_beacon");
	llassert(mShowBeaconCheckBox != NULL);

	mTakeButton = findChild<LLButton>("take_characters");
	llassert(mTakeButton != NULL)
	mTakeButton->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onTakeCharactersClicked, this));

	mTakeCopyButton = findChild<LLButton>("take_copy_characters");
	llassert(mTakeCopyButton != NULL)
	mTakeCopyButton->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onTakeCopyCharactersClicked, this));

	mReturnButton = findChild<LLButton>("return_characters");
	llassert(mReturnButton != NULL)
	mReturnButton->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onReturnCharactersClicked, this));

	mDeleteButton = findChild<LLButton>("delete_characters");
	llassert(mDeleteButton != NULL)
	mDeleteButton->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onDeleteCharactersClicked, this));

	mTeleportButton = findChild<LLButton>("teleport_to_character");
	llassert(mTeleportButton != NULL)
	mTeleportButton->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onTeleportCharacterToMeClicked, this));

	return LLFloater::postBuild();
}

void LLFloaterPathfindingCharacters::onOpen(const LLSD& pKey)
{
	LLFloater::onOpen(pKey);

	requestGetCharacters();
	selectNoneCharacters();
	mCharactersScrollList->setCommitOnSelectionChange(true);

	if (!mSelectionUpdateSlot.connected())
	{
		mSelectionUpdateSlot = LLSelectMgr::getInstance()->mUpdateSignal.connect(boost::bind(&LLFloaterPathfindingCharacters::updateControls, this));
	}

	if (!mRegionBoundarySlot.connected())
	{
		mRegionBoundarySlot = LLEnvManagerNew::instance().setRegionChangeCallback(boost::bind(&LLFloaterPathfindingCharacters::onRegionBoundaryCross, this));
	}
}

void LLFloaterPathfindingCharacters::onClose(bool pAppQuitting)
{
	if (mRegionBoundarySlot.connected())
	{
		mRegionBoundarySlot.disconnect();
	}

	if (mSelectionUpdateSlot.connected())
	{
		mSelectionUpdateSlot.disconnect();
	}

	mCharactersScrollList->setCommitOnSelectionChange(false);
	selectNoneCharacters();
	if (mCharacterSelection.notNull())
	{
		mCharacterSelection.clear();
	}

	LLFloater::onClose(pAppQuitting);
}

void LLFloaterPathfindingCharacters::draw()
{
	if (mShowBeaconCheckBox->get())
	{
		std::vector<LLScrollListItem*> selectedItems = mCharactersScrollList->getAllSelected();
		if (!selectedItems.empty())
		{
			int numSelectedItems = selectedItems.size();

			std::vector<LLViewerObject *> viewerObjects;
			viewerObjects.reserve(numSelectedItems);

			for (std::vector<LLScrollListItem*>::const_iterator selectedItemIter = selectedItems.begin();
				selectedItemIter != selectedItems.end(); ++selectedItemIter)
			{
				const LLScrollListItem *selectedItem = *selectedItemIter;

				const std::string &objectName = selectedItem->getColumn(0)->getValue().asString();

				LLViewerObject *viewerObject = gObjectList.findObject(selectedItem->getUUID());
				if (viewerObject != NULL)
				{
					gObjectList.addDebugBeacon(viewerObject->getPositionAgent(), objectName, LLColor4(0.f, 0.f, 1.f, 0.8f), LLColor4(1.f, 1.f, 1.f, 1.f), 6);
				}
			}
		}
	}

	LLFloater::draw();
}

void LLFloaterPathfindingCharacters::openCharactersViewer()
{
	LLFloaterReg::toggleInstanceOrBringToFront("pathfinding_characters");
}

LLFloaterPathfindingCharacters::LLFloaterPathfindingCharacters(const LLSD& pSeed)
	: LLFloater(pSeed),
	mCharactersScrollList(NULL),
	mCharactersStatus(NULL),
	mRefreshListButton(NULL),
	mSelectAllButton(NULL),
	mSelectNoneButton(NULL),
	mShowBeaconCheckBox(NULL),
	mTakeButton(NULL),
	mTakeCopyButton(NULL),
	mReturnButton(NULL),
	mDeleteButton(NULL),
	mTeleportButton(NULL),
	mMessagingState(kMessagingUnknown),
	mMessagingRequestId(0U),
	mCharacterListPtr(),
	mCharacterSelection(),
	mSelectionUpdateSlot()
{
}

LLFloaterPathfindingCharacters::~LLFloaterPathfindingCharacters()
{
}

LLFloaterPathfindingCharacters::EMessagingState LLFloaterPathfindingCharacters::getMessagingState() const
{
	return mMessagingState;
}

void LLFloaterPathfindingCharacters::setMessagingState(EMessagingState pMessagingState)
{
	mMessagingState = pMessagingState;
	updateControls();
}

void LLFloaterPathfindingCharacters::requestGetCharacters()
{
	switch (LLPathfindingManager::getInstance()->requestGetCharacters(++mMessagingRequestId, boost::bind(&LLFloaterPathfindingCharacters::handleNewCharacters, this, _1, _2, _3)))
	{
	case LLPathfindingManager::kRequestStarted :
		setMessagingState(kMessagingGetRequestSent);
		break;
	case LLPathfindingManager::kRequestCompleted :
		clearCharacters();
		setMessagingState(kMessagingComplete);
		break;
	case LLPathfindingManager::kRequestNotEnabled :
		clearCharacters();
		setMessagingState(kMessagingNotEnabled);
		break;
	case LLPathfindingManager::kRequestError :
		setMessagingState(kMessagingGetError);
		break;
	default :
		setMessagingState(kMessagingGetError);
		llassert(0);
		break;
	}
}

void LLFloaterPathfindingCharacters::handleNewCharacters(LLPathfindingManager::request_id_t pRequestId, LLPathfindingManager::ERequestStatus pCharacterRequestStatus, LLPathfindingCharacterListPtr pCharacterListPtr)
{
	llassert(pRequestId <= mMessagingRequestId);
	if (pRequestId == mMessagingRequestId)
	{
		mCharacterListPtr = pCharacterListPtr;
		updateScrollList();

		switch (pCharacterRequestStatus)
		{
		case LLPathfindingManager::kRequestCompleted :
			setMessagingState(kMessagingComplete);
			break;
		case LLPathfindingManager::kRequestError :
			setMessagingState(kMessagingGetError);
			break;
		default :
			setMessagingState(kMessagingGetError);
			llassert(0);
			break;
		}
	}
}

void LLFloaterPathfindingCharacters::onCharactersSelectionChange()
{
	mCharacterSelection.clear();
	LLSelectMgr::getInstance()->deselectAll();

	std::vector<LLScrollListItem*> selectedItems = mCharactersScrollList->getAllSelected();
	if (!selectedItems.empty())
	{
		int numSelectedItems = selectedItems.size();

		std::vector<LLViewerObject *> viewerObjects;
		viewerObjects.reserve(numSelectedItems);

		for (std::vector<LLScrollListItem*>::const_iterator selectedItemIter = selectedItems.begin();
			selectedItemIter != selectedItems.end(); ++selectedItemIter)
		{
			const LLScrollListItem *selectedItem = *selectedItemIter;

			LLViewerObject *viewerObject = gObjectList.findObject(selectedItem->getUUID());
			if (viewerObject != NULL)
			{
				viewerObjects.push_back(viewerObject);
			}
		}

		if (!viewerObjects.empty())
		{
			mCharacterSelection = LLSelectMgr::getInstance()->selectObjectAndFamily(viewerObjects);
		}
	}

	updateControls();
}

void LLFloaterPathfindingCharacters::onRefreshCharactersClicked()
{
	requestGetCharacters();
}

void LLFloaterPathfindingCharacters::onSelectAllCharactersClicked()
{
	selectAllCharacters();
}

void LLFloaterPathfindingCharacters::onSelectNoneCharactersClicked()
{
	selectNoneCharacters();
}

void LLFloaterPathfindingCharacters::onTakeCharactersClicked()
{
	handle_take();
}

void LLFloaterPathfindingCharacters::onTakeCopyCharactersClicked()
{
	handle_take_copy();
}

void LLFloaterPathfindingCharacters::onReturnCharactersClicked()
{
	handle_object_return();
}

void LLFloaterPathfindingCharacters::onDeleteCharactersClicked()
{
	handle_object_delete();
}

void LLFloaterPathfindingCharacters::onTeleportCharacterToMeClicked()
{
	std::vector<LLScrollListItem*> selectedItems = mCharactersScrollList->getAllSelected();
	llassert(selectedItems.size() == 1);
	if (selectedItems.size() == 1)
	{
		std::vector<LLScrollListItem*>::const_reference selectedItemRef = selectedItems.front();
		const LLScrollListItem *selectedItem = selectedItemRef;
		LLPathfindingCharacterList::const_iterator characterIter = mCharacterListPtr->find(selectedItem->getUUID().asString());
		const LLPathfindingCharacterPtr &characterPtr = characterIter->second;
		const LLVector3 &characterLocation = characterPtr->getLocation();

		LLViewerRegion* region = gAgent.getRegion();
		if (region != NULL)
		{
			gAgent.teleportRequest(region->getHandle(), characterLocation, true);
		}
	}
}

void LLFloaterPathfindingCharacters::onRegionBoundaryCross()
{
	requestGetCharacters();
}

void LLFloaterPathfindingCharacters::selectAllCharacters()
{
	mCharactersScrollList->selectAll();
}

void LLFloaterPathfindingCharacters::selectNoneCharacters()
{
	mCharactersScrollList->deselectAllItems();
}

void LLFloaterPathfindingCharacters::clearCharacters()
{
	if (mCharacterListPtr != NULL)
	{
		mCharacterListPtr->clear();
	}
	updateScrollList();
}

void LLFloaterPathfindingCharacters::updateControls()
{
	updateStatusMessage();
	updateEnableStateOnListActions();
	updateEnableStateOnEditFields();
}

void LLFloaterPathfindingCharacters::updateScrollList()
{
	std::vector<LLScrollListItem*> selectedItems = mCharactersScrollList->getAllSelected();
	int numSelectedItems = selectedItems.size();
	uuid_vec_t selectedUUIDs;
	if (numSelectedItems > 0)
	{
		selectedUUIDs.reserve(selectedItems.size());
		for (std::vector<LLScrollListItem*>::const_iterator itemIter = selectedItems.begin();
			itemIter != selectedItems.end(); ++itemIter)
		{
			const LLScrollListItem *listItem = *itemIter;
			selectedUUIDs.push_back(listItem->getUUID());
		}
	}

	S32 origScrollPosition = mCharactersScrollList->getScrollPos();
	mCharactersScrollList->deleteAllItems();

	if (mCharacterListPtr != NULL)
	{
		for (LLPathfindingCharacterList::const_iterator characterIter = mCharacterListPtr->begin();
			characterIter != mCharacterListPtr->end(); ++characterIter)
		{
			const LLPathfindingCharacterPtr& character(characterIter->second);
			LLSD element = buildCharacterScrollListElement(character);
			mCharactersScrollList->addElement(element);
		}
	}

	mCharactersScrollList->selectMultiple(selectedUUIDs);
	mCharactersScrollList->setScrollPos(origScrollPosition);
	updateControls();
}

LLSD LLFloaterPathfindingCharacters::buildCharacterScrollListElement(const LLPathfindingCharacterPtr pCharacterPtr) const
{
	LLSD columns;

	columns[0]["column"] = "name";
	columns[0]["value"] = pCharacterPtr->getName();
	columns[0]["font"] = "SANSSERIF";

	columns[1]["column"] = "description";
	columns[1]["value"] = pCharacterPtr->getDescription();
	columns[1]["font"] = "SANSSERIF";

	columns[2]["column"] = "owner";
	columns[2]["value"] = pCharacterPtr->getOwnerName();
	columns[2]["font"] = "SANSSERIF";

	S32 cpuTime = llround(pCharacterPtr->getCPUTime());
	std::string cpuTimeString = llformat("%d", cpuTime);
	LLStringUtil::format_map_t string_args;
	string_args["[CPU_TIME]"] = cpuTimeString;

	columns[3]["column"] = "cpu_time";
	columns[3]["value"] = getString("character_cpu_time", string_args);
	columns[3]["font"] = "SANSSERIF";

	columns[4]["column"] = "altitude";
	columns[4]["value"] = llformat("%1.0f m", pCharacterPtr->getLocation()[2]);
	columns[4]["font"] = "SANSSERIF";

	LLSD element;
	element["id"] = pCharacterPtr->getUUID().asString();
	element["column"] = columns;

	return element;
}

void LLFloaterPathfindingCharacters::updateStatusMessage()
{
	static const LLColor4 errorColor = LLUIColorTable::instance().getColor("PathfindingErrorColor");
	static const LLColor4 warningColor = LLUIColorTable::instance().getColor("PathfindingWarningColor");

	std::string statusText("");
	LLStyle::Params styleParams;

	switch (getMessagingState())
	{
	case kMessagingUnknown:
		statusText = getString("characters_messaging_initial");
		styleParams.color = errorColor;
		break;
	case kMessagingGetRequestSent :
		statusText = getString("characters_messaging_get_inprogress");
		styleParams.color = warningColor;
		break;
	case kMessagingGetError :
		statusText = getString("characters_messaging_get_error");
		styleParams.color = errorColor;
		break;
	case kMessagingComplete :
		if (mCharactersScrollList->isEmpty())
		{
			statusText = getString("characters_messaging_complete_none_found");
		}
		else
		{
			S32 numItems = mCharactersScrollList->getItemCount();
			S32 numSelectedItems = mCharactersScrollList->getNumSelected();

			LLLocale locale(LLStringUtil::getLocale());
			std::string numItemsString;
			LLResMgr::getInstance()->getIntegerString(numItemsString, numItems);

			std::string numSelectedItemsString;
			LLResMgr::getInstance()->getIntegerString(numSelectedItemsString, numSelectedItems);

			LLStringUtil::format_map_t string_args;
			string_args["[NUM_SELECTED]"] = numSelectedItemsString;
			string_args["[NUM_TOTAL]"] = numItemsString;
			statusText = getString("characters_messaging_complete_available", string_args);
		}
		break;
	case kMessagingNotEnabled:
		statusText = getString("characters_messaging_not_enabled");
		styleParams.color = errorColor;
		break;
	default:
		statusText = getString("characters_messaging_initial");
		styleParams.color = errorColor;
		llassert(0);
		break;
	}

	mCharactersStatus->setText((LLStringExplicit)statusText, styleParams);
}

void LLFloaterPathfindingCharacters::updateEnableStateOnListActions()
{
	switch (getMessagingState())
	{
	case kMessagingUnknown:
	case kMessagingGetRequestSent :
		mRefreshListButton->setEnabled(FALSE);
		mSelectAllButton->setEnabled(FALSE);
		mSelectNoneButton->setEnabled(FALSE);
		break;
	case kMessagingGetError :
	case kMessagingNotEnabled :
		mRefreshListButton->setEnabled(TRUE);
		mSelectAllButton->setEnabled(FALSE);
		mSelectNoneButton->setEnabled(FALSE);
		break;
	case kMessagingComplete :
		{
			int numItems = mCharactersScrollList->getItemCount();
			int numSelectedItems = mCharactersScrollList->getNumSelected();
			mRefreshListButton->setEnabled(TRUE);
			mSelectAllButton->setEnabled(numSelectedItems < numItems);
			mSelectNoneButton->setEnabled(numSelectedItems > 0);
		}
		break;
	default:
		llassert(0);
		break;
	}
}

void LLFloaterPathfindingCharacters::updateEnableStateOnEditFields()
{
	int numSelectedItems = mCharactersScrollList->getNumSelected();
	bool isEditEnabled = (numSelectedItems > 0);

	mShowBeaconCheckBox->setEnabled(isEditEnabled);
	mTakeButton->setEnabled(isEditEnabled && visible_take_object());
	mTakeCopyButton->setEnabled(isEditEnabled && enable_object_take_copy());
	mReturnButton->setEnabled(isEditEnabled && enable_object_return());
	mDeleteButton->setEnabled(isEditEnabled && enable_object_delete());
	mTeleportButton->setEnabled(numSelectedItems == 1);
}
