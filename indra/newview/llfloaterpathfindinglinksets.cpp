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
#include "llhandle.h"
#include "lltextbase.h"
#include "lllineeditor.h"
#include "llscrolllistitem.h"
#include "llscrolllistctrl.h"
#include "llcombobox.h"
#include "llbutton.h"
#include "llresmgr.h"
#include "llviewerregion.h"
#include "llselectmgr.h"
#include "llviewermenu.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llhttpclient.h"
#include "llpathfindinglinkset.h"
#include "llfilteredpathfindinglinksets.h"

#define XUI_LINKSET_USE_NONE             0
#define XUI_LINKSET_USE_WALKABLE         1
#define XUI_LINKSET_USE_STATIC_OBSTACLE  2
#define XUI_LINKSET_USE_DYNAMIC_OBSTACLE 3
#define XUI_LINKSET_USE_MATERIAL_VOLUME  4
#define XUI_LINKSET_USE_EXCLUSION_VOLUME 5
#define XUI_LINKSET_USE_DYNAMIC_PHANTOM  6

//---------------------------------------------------------------------------
// NavMeshDataGetResponder
//---------------------------------------------------------------------------

class NavMeshDataGetResponder : public LLHTTPClient::Responder
{
public:
	NavMeshDataGetResponder(const std::string& pNavMeshDataGetURL,
		const LLHandle<LLFloaterPathfindingLinksets> &pLinksetsHandle);
	virtual ~NavMeshDataGetResponder();

	virtual void result(const LLSD& pContent);
	virtual void error(U32 pStatus, const std::string& pReason);

private:
	NavMeshDataGetResponder(const NavMeshDataGetResponder& pOther);

	std::string                            mNavMeshDataGetURL;
	LLHandle<LLFloaterPathfindingLinksets> mLinksetsFloaterHandle;
};

//---------------------------------------------------------------------------
// NavMeshDataPutResponder
//---------------------------------------------------------------------------

class NavMeshDataPutResponder : public LLHTTPClient::Responder
{
public:
	NavMeshDataPutResponder(const std::string& pNavMeshDataPutURL,
		const LLHandle<LLFloaterPathfindingLinksets> &pLinksetsHandle);
	virtual ~NavMeshDataPutResponder();

	virtual void result(const LLSD& pContent);
	virtual void error(U32 pStatus, const std::string& pReason);

private:
	NavMeshDataPutResponder(const NavMeshDataPutResponder& pOther);

	std::string                            mNavMeshDataPutURL;
	LLHandle<LLFloaterPathfindingLinksets> mLinksetsFloaterHandle;
};

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

	mLabelWalkabilityCoefficients = findChild<LLTextBase>("walkability_coefficients_label");
	llassert(mLabelWalkabilityCoefficients != NULL);

	mLabelEditA = findChild<LLTextBase>("edit_a_label");
	llassert(mLabelEditA != NULL);

	mEditA = findChild<LLLineEditor>("edit_a_value");
	llassert(mEditA != NULL);
	mEditA->setPrevalidate(LLTextValidate::validatePositiveS32);

	mLabelEditB = findChild<LLTextBase>("edit_b_label");
	llassert(mLabelEditB != NULL);

	mEditB = findChild<LLLineEditor>("edit_b_value");
	llassert(mEditB != NULL);
	mEditB->setPrevalidate(LLTextValidate::validatePositiveS32);

	mLabelEditC = findChild<LLTextBase>("edit_c_label");
	llassert(mLabelEditC != NULL);

	mEditC = findChild<LLLineEditor>("edit_c_value");
	llassert(mEditC != NULL);
	mEditC->setPrevalidate(LLTextValidate::validatePositiveS32);

	mLabelEditD = findChild<LLTextBase>("edit_d_label");
	llassert(mLabelEditD != NULL);

	mEditD = findChild<LLLineEditor>("edit_d_value");
	llassert(mEditD != NULL);
	mEditD->setPrevalidate(LLTextValidate::validatePositiveS32);

	mApplyEditsButton = findChild<LLButton>("apply_edit_values");
	llassert(mApplyEditsButton != NULL);
	mApplyEditsButton->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onApplyChangesClicked, this));

	setEnableActionAndEditFields(false);
	setMessagingState(kMessagingInitial);

	return LLFloater::postBuild();
}

