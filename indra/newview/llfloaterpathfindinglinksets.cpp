/** 
* @file llfloaterpathfindinglinksets.cpp
* @brief "Pathfinding linksets" floater, allowing manipulation of the linksets on the current region.
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

#include "llfloaterpathfindinglinksets.h"

#include <string>

#include <boost/bind.hpp>

#include "llagent.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llfloaterpathfindingobjects.h"
#include "llfloaterreg.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llpathfindinglinkset.h"
#include "llpathfindinglinksetlist.h"
#include "llpathfindingmanager.h"
#include "llsearcheditor.h"
#include "llscrolllistitem.h"
#include "llsd.h"
#include "lltextbase.h"
#include "lltextvalidate.h"
#include "lluicolortable.h"
#include "lluictrl.h"
#include "v3math.h"
#include "v4color.h"

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

void LLFloaterPathfindingLinksets::openLinksetsWithSelectedObjects()
{
	LLFloaterPathfindingLinksets *linksetsFloater = LLFloaterReg::getTypedInstance<LLFloaterPathfindingLinksets>("pathfinding_linksets");
	linksetsFloater->clearFilters();
	linksetsFloater->showFloaterWithSelectionObjects();
}

LLFloaterPathfindingLinksets::LLFloaterPathfindingLinksets(const LLSD& pSeed)
	: LLFloaterPathfindingObjects(pSeed),
	mFilterByName(NULL),
	mFilterByDescription(NULL),
	mFilterByLinksetUse(NULL),
	mEditLinksetUse(NULL),
	mEditLinksetUseWalkable(NULL),
	mEditLinksetUseStaticObstacle(NULL),
	mEditLinksetUseDynamicObstacle(NULL),
	mEditLinksetUseMaterialVolume(NULL),
	mEditLinksetUseExclusionVolume(NULL),
	mEditLinksetUseDynamicPhantom(NULL),
	mLabelWalkabilityCoefficients(NULL),
	mLabelEditA(NULL),
	mLabelSuggestedUseA(NULL),
	mEditA(NULL),
	mLabelEditB(NULL),
	mLabelSuggestedUseB(NULL),
	mEditB(NULL),
	mLabelEditC(NULL),
	mLabelSuggestedUseC(NULL),
	mEditC(NULL),
	mLabelEditD(NULL),
	mLabelSuggestedUseD(NULL),
	mEditD(NULL),
	mApplyEditsButton(NULL),
	mBeaconColor(),
	mPreviousValueA(LLPathfindingLinkset::MAX_WALKABILITY_VALUE),
	mPreviousValueB(LLPathfindingLinkset::MAX_WALKABILITY_VALUE),
	mPreviousValueC(LLPathfindingLinkset::MAX_WALKABILITY_VALUE),
	mPreviousValueD(LLPathfindingLinkset::MAX_WALKABILITY_VALUE)
{
}

LLFloaterPathfindingLinksets::~LLFloaterPathfindingLinksets()
{
}

BOOL LLFloaterPathfindingLinksets::postBuild()
{
	mBeaconColor = LLUIColorTable::getInstance()->getColor("PathfindingLinksetBeaconColor");

    mFilterByName = getChild<LLSearchEditor>("filter_by_name");
    mFilterByName->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onApplyAllFilters, this));
    mFilterByName->setCommitOnFocusLost(true);

    mFilterByDescription = getChild<LLSearchEditor>("filter_by_description");
    mFilterByDescription->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onApplyAllFilters, this));
    mFilterByDescription->setCommitOnFocusLost(true);

	mFilterByLinksetUse = findChild<LLComboBox>("filter_by_linkset_use");
	llassert(mFilterByLinksetUse != NULL);
	mFilterByLinksetUse->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onApplyAllFilters, this));

	childSetAction("apply_filters", boost::bind(&LLFloaterPathfindingLinksets::onApplyAllFilters, this));
	childSetAction("clear_filters", boost::bind(&LLFloaterPathfindingLinksets::onClearFiltersClicked, this));

	mEditLinksetUse = findChild<LLComboBox>("edit_linkset_use");
	llassert(mEditLinksetUse != NULL);
	mEditLinksetUse->clearRows();

	mEditLinksetUseUnset = mEditLinksetUse->addElement(buildLinksetUseScrollListData(getString("linkset_choose_use"), XUI_LINKSET_USE_NONE));
	llassert(mEditLinksetUseUnset != NULL);

	mEditLinksetUseWalkable = mEditLinksetUse->addElement(buildLinksetUseScrollListData(getLinksetUseString(LLPathfindingLinkset::kWalkable), XUI_LINKSET_USE_WALKABLE));
	llassert(mEditLinksetUseWalkable != NULL);

	mEditLinksetUseStaticObstacle = mEditLinksetUse->addElement(buildLinksetUseScrollListData(getLinksetUseString(LLPathfindingLinkset::kStaticObstacle), XUI_LINKSET_USE_STATIC_OBSTACLE));
	llassert(mEditLinksetUseStaticObstacle != NULL);

	mEditLinksetUseDynamicObstacle = mEditLinksetUse->addElement(buildLinksetUseScrollListData(getLinksetUseString(LLPathfindingLinkset::kDynamicObstacle), XUI_LINKSET_USE_DYNAMIC_OBSTACLE));
	llassert(mEditLinksetUseDynamicObstacle != NULL);

	mEditLinksetUseMaterialVolume = mEditLinksetUse->addElement(buildLinksetUseScrollListData(getLinksetUseString(LLPathfindingLinkset::kMaterialVolume), XUI_LINKSET_USE_MATERIAL_VOLUME));
	llassert(mEditLinksetUseMaterialVolume != NULL);

	mEditLinksetUseExclusionVolume = mEditLinksetUse->addElement(buildLinksetUseScrollListData(getLinksetUseString(LLPathfindingLinkset::kExclusionVolume), XUI_LINKSET_USE_EXCLUSION_VOLUME));
	llassert(mEditLinksetUseExclusionVolume != NULL);

	mEditLinksetUseDynamicPhantom = mEditLinksetUse->addElement(buildLinksetUseScrollListData(getLinksetUseString(LLPathfindingLinkset::kDynamicPhantom), XUI_LINKSET_USE_DYNAMIC_PHANTOM));
	llassert(mEditLinksetUseDynamicPhantom != NULL);

	mEditLinksetUse->selectFirstItem();

	mLabelWalkabilityCoefficients = findChild<LLTextBase>("walkability_coefficients_label");
	llassert(mLabelWalkabilityCoefficients != NULL);

	mLabelEditA = findChild<LLTextBase>("edit_a_label");
	llassert(mLabelEditA != NULL);

	mLabelSuggestedUseA = findChild<LLTextBase>("suggested_use_a_label");
	llassert(mLabelSuggestedUseA != NULL);

	mEditA = findChild<LLLineEditor>("edit_a_value");
	llassert(mEditA != NULL);
	mEditA->setPrevalidate(LLTextValidate::validateNonNegativeS32);
	mEditA->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onWalkabilityCoefficientEntered, this, _1, mPreviousValueA));

	mLabelEditB = findChild<LLTextBase>("edit_b_label");
	llassert(mLabelEditB != NULL);

	mLabelSuggestedUseB = findChild<LLTextBase>("suggested_use_b_label");
	llassert(mLabelSuggestedUseB != NULL);

	mEditB = findChild<LLLineEditor>("edit_b_value");
	llassert(mEditB != NULL);
	mEditB->setPrevalidate(LLTextValidate::validateNonNegativeS32);
	mEditB->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onWalkabilityCoefficientEntered, this, _1, mPreviousValueB));

	mLabelEditC = findChild<LLTextBase>("edit_c_label");
	llassert(mLabelEditC != NULL);

	mLabelSuggestedUseC = findChild<LLTextBase>("suggested_use_c_label");
	llassert(mLabelSuggestedUseC != NULL);

	mEditC = findChild<LLLineEditor>("edit_c_value");
	llassert(mEditC != NULL);
	mEditC->setPrevalidate(LLTextValidate::validateNonNegativeS32);
	mEditC->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onWalkabilityCoefficientEntered, this, _1, mPreviousValueC));

	mLabelEditD = findChild<LLTextBase>("edit_d_label");
	llassert(mLabelEditD != NULL);

	mLabelSuggestedUseD = findChild<LLTextBase>("suggested_use_d_label");
	llassert(mLabelSuggestedUseD != NULL);

	mEditD = findChild<LLLineEditor>("edit_d_value");
	llassert(mEditD != NULL);
	mEditD->setPrevalidate(LLTextValidate::validateNonNegativeS32);
	mEditD->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onWalkabilityCoefficientEntered, this, _1, mPreviousValueD));

	mApplyEditsButton = findChild<LLButton>("apply_edit_values");
	llassert(mApplyEditsButton != NULL);
	mApplyEditsButton->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onApplyChangesClicked, this));

	return LLFloaterPathfindingObjects::postBuild();
}

void LLFloaterPathfindingLinksets::requestGetObjects()
{
	LLPathfindingManager::getInstance()->requestGetLinksets(getNewRequestId(), boost::bind(&LLFloaterPathfindingLinksets::handleNewObjectList, this, _1, _2, _3));
}

void LLFloaterPathfindingLinksets::buildObjectsScrollList(const LLPathfindingObjectListPtr pObjectListPtr)
{
	llassert(pObjectListPtr != NULL);
	llassert(!pObjectListPtr->isEmpty());

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
		for (LLPathfindingObjectList::const_iterator objectIter = pObjectListPtr->begin();	objectIter != pObjectListPtr->end(); ++objectIter)
		{
			const LLPathfindingObjectPtr objectPtr = objectIter->second;
			const LLPathfindingLinkset *linksetPtr = dynamic_cast<const LLPathfindingLinkset *>(objectPtr.get());
			llassert(linksetPtr != NULL);

			std::string linksetName = (linksetPtr->isTerrain() ? getString("linkset_terrain_name") : linksetPtr->getName());
			std::string linksetDescription = linksetPtr->getDescription();
			LLStringUtil::toUpper(linksetName);
			LLStringUtil::toUpper(linksetDescription);

			if ((!isFilteringName || (linksetName.find(nameFilter) != std::string::npos)) &&
				(!isFilteringDescription || (linksetDescription.find(descriptionFilter) != std::string::npos)) &&
				(!isFilteringLinksetUse || (linksetPtr->getLinksetUse() == linksetUseFilter)))
			{
				LLSD scrollListItemData = buildLinksetScrollListItemData(linksetPtr, avatarPosition);
				addObjectToScrollList(objectPtr, scrollListItemData);
			}
		}
	}
	else
	{
		for (LLPathfindingObjectList::const_iterator objectIter = pObjectListPtr->begin();	objectIter != pObjectListPtr->end(); ++objectIter)
		{
			const LLPathfindingObjectPtr objectPtr = objectIter->second;
			const LLPathfindingLinkset *linksetPtr = dynamic_cast<const LLPathfindingLinkset *>(objectPtr.get());
			llassert(linksetPtr != NULL);

			LLSD scrollListItemData = buildLinksetScrollListItemData(linksetPtr, avatarPosition);
			addObjectToScrollList(objectPtr, scrollListItemData);
		}
	}
}

void LLFloaterPathfindingLinksets::updateControlsOnScrollListChange()
{
	LLFloaterPathfindingObjects::updateControlsOnScrollListChange();
	updateEditFieldValues();
	updateStateOnEditFields();
	updateStateOnEditLinksetUse();
}

S32 LLFloaterPathfindingLinksets::getNameColumnIndex() const
{
	return 0;
}

S32 LLFloaterPathfindingLinksets::getOwnerNameColumnIndex() const
{
	return 2;
}

std::string LLFloaterPathfindingLinksets::getOwnerName(const LLPathfindingObject *pObject) const
{
	return (pObject->hasOwner()
		? (pObject->hasOwnerName()
		? (pObject->isGroupOwned()
		? (pObject->getOwnerName() + " " + getString("linkset_owner_group"))
		: pObject->getOwnerName())
		: getString("linkset_owner_loading"))
		: getString("linkset_owner_unknown"));
}

const LLColor4 &LLFloaterPathfindingLinksets::getBeaconColor() const
{
	return mBeaconColor;
}

LLPathfindingObjectListPtr LLFloaterPathfindingLinksets::getEmptyObjectList() const
{
	LLPathfindingObjectListPtr objectListPtr(new LLPathfindingLinksetList());
	return objectListPtr;
}

void LLFloaterPathfindingLinksets::requestSetLinksets(LLPathfindingObjectListPtr pLinksetList, LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD)
{
	LLPathfindingManager::getInstance()->requestSetLinksets(getNewRequestId(), pLinksetList, pLinksetUse, pA, pB, pC, pD, boost::bind(&LLFloaterPathfindingLinksets::handleUpdateObjectList, this, _1, _2, _3));
}

void LLFloaterPathfindingLinksets::onApplyAllFilters()
{
	rebuildObjectsScrollList();
}

void LLFloaterPathfindingLinksets::onClearFiltersClicked()
{
	clearFilters();
	rebuildObjectsScrollList();
}

void LLFloaterPathfindingLinksets::onWalkabilityCoefficientEntered(LLUICtrl *pUICtrl, LLSD &pPreviousValue)
{
	LLLineEditor *pLineEditor = static_cast<LLLineEditor *>(pUICtrl);
	llassert(pLineEditor != NULL);

	const std::string &valueString = pLineEditor->getText();

	S32 intValue;
	LLSD value;
	bool doResetValue = false;

	if (valueString.empty())
	{
		value = pPreviousValue;
		doResetValue = true;
	}
	else if (LLStringUtil::convertToS32(valueString, intValue))
	{
		doResetValue = ((intValue < LLPathfindingLinkset::MIN_WALKABILITY_VALUE) || (intValue > LLPathfindingLinkset::MAX_WALKABILITY_VALUE));
		value = LLSD(llclamp(intValue, LLPathfindingLinkset::MIN_WALKABILITY_VALUE, LLPathfindingLinkset::MAX_WALKABILITY_VALUE));
	}
	else
	{
		value = LLSD(LLPathfindingLinkset::MAX_WALKABILITY_VALUE);
		doResetValue = true;
	}

	if (doResetValue)
	{
		pLineEditor->setValue(value);
	}
	pPreviousValue = value;
}

void LLFloaterPathfindingLinksets::onApplyChangesClicked()
{
	applyEdit();
}

void LLFloaterPathfindingLinksets::clearFilters()
{
	mFilterByName->clear();
	mFilterByDescription->clear();
	setFilterLinksetUse(LLPathfindingLinkset::kUnknown);
}

void LLFloaterPathfindingLinksets::updateEditFieldValues()
{
	int numSelectedObjects = getNumSelectedObjects();
	if (numSelectedObjects <= 0)
	{
		mEditLinksetUse->selectFirstItem();
		mEditA->clear();
		mEditB->clear();
		mEditC->clear();
		mEditD->clear();
	}
	else
	{
		LLPathfindingObjectPtr firstSelectedObjectPtr = getFirstSelectedObject();
		llassert(firstSelectedObjectPtr != NULL);

		const LLPathfindingLinkset *linkset = dynamic_cast<const LLPathfindingLinkset *>(firstSelectedObjectPtr.get());

		setEditLinksetUse(linkset->getLinksetUse());
		mPreviousValueA = LLSD(linkset->getWalkabilityCoefficientA());
		mPreviousValueB = LLSD(linkset->getWalkabilityCoefficientB());
		mPreviousValueC = LLSD(linkset->getWalkabilityCoefficientC());
		mPreviousValueD = LLSD(linkset->getWalkabilityCoefficientD());
		mEditA->setValue(mPreviousValueA);
		mEditB->setValue(mPreviousValueB);
		mEditC->setValue(mPreviousValueC);
		mEditD->setValue(mPreviousValueD);
	}
}

LLSD LLFloaterPathfindingLinksets::buildLinksetScrollListItemData(const LLPathfindingLinkset *pLinksetPtr, const LLVector3 &pAvatarPosition) const
{
	llassert(pLinksetPtr != NULL);
	LLSD columns = LLSD::emptyArray();

	if (pLinksetPtr->isTerrain())
	{
		columns[0]["column"] = "name";
		columns[0]["value"] = getString("linkset_terrain_name");

		columns[1]["column"] = "description";
		columns[1]["value"] = getString("linkset_terrain_description");

		columns[2]["column"] = "owner";
		columns[2]["value"] = getString("linkset_terrain_owner");

		columns[3]["column"] = "scripted";
		columns[3]["value"] = getString("linkset_terrain_scripted");

		columns[4]["column"] = "land_impact";
		columns[4]["value"] = getString("linkset_terrain_land_impact");

		columns[5]["column"] = "dist_from_you";
		columns[5]["value"] = getString("linkset_terrain_dist_from_you");
	}
	else
	{
		columns[0]["column"] = "name";
		columns[0]["value"] = pLinksetPtr->getName();

		columns[1]["column"] = "description";
		columns[1]["value"] = pLinksetPtr->getDescription();

		columns[2]["column"] = "owner";
		columns[2]["value"] = getOwnerName(pLinksetPtr);

		columns[3]["column"] = "scripted";
		columns[3]["value"] = (pLinksetPtr->hasIsScripted()
			? (pLinksetPtr->isScripted()
			? getString("linkset_is_scripted")
			: getString("linkset_is_not_scripted"))
			: getString("linkset_is_unknown_scripted"));

		columns[4]["column"] = "land_impact";
		columns[4]["value"] = llformat("%1d", pLinksetPtr->getLandImpact());

		columns[5]["column"] = "dist_from_you";
		columns[5]["value"] = llformat("%1.0f m", dist_vec(pAvatarPosition, pLinksetPtr->getLocation()));
	}

	columns[6]["column"] = "linkset_use";
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
	columns[6]["value"] = linksetUse;

	columns[7]["column"] = "a_percent";
	columns[7]["value"] = llformat("%3d", pLinksetPtr->getWalkabilityCoefficientA());

	columns[8]["column"] = "b_percent";
	columns[8]["value"] = llformat("%3d", pLinksetPtr->getWalkabilityCoefficientB());

	columns[9]["column"] = "c_percent";
	columns[9]["value"] = llformat("%3d", pLinksetPtr->getWalkabilityCoefficientC());

	columns[10]["column"] = "d_percent";
	columns[10]["value"] = llformat("%3d", pLinksetPtr->getWalkabilityCoefficientD());

	return columns;
}

LLSD LLFloaterPathfindingLinksets::buildLinksetUseScrollListData(const std::string &pLabel, S32 pValue) const
{
	LLSD columns;

	columns[0]["column"] = "name";
	columns[0]["value"] = pLabel;
	columns[0]["font"] = "SANSSERIF";

	LLSD element;
	element["value"] = pValue;
	element["column"] = columns;

	return element;
}

bool LLFloaterPathfindingLinksets::isShowUnmodifiablePhantomWarning(LLPathfindingLinkset::ELinksetUse pLinksetUse) const
{
	bool isShowWarning = false;

	if (pLinksetUse != LLPathfindingLinkset::kUnknown)
	{
		LLPathfindingObjectListPtr selectedObjects = getSelectedObjects();
		if ((selectedObjects != NULL) && !selectedObjects->isEmpty())
		{
			const LLPathfindingLinksetList *linksetList = dynamic_cast<const LLPathfindingLinksetList *>(selectedObjects.get());
			isShowWarning = linksetList->isShowUnmodifiablePhantomWarning(pLinksetUse);
		}
	}

	return isShowWarning;
}

bool LLFloaterPathfindingLinksets::isShowPhantomToggleWarning(LLPathfindingLinkset::ELinksetUse pLinksetUse) const
{
	bool isShowWarning = false;

	if (pLinksetUse != LLPathfindingLinkset::kUnknown)
	{
		LLPathfindingObjectListPtr selectedObjects = getSelectedObjects();
		if ((selectedObjects != NULL) && !selectedObjects->isEmpty())
		{
			const LLPathfindingLinksetList *linksetList = dynamic_cast<const LLPathfindingLinksetList *>(selectedObjects.get());
			isShowWarning = linksetList->isShowPhantomToggleWarning(pLinksetUse);
		}
	}

	return isShowWarning;
}

bool LLFloaterPathfindingLinksets::isShowCannotBeVolumeWarning(LLPathfindingLinkset::ELinksetUse pLinksetUse) const
{
	bool isShowWarning = false;

	if (pLinksetUse != LLPathfindingLinkset::kUnknown)
	{
		LLPathfindingObjectListPtr selectedObjects = getSelectedObjects();
		if ((selectedObjects != NULL) && !selectedObjects->isEmpty())
		{
			const LLPathfindingLinksetList *linksetList = dynamic_cast<const LLPathfindingLinksetList *>(selectedObjects.get());
			isShowWarning = linksetList->isShowCannotBeVolumeWarning(pLinksetUse);
		}
	}

	return isShowWarning;
}

void LLFloaterPathfindingLinksets::updateStateOnEditFields()
{
	int numSelectedItems = getNumSelectedObjects();
	bool isEditEnabled = (numSelectedItems > 0);

	mEditLinksetUse->setEnabled(isEditEnabled);

	mLabelWalkabilityCoefficients->setEnabled(isEditEnabled);
	mLabelEditA->setEnabled(isEditEnabled);
	mLabelEditB->setEnabled(isEditEnabled);
	mLabelEditC->setEnabled(isEditEnabled);
	mLabelEditD->setEnabled(isEditEnabled);
	mLabelSuggestedUseA->setEnabled(isEditEnabled);
	mLabelSuggestedUseB->setEnabled(isEditEnabled);
	mLabelSuggestedUseC->setEnabled(isEditEnabled);
	mLabelSuggestedUseD->setEnabled(isEditEnabled);
	mEditA->setEnabled(isEditEnabled);
	mEditB->setEnabled(isEditEnabled);
	mEditC->setEnabled(isEditEnabled);
	mEditD->setEnabled(isEditEnabled);

	mApplyEditsButton->setEnabled(isEditEnabled && (getMessagingState() == kMessagingComplete));
}

void LLFloaterPathfindingLinksets::updateStateOnEditLinksetUse()
{
	BOOL useWalkable = FALSE;
	BOOL useStaticObstacle = FALSE;
	BOOL useDynamicObstacle = FALSE;
	BOOL useMaterialVolume = FALSE;
	BOOL useExclusionVolume = FALSE;
	BOOL useDynamicPhantom = FALSE;

	LLPathfindingObjectListPtr selectedObjects = getSelectedObjects();
	if ((selectedObjects != NULL) && !selectedObjects->isEmpty())
	{
		const LLPathfindingLinksetList *linksetList = dynamic_cast<const LLPathfindingLinksetList *>(selectedObjects.get());
		linksetList->determinePossibleStates(useWalkable, useStaticObstacle, useDynamicObstacle, useMaterialVolume, useExclusionVolume, useDynamicPhantom);
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

	bool showPhantomToggleWarning = isShowPhantomToggleWarning(linksetUse);
	bool showUnmodifiablePhantomWarning = isShowUnmodifiablePhantomWarning(linksetUse);
	bool showCannotBeVolumeWarning = isShowCannotBeVolumeWarning(linksetUse);

	if (showPhantomToggleWarning || showUnmodifiablePhantomWarning || showCannotBeVolumeWarning)
	{
		LLPathfindingLinkset::ELinksetUse restrictedLinksetUse = LLPathfindingLinkset::getLinksetUseWithToggledPhantom(linksetUse);
		LLSD substitutions;
		substitutions["REQUESTED_TYPE"] = getLinksetUseString(linksetUse);
		substitutions["RESTRICTED_TYPE"] = getLinksetUseString(restrictedLinksetUse);

		// Build one of the following notifications names
		//   - PathfindingLinksets_WarnOnPhantom
		//   - PathfindingLinksets_WarnOnPhantom_MismatchOnRestricted
		//   - PathfindingLinksets_WarnOnPhantom_MismatchOnVolume
		//   - PathfindingLinksets_WarnOnPhantom_MismatchOnRestricted_MismatchOnVolume
		//   - PathfindingLinksets_MismatchOnRestricted
		//   - PathfindingLinksets_MismatchOnVolume
		//   - PathfindingLinksets_MismatchOnRestricted_MismatchOnVolume

		std::string notificationName = "PathfindingLinksets";

		if (showPhantomToggleWarning)
		{
			notificationName += "_WarnOnPhantom";
		}
		if (showUnmodifiablePhantomWarning)
		{
			notificationName += "_MismatchOnRestricted";
		}
		if (showCannotBeVolumeWarning)
		{
			notificationName += "_MismatchOnVolume";
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
	LLPathfindingObjectListPtr selectedObjects = getSelectedObjects();
	if ((selectedObjects != NULL) && !selectedObjects->isEmpty())
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


		requestSetLinksets(selectedObjects, linksetUse, aValue, bValue, cValue, dValue);
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
