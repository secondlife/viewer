/** 
* @file llfloaterpathfindinglinksets.cpp
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
#include "llfloaterreg.h"
#include "llfloaterpathfindinglinksets.h"
#include "llsd.h"
#include "lluuid.h"
#include "v3math.h"
#include "lltextvalidate.h"
#include "llagent.h"
#include "lltextbase.h"
#include "lllineeditor.h"
#include "llscrolllistitem.h"
#include "llscrolllistctrl.h"
#include "llcombobox.h"
#include "llcheckboxctrl.h"
#include "llbutton.h"
#include "llresmgr.h"
#include "llviewerregion.h"
#include "llselectmgr.h"
#include "llviewermenu.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llpathfindinglinkset.h"
#include "llpathfindinglinksetlist.h"
#include "llpathfindingmanager.h"
#include "llnotificationsutil.h"

#include <boost/bind.hpp>
#include <boost/signals2.hpp>

#define XUI_LINKSET_USE_NONE             0
#define XUI_LINKSET_USE_WALKABLE         1
#define XUI_LINKSET_USE_STATIC_OBSTACLE  2
#define XUI_LINKSET_USE_DYNAMIC_OBSTACLE 3
#define XUI_LINKSET_USE_MATERIAL_VOLUME  4
#define XUI_LINKSET_USE_EXCLUSION_VOLUME 5
#define XUI_LINKSET_USE_DYNAMIC_PHANTOM  6

//---------------------------------------------------------------------------
// LLFloaterPathfindingLinksets
//---------------------------------------------------------------------------

BOOL LLFloaterPathfindingLinksets::postBuild()
{
	childSetAction("apply_filters", boost::bind(&LLFloaterPathfindingLinksets::onApplyAllFilters, this));
	childSetAction("clear_filters", boost::bind(&LLFloaterPathfindingLinksets::onClearFiltersClicked, this));

	mFilterByName = findChild<LLLineEditor>("filter_by_name");
	llassert(mFilterByName != NULL);
	mFilterByName->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onApplyAllFilters, this));
	mFilterByName->setSelectAllonFocusReceived(true);
	mFilterByName->setCommitOnFocusLost(true);

	mFilterByDescription = findChild<LLLineEditor>("filter_by_description");
	llassert(mFilterByDescription != NULL);
	mFilterByDescription->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onApplyAllFilters, this));
	mFilterByDescription->setSelectAllonFocusReceived(true);
	mFilterByDescription->setCommitOnFocusLost(true);

	mFilterByLinksetUse = findChild<LLComboBox>("filter_by_linkset_use");
	llassert(mFilterByLinksetUse != NULL);
	mFilterByLinksetUse->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onApplyAllFilters, this));

	mLinksetsScrollList = findChild<LLScrollListCtrl>("pathfinding_linksets");
	llassert(mLinksetsScrollList != NULL);
	mLinksetsScrollList->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onLinksetsSelectionChange, this));
	mLinksetsScrollList->sortByColumnIndex(0, true);

	mLinksetsStatus = findChild<LLTextBase>("linksets_status");
	llassert(mLinksetsStatus != NULL);

	mRefreshListButton = findChild<LLButton>("refresh_linksets_list");
	llassert(mRefreshListButton != NULL);
	mRefreshListButton->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onRefreshLinksetsClicked, this));

	mSelectAllButton = findChild<LLButton>("select_all_linksets");
	llassert(mSelectAllButton != NULL);
	mSelectAllButton->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onSelectAllLinksetsClicked, this));

	mSelectNoneButton = findChild<LLButton>("select_none_linksets");
	llassert(mSelectNoneButton != NULL);
	mSelectNoneButton->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onSelectNoneLinksetsClicked, this));

	mShowBeaconCheckBox = findChild<LLCheckBoxCtrl>("show_beacon");
	llassert(mShowBeaconCheckBox != NULL);

	mTakeButton = findChild<LLButton>("take_linksets");
	llassert(mTakeButton != NULL);
	mTakeButton->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onTakeClicked, this));

	mTakeCopyButton = findChild<LLButton>("take_copy_linksets");
	llassert(mTakeCopyButton != NULL);
	mTakeCopyButton->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onTakeCopyClicked, this));

	mReturnButton = findChild<LLButton>("return_linksets");
	llassert(mReturnButton != NULL);
	mReturnButton->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onReturnClicked, this));

	mDeleteButton = findChild<LLButton>("delete_linksets");
	llassert(mDeleteButton != NULL);
	mDeleteButton->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onDeleteClicked, this));

	mTeleportButton = findChild<LLButton>("teleport_me_to_linkset");
	llassert(mTeleportButton != NULL);
	mTeleportButton->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onTeleportClicked, this));

	mEditLinksetUse = findChild<LLComboBox>("edit_linkset_use");
	llassert(mEditLinksetUse != NULL);

	mEditLinksetUse->clearRows();

	mEditLinksetUseUnset = mEditLinksetUse->addElement(buildLinksetUseScrollListElement(getString("linkset_choose_use"), XUI_LINKSET_USE_NONE));
	llassert(mEditLinksetUseUnset != NULL);

	mEditLinksetUseWalkable = mEditLinksetUse->addElement(buildLinksetUseScrollListElement(getLinksetUseString(LLPathfindingLinkset::kWalkable), XUI_LINKSET_USE_WALKABLE));
	llassert(mEditLinksetUseWalkable != NULL);

	mEditLinksetUseStaticObstacle = mEditLinksetUse->addElement(buildLinksetUseScrollListElement(getLinksetUseString(LLPathfindingLinkset::kStaticObstacle), XUI_LINKSET_USE_STATIC_OBSTACLE));
	llassert(mEditLinksetUseStaticObstacle != NULL);

	mEditLinksetUseDynamicObstacle = mEditLinksetUse->addElement(buildLinksetUseScrollListElement(getLinksetUseString(LLPathfindingLinkset::kDynamicObstacle), XUI_LINKSET_USE_DYNAMIC_OBSTACLE));
	llassert(mEditLinksetUseDynamicObstacle != NULL);

	mEditLinksetUseMaterialVolume = mEditLinksetUse->addElement(buildLinksetUseScrollListElement(getLinksetUseString(LLPathfindingLinkset::kMaterialVolume), XUI_LINKSET_USE_MATERIAL_VOLUME));
	llassert(mEditLinksetUseMaterialVolume != NULL);

	mEditLinksetUseExclusionVolume = mEditLinksetUse->addElement(buildLinksetUseScrollListElement(getLinksetUseString(LLPathfindingLinkset::kExclusionVolume), XUI_LINKSET_USE_EXCLUSION_VOLUME));
	llassert(mEditLinksetUseExclusionVolume != NULL);

	mEditLinksetUseDynamicPhantom = mEditLinksetUse->addElement(buildLinksetUseScrollListElement(getLinksetUseString(LLPathfindingLinkset::kDynamicPhantom), XUI_LINKSET_USE_DYNAMIC_PHANTOM));
	llassert(mEditLinksetUseDynamicPhantom != NULL);

	mEditLinksetUse->selectFirstItem();

	mLabelWalkabilityCoefficients = findChild<LLTextBase>("walkability_coefficients_label");
	llassert(mLabelWalkabilityCoefficients != NULL);

	mLabelEditA = findChild<LLTextBase>("edit_a_label");
	llassert(mLabelEditA != NULL);

	mEditA = findChild<LLLineEditor>("edit_a_value");
	llassert(mEditA != NULL);
	mEditA->setPrevalidate(LLTextValidate::validateNonNegativeS32);
	mEditA->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onWalkabilityCoefficientEntered, this, _1));

	mLabelEditB = findChild<LLTextBase>("edit_b_label");
	llassert(mLabelEditB != NULL);

	mEditB = findChild<LLLineEditor>("edit_b_value");
	llassert(mEditB != NULL);
	mEditB->setPrevalidate(LLTextValidate::validateNonNegativeS32);
	mEditB->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onWalkabilityCoefficientEntered, this, _1));

	mLabelEditC = findChild<LLTextBase>("edit_c_label");
	llassert(mLabelEditC != NULL);

	mEditC = findChild<LLLineEditor>("edit_c_value");
	llassert(mEditC != NULL);
	mEditC->setPrevalidate(LLTextValidate::validateNonNegativeS32);
	mEditC->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onWalkabilityCoefficientEntered, this, _1));

	mLabelEditD = findChild<LLTextBase>("edit_d_label");
	llassert(mLabelEditD != NULL);

	mEditD = findChild<LLLineEditor>("edit_d_value");
	llassert(mEditD != NULL);
	mEditD->setPrevalidate(LLTextValidate::validateNonNegativeS32);
	mEditD->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onWalkabilityCoefficientEntered, this, _1));

	mApplyEditsButton = findChild<LLButton>("apply_edit_values");
	llassert(mApplyEditsButton != NULL);
	mApplyEditsButton->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onApplyChangesClicked, this));

	return LLFloater::postBuild();
}

void LLFloaterPathfindingLinksets::onOpen(const LLSD& pKey)
{
	LLFloater::onOpen(pKey);

	requestGetLinksets();
	selectNoneLinksets();
	mLinksetsScrollList->setCommitOnSelectionChange(true);

	if (!mAgentStateSlot.connected())
	{
		mAgentStateSlot = LLPathfindingManager::getInstance()->registerAgentStateListener(boost::bind(&LLFloaterPathfindingLinksets::onAgentStateCB, this, _1));
	}

	if (!mSelectionUpdateSlot.connected())
	{
		mSelectionUpdateSlot = LLSelectMgr::getInstance()->mUpdateSignal.connect(boost::bind(&LLFloaterPathfindingLinksets::updateControls, this));
	}
}

void LLFloaterPathfindingLinksets::onClose(bool pAppQuitting)
{
	if (mSelectionUpdateSlot.connected())
	{
		mSelectionUpdateSlot.disconnect();
	}

	if (mAgentStateSlot.connected())
	{
		mAgentStateSlot.disconnect();
	}

	mLinksetsScrollList->setCommitOnSelectionChange(false);
	selectNoneLinksets();
	if (mLinksetsSelection.notNull())
	{
		mLinksetsSelection.clear();
	}

	LLFloater::onClose(pAppQuitting);
}

void LLFloaterPathfindingLinksets::draw()
{
	if (mShowBeaconCheckBox->get())
	{
		std::vector<LLScrollListItem*> selectedItems = mLinksetsScrollList->getAllSelected();
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
					const std::string &objectName = selectedItem->getColumn(0)->getValue().asString();
					gObjectList.addDebugBeacon(viewerObject->getPositionAgent(), objectName, LLColor4(0.f, 0.f, 1.f, 0.8f), LLColor4(1.f, 1.f, 1.f, 1.f), 6);
				}
			}
		}
	}

	LLFloater::draw();
}

void LLFloaterPathfindingLinksets::openLinksetsEditor()
{
	LLFloaterReg::toggleInstanceOrBringToFront("pathfinding_linksets");
}

LLFloaterPathfindingLinksets::EMessagingState LLFloaterPathfindingLinksets::getMessagingState() const
{
	return mMessagingState;
}

bool LLFloaterPathfindingLinksets::isMessagingInProgress() const
{
	return ((mMessagingState == kMessagingGetRequestSent) || (mMessagingState == kMessagingSetRequestSent));
}

LLFloaterPathfindingLinksets::LLFloaterPathfindingLinksets(const LLSD& pSeed)
	: LLFloater(pSeed),
	mFilterByName(NULL),
	mFilterByDescription(NULL),
	mFilterByLinksetUse(NULL),
	mLinksetsScrollList(NULL),
	mLinksetsStatus(NULL),
	mRefreshListButton(NULL),
	mSelectAllButton(NULL),
	mSelectNoneButton(NULL),
	mShowBeaconCheckBox(NULL),
	mTakeButton(NULL),
	mTakeCopyButton(NULL),
	mReturnButton(NULL),
	mDeleteButton(NULL),
	mTeleportButton(NULL),
	mEditLinksetUse(NULL),
	mLabelWalkabilityCoefficients(NULL),
	mLabelEditA(NULL),
	mEditA(NULL),
	mLabelEditB(NULL),
	mEditB(NULL),
	mLabelEditC(NULL),
	mEditC(NULL),
	mLabelEditD(NULL),
	mEditD(NULL),
	mApplyEditsButton(NULL),
	mMessagingState(kMessagingUnknown),
	mLinksetsListPtr(),
	mLinksetsSelection(),
	mAgentStateSlot(),
	mSelectionUpdateSlot()
{
}

LLFloaterPathfindingLinksets::~LLFloaterPathfindingLinksets()
{
}

void LLFloaterPathfindingLinksets::setMessagingState(EMessagingState pMessagingState)
{
	mMessagingState = pMessagingState;
	updateControls();
}

void LLFloaterPathfindingLinksets::requestGetLinksets()
{
	llassert(!isMessagingInProgress());
	if (!isMessagingInProgress())
	{
		switch (LLPathfindingManager::getInstance()->requestGetLinksets(boost::bind(&LLFloaterPathfindingLinksets::handleNewLinksets, this, _1, _2)))
		{
		case LLPathfindingManager::kRequestStarted :
			setMessagingState(kMessagingGetRequestSent);
			break;
		case LLPathfindingManager::kRequestCompleted :
			clearLinksets();
			setMessagingState(kMessagingComplete);
			break;
		case LLPathfindingManager::kRequestNotEnabled :
			clearLinksets();
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

void LLFloaterPathfindingLinksets::requestSetLinksets(LLPathfindingLinksetListPtr pLinksetList, LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD)
{
	llassert(!isMessagingInProgress());
	if (!isMessagingInProgress())
	{
		switch (LLPathfindingManager::getInstance()->requestSetLinksets(pLinksetList, pLinksetUse, pA, pB, pC, pD, boost::bind(&LLFloaterPathfindingLinksets::handleUpdateLinksets, this, _1, _2)))
		{
		case LLPathfindingManager::kRequestStarted :
			setMessagingState(kMessagingSetRequestSent);
			break;
		case LLPathfindingManager::kRequestCompleted :
			setMessagingState(kMessagingComplete);
			break;
		case LLPathfindingManager::kRequestNotEnabled :
			clearLinksets();
			setMessagingState(kMessagingNotEnabled);
			break;
		case LLPathfindingManager::kRequestError :
			setMessagingState(kMessagingSetError);
			break;
		default :
			setMessagingState(kMessagingSetError);
			llassert(0);
			break;
		}
	}
}

void LLFloaterPathfindingLinksets::handleNewLinksets(LLPathfindingManager::ERequestStatus pLinksetsRequestStatus, LLPathfindingLinksetListPtr pLinksetsListPtr)
{
	mLinksetsListPtr = pLinksetsListPtr;
	updateScrollList();

	switch (pLinksetsRequestStatus)
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

void LLFloaterPathfindingLinksets::handleUpdateLinksets(LLPathfindingManager::ERequestStatus pLinksetsRequestStatus, LLPathfindingLinksetListPtr pLinksetsListPtr)
{
	if (mLinksetsListPtr == NULL)
	{
		mLinksetsListPtr = pLinksetsListPtr;
	}
	else
	{
		mLinksetsListPtr->update(*pLinksetsListPtr);
	}
	updateScrollList();

	switch (pLinksetsRequestStatus)
	{
	case LLPathfindingManager::kRequestCompleted :
		setMessagingState(kMessagingComplete);
		break;
	case LLPathfindingManager::kRequestError :
		setMessagingState(kMessagingSetError);
		break;
	default :
		setMessagingState(kMessagingSetError);
		llassert(0);
		break;
	}
}

void LLFloaterPathfindingLinksets::onApplyAllFilters()
{
	applyFilters();
}

void LLFloaterPathfindingLinksets::onClearFiltersClicked()
{
	clearFilters();
}

void LLFloaterPathfindingLinksets::onLinksetsSelectionChange()
{
	mLinksetsSelection.clear();
	LLSelectMgr::getInstance()->deselectAll();

	std::vector<LLScrollListItem *> selectedItems = mLinksetsScrollList->getAllSelected();
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
			mLinksetsSelection = LLSelectMgr::getInstance()->selectObjectAndFamily(viewerObjects);
		}
	}

	updateEditFieldValues();
	updateControls();
}

void LLFloaterPathfindingLinksets::onRefreshLinksetsClicked()
{
	requestGetLinksets();
}

void LLFloaterPathfindingLinksets::onSelectAllLinksetsClicked()
{
	selectAllLinksets();
}

void LLFloaterPathfindingLinksets::onSelectNoneLinksetsClicked()
{
	selectNoneLinksets();
}

void LLFloaterPathfindingLinksets::onTakeClicked()
{
	handle_take();
}

void LLFloaterPathfindingLinksets::onTakeCopyClicked()
{
	handle_take_copy();
}

void LLFloaterPathfindingLinksets::onReturnClicked()
{
	handle_object_return();
}

void LLFloaterPathfindingLinksets::onDeleteClicked()
{
	handle_object_delete();
}

void LLFloaterPathfindingLinksets::onTeleportClicked()
{
	std::vector<LLScrollListItem*> selectedItems = mLinksetsScrollList->getAllSelected();
	llassert(selectedItems.size() == 1);
	if (selectedItems.size() == 1)
	{
		std::vector<LLScrollListItem*>::const_reference selectedItemRef = selectedItems.front();
		const LLScrollListItem *selectedItem = selectedItemRef;
		llassert(mLinksetsListPtr != NULL);
		LLPathfindingLinksetList::const_iterator linksetIter = mLinksetsListPtr->find(selectedItem->getUUID().asString());
		const LLPathfindingLinksetPtr linksetPtr = linksetIter->second;
		const LLVector3 &linksetLocation = linksetPtr->getLocation();

		LLViewerRegion* region = gAgent.getRegion();
		if (region != NULL)
		{
			gAgent.teleportRequest(region->getHandle(), linksetLocation, true);
		}
	}
}

void LLFloaterPathfindingLinksets::onWalkabilityCoefficientEntered(LLUICtrl *pUICtrl)
{
	LLLineEditor *pLineEditor = static_cast<LLLineEditor *>(pUICtrl);
	llassert(pLineEditor != NULL);

	const std::string &valueString = pLineEditor->getText();
	S32 value = static_cast<S32>(atoi(valueString.c_str()));

	if ((value < LLPathfindingLinkset::MIN_WALKABILITY_VALUE) || (value > LLPathfindingLinkset::MAX_WALKABILITY_VALUE))
	{
		value = llclamp(value, LLPathfindingLinkset::MIN_WALKABILITY_VALUE, LLPathfindingLinkset::MAX_WALKABILITY_VALUE);
		pLineEditor->setValue(LLSD(value));
	}
}

void LLFloaterPathfindingLinksets::onApplyChangesClicked()
{
	applyEdit();
}

void LLFloaterPathfindingLinksets::onAgentStateCB(LLPathfindingManager::EAgentState pAgentState)
{
	updateControls();
}

void LLFloaterPathfindingLinksets::applyFilters()
{
	updateScrollList();
}

void LLFloaterPathfindingLinksets::clearFilters()
{
	mFilterByName->clear();
	mFilterByDescription->clear();
	setFilterLinksetUse(LLPathfindingLinkset::kUnknown);
	updateScrollList();
}

void LLFloaterPathfindingLinksets::selectAllLinksets()
{
	mLinksetsScrollList->selectAll();
}

void LLFloaterPathfindingLinksets::selectNoneLinksets()
{
	mLinksetsScrollList->deselectAllItems();
}

void LLFloaterPathfindingLinksets::clearLinksets()
{
	if (mLinksetsListPtr != NULL)
	{
		mLinksetsListPtr->clear();
	}
	updateScrollList();
}

void LLFloaterPathfindingLinksets::updateControls()
{
	updateStatusMessage();
	updateEnableStateOnListActions();
	updateEnableStateOnEditFields();
}

void LLFloaterPathfindingLinksets::updateEditFieldValues()
{
	std::vector<LLScrollListItem*> selectedItems = mLinksetsScrollList->getAllSelected();
	int numSelectedItems = selectedItems.size();
	if (numSelectedItems <= 0)
	{
		mEditLinksetUse->selectFirstItem();
		mEditA->clear();
		mEditB->clear();
		mEditC->clear();
		mEditD->clear();
	}
	else
	{
		LLScrollListItem *firstItem = selectedItems.front();

		llassert(mLinksetsListPtr != NULL);
		LLPathfindingLinksetList::const_iterator linksetIter = mLinksetsListPtr->find(firstItem->getUUID().asString());
		const LLPathfindingLinksetPtr linksetPtr(linksetIter->second);

		setEditLinksetUse(linksetPtr->getLinksetUse());
		mEditA->setValue(LLSD(linksetPtr->getWalkabilityCoefficientA()));
		mEditB->setValue(LLSD(linksetPtr->getWalkabilityCoefficientB()));
		mEditC->setValue(LLSD(linksetPtr->getWalkabilityCoefficientC()));
		mEditD->setValue(LLSD(linksetPtr->getWalkabilityCoefficientD()));
		updateEnableStateOnEditLinksetUse();
	}
}

void LLFloaterPathfindingLinksets::updateScrollList()
{
	std::vector<LLScrollListItem*> selectedItems = mLinksetsScrollList->getAllSelected();
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

	S32 origScrollPosition = mLinksetsScrollList->getScrollPos();
	mLinksetsScrollList->deleteAllItems();

	if (mLinksetsListPtr != NULL)
	{
		std::string nameFilter = mFilterByName->getText();
		std::string descriptionFilter = mFilterByDescription->getText();
		LLPathfindingLinkset::ELinksetUse linksetUseFilter = getFilterLinksetUse();
		bool isFilteringName = !nameFilter.empty();
		bool isFilteringDescription = !descriptionFilter.empty();
		bool isFilteringLinksetUse = (linksetUseFilter != LLPathfindingLinkset::kUnknown);

		const LLVector3& avatarPosition = gAgent.getPositionAgent();

		if (isFilteringName || isFilteringDescription || isFilteringLinksetUse)
		{
			LLStringUtil::toUpper(nameFilter);
			LLStringUtil::toUpper(descriptionFilter);
			for (LLPathfindingLinksetList::const_iterator linksetIter = mLinksetsListPtr->begin();
				linksetIter != mLinksetsListPtr->end(); ++linksetIter)
			{
				const LLPathfindingLinksetPtr linksetPtr(linksetIter->second);
				std::string linksetName = (linksetPtr->isTerrain() ? getString("linkset_terrain_name") : linksetPtr->getName());
				std::string linksetDescription = linksetPtr->getDescription();
				LLStringUtil::toUpper(linksetName);
				LLStringUtil::toUpper(linksetDescription);
				if ((!isFilteringName || (linksetName.find(nameFilter) != std::string::npos)) &&
					(!isFilteringDescription || (linksetDescription.find(descriptionFilter) != std::string::npos)) &&
					(!isFilteringLinksetUse || (linksetPtr->getLinksetUse() == linksetUseFilter)))
				{
					LLSD element = buildLinksetScrollListElement(linksetPtr, avatarPosition);
					mLinksetsScrollList->addElement(element);
				}
			}
		}
		else
		{
			for (LLPathfindingLinksetList::const_iterator linksetIter = mLinksetsListPtr->begin();
				linksetIter != mLinksetsListPtr->end(); ++linksetIter)
			{
				const LLPathfindingLinksetPtr linksetPtr(linksetIter->second);
				LLSD element = buildLinksetScrollListElement(linksetPtr, avatarPosition);
				mLinksetsScrollList->addElement(element);
			}
		}
	}

	mLinksetsScrollList->selectMultiple(selectedUUIDs);
	mLinksetsScrollList->setScrollPos(origScrollPosition);
	updateEditFieldValues();
	updateControls();
}

LLSD LLFloaterPathfindingLinksets::buildLinksetScrollListElement(const LLPathfindingLinksetPtr pLinksetPtr, const LLVector3 &pAvatarPosition) const
{
	LLSD columns;

	if (pLinksetPtr->isTerrain())
	{
		columns[0]["column"] = "name";
		columns[0]["value"] = getString("linkset_terrain_name");
		columns[0]["font"] = "SANSSERIF";

		columns[1]["column"] = "description";
		columns[1]["value"] = getString("linkset_terrain_description");
		columns[1]["font"] = "SANSSERIF";

		columns[2]["column"] = "land_impact";
		columns[2]["value"] = getString("linkset_terrain_land_impact");
		columns[2]["font"] = "SANSSERIF";

		columns[3]["column"] = "dist_from_you";
		columns[3]["value"] = getString("linkset_terrain_dist_from_you");
		columns[3]["font"] = "SANSSERIF";
	}
	else
	{
		columns[0]["column"] = "name";
		columns[0]["value"] = pLinksetPtr->getName();
		columns[0]["font"] = "SANSSERIF";

		columns[1]["column"] = "description";
		columns[1]["value"] = pLinksetPtr->getDescription();
		columns[1]["font"] = "SANSSERIF";

		columns[2]["column"] = "land_impact";
		columns[2]["value"] = llformat("%1d", pLinksetPtr->getLandImpact());
		columns[2]["font"] = "SANSSERIF";

		columns[3]["column"] = "dist_from_you";
		columns[3]["value"] = llformat("%1.0f m", dist_vec(pAvatarPosition, pLinksetPtr->getLocation()));
		columns[3]["font"] = "SANSSERIF";
	}

	columns[4]["column"] = "linkset_use";
	std::string linksetUse = getLinksetUseString(pLinksetPtr->getLinksetUse());
	if (pLinksetPtr->isTerrain())
	{
		linksetUse += (" " + getString("linkset_is_terrain"));
	}
	else if (!pLinksetPtr->isModifiable() && pLinksetPtr->canBeVolume())
	{
		linksetUse += (" " + getString("linkset_is_restricted_state"));
	}
	else if (pLinksetPtr->isModifiable() && !pLinksetPtr->canBeVolume())
	{
		linksetUse += (" " + getString("linkset_is_non_volume_state"));
	}
	else if (!pLinksetPtr->isModifiable() && !pLinksetPtr->canBeVolume())
	{
		linksetUse += (" " + getString("linkset_is_restricted_non_volume_state"));
	}
	columns[4]["value"] = linksetUse;
	columns[4]["font"] = "SANSSERIF";

	columns[5]["column"] = "a_percent";
	columns[5]["value"] = llformat("%3d", pLinksetPtr->getWalkabilityCoefficientA());
	columns[5]["font"] = "SANSSERIF";

	columns[6]["column"] = "b_percent";
	columns[6]["value"] = llformat("%3d", pLinksetPtr->getWalkabilityCoefficientB());
	columns[6]["font"] = "SANSSERIF";

	columns[7]["column"] = "c_percent";
	columns[7]["value"] = llformat("%3d", pLinksetPtr->getWalkabilityCoefficientC());
	columns[7]["font"] = "SANSSERIF";

	columns[8]["column"] = "d_percent";
	columns[8]["value"] = llformat("%3d", pLinksetPtr->getWalkabilityCoefficientD());
	columns[8]["font"] = "SANSSERIF";

	LLSD element;
	element["id"] = pLinksetPtr->getUUID().asString();
	element["column"] = columns;

	return element;
}

LLSD LLFloaterPathfindingLinksets::buildLinksetUseScrollListElement(const std::string &label, S32 value) const
{
	LLSD columns;

	columns[0]["column"] = "name";
	columns[0]["relwidth"] = static_cast<LLSD::Real>(100.0f);
	columns[0]["value"] = label;
	columns[0]["font"] = "SANSSERIF";

	LLSD element;
	element["value"] = value;
	element["column"] = columns;

	return element;
}

bool LLFloaterPathfindingLinksets::isShowUnmodifiablePhantomWarning(LLPathfindingLinkset::ELinksetUse linksetUse) const
{
	bool showWarning = false;

	if (linksetUse != LLPathfindingLinkset::kUnknown)
	{
		std::vector<LLScrollListItem*> selectedItems = mLinksetsScrollList->getAllSelected();
		if (!selectedItems.empty())
		{
			for (std::vector<LLScrollListItem*>::const_iterator selectedItemIter = selectedItems.begin();
				!showWarning && (selectedItemIter != selectedItems.end()); ++selectedItemIter)
			{
				const LLScrollListItem *selectedItem = *selectedItemIter;
				llassert(mLinksetsListPtr != NULL);
				LLPathfindingLinksetList::const_iterator linksetIter = mLinksetsListPtr->find(selectedItem->getUUID().asString());
				llassert(linksetIter != mLinksetsListPtr->end());
				const LLPathfindingLinksetPtr linksetPtr = linksetIter->second;
				showWarning = linksetPtr->isShowUnmodifiablePhantomWarning(linksetUse);
			}
		}
	}

	return showWarning;
}

bool LLFloaterPathfindingLinksets::isShowCannotBeVolumeWarning(LLPathfindingLinkset::ELinksetUse linksetUse) const
{
	bool showWarning = false;

	if (linksetUse != LLPathfindingLinkset::kUnknown)
	{
		std::vector<LLScrollListItem*> selectedItems = mLinksetsScrollList->getAllSelected();
		if (!selectedItems.empty())
		{
			for (std::vector<LLScrollListItem*>::const_iterator selectedItemIter = selectedItems.begin();
				!showWarning && (selectedItemIter != selectedItems.end()); ++selectedItemIter)
			{
				const LLScrollListItem *selectedItem = *selectedItemIter;
				llassert(mLinksetsListPtr != NULL);
				LLPathfindingLinksetList::const_iterator linksetIter = mLinksetsListPtr->find(selectedItem->getUUID().asString());
				llassert(linksetIter != mLinksetsListPtr->end());
				const LLPathfindingLinksetPtr linksetPtr = linksetIter->second;
				showWarning = linksetPtr->isShowCannotBeVolumeWarning(linksetUse);
			}
		}
	}

	return showWarning;
}

void LLFloaterPathfindingLinksets::updateStatusMessage()
{
	static const LLColor4 warningColor = LLUIColorTable::instance().getColor("DrYellow");

	std::string statusText("");
	LLStyle::Params styleParams;

	switch (getMessagingState())
	{
	case kMessagingUnknown:
		statusText = getString("linksets_messaging_initial");
		break;
	case kMessagingGetRequestSent :
		statusText = getString("linksets_messaging_get_inprogress");
		break;
	case kMessagingGetError :
		statusText = getString("linksets_messaging_get_error");
		styleParams.color = warningColor;
		break;
	case kMessagingSetRequestSent :
		statusText = getString("linksets_messaging_set_inprogress");
		break;
	case kMessagingSetError :
		statusText = getString("linksets_messaging_set_error");
		styleParams.color = warningColor;
		break;
	case kMessagingComplete :
		if (mLinksetsScrollList->isEmpty())
		{
			statusText = getString("linksets_messaging_complete_none_found");
		}
		else
		{
			S32 numItems = mLinksetsScrollList->getItemCount();
			S32 numSelectedItems = mLinksetsScrollList->getNumSelected();

			LLLocale locale(LLStringUtil::getLocale());
			std::string numItemsString;
			LLResMgr::getInstance()->getIntegerString(numItemsString, numItems);

			std::string numSelectedItemsString;
			LLResMgr::getInstance()->getIntegerString(numSelectedItemsString, numSelectedItems);

			LLStringUtil::format_map_t string_args;
			string_args["[NUM_SELECTED]"] = numSelectedItemsString;
			string_args["[NUM_TOTAL]"] = numItemsString;
			statusText = getString("linksets_messaging_complete_available", string_args);
		}
		break;
	case kMessagingNotEnabled :
		statusText = getString("linksets_messaging_not_enabled");
		styleParams.color = warningColor;
		break;
	default:
		statusText = getString("linksets_messaging_initial");
		llassert(0);
		break;
	}

	mLinksetsStatus->setText((LLStringExplicit)statusText, styleParams);
}

void LLFloaterPathfindingLinksets::updateEnableStateOnListActions()
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
			int numItems = mLinksetsScrollList->getItemCount();
			int numSelectedItems = mLinksetsScrollList->getNumSelected();
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

void LLFloaterPathfindingLinksets::updateEnableStateOnEditFields()
{
	int numSelectedItems = mLinksetsScrollList->getNumSelected();
	bool isEditEnabled = ((numSelectedItems > 0) && LLPathfindingManager::getInstance()->isAllowAlterPermanent());

	mShowBeaconCheckBox->setEnabled(numSelectedItems > 0);
	mTakeButton->setEnabled(isEditEnabled && visible_take_object());
	mTakeCopyButton->setEnabled(isEditEnabled && enable_object_take_copy());
	mReturnButton->setEnabled(isEditEnabled && enable_object_return());
	mDeleteButton->setEnabled(isEditEnabled && enable_object_delete());
	mTeleportButton->setEnabled(numSelectedItems == 1);

	mEditLinksetUse->setEnabled(isEditEnabled);

	mLabelWalkabilityCoefficients->setEnabled(isEditEnabled);
	mLabelEditA->setEnabled(isEditEnabled);
	mLabelEditB->setEnabled(isEditEnabled);
	mLabelEditC->setEnabled(isEditEnabled);
	mLabelEditD->setEnabled(isEditEnabled);
	mEditA->setEnabled(isEditEnabled);
	mEditB->setEnabled(isEditEnabled);
	mEditC->setEnabled(isEditEnabled);
	mEditD->setEnabled(isEditEnabled);

	mApplyEditsButton->setEnabled(isEditEnabled && (getMessagingState() == kMessagingComplete));
}

void LLFloaterPathfindingLinksets::updateEnableStateOnEditLinksetUse()
{
	BOOL useWalkable = FALSE;
	BOOL useStaticObstacle = FALSE;
	BOOL useDynamicObstacle = FALSE;
	BOOL useMaterialVolume = FALSE;
	BOOL useExclusionVolume = FALSE;
	BOOL useDynamicPhantom = FALSE;

	std::vector<LLScrollListItem*> selectedItems = mLinksetsScrollList->getAllSelected();
	if (!selectedItems.empty())
	{
		for (std::vector<LLScrollListItem*>::const_iterator selectedItemIter = selectedItems.begin();
			!(useWalkable && useStaticObstacle && useDynamicObstacle && useMaterialVolume && useExclusionVolume && useDynamicPhantom) && (selectedItemIter != selectedItems.end());
			++selectedItemIter)
		{
			const LLScrollListItem *selectedItem = *selectedItemIter;
			llassert(mLinksetsListPtr != NULL);
			LLPathfindingLinksetList::const_iterator linksetIter = mLinksetsListPtr->find(selectedItem->getUUID().asString());
			llassert(linksetIter != mLinksetsListPtr->end());
			const LLPathfindingLinksetPtr linksetPtr = linksetIter->second;

			if (linksetPtr->isTerrain())
			{
				useWalkable = TRUE;
			}
			else
			{
				if (linksetPtr->isModifiable())
				{
					useWalkable = TRUE;
					useStaticObstacle = TRUE;
					useDynamicObstacle = TRUE;
					useDynamicPhantom = TRUE;
					if (linksetPtr->canBeVolume())
					{
						useMaterialVolume = TRUE;
						useExclusionVolume = TRUE;
					}
				}
				else if (linksetPtr->isPhantom())
				{
					useDynamicPhantom = TRUE;
					if (linksetPtr->canBeVolume())
					{
						useMaterialVolume = TRUE;
						useExclusionVolume = TRUE;
					}
				}
				else
				{
					useWalkable = TRUE;
					useStaticObstacle = TRUE;
					useDynamicObstacle = TRUE;
				}
			}
		}
	}

	mEditLinksetUseWalkable->setEnabled(useWalkable);
	mEditLinksetUseStaticObstacle->setEnabled(useStaticObstacle);
	mEditLinksetUseDynamicObstacle->setEnabled(useDynamicObstacle);
	mEditLinksetUseMaterialVolume->setEnabled(useMaterialVolume);
	mEditLinksetUseExclusionVolume->setEnabled(useExclusionVolume);
	mEditLinksetUseDynamicPhantom->setEnabled(useDynamicPhantom);
}

void LLFloaterPathfindingLinksets::applyEdit()
{
	LLPathfindingLinkset::ELinksetUse linksetUse = getEditLinksetUse();

	bool showUnmodifiablePhantomWarning = isShowUnmodifiablePhantomWarning(linksetUse);
	bool showCannotBeVolumeWarning = isShowCannotBeVolumeWarning(linksetUse);

	if (showUnmodifiablePhantomWarning || showCannotBeVolumeWarning)
	{
		LLPathfindingLinkset::ELinksetUse restrictedLinksetUse = LLPathfindingLinkset::getLinksetUseWithToggledPhantom(linksetUse);
		LLSD substitutions;
		substitutions["REQUESTED_TYPE"] = getLinksetUseString(linksetUse);
		substitutions["RESTRICTED_TYPE"] = getLinksetUseString(restrictedLinksetUse);

		std::string notificationName;
		if (showUnmodifiablePhantomWarning && showCannotBeVolumeWarning)
		{
			notificationName = "PathfindingLinksets_SetLinksetUseMismatchOnRestrictedAndVolume";
		}
		else if (showUnmodifiablePhantomWarning)
		{
			notificationName = "PathfindingLinksets_SetLinksetUseMismatchOnRestricted";
		}
		else
		{
			notificationName = "PathfindingLinksets_SetLinksetUseMismatchOnVolume";
		}
		LLNotificationsUtil::add(notificationName, substitutions, LLSD(), boost::bind(&LLFloaterPathfindingLinksets::handleApplyEdit, this, _1, _2));
	}
	else
	{
		doApplyEdit();
	}
}

void LLFloaterPathfindingLinksets::handleApplyEdit(const LLSD &pNotification, const LLSD &pResponse)
{
	if (LLNotificationsUtil::getSelectedOption(pNotification, pResponse) == 0)
	{
		doApplyEdit();
	}
}

void LLFloaterPathfindingLinksets::doApplyEdit()
{
	std::vector<LLScrollListItem*> selectedItems = mLinksetsScrollList->getAllSelected();
	if (!selectedItems.empty())
	{
		LLPathfindingLinkset::ELinksetUse linksetUse = getEditLinksetUse();
		const std::string &aString = mEditA->getText();
		const std::string &bString = mEditB->getText();
		const std::string &cString = mEditC->getText();
		const std::string &dString = mEditD->getText();
		S32 aValue = static_cast<S32>(atoi(aString.c_str()));
		S32 bValue = static_cast<S32>(atoi(bString.c_str()));
		S32 cValue = static_cast<S32>(atoi(cString.c_str()));
		S32 dValue = static_cast<S32>(atoi(dString.c_str()));

		LLPathfindingLinksetListPtr editListPtr(new LLPathfindingLinksetList());
		for (std::vector<LLScrollListItem*>::const_iterator itemIter = selectedItems.begin();
			itemIter != selectedItems.end(); ++itemIter)
		{
			const LLScrollListItem *listItem = *itemIter;
			LLUUID uuid = listItem->getUUID();
			const std::string &uuidString = uuid.asString();
			llassert(mLinksetsListPtr != NULL);
			LLPathfindingLinksetList::iterator linksetIter = mLinksetsListPtr->find(uuidString);
			llassert(linksetIter != mLinksetsListPtr->end());
			LLPathfindingLinksetPtr linksetPtr = linksetIter->second;
			editListPtr->insert(std::pair<std::string, LLPathfindingLinksetPtr>(uuidString, linksetPtr));
		}

		requestSetLinksets(editListPtr, linksetUse, aValue, bValue, cValue, dValue);
	}
}

std::string LLFloaterPathfindingLinksets::getLinksetUseString(LLPathfindingLinkset::ELinksetUse pLinksetUse) const
{
	std::string linksetUse;

	switch (pLinksetUse)
	{
	case LLPathfindingLinkset::kWalkable :
		linksetUse = getString("linkset_use_walkable");
		break;
	case LLPathfindingLinkset::kStaticObstacle :
		linksetUse = getString("linkset_use_static_obstacle");
		break;
	case LLPathfindingLinkset::kDynamicObstacle :
		linksetUse = getString("linkset_use_dynamic_obstacle");
		break;
	case LLPathfindingLinkset::kMaterialVolume :
		linksetUse = getString("linkset_use_material_volume");
		break;
	case LLPathfindingLinkset::kExclusionVolume :
		linksetUse = getString("linkset_use_exclusion_volume");
		break;
	case LLPathfindingLinkset::kDynamicPhantom :
		linksetUse = getString("linkset_use_dynamic_phantom");
		break;
	case LLPathfindingLinkset::kUnknown :
	default :
		linksetUse = getString("linkset_use_dynamic_obstacle");
		llassert(0);
		break;
	}

	return linksetUse;
}

LLPathfindingLinkset::ELinksetUse LLFloaterPathfindingLinksets::getFilterLinksetUse() const
{
	return convertToLinksetUse(mFilterByLinksetUse->getValue());
}

void LLFloaterPathfindingLinksets::setFilterLinksetUse(LLPathfindingLinkset::ELinksetUse pLinksetUse)
{
	mFilterByLinksetUse->setValue(convertToXuiValue(pLinksetUse));
}

LLPathfindingLinkset::ELinksetUse LLFloaterPathfindingLinksets::getEditLinksetUse() const
{
	return convertToLinksetUse(mEditLinksetUse->getValue());
}

void LLFloaterPathfindingLinksets::setEditLinksetUse(LLPathfindingLinkset::ELinksetUse pLinksetUse)
{
	mEditLinksetUse->setValue(convertToXuiValue(pLinksetUse));
}

LLPathfindingLinkset::ELinksetUse LLFloaterPathfindingLinksets::convertToLinksetUse(LLSD pXuiValue) const
{
	LLPathfindingLinkset::ELinksetUse linkUse;

	switch (pXuiValue.asInteger())
	{
	case XUI_LINKSET_USE_NONE :
		linkUse = LLPathfindingLinkset::kUnknown;
		break;
	case XUI_LINKSET_USE_WALKABLE :
		linkUse = LLPathfindingLinkset::kWalkable;
		break;
	case XUI_LINKSET_USE_STATIC_OBSTACLE :
		linkUse = LLPathfindingLinkset::kStaticObstacle;
		break;
	case XUI_LINKSET_USE_DYNAMIC_OBSTACLE :
		linkUse = LLPathfindingLinkset::kDynamicObstacle;
		break;
	case XUI_LINKSET_USE_MATERIAL_VOLUME :
		linkUse = LLPathfindingLinkset::kMaterialVolume;
		break;
	case XUI_LINKSET_USE_EXCLUSION_VOLUME :
		linkUse = LLPathfindingLinkset::kExclusionVolume;
		break;
	case XUI_LINKSET_USE_DYNAMIC_PHANTOM :
		linkUse = LLPathfindingLinkset::kDynamicPhantom;
		break;
	default :
		linkUse = LLPathfindingLinkset::kUnknown;
		llassert(0);
		break;
	}

	return linkUse;
}

LLSD LLFloaterPathfindingLinksets::convertToXuiValue(LLPathfindingLinkset::ELinksetUse pLinksetUse) const
{
	LLSD xuiValue;

	switch (pLinksetUse)
	{
	case LLPathfindingLinkset::kUnknown :
		xuiValue = XUI_LINKSET_USE_NONE;
		break;
	case LLPathfindingLinkset::kWalkable :
		xuiValue = XUI_LINKSET_USE_WALKABLE;
		break;
	case LLPathfindingLinkset::kStaticObstacle :
		xuiValue = XUI_LINKSET_USE_STATIC_OBSTACLE;
		break;
	case LLPathfindingLinkset::kDynamicObstacle :
		xuiValue = XUI_LINKSET_USE_DYNAMIC_OBSTACLE;
		break;
	case LLPathfindingLinkset::kMaterialVolume :
		xuiValue = XUI_LINKSET_USE_MATERIAL_VOLUME;
		break;
	case LLPathfindingLinkset::kExclusionVolume :
		xuiValue = XUI_LINKSET_USE_EXCLUSION_VOLUME;
		break;
	case LLPathfindingLinkset::kDynamicPhantom :
		xuiValue = XUI_LINKSET_USE_DYNAMIC_PHANTOM;
		break;
	default :
		xuiValue = XUI_LINKSET_USE_NONE;
		llassert(0);
		break;
	}

	return xuiValue;
}