void LLFloaterPathfindingLinksets::onOpen(const LLSD& pKey)
{
	sendNavMeshDataGetRequest();
	selectNoneLinksets();
	mLinksetsScrollList->setCommitOnSelectionChange(true);
}

void LLFloaterPathfindingLinksets::onClose(bool app_quitting)
{
	mLinksetsScrollList->setCommitOnSelectionChange(false);
	selectNoneLinksets();
	if (mLinksetsSelection.notNull())
	{
		std::vector<LLViewerObject *> selectedObjects;

		LLObjectSelection *linksetsSelected = mLinksetsSelection.get();
		for (LLObjectSelection::valid_iterator linksetsIter = linksetsSelected->valid_begin();
			linksetsIter != linksetsSelected->valid_end(); ++linksetsIter)
		{
			LLSelectNode *linksetsNode = *linksetsIter;
			selectedObjects.push_back(linksetsNode->getObject());
		}

		for (std::vector<LLViewerObject *>::const_iterator selectedObjectIter = selectedObjects.begin();
			selectedObjectIter != selectedObjects.end(); ++selectedObjectIter)
		{
			LLViewerObject *selectedObject = *selectedObjectIter;
			LLSelectMgr::getInstance()->deselectObjectAndFamily(selectedObject);
		}

		mLinksetsSelection.clear();
	}
}

void LLFloaterPathfindingLinksets::openLinksetsEditor()
{
	LLFloaterReg::toggleInstanceOrBringToFront("pathfinding_linksets");
}

LLFloaterPathfindingLinksets::EMessagingState LLFloaterPathfindingLinksets::getMessagingState() const
{
	return mMessagingState;
}

BOOL LLFloaterPathfindingLinksets::isMessagingInProgress() const
{
	BOOL retVal;
	switch (getMessagingState())
	{
	case kMessagingFetchStarting :
	case kMessagingFetchRequestSent :
	case kMessagingFetchRequestSent_MultiRequested :
	case kMessagingFetchReceived :
	case kMessagingModifyStarting :
	case kMessagingModifyRequestSent :
	case kMessagingModifyReceived :
		retVal = true;
		break;
	default :
		retVal = false;
		break;
	}

	return retVal;
}

LLFloaterPathfindingLinksets::LLFloaterPathfindingLinksets(const LLSD& pSeed)
	: LLFloater(pSeed),
	mSelfHandle(),
	mFilterByName(NULL),
	mFilterByDescription(NULL),
	mFilterByLinksetUse(NULL),
	mLinksetsScrollList(NULL),
	mLinksetsStatus(NULL),
	mRefreshListButton(NULL),
	mSelectAllButton(NULL),
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
	mPathfindingLinksets(),
	mMessagingState(kMessagingInitial),
	mLinksetsSelection()
{
	mSelfHandle.bind(this);
}

LLFloaterPathfindingLinksets::~LLFloaterPathfindingLinksets()
{
}

void LLFloaterPathfindingLinksets::sendNavMeshDataGetRequest()
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
		mPathfindingLinksets.clearPathfindingLinksets();
		updateLinksetsList();

		std::string navMeshDataURL = getCapabilityURL();
		if (navMeshDataURL.empty())
		{
			setMessagingState(kMessagingServiceNotAvailable);
			llwarns << "cannot query object navmesh properties from current region '" << getRegionName() << "'" << llendl;
		}
		else
		{
			setMessagingState(kMessagingFetchRequestSent);
			LLHTTPClient::get(navMeshDataURL, new NavMeshDataGetResponder(navMeshDataURL, mSelfHandle));
		}
	}
}

