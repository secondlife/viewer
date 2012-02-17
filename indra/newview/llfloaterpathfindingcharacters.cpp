/** 
 * @file llfloaterpathfindingcharacters.cpp
 * @author William Todd Stinson
 * @brief "Pathfinding linksets" floater, allowing manipulation of the Havok AI pathfinding settings.
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
#include "llpathfindingcharacter.h"
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

//---------------------------------------------------------------------------
// CharactersGetResponder
//---------------------------------------------------------------------------

class CharactersGetResponder : public LLHTTPClient::Responder
{
public:
	CharactersGetResponder(const std::string& pCharactersDataGetURL,
		const LLHandle<LLFloaterPathfindingCharacters> &pCharactersFloaterHandle);
	virtual ~CharactersGetResponder();

	virtual void result(const LLSD& pContent);
	virtual void error(U32 pStatus, const std::string& pReason);

private:
	CharactersGetResponder(const CharactersGetResponder& pOther);

	std::string                              mCharactersDataGetURL;
	LLHandle<LLFloaterPathfindingCharacters> mCharactersFloaterHandle;
};

//---------------------------------------------------------------------------
// LLFloaterPathfindingCharacters
//---------------------------------------------------------------------------

BOOL LLFloaterPathfindingCharacters::postBuild()
{
	childSetAction("refresh_characters_list", boost::bind(&LLFloaterPathfindingCharacters::onRefreshCharactersClicked, this));
	childSetAction("select_all_characters", boost::bind(&LLFloaterPathfindingCharacters::onSelectAllCharactersClicked, this));
	childSetAction("select_none_characters", boost::bind(&LLFloaterPathfindingCharacters::onSelectNoneCharactersClicked, this));

	mCharactersScrollList = findChild<LLScrollListCtrl>("pathfinding_characters");
	llassert(mCharactersScrollList != NULL);
	mCharactersScrollList->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onCharactersSelectionChange, this));
	mCharactersScrollList->sortByColumnIndex(0, true);

	mCharactersStatus = findChild<LLTextBase>("characters_status");
	llassert(mCharactersStatus != NULL);

	mLabelActions = findChild<LLTextBase>("actions_label");
	llassert(mLabelActions != NULL);

	mShowBeaconCheckBox = findChild<LLCheckBoxCtrl>("show_beacon");
	llassert(mShowBeaconCheckBox != NULL);

	mTakeBtn = findChild<LLButton>("take_characters");
	llassert(mTakeBtn != NULL)
	mTakeBtn->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onTakeCharactersClicked, this));

	mTakeCopyBtn = findChild<LLButton>("take_copy_characters");
	llassert(mTakeCopyBtn != NULL)
	mTakeCopyBtn->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onTakeCopyCharactersClicked, this));

	mReturnBtn = findChild<LLButton>("return_characters");
	llassert(mReturnBtn != NULL)
	mReturnBtn->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onReturnCharactersClicked, this));

	mDeleteBtn = findChild<LLButton>("delete_characters");
	llassert(mDeleteBtn != NULL)
	mDeleteBtn->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onDeleteCharactersClicked, this));

	mTeleportBtn = findChild<LLButton>("teleport_to_character");
	llassert(mTeleportBtn != NULL)
	mTeleportBtn->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onTeleportCharacterToMeClicked, this));

	setEnableActionFields(false);
	setMessagingState(kMessagingInitial);

	return LLFloater::postBuild();
}

void LLFloaterPathfindingCharacters::onOpen(const LLSD& pKey)
{
	sendCharactersDataGetRequest();
	selectNoneCharacters();
	mCharactersScrollList->setCommitOnSelectionChange(true);
}

void LLFloaterPathfindingCharacters::onClose(bool app_quitting)
{
	mCharactersScrollList->setCommitOnSelectionChange(false);
	selectNoneCharacters();
	if (mCharacterSelection.notNull())
	{
		std::vector<LLViewerObject *> selectedObjects;

		LLObjectSelection *charactersSelected = mCharacterSelection.get();
		for (LLObjectSelection::valid_iterator characterIter = charactersSelected->valid_begin();
			characterIter != charactersSelected->valid_end();  ++characterIter)
		{
			LLSelectNode *characterNode = *characterIter;
			selectedObjects.push_back(characterNode->getObject());
		}

		for (std::vector<LLViewerObject *>::const_iterator selectedObjectIter = selectedObjects.begin();
			selectedObjectIter != selectedObjects.end(); ++selectedObjectIter)
		{
			LLViewerObject *selectedObject = *selectedObjectIter;
			LLSelectMgr::getInstance()->deselectObjectAndFamily(selectedObject);
		}

		mCharacterSelection.clear();
	}
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

LLFloaterPathfindingCharacters::EMessagingState LLFloaterPathfindingCharacters::getMessagingState() const
{
	return mMessagingState;
}

BOOL LLFloaterPathfindingCharacters::isMessagingInProgress() const
{
	BOOL retVal;
	switch (getMessagingState())
	{
	case kMessagingFetchStarting :
	case kMessagingFetchRequestSent :
	case kMessagingFetchRequestSent_MultiRequested :
	case kMessagingFetchReceived :
		retVal = true;
		break;
	default :
		retVal = false;
		break;
	}

	return retVal;
}

LLFloaterPathfindingCharacters::LLFloaterPathfindingCharacters(const LLSD& pSeed)
	: LLFloater(pSeed),
	mSelfHandle(),
	mPathfindingCharacters(),
	mMessagingState(kMessagingInitial),
	mCharactersScrollList(NULL),
	mCharactersStatus(NULL),
	mLabelActions(NULL),
	mShowBeaconCheckBox(NULL),
	mTakeBtn(NULL),
	mTakeCopyBtn(NULL),
	mReturnBtn(NULL),
	mDeleteBtn(NULL),
	mTeleportBtn(NULL),
	mCharacterSelection()
{
	mSelfHandle.bind(this);
}

LLFloaterPathfindingCharacters::~LLFloaterPathfindingCharacters()
{
	mPathfindingCharacters.clear();
	mCharacterSelection.clear();
}

void LLFloaterPathfindingCharacters::sendCharactersDataGetRequest()
{
	if (isMessagingInProgress())
	{
		if (getMessagingState() == kMessagingFetchRequestSent)
		{
			setMessagingState(kMessagingFetchRequestSent_MultiRequested);
		}
	}
	else
	{
		setMessagingState(kMessagingFetchStarting);
		mPathfindingCharacters.clear();
		updateCharactersList();

		std::string charactersDataURL = getCapabilityURL();
		if (charactersDataURL.empty())
		{
			setMessagingState(kMessagingServiceNotAvailable);
			llwarns << "cannot query pathfinding characters from current region '" << getRegionName() << "'" << llendl;
		}
		else
		{
			setMessagingState(kMessagingFetchRequestSent);
			LLHTTPClient::get(charactersDataURL, new CharactersGetResponder(charactersDataURL, mSelfHandle));
		}
	}
}

void LLFloaterPathfindingCharacters::handleCharactersDataGetReply(const LLSD& pCharactersData)
{
	setMessagingState(kMessagingFetchReceived);
	mPathfindingCharacters.clear();
	parseCharactersData(pCharactersData);
	updateCharactersList();
	setMessagingState(kMessagingComplete);
}

void LLFloaterPathfindingCharacters::handleCharactersDataGetError(const std::string& pURL, const std::string& pErrorReason)
{
	setMessagingState(kMessagingFetchError);
	mPathfindingCharacters.clear();
	updateCharactersList();
	llwarns << "Error fetching pathfinding characters from URL '" << pURL << "' because " << pErrorReason << llendl;
}

std::string LLFloaterPathfindingCharacters::getRegionName() const
{
	std::string regionName("");

	LLViewerRegion* region = gAgent.getRegion();
	if (region != NULL)
	{
		regionName = region->getName();
	}

	return regionName;
}

std::string LLFloaterPathfindingCharacters::getCapabilityURL() const
{
	std::string charactersDataURL("");

	LLViewerRegion* region = gAgent.getRegion();
	if (region != NULL)
	{
		charactersDataURL = region->getCapability("CharacterProperties");
	}

	return charactersDataURL;
}

void LLFloaterPathfindingCharacters::parseCharactersData(const LLSD &pCharactersData)
{
	for (LLSD::map_const_iterator characterItemIter = pCharactersData.beginMap();
		characterItemIter != pCharactersData.endMap(); ++characterItemIter)
	{
		const std::string &uuid(characterItemIter->first);
		const LLSD &characterData(characterItemIter->second);
		LLPathfindingCharacter character(uuid, characterData);

		mPathfindingCharacters.insert(std::pair<std::string, LLPathfindingCharacter>(uuid, character));
	}
}

void LLFloaterPathfindingCharacters::setMessagingState(EMessagingState pMessagingState)
{
	mMessagingState = pMessagingState;
	updateCharactersList();
	updateActionFields();
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

	updateCharactersStatusMessage();
	updateActionFields();
}

void LLFloaterPathfindingCharacters::onRefreshCharactersClicked()
{
	sendCharactersDataGetRequest();
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
		PathfindingCharacterMap::const_iterator characterIter = mPathfindingCharacters.find(selectedItem->getUUID().asString());
		const LLPathfindingCharacter &character = characterIter->second;
		LLVector3 characterLocation = character.getLocation();

		LLViewerRegion* region = gAgent.getRegion();
		if (region != NULL)
		{
			gAgent.teleportRequest(region->getHandle(), characterLocation, true);
		}
	}
}

void LLFloaterPathfindingCharacters::updateCharactersList()
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

	mCharactersScrollList->deleteAllItems();
	updateCharactersStatusMessage();

	LLLocale locale(LLStringUtil::getLocale());
	for (PathfindingCharacterMap::const_iterator characterIter = mPathfindingCharacters.begin();
		characterIter != mPathfindingCharacters.end(); ++characterIter)
	{
		const LLPathfindingCharacter& character(characterIter->second);

		LLSD columns;

		columns[0]["column"] = "name";
		columns[0]["value"] = character.getName();
		columns[0]["font"] = "SANSSERIF";

		columns[1]["column"] = "description";
		columns[1]["value"] = character.getDescription();
		columns[1]["font"] = "SANSSERIF";

		columns[2]["column"] = "owner";
		columns[2]["value"] = character.getOwnerName();
		columns[2]["font"] = "SANSSERIF";

		S32 cpuTime = llround(character.getCPUTime());
		std::string cpuTimeString;
		LLResMgr::getInstance()->getIntegerString(cpuTimeString, cpuTime);

		LLStringUtil::format_map_t string_args;
		string_args["[CPU_TIME]"] = cpuTimeString;

		columns[3]["column"] = "cpu_time";
		columns[3]["value"] = getString("character_cpu_time", string_args);
		columns[3]["font"] = "SANSSERIF";

		columns[4]["column"] = "altitude";
		columns[4]["value"] = llformat("%1.0f m", character.getLocation()[2]);
		columns[4]["font"] = "SANSSERIF";

		LLSD element;
		element["id"] = character.getUUID().asString();
		element["column"] = columns;

		mCharactersScrollList->addElement(element);
	}

	mCharactersScrollList->selectMultiple(selectedUUIDs);
	updateCharactersStatusMessage();
	updateActionFields();
}

void LLFloaterPathfindingCharacters::selectAllCharacters()
{
	mCharactersScrollList->selectAll();
}

void LLFloaterPathfindingCharacters::selectNoneCharacters()
{
	mCharactersScrollList->deselectAllItems();
}

void LLFloaterPathfindingCharacters::updateCharactersStatusMessage()
{
	static const LLColor4 warningColor = LLUIColorTable::instance().getColor("DrYellow");

	std::string statusText("");
	LLStyle::Params styleParams;

	switch (getMessagingState())
	{
	case kMessagingInitial:
		statusText = getString("characters_messaging_initial");
		break;
	case kMessagingFetchStarting :
		statusText = getString("characters_messaging_fetch_starting");
		break;
	case kMessagingFetchRequestSent :
		statusText = getString("characters_messaging_fetch_inprogress");
		break;
	case kMessagingFetchRequestSent_MultiRequested :
		statusText = getString("characters_messaging_fetch_inprogress_multi_request");
		break;
	case kMessagingFetchReceived :
		statusText = getString("characters_messaging_fetch_received");
		break;
	case kMessagingFetchError :
		statusText = getString("characters_messaging_fetch_error");
		styleParams.color = warningColor;
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
	case kMessagingServiceNotAvailable:
		statusText = getString("characters_messaging_service_not_available");
		styleParams.color = warningColor;
		break;
	default:
		statusText = getString("characters_messaging_initial");
		llassert(0);
		break;
	}

	mCharactersStatus->setText((LLStringExplicit)statusText, styleParams);
}

void LLFloaterPathfindingCharacters::updateActionFields()
{
	std::vector<LLScrollListItem*> selectedItems = mCharactersScrollList->getAllSelected();
	setEnableActionFields(!selectedItems.empty());
}

void LLFloaterPathfindingCharacters::setEnableActionFields(BOOL pEnabled)
{
	mLabelActions->setEnabled(pEnabled);
	mShowBeaconCheckBox->setEnabled(pEnabled);
	mTakeBtn->setEnabled(pEnabled && tools_visible_take_object());
	mTakeCopyBtn->setEnabled(pEnabled && enable_object_take_copy());
	mReturnBtn->setEnabled(pEnabled && enable_object_return());
	mDeleteBtn->setEnabled(pEnabled && enable_object_delete());
	mTeleportBtn->setEnabled(pEnabled && (mCharactersScrollList->getNumSelected() == 1));
}

//---------------------------------------------------------------------------
// CharactersGetResponder
//---------------------------------------------------------------------------

CharactersGetResponder::CharactersGetResponder(const std::string& pCharactersDataGetURL,
	const LLHandle<LLFloaterPathfindingCharacters> &pCharactersFloaterHandle)
	: mCharactersDataGetURL(pCharactersDataGetURL),
	mCharactersFloaterHandle(pCharactersFloaterHandle)
{
}

CharactersGetResponder::~CharactersGetResponder()
{
}

void CharactersGetResponder::result(const LLSD& pContent)
{
	LLFloaterPathfindingCharacters *charactersFloater = mCharactersFloaterHandle.get();
	if (charactersFloater != NULL)
	{
		charactersFloater->handleCharactersDataGetReply(pContent);
	}
}

void CharactersGetResponder::error(U32 status, const std::string& reason)
{
	LLFloaterPathfindingCharacters *charactersFloater = mCharactersFloaterHandle.get();
	if (charactersFloater != NULL)
	{
		charactersFloater->handleCharactersDataGetError(mCharactersDataGetURL, reason);
	}
}
