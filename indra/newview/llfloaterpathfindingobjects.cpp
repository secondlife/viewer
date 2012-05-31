/** 
* @file llfloaterpathfindingobjects.cpp
* @brief Implementation of llfloaterpathfindingobjects
* @author Stinson@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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

#include "llfloaterpathfindingobjects.h"

#include <vector>

#include <boost/bind.hpp>
#include <boost/signals2.hpp>

#include "v4color.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llenvmanager.h"
#include "lllineeditor.h"
#include "llfloater.h"
#include "llpathfindingmanager.h"
#include "llresmgr.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llselectmgr.h"
#include "llsd.h"
#include "llstring.h"
#include "llstyle.h"
#include "lltextbase.h"
#include "lluicolortable.h"
#include "llviewermenu.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"

#define DEFAULT_BEACON_WIDTH 6

//---------------------------------------------------------------------------
// LLFloaterPathfindingObjects
//---------------------------------------------------------------------------

void LLFloaterPathfindingObjects::onOpen(const LLSD &pKey)
{
	LLFloater::onOpen(pKey);

	selectNoneObjects();
	mObjectsScrollList->setCommitOnSelectionChange(TRUE);

	if (!mSelectionUpdateSlot.connected())
	{
		mSelectionUpdateSlot = LLSelectMgr::getInstance()->mUpdateSignal.connect(boost::bind(&LLFloaterPathfindingObjects::onSelectionListChanged, this));
	}

	if (!mRegionBoundaryCrossingSlot.connected())
	{
		mRegionBoundaryCrossingSlot = LLEnvManagerNew::getInstance()->setRegionChangeCallback(boost::bind(&LLFloaterPathfindingObjects::onRegionBoundaryCrossed, this));
	}

	requestGetObjects();
}

void LLFloaterPathfindingObjects::onClose(bool pIsAppQuitting)
{
	if (mRegionBoundaryCrossingSlot.connected())
	{
		mRegionBoundaryCrossingSlot.disconnect();
	}

	if (mSelectionUpdateSlot.connected())
	{
		mSelectionUpdateSlot.disconnect();
	}

	mObjectsScrollList->setCommitOnSelectionChange(FALSE);
	selectNoneObjects();

	if (mObjectsSelection.notNull())
	{
		mObjectsSelection.clear();
	}
}

void LLFloaterPathfindingObjects::draw()
{
	LLFloater::draw();

	if (isShowBeacons())
	{
		std::vector<LLScrollListItem *> selectedItems = mObjectsScrollList->getAllSelected();
		if (!selectedItems.empty())
		{
			int numSelectedItems = selectedItems.size();
			S32 nameColumnIndex = getNameColumnIndex();
			const LLColor4 &beaconColor = getBeaconColor();
			const LLColor4 &beaconTextColor = getBeaconTextColor();
			S32 beaconWidth = getBeaconWidth();

			std::vector<LLViewerObject *> viewerObjects;
			viewerObjects.reserve(numSelectedItems);

			for (std::vector<LLScrollListItem *>::const_iterator selectedItemIter = selectedItems.begin();
				selectedItemIter != selectedItems.end(); ++selectedItemIter)
			{
				const LLScrollListItem *selectedItem = *selectedItemIter;

				LLViewerObject *viewerObject = gObjectList.findObject(selectedItem->getUUID());
				if (viewerObject != NULL)
				{
					const std::string &objectName = selectedItem->getColumn(nameColumnIndex)->getValue().asString();
					gObjectList.addDebugBeacon(viewerObject->getPositionAgent(), objectName, beaconColor, beaconTextColor, beaconWidth);
				}
			}
		}
	}
}

LLFloaterPathfindingObjects::LLFloaterPathfindingObjects(const LLSD &pSeed)
	: LLFloater(pSeed),
	mObjectsScrollList(NULL),
	mMessagingStatus(NULL),
	mRefreshListButton(NULL),
	mSelectAllButton(NULL),
	mSelectNoneButton(NULL),
	mShowBeaconCheckBox(NULL),
	mTakeButton(NULL),
	mTakeCopyButton(NULL),
	mReturnButton(NULL),
	mDeleteButton(NULL),
	mTeleportButton(NULL),
	mDefaultBeaconColor(),
	mDefaultBeaconTextColor(),
	mErrorTextColor(),
	mWarningTextColor(),
	mMessagingState(kMessagingUnknown),
	mMessagingRequestId(0U),
	mObjectList(),
	mObjectsSelection(),
	mSelectionUpdateSlot(),
	mRegionBoundaryCrossingSlot()
{
}

LLFloaterPathfindingObjects::~LLFloaterPathfindingObjects()
{
}

BOOL LLFloaterPathfindingObjects::postBuild()
{
	mDefaultBeaconColor = LLUIColorTable::getInstance()->getColor("PathfindingDefaultBeaconColor");
	mDefaultBeaconTextColor = LLUIColorTable::getInstance()->getColor("PathfindingDefaultBeaconTextColor");
	mErrorTextColor = LLUIColorTable::getInstance()->getColor("PathfindingErrorColor");
	mWarningTextColor = LLUIColorTable::getInstance()->getColor("PathfindingWarningColor");

	mObjectsScrollList = findChild<LLScrollListCtrl>("objects_scroll_list");
	llassert(mObjectsScrollList != NULL);
	mObjectsScrollList->setCommitCallback(boost::bind(&LLFloaterPathfindingObjects::onScrollListSelectionChanged, this));
	mObjectsScrollList->sortByColumnIndex(static_cast<U32>(getNameColumnIndex()), TRUE);

	mMessagingStatus = findChild<LLTextBase>("messaging_status");
	llassert(mMessagingStatus != NULL);

	mRefreshListButton = findChild<LLButton>("refresh_objects_list");
	llassert(mRefreshListButton != NULL);
	mRefreshListButton->setCommitCallback(boost::bind(&LLFloaterPathfindingObjects::onRefreshObjectsClicked, this));

	mSelectAllButton = findChild<LLButton>("select_all_objects");
	llassert(mSelectAllButton != NULL);
	mSelectAllButton->setCommitCallback(boost::bind(&LLFloaterPathfindingObjects::onSelectAllObjectsClicked, this));

	mSelectNoneButton = findChild<LLButton>("select_none_objects");
	llassert(mSelectNoneButton != NULL);
	mSelectNoneButton->setCommitCallback(boost::bind(&LLFloaterPathfindingObjects::onSelectNoneObjectsClicked, this));

	mShowBeaconCheckBox = findChild<LLCheckBoxCtrl>("show_beacon");
	llassert(mShowBeaconCheckBox != NULL);

	mTakeButton = findChild<LLButton>("take_objects");
	llassert(mTakeButton != NULL);
	mTakeButton->setCommitCallback(boost::bind(&LLFloaterPathfindingObjects::onTakeClicked, this));

	mTakeCopyButton = findChild<LLButton>("take_copy_objects");
	llassert(mTakeCopyButton != NULL);
	mTakeCopyButton->setCommitCallback(boost::bind(&LLFloaterPathfindingObjects::onTakeCopyClicked, this));

	mReturnButton = findChild<LLButton>("return_objects");
	llassert(mReturnButton != NULL);
	mReturnButton->setCommitCallback(boost::bind(&LLFloaterPathfindingObjects::onReturnClicked, this));

	mDeleteButton = findChild<LLButton>("delete_objects");
	llassert(mDeleteButton != NULL);
	mDeleteButton->setCommitCallback(boost::bind(&LLFloaterPathfindingObjects::onDeleteClicked, this));

	mTeleportButton = findChild<LLButton>("teleport_me_to_object");
	llassert(mTeleportButton != NULL);
	mTeleportButton->setCommitCallback(boost::bind(&LLFloaterPathfindingObjects::onTeleportClicked, this));

	return LLFloater::postBuild();
}

void LLFloaterPathfindingObjects::requestGetObjects()
{
}

LLPathfindingManager::request_id_t LLFloaterPathfindingObjects::getNewRequestId()
{
	return ++mMessagingRequestId;
}

void LLFloaterPathfindingObjects::handleNewObjectList(LLPathfindingManager::request_id_t pRequestId, LLPathfindingManager::ERequestStatus pRequestStatus, LLPathfindingObjectListPtr pObjectList)
{
	llassert(pRequestId <= mMessagingRequestId);
	if (pRequestId == mMessagingRequestId)
	{
		switch (pRequestStatus)
		{
		case LLPathfindingManager::kRequestStarted :
			setMessagingState(kMessagingGetRequestSent);
			break;
		case LLPathfindingManager::kRequestCompleted :
			mObjectList = pObjectList;
			rebuildObjectsScrollList();
			setMessagingState(kMessagingComplete);
			break;
		case LLPathfindingManager::kRequestNotEnabled :
			clearAllObjects();
			setMessagingState(kMessagingNotEnabled);
			break;
		case LLPathfindingManager::kRequestError :
			clearAllObjects();
			setMessagingState(kMessagingGetError);
			break;
		default :
			clearAllObjects();
			setMessagingState(kMessagingGetError);
			llassert(0);
			break;
		}
	}
}

void LLFloaterPathfindingObjects::handleUpdateObjectList(LLPathfindingManager::request_id_t pRequestId, LLPathfindingManager::ERequestStatus pRequestStatus, LLPathfindingObjectListPtr pObjectList)
{
	llassert(pRequestId <= mMessagingRequestId);
	if (pRequestId == mMessagingRequestId)
	{
		switch (pRequestStatus)
		{
		case LLPathfindingManager::kRequestStarted :
			setMessagingState(kMessagingGetRequestSent);
			break;
		case LLPathfindingManager::kRequestCompleted :
			if (mObjectList == NULL)
			{
				mObjectList = pObjectList;
			}
			else
			{
				mObjectList->update(pObjectList);
			}
			rebuildObjectsScrollList();
			setMessagingState(kMessagingComplete);
			break;
		case LLPathfindingManager::kRequestNotEnabled :
			clearAllObjects();
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
}

void LLFloaterPathfindingObjects::rebuildObjectsScrollList()
{
	std::vector<LLScrollListItem*> selectedItems = mObjectsScrollList->getAllSelected();
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

	S32 origScrollPosition = mObjectsScrollList->getScrollPos();
	mObjectsScrollList->deleteAllItems();

	if ((mObjectList != NULL) && !mObjectList->isEmpty())
	{
		LLSD scrollListData = convertObjectsIntoScrollListData(mObjectList);
		llassert(scrollListData.isArray());
		for (LLSD::array_const_iterator elementIter = scrollListData.beginArray(); elementIter != scrollListData.endArray(); ++elementIter)
		{
			const LLSD &element = *elementIter;
			mObjectsScrollList->addElement(element);
		}
	}

	mObjectsScrollList->selectMultiple(selectedUUIDs);
	mObjectsScrollList->setScrollPos(origScrollPosition);
	updateControls();
}

LLSD LLFloaterPathfindingObjects::convertObjectsIntoScrollListData(const LLPathfindingObjectListPtr pObjectListPtr) const
{
	llassert(0);
	LLSD nullObjs = LLSD::emptyArray();
	return nullObjs;
}

void LLFloaterPathfindingObjects::updateControls()
{
	updateMessagingStatus();
	updateStateOnListActionControls();
	updateStateOnEditFields();
}

void LLFloaterPathfindingObjects::updateSelection()
{
	mObjectsSelection.clear();
	LLSelectMgr::getInstance()->deselectAll();

	std::vector<LLScrollListItem *> selectedItems = mObjectsScrollList->getAllSelected();
	if (!selectedItems.empty())
	{
		int numSelectedItems = selectedItems.size();

		std::vector<LLViewerObject *>viewerObjects;
		viewerObjects.reserve(numSelectedItems);

		for (std::vector<LLScrollListItem *>::const_iterator selectedItemIter = selectedItems.begin();
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
			mObjectsSelection = LLSelectMgr::getInstance()->selectObjectAndFamily(viewerObjects);
		}
	}

	updateControls();
}

S32 LLFloaterPathfindingObjects::getNameColumnIndex() const
{
	return 0;
}

const LLColor4 &LLFloaterPathfindingObjects::getBeaconColor() const
{
	return mDefaultBeaconColor;
}

const LLColor4 &LLFloaterPathfindingObjects::getBeaconTextColor() const
{
	return mDefaultBeaconTextColor;
}

S32 LLFloaterPathfindingObjects::getBeaconWidth() const
{
	return DEFAULT_BEACON_WIDTH;
}

BOOL LLFloaterPathfindingObjects::isShowBeacons() const
{
	return mShowBeaconCheckBox->get();
}

void LLFloaterPathfindingObjects::clearAllObjects()
{
	selectNoneObjects();
	mObjectsScrollList->clear();
	mObjectList.reset();
}

void LLFloaterPathfindingObjects::selectAllObjects()
{
	mObjectsScrollList->selectAll();
}

void LLFloaterPathfindingObjects::selectNoneObjects()
{
	mObjectsScrollList->deselectAllItems();
}

void LLFloaterPathfindingObjects::teleportToSelectedObject()
{
	std::vector<LLScrollListItem*> selectedItems = mObjectsScrollList->getAllSelected();
	llassert(selectedItems.size() == 1);
	if (selectedItems.size() == 1)
	{
		std::vector<LLScrollListItem*>::const_reference selectedItemRef = selectedItems.front();
		const LLScrollListItem *selectedItem = selectedItemRef;
		llassert(mObjectList != NULL);
		const LLPathfindingObjectPtr objectPtr = mObjectList->find(selectedItem->getUUID().asString());
		const LLVector3 &objectLocation = objectPtr->getLocation();

		LLViewerRegion* region = gAgent.getRegion();
		if (region != NULL)
		{
			gAgent.teleportRequest(region->getHandle(), objectLocation, true);
		}
	}
}

LLPathfindingObjectListPtr LLFloaterPathfindingObjects::getEmptyObjectList() const
{
	llassert(0);
	LLPathfindingObjectListPtr objectListPtr(new LLPathfindingObjectList());
	return objectListPtr;
}

int LLFloaterPathfindingObjects::getNumSelectedObjects() const
{
	return mObjectsScrollList->getNumSelected();
}

LLPathfindingObjectListPtr LLFloaterPathfindingObjects::getSelectedObjects() const
{
	LLPathfindingObjectListPtr selectedObjects = getEmptyObjectList();

	std::vector<LLScrollListItem*> selectedItems = mObjectsScrollList->getAllSelected();
	if (!selectedItems.empty())
	{
		for (std::vector<LLScrollListItem*>::const_iterator itemIter = selectedItems.begin();
			itemIter != selectedItems.end(); ++itemIter)
		{
			LLPathfindingObjectPtr objectPtr = findObject(*itemIter);
			if (objectPtr != NULL)
			{
				selectedObjects->update(objectPtr);
			}
		}
	}

	return selectedObjects;
}

LLPathfindingObjectPtr LLFloaterPathfindingObjects::getFirstSelectedObject() const
{
	LLPathfindingObjectPtr objectPtr;

	std::vector<LLScrollListItem*> selectedItems = mObjectsScrollList->getAllSelected();
	if (!selectedItems.empty())
	{
		objectPtr = findObject(selectedItems.front());
	}

	return objectPtr;
}

LLFloaterPathfindingObjects::EMessagingState LLFloaterPathfindingObjects::getMessagingState() const
{
	return mMessagingState;
}

void LLFloaterPathfindingObjects::setMessagingState(EMessagingState pMessagingState)
{
	mMessagingState = pMessagingState;
	updateControls();
}

void LLFloaterPathfindingObjects::onRefreshObjectsClicked()
{
	requestGetObjects();
}

void LLFloaterPathfindingObjects::onSelectAllObjectsClicked()
{
	selectAllObjects();
}

void LLFloaterPathfindingObjects::onSelectNoneObjectsClicked()
{
	selectNoneObjects();
}

void LLFloaterPathfindingObjects::onTakeClicked()
{
	handle_take();
}

void LLFloaterPathfindingObjects::onTakeCopyClicked()
{
	handle_take_copy();
}

void LLFloaterPathfindingObjects::onReturnClicked()
{
	handle_object_return();
}

void LLFloaterPathfindingObjects::onDeleteClicked()
{
	handle_object_delete();
}

void LLFloaterPathfindingObjects::onTeleportClicked()
{
	teleportToSelectedObject();
}

void LLFloaterPathfindingObjects::onScrollListSelectionChanged()
{
	updateSelection();
}

void LLFloaterPathfindingObjects::onSelectionListChanged()
{
	updateControls();
}

void LLFloaterPathfindingObjects::onRegionBoundaryCrossed()
{
	requestGetObjects();
}

void LLFloaterPathfindingObjects::updateMessagingStatus()
{
	std::string statusText("");
	LLStyle::Params styleParams;

	switch (getMessagingState())
	{
	case kMessagingUnknown:
		statusText = getString("messaging_initial");
		styleParams.color = mErrorTextColor;
		break;
	case kMessagingGetRequestSent :
		statusText = getString("messaging_get_inprogress");
		styleParams.color = mWarningTextColor;
		break;
	case kMessagingGetError :
		statusText = getString("messaging_get_error");
		styleParams.color = mErrorTextColor;
		break;
	case kMessagingSetRequestSent :
		statusText = getString("messaging_set_inprogress");
		styleParams.color = mWarningTextColor;
		break;
	case kMessagingSetError :
		statusText = getString("messaging_set_error");
		styleParams.color = mErrorTextColor;
		break;
	case kMessagingComplete :
		if (mObjectsScrollList->isEmpty())
		{
			statusText = getString("messaging_complete_none_found");
		}
		else
		{
			S32 numItems = mObjectsScrollList->getItemCount();
			S32 numSelectedItems = mObjectsScrollList->getNumSelected();

			LLLocale locale(LLStringUtil::getLocale());
			std::string numItemsString;
			LLResMgr::getInstance()->getIntegerString(numItemsString, numItems);

			std::string numSelectedItemsString;
			LLResMgr::getInstance()->getIntegerString(numSelectedItemsString, numSelectedItems);

			LLStringUtil::format_map_t string_args;
			string_args["[NUM_SELECTED]"] = numSelectedItemsString;
			string_args["[NUM_TOTAL]"] = numItemsString;
			statusText = getString("messaging_complete_available", string_args);
		}
		break;
	case kMessagingNotEnabled :
		statusText = getString("messaging_not_enabled");
		styleParams.color = mErrorTextColor;
		break;
	default:
		statusText = getString("messaging_initial");
		styleParams.color = mErrorTextColor;
		llassert(0);
		break;
	}

	mMessagingStatus->setText((LLStringExplicit)statusText, styleParams);
}

void LLFloaterPathfindingObjects::updateStateOnListActionControls()
{
	switch (getMessagingState())
	{
	case kMessagingUnknown:
	case kMessagingGetRequestSent :
	case kMessagingSetRequestSent :
		mRefreshListButton->setEnabled(FALSE);
		mSelectAllButton->setEnabled(FALSE);
		mSelectNoneButton->setEnabled(FALSE);
		break;
	case kMessagingGetError :
	case kMessagingSetError :
	case kMessagingNotEnabled :
		mRefreshListButton->setEnabled(TRUE);
		mSelectAllButton->setEnabled(FALSE);
		mSelectNoneButton->setEnabled(FALSE);
		break;
	case kMessagingComplete :
		{
			int numItems = mObjectsScrollList->getItemCount();
			int numSelectedItems = mObjectsScrollList->getNumSelected();
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

void LLFloaterPathfindingObjects::updateStateOnEditFields()
{
	int numSelectedItems = mObjectsScrollList->getNumSelected();
	bool isEditEnabled = (numSelectedItems > 0);

	mShowBeaconCheckBox->setEnabled(isEditEnabled);
	mTakeButton->setEnabled(isEditEnabled && visible_take_object());
	mTakeCopyButton->setEnabled(isEditEnabled && enable_object_take_copy());
	mReturnButton->setEnabled(isEditEnabled && enable_object_return());
	mDeleteButton->setEnabled(isEditEnabled && enable_object_delete());
	mTeleportButton->setEnabled(numSelectedItems == 1);
}

LLPathfindingObjectPtr LLFloaterPathfindingObjects::findObject(const LLScrollListItem *pListItem) const
{
	LLPathfindingObjectPtr objectPtr;

	LLUUID uuid = pListItem->getUUID();
	const std::string &uuidString = uuid.asString();
	llassert(mObjectList != NULL);
	objectPtr = mObjectList->find(uuidString);

	return objectPtr;
}