void LLFloaterPathfindingLinksets::sendNavMeshDataPutRequest(const LLSD& pPostData)
{
	if (!isMessagingInProgress())
	{
		setMessagingState(kMessagingModifyStarting);
		std::string navMeshDataURL = getCapabilityURL();
		if (navMeshDataURL.empty())
		{
			setMessagingState(kMessagingServiceNotAvailable);
			llwarns << "cannot put object navmesh properties for current region '" << getRegionName() << "'" << llendl;
		}
		else
		{
			setMessagingState(kMessagingModifyRequestSent);
			LLHTTPClient::put(navMeshDataURL, pPostData, new NavMeshDataPutResponder(navMeshDataURL, mSelfHandle));
		}
	}
}

void LLFloaterPathfindingLinksets::handleNavMeshDataGetReply(const LLSD& pNavMeshData)
{
	setMessagingState(kMessagingFetchReceived);
	mPathfindingLinksets.setPathfindingLinksets(pNavMeshData);
	updateLinksetsList();
	setMessagingState(kMessagingComplete);
}

void LLFloaterPathfindingLinksets::handleNavMeshDataGetError(const std::string& pURL, const std::string& pErrorReason)
{
	setMessagingState(kMessagingFetchError);
	mPathfindingLinksets.clearPathfindingLinksets();
	updateLinksetsList();
	llwarns << "Error fetching object navmesh properties from URL '" << pURL << "' because " << pErrorReason << llendl;
}

void LLFloaterPathfindingLinksets::handleNavMeshDataPutReply(const LLSD& pModifiedData)
{
	setMessagingState(kMessagingModifyReceived);
	mPathfindingLinksets.updatePathfindingLinksets(pModifiedData);
	updateLinksetsList();
	setMessagingState(kMessagingComplete);
}

void LLFloaterPathfindingLinksets::handleNavMeshDataPutError(const std::string& pURL, const std::string& pErrorReason)
{
	setMessagingState(kMessagingModifyError);
	llwarns << "Error putting object navmesh properties to URL '" << pURL << "' because " << pErrorReason << llendl;
}

std::string LLFloaterPathfindingLinksets::getRegionName() const
{
	std::string regionName("");

	LLViewerRegion* region = gAgent.getRegion();
	if (region != NULL)
	{
		regionName = region->getName();
	}

	return regionName;
}

std::string LLFloaterPathfindingLinksets::getCapabilityURL() const
{
	std::string navMeshDataURL("");

	LLViewerRegion* region = gAgent.getRegion();
	if (region != NULL)
	{
		navMeshDataURL = region->getCapability("ObjectNavMeshProperties");
	}

	return navMeshDataURL;
}

void LLFloaterPathfindingLinksets::setMessagingState(EMessagingState pMessagingState)
{
	mMessagingState = pMessagingState;
	updateLinksetsStatusMessage();
	updateActionAndEditFields();
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

	updateLinksetsStatusMessage();
	updateActionAndEditFields();
}

void LLFloaterPathfindingLinksets::onRefreshLinksetsClicked()
{
	sendNavMeshDataGetRequest();
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
		LLFilteredPathfindingLinksets::PathfindingLinksetMap::const_iterator linksetIter = mPathfindingLinksets.getFilteredLinksets().find(selectedItem->getUUID().asString());
		const LLPathfindingLinkset &linkset = linksetIter->second;
		LLVector3 linksetLocation = linkset.getLocation();

		LLViewerRegion* region = gAgent.getRegion();
		if (region != NULL)
		{
			gAgent.teleportRequest(region->getHandle(), linksetLocation, true);
		}
	}
}

void LLFloaterPathfindingLinksets::onApplyChangesClicked()
{
	applyEditFields();
}

void LLFloaterPathfindingLinksets::applyFilters()
{
	mPathfindingLinksets.setNameFilter(mFilterByName->getText());
	mPathfindingLinksets.setDescriptionFilter(mFilterByDescription->getText());
	mPathfindingLinksets.setLinksetUseFilter(getFilterLinksetUse());
	updateLinksetsList();
}

void LLFloaterPathfindingLinksets::clearFilters()
{
	mPathfindingLinksets.clearFilters();
	mFilterByName->setText(LLStringExplicit(mPathfindingLinksets.getNameFilter()));
	mFilterByDescription->setText(LLStringExplicit(mPathfindingLinksets.getDescriptionFilter()));
	setFilterLinksetUse(mPathfindingLinksets.getLinksetUseFilter());
	updateLinksetsList();
}

void LLFloaterPathfindingLinksets::updateLinksetsList()
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

	mLinksetsScrollList->deleteAllItems();
	updateLinksetsStatusMessage();

	const LLVector3& avatarPosition = gAgent.getPositionAgent();
	const LLFilteredPathfindingLinksets::PathfindingLinksetMap& linksetMap = mPathfindingLinksets.getFilteredLinksets();

	for (LLFilteredPathfindingLinksets::PathfindingLinksetMap::const_iterator linksetIter = linksetMap.begin();
		linksetIter != linksetMap.end(); ++linksetIter)
	{
		const LLPathfindingLinkset& linkset(linksetIter->second);

		LLSD columns;

		columns[0]["column"] = "name";
		columns[0]["value"] = linkset.getName();
		columns[0]["font"] = "SANSSERIF";

		columns[1]["column"] = "description";
		columns[1]["value"] = linkset.getDescription();
		columns[1]["font"] = "SANSSERIF";

		columns[2]["column"] = "land_impact";
		columns[2]["value"] = llformat("%1d", linkset.getLandImpact());
		columns[2]["font"] = "SANSSERIF";

		columns[3]["column"] = "dist_from_you";
		columns[3]["value"] = llformat("%1.0f m", dist_vec(avatarPosition, linkset.getLocation()));
		columns[3]["font"] = "SANSSERIF";

		columns[4]["column"] = "linkset_use";
		switch (linkset.getLinksetUse())
		{
		case LLPathfindingLinkset::kWalkable :
			columns[4]["value"] = getString("linkset_use_walkable");
			break;
		case LLPathfindingLinkset::kStaticObstacle :
			columns[4]["value"] = getString("linkset_use_static_obstacle");
			break;
		case LLPathfindingLinkset::kDynamicObstacle :
			columns[4]["value"] = getString("linkset_use_dynamic_obstacle");
			break;
		case LLPathfindingLinkset::kMaterialVolume :
			columns[4]["value"] = getString("linkset_use_material_volume");
			break;
		case LLPathfindingLinkset::kExclusionVolume :
			columns[4]["value"] = getString("linkset_use_exclusion_volume");
			break;
		case LLPathfindingLinkset::kDynamicPhantom :
			columns[4]["value"] = getString("linkset_use_dynamic_phantom");
			break;
		case LLPathfindingLinkset::kUnknown :
		default :
			columns[4]["value"] = getString("linkset_use_dynamic_obstacle");
			llassert(0);
			break;
		}
		columns[4]["font"] = "SANSSERIF";

		columns[5]["column"] = "a_percent";
		columns[5]["value"] = llformat("%3d", linkset.getWalkabilityCoefficientA());
		columns[5]["font"] = "SANSSERIF";

		columns[6]["column"] = "b_percent";
		columns[6]["value"] = llformat("%3d", linkset.getWalkabilityCoefficientB());
		columns[6]["font"] = "SANSSERIF";

		columns[7]["column"] = "c_percent";
		columns[7]["value"] = llformat("%3d", linkset.getWalkabilityCoefficientC());
		columns[7]["font"] = "SANSSERIF";

		columns[8]["column"] = "d_percent";
		columns[8]["value"] = llformat("%3d", linkset.getWalkabilityCoefficientD());
		columns[8]["font"] = "SANSSERIF";

		LLSD element;
		element["id"] = linkset.getUUID().asString();
		element["column"] = columns;

		mLinksetsScrollList->addElement(element);
	}

	mLinksetsScrollList->selectMultiple(selectedUUIDs);
	updateLinksetsStatusMessage();
	updateActionAndEditFields();
}

void LLFloaterPathfindingLinksets::selectAllLinksets()
{
	mLinksetsScrollList->selectAll();
}

void LLFloaterPathfindingLinksets::selectNoneLinksets()
{
	mLinksetsScrollList->deselectAllItems();
}

void LLFloaterPathfindingLinksets::updateLinksetsStatusMessage()
{
	static const LLColor4 warningColor = LLUIColorTable::instance().getColor("DrYellow");

	std::string statusText("");
	LLStyle::Params styleParams;

	switch (getMessagingState())
	{
	case kMessagingInitial:
		statusText = getString("linksets_messaging_initial");
		break;
	case kMessagingFetchStarting :
		statusText = getString("linksets_messaging_fetch_starting");
		break;
	case kMessagingFetchRequestSent :
		statusText = getString("linksets_messaging_fetch_inprogress");
		break;
	case kMessagingFetchRequestSent_MultiRequested :
		statusText = getString("linksets_messaging_fetch_inprogress_multi_request");
		break;
	case kMessagingFetchReceived :
		statusText = getString("linksets_messaging_fetch_received");
		break;
	case kMessagingFetchError :
		statusText = getString("linksets_messaging_fetch_error");
		styleParams.color = warningColor;
		break;
	case kMessagingModifyStarting :
		statusText = getString("linksets_messaging_modify_starting");
		break;
	case kMessagingModifyRequestSent :
		statusText = getString("linksets_messaging_modify_inprogress");
		break;
	case kMessagingModifyReceived :
		statusText = getString("linksets_messaging_modify_received");
		break;
	case kMessagingModifyError :
		statusText = getString("linksets_messaging_modify_error");
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
	case kMessagingServiceNotAvailable :
		statusText = getString("linksets_messaging_service_not_available");
		styleParams.color = warningColor;
		break;
	default:
		statusText = getString("linksets_messaging_initial");
		llassert(0);
		break;
	}

	mLinksetsStatus->setText((LLStringExplicit)statusText, styleParams);
}

void LLFloaterPathfindingLinksets::updateActionAndEditFields()
{
	std::vector<LLScrollListItem*> selectedItems = mLinksetsScrollList->getAllSelected();
	if (selectedItems.empty())
	{
		mEditLinksetUse->clear();
		mEditA->clear();
		mEditB->clear();
		mEditC->clear();
		mEditD->clear();

		setEnableActionAndEditFields(false);
	}
	else
	{
		LLScrollListItem *firstItem = selectedItems.front();

		const LLFilteredPathfindingLinksets::PathfindingLinksetMap &linksetsMap = mPathfindingLinksets.getAllLinksets();
		LLFilteredPathfindingLinksets::PathfindingLinksetMap::const_iterator linksetIter = linksetsMap.find(firstItem->getUUID().asString());
		const LLPathfindingLinkset &linkset(linksetIter->second);

		setEditLinksetUse(linkset.getLinksetUse());
		mEditA->setValue(LLSD(linkset.getWalkabilityCoefficientA()));
		mEditB->setValue(LLSD(linkset.getWalkabilityCoefficientB()));
		mEditC->setValue(LLSD(linkset.getWalkabilityCoefficientC()));
		mEditD->setValue(LLSD(linkset.getWalkabilityCoefficientD()));

		setEnableActionAndEditFields(true);
	}
}

void LLFloaterPathfindingLinksets::setEnableActionAndEditFields(BOOL pEnabled)
{
	mTakeButton->setEnabled(pEnabled && tools_visible_take_object());
	mTakeCopyButton->setEnabled(pEnabled && enable_object_take_copy());
	mReturnButton->setEnabled(pEnabled && enable_object_return());
	mDeleteButton->setEnabled(pEnabled && enable_object_delete());
	mTeleportButton->setEnabled(pEnabled && (mLinksetsScrollList->getNumSelected() == 1));
	mEditLinksetUse->setEnabled(pEnabled);
	mLabelWalkabilityCoefficients->setEnabled(pEnabled);
	mLabelEditA->setEnabled(pEnabled);
	mLabelEditB->setEnabled(pEnabled);
	mLabelEditC->setEnabled(pEnabled);
	mLabelEditD->setEnabled(pEnabled);
	mEditA->setEnabled(pEnabled);
	mEditB->setEnabled(pEnabled);
	mEditC->setEnabled(pEnabled);
	mEditD->setEnabled(pEnabled);
	mApplyEditsButton->setEnabled(pEnabled);
}

void LLFloaterPathfindingLinksets::applyEditFields()
{
	std::vector<LLScrollListItem*> selectedItems = mLinksetsScrollList->getAllSelected();
	if (!selectedItems.empty())
	{
		LLPathfindingLinkset::ELinksetUse pathState = getEditLinksetUse(); // XXX this and pathState
		const std::string &aString = mEditA->getText();
		const std::string &bString = mEditB->getText();
		const std::string &cString = mEditC->getText();
		const std::string &dString = mEditD->getText();
		S32 aValue = static_cast<S32>(atoi(aString.c_str()));
		S32 bValue = static_cast<S32>(atoi(bString.c_str()));
		S32 cValue = static_cast<S32>(atoi(cString.c_str()));
		S32 dValue = static_cast<S32>(atoi(dString.c_str()));

		const LLFilteredPathfindingLinksets::PathfindingLinksetMap &linksetsMap = mPathfindingLinksets.getAllLinksets();

		LLSD editData;
		for (std::vector<LLScrollListItem*>::const_iterator itemIter = selectedItems.begin();
			itemIter != selectedItems.end(); ++itemIter)
		{
			const LLScrollListItem *listItem = *itemIter;
			LLUUID uuid = listItem->getUUID();

			const LLFilteredPathfindingLinksets::PathfindingLinksetMap::const_iterator linksetIter = linksetsMap.find(uuid.asString());
			const LLPathfindingLinkset &linkset = linksetIter->second;

			LLSD itemData = linkset.encodeAlteredFields(pathState, aValue, bValue, cValue, dValue);

			if (!itemData.isUndefined())
			{
				editData[uuid.asString()] = itemData;
			}
		}

		if (editData.isUndefined())
		{
			llwarns << "No PUT data specified" << llendl;
		}
		else
		{
			sendNavMeshDataPutRequest(editData);
		}
	}
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

//---------------------------------------------------------------------------
// NavMeshDataGetResponder
//---------------------------------------------------------------------------

NavMeshDataGetResponder::NavMeshDataGetResponder(const std::string& pNavMeshDataGetURL,
	const LLHandle<LLFloaterPathfindingLinksets> &pLinksetsHandle)
	: mNavMeshDataGetURL(pNavMeshDataGetURL),
	mLinksetsFloaterHandle(pLinksetsHandle)
{
}

NavMeshDataGetResponder::~NavMeshDataGetResponder()
{
}

void NavMeshDataGetResponder::result(const LLSD& pContent)
{
	LLFloaterPathfindingLinksets *linksetsFloater = mLinksetsFloaterHandle.get();
	if (linksetsFloater != NULL)
	{
		linksetsFloater->handleNavMeshDataGetReply(pContent);
	}
}

void NavMeshDataGetResponder::error(U32 status, const std::string& reason)
{
	LLFloaterPathfindingLinksets *linksetsFloater = mLinksetsFloaterHandle.get();
	if (linksetsFloater != NULL)
	{
		linksetsFloater->handleNavMeshDataGetError(mNavMeshDataGetURL, reason);
	}
}

//---------------------------------------------------------------------------
// NavMeshDataPutResponder
//---------------------------------------------------------------------------

NavMeshDataPutResponder::NavMeshDataPutResponder(const std::string& pNavMeshDataPutURL,
	const LLHandle<LLFloaterPathfindingLinksets> &pLinksetsHandle)
	: mNavMeshDataPutURL(pNavMeshDataPutURL),
	mLinksetsFloaterHandle(pLinksetsHandle)
{
}

NavMeshDataPutResponder::~NavMeshDataPutResponder()
{
}

void NavMeshDataPutResponder::result(const LLSD& pContent)
{
	LLFloaterPathfindingLinksets *linksetsFloater = mLinksetsFloaterHandle.get();
	if (linksetsFloater != NULL)
	{
		linksetsFloater->handleNavMeshDataPutReply(pContent);
	}
}

void NavMeshDataPutResponder::error(U32 status, const std::string& reason)
{
	LLFloaterPathfindingLinksets *linksetsFloater = mLinksetsFloaterHandle.get();
	if (linksetsFloater != NULL)
	{
		linksetsFloater->handleNavMeshDataPutError(mNavMeshDataPutURL, reason);
	}
}
