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
#include "llfloaterpathfindinglinksets.h"
#include "llsd.h"
#include "v3math.h"
#include "lltextvalidate.h"
#include "llagent.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "lltextbase.h"
#include "lllineeditor.h"
#include "llscrolllistitem.h"
#include "llscrolllistctrl.h"
#include "llcheckboxctrl.h"
#include "llradiogroup.h"
#include "llbutton.h"
#include "llresmgr.h"
#include "llviewerregion.h"
#include "llhttpclient.h"
#include "lluuid.h"
#include "llpathfindinglinkset.h"
#include "llfilteredpathfindinglinksets.h"

#define XUI_PATH_STATE_WALKABLE 1
#define XUI_PATH_STATE_OBSTACLE 2
#define XUI_PATH_STATE_IGNORED 3

//---------------------------------------------------------------------------
// NavMeshDataGetResponder
//---------------------------------------------------------------------------

class NavMeshDataGetResponder : public LLHTTPClient::Responder
{
public:
	NavMeshDataGetResponder(const std::string& pNavMeshDataGetURL, LLFloaterPathfindingLinksets *pLinksetsFloater);
	virtual ~NavMeshDataGetResponder();

	virtual void result(const LLSD& pContent);
	virtual void error(U32 pStatus, const std::string& pReason);

private:
	NavMeshDataGetResponder(const NavMeshDataGetResponder& pOther);

	std::string                  mNavMeshDataGetURL;
	LLFloaterPathfindingLinksets *mLinksetsFloater;
};

//---------------------------------------------------------------------------
// NavMeshDataPutResponder
//---------------------------------------------------------------------------

class NavMeshDataPutResponder : public LLHTTPClient::Responder
{
public:
	NavMeshDataPutResponder(const std::string& pNavMeshDataPutURL, LLFloaterPathfindingLinksets *pLinksetsFloater);
	virtual ~NavMeshDataPutResponder();

	virtual void result(const LLSD& pContent);
	virtual void error(U32 pStatus, const std::string& pReason);

private:
	NavMeshDataPutResponder(const NavMeshDataPutResponder& pOther);

	std::string                  mNavMeshDataPutURL;
	LLFloaterPathfindingLinksets *mLinksetsFloater;
};

//---------------------------------------------------------------------------
// LLFloaterPathfindingLinksets
//---------------------------------------------------------------------------

BOOL LLFloaterPathfindingLinksets::postBuild()
{
	childSetAction("apply_filters", boost::bind(&LLFloaterPathfindingLinksets::onApplyFiltersClicked, this));
	childSetAction("clear_filters", boost::bind(&LLFloaterPathfindingLinksets::onClearFiltersClicked, this));
	childSetAction("refresh_linksets_list", boost::bind(&LLFloaterPathfindingLinksets::onRefreshLinksetsClicked, this));
	childSetAction("select_all_linksets", boost::bind(&LLFloaterPathfindingLinksets::onSelectAllLinksetsClicked, this));
	childSetAction("select_none_linksets", boost::bind(&LLFloaterPathfindingLinksets::onSelectNoneLinksetsClicked, this));

	mLinksetsScrollList = findChild<LLScrollListCtrl>("pathfinding_linksets");
	llassert(mLinksetsScrollList != NULL);
	mLinksetsScrollList->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onLinksetsSelectionChange, this));
	mLinksetsScrollList->setCommitOnSelectionChange(true);
	mLinksetsScrollList->sortByColumnIndex(0, true);

	mLinksetsStatus = findChild<LLTextBase>("linksets_status");
	llassert(mLinksetsStatus != NULL);

	mFilterByName = findChild<LLLineEditor>("filter_by_name");
	llassert(mFilterByName != NULL);
	mFilterByName->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onApplyFiltersClicked, this));
	mFilterByName->setSelectAllonFocusReceived(true);
	mFilterByName->setCommitOnFocusLost(true);

	mFilterByDescription = findChild<LLLineEditor>("filter_by_description");
	llassert(mFilterByDescription != NULL);
	mFilterByDescription->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onApplyFiltersClicked, this));
	mFilterByDescription->setSelectAllonFocusReceived(true);
	mFilterByDescription->setCommitOnFocusLost(true);

	mFilterByWalkable = findChild<LLCheckBoxCtrl>("filter_by_walkable");
	llassert(mFilterByWalkable != NULL);
	mFilterByWalkable->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onApplyFiltersClicked, this));

	mFilterByObstacle = findChild<LLCheckBoxCtrl>("filter_by_obstacle");
	llassert(mFilterByObstacle != NULL);
	mFilterByObstacle->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onApplyFiltersClicked, this));

	mFilterByIgnored = findChild<LLCheckBoxCtrl>("filter_by_ignored");
	llassert(mFilterByIgnored != NULL);
	mFilterByIgnored->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onApplyFiltersClicked, this));

	mEditPathState = findChild<LLRadioGroup>("edit_path_state");
	llassert(mEditPathState != NULL);

	mEditPathStateWalkable = findChild<LLUICtrl>("edit_pathing_state_walkable");
	llassert(mEditPathStateWalkable != NULL);

	mEditPathStateObstacle = findChild<LLUICtrl>("edit_pathing_state_obstacle");
	llassert(mEditPathStateObstacle != NULL);

	mEditPathStateIgnored = findChild<LLUICtrl>("edit_pathing_state_ignored");
	llassert(mEditPathStateIgnored != NULL);

	mLabelWalkabilityCoefficients = findChild<LLTextBase>("walkability_coefficients_label");
	llassert(mLabelWalkabilityCoefficients != NULL);

	mLabelEditA = findChild<LLTextBase>("edit_a_label");
	llassert(mLabelEditA != NULL);

	mLabelEditB = findChild<LLTextBase>("edit_b_label");
	llassert(mLabelEditB != NULL);

	mLabelEditC = findChild<LLTextBase>("edit_c_label");
	llassert(mLabelEditC != NULL);

	mLabelEditD = findChild<LLTextBase>("edit_d_label");
	llassert(mLabelEditD != NULL);

	mEditA = findChild<LLLineEditor>("edit_a_value");
	llassert(mEditA != NULL);
	mEditA->setPrevalidate(LLTextValidate::validatePositiveS32);

	mEditB = findChild<LLLineEditor>("edit_b_value");
	llassert(mEditB != NULL);
	mEditB->setPrevalidate(LLTextValidate::validatePositiveS32);

	mEditC = findChild<LLLineEditor>("edit_c_value");
	llassert(mEditC != NULL);
	mEditC->setPrevalidate(LLTextValidate::validatePositiveS32);

	mEditD = findChild<LLLineEditor>("edit_d_value");
	llassert(mEditD != NULL);
	mEditD->setPrevalidate(LLTextValidate::validatePositiveS32);

	mEditPhantom = findChild<LLCheckBoxCtrl>("edit_phantom_value");
	llassert(mEditPhantom != NULL);

	mApplyEdits = findChild<LLButton>("apply_edit_values");
	llassert(mApplyEdits != NULL);
	mApplyEdits->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onApplyChangesClicked, this));

	setEnableEditFields(false);
	setMessagingState(kMessagingInitial);

	return LLFloater::postBuild();
}

void LLFloaterPathfindingLinksets::onOpen(const LLSD& pKey)
{
	sendNavMeshDataGetRequest();
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
	mPathfindingLinksets(),
	mMessagingState(kMessagingInitial),
	mLinksetsScrollList(NULL),
	mLinksetsStatus(NULL),
	mFilterByName(NULL),
	mFilterByDescription(NULL),
	mFilterByWalkable(NULL),
	mFilterByObstacle(NULL),
	mFilterByIgnored(NULL),
	mEditPathState(NULL),
	mEditPathStateWalkable(NULL),
	mEditPathStateObstacle(NULL),
	mEditPathStateIgnored(NULL),
	mLabelWalkabilityCoefficients(NULL),
	mLabelEditA(NULL),
	mLabelEditB(NULL),
	mLabelEditC(NULL),
	mLabelEditD(NULL),
	mEditA(NULL),
	mEditB(NULL),
	mEditC(NULL),
	mEditD(NULL),
	mEditPhantom(NULL),
	mApplyEdits(NULL)
{
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
			setMessagingState(kMessagingComplete);
			llwarns << "cannot query object navmesh properties from current region '" << getRegionName() << "'" << llendl;
		}
		else
		{
			setMessagingState(kMessagingFetchRequestSent);
			LLHTTPClient::get(navMeshDataURL, new NavMeshDataGetResponder(navMeshDataURL, this));
		}
	}
}

void LLFloaterPathfindingLinksets::sendNavMeshDataPutRequest(const LLSD& pPostData)
{
	if (!isMessagingInProgress())
	{
		std::string navMeshDataURL = getCapabilityURL();
		if (navMeshDataURL.empty())
		{
			llwarns << "cannot put object navmesh properties for current region '" << getRegionName() << "'" << llendl;
		}
		else
		{
			LLHTTPClient::put(navMeshDataURL, pPostData, new NavMeshDataPutResponder(navMeshDataURL, this));
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
}

void LLFloaterPathfindingLinksets::onApplyFiltersClicked()
{
	applyFilters();
}

void LLFloaterPathfindingLinksets::onClearFiltersClicked()
{
	clearFilters();
}

void LLFloaterPathfindingLinksets::onLinksetsSelectionChange()
{
	updateLinksetsStatusMessage();
	updateEditFields();
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

void LLFloaterPathfindingLinksets::onApplyChangesClicked()
{
	applyEditFields();
}

void LLFloaterPathfindingLinksets::applyFilters()
{
	mPathfindingLinksets.setNameFilter(mFilterByName->getText());
	mPathfindingLinksets.setDescriptionFilter(mFilterByDescription->getText());
	mPathfindingLinksets.setWalkableFilter(mFilterByWalkable->get());
	mPathfindingLinksets.setObstacleFilter(mFilterByObstacle->get());
	mPathfindingLinksets.setIgnoredFilter(mFilterByIgnored->get());
	updateLinksetsList();
}

void LLFloaterPathfindingLinksets::clearFilters()
{
	mPathfindingLinksets.clearFilters();
	mFilterByName->setText(LLStringExplicit(mPathfindingLinksets.getNameFilter()));
	mFilterByDescription->setText(LLStringExplicit(mPathfindingLinksets.getDescriptionFilter()));
	mFilterByWalkable->set(mPathfindingLinksets.isWalkableFilter());
	mFilterByObstacle->set(mPathfindingLinksets.isObstacleFilter());
	mFilterByIgnored->set(mPathfindingLinksets.isIgnoredFilter());
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

		columns[4]["column"] = "path_state";
		switch (linkset.getPathState())
		{
		case LLPathfindingLinkset::kWalkable :
			columns[4]["value"] = getString("linkset_path_state_walkable");
			break;
		case LLPathfindingLinkset::kObstacle :
			columns[4]["value"] = getString("linkset_path_state_obstacle");
			break;
		case LLPathfindingLinkset::kIgnored :
			columns[4]["value"] = getString("linkset_path_state_ignored");
			break;
		default :
			columns[4]["value"] = getString("linkset_path_state_ignored");
			llassert(0);
			break;
		}
		columns[4]["font"] = "SANSSERIF";

		columns[5]["column"] = "is_phantom";
		columns[5]["value"] = getString(linkset.isPhantom() ? "linkset_is_phantom" : "linkset_is_not_phantom");
		columns[5]["font"] = "SANSSERIF";

		columns[6]["column"] = "a_percent";
		columns[6]["value"] = llformat("%3d", linkset.getWalkabilityCoefficientA());
		columns[6]["font"] = "SANSSERIF";

		columns[7]["column"] = "b_percent";
		columns[7]["value"] = llformat("%3d", linkset.getWalkabilityCoefficientB());
		columns[7]["font"] = "SANSSERIF";

		columns[8]["column"] = "c_percent";
		columns[8]["value"] = llformat("%3d", linkset.getWalkabilityCoefficientC());
		columns[8]["font"] = "SANSSERIF";

		columns[9]["column"] = "d_percent";
		columns[9]["value"] = llformat("%3d", linkset.getWalkabilityCoefficientD());
		columns[9]["font"] = "SANSSERIF";

		LLSD element;
		element["id"] = linkset.getUUID().asString();
		element["column"] = columns;

		mLinksetsScrollList->addElement(element);
	}

	mLinksetsScrollList->selectMultiple(selectedUUIDs);
	updateLinksetsStatusMessage();
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
	default:
		statusText = getString("linksets_messaging_initial");
		llassert(0);
		break;
	}

	mLinksetsStatus->setText((LLStringExplicit)statusText, styleParams);
}

void LLFloaterPathfindingLinksets::updateEditFields()
{
	std::vector<LLScrollListItem*> selectedItems = mLinksetsScrollList->getAllSelected();
	if (selectedItems.empty())
	{
		mEditPathState->clear();
		mEditA->clear();
		mEditB->clear();
		mEditC->clear();
		mEditD->clear();
		mEditPhantom->clear();

		setEnableEditFields(false);
	}
	else
	{
		LLScrollListItem *firstItem = selectedItems.front();

		const LLFilteredPathfindingLinksets::PathfindingLinksetMap &linksetsMap = mPathfindingLinksets.getAllLinksets();
		LLFilteredPathfindingLinksets::PathfindingLinksetMap::const_iterator linksetIter = linksetsMap.find(firstItem->getUUID().asString());
		const LLPathfindingLinkset &linkset(linksetIter->second);

		setPathState(linkset.getPathState());
		mEditA->setValue(LLSD(linkset.getWalkabilityCoefficientA()));
		mEditB->setValue(LLSD(linkset.getWalkabilityCoefficientB()));
		mEditC->setValue(LLSD(linkset.getWalkabilityCoefficientC()));
		mEditD->setValue(LLSD(linkset.getWalkabilityCoefficientD()));
		mEditPhantom->set(linkset.isPhantom());

		setEnableEditFields(true);
	}
}

void LLFloaterPathfindingLinksets::applyEditFields()
{
	std::vector<LLScrollListItem*> selectedItems = mLinksetsScrollList->getAllSelected();
	if (!selectedItems.empty())
	{
		LLPathfindingLinkset::EPathState pathState = getPathState();
		const std::string &aString = mEditA->getText();
		const std::string &bString = mEditB->getText();
		const std::string &cString = mEditC->getText();
		const std::string &dString = mEditD->getText();
		S32 aValue = static_cast<S32>(atoi(aString.c_str()));
		S32 bValue = static_cast<S32>(atoi(bString.c_str()));
		S32 cValue = static_cast<S32>(atoi(cString.c_str()));
		S32 dValue = static_cast<S32>(atoi(dString.c_str()));
		BOOL isPhantom = mEditPhantom->getValue();

		const LLFilteredPathfindingLinksets::PathfindingLinksetMap &linksetsMap = mPathfindingLinksets.getAllLinksets();

		LLSD editData;
		for (std::vector<LLScrollListItem*>::const_iterator itemIter = selectedItems.begin();
			itemIter != selectedItems.end(); ++itemIter)
		{
			const LLScrollListItem *listItem = *itemIter;
			LLUUID uuid = listItem->getUUID();

			const LLFilteredPathfindingLinksets::PathfindingLinksetMap::const_iterator linksetIter = linksetsMap.find(uuid.asString());
			const LLPathfindingLinkset &linkset = linksetIter->second;

			LLSD itemData = linkset.getAlteredFields(pathState, aValue, bValue, cValue, dValue, isPhantom);

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

void LLFloaterPathfindingLinksets::setEnableEditFields(BOOL pEnabled)
{
	mEditPathState->setEnabled(pEnabled);
	mEditPathStateWalkable->setEnabled(pEnabled);
	mEditPathStateObstacle->setEnabled(pEnabled);
	mEditPathStateIgnored->setEnabled(pEnabled);
	mLabelWalkabilityCoefficients->setEnabled(pEnabled);
	mLabelEditA->setEnabled(pEnabled);
	mLabelEditB->setEnabled(pEnabled);
	mLabelEditC->setEnabled(pEnabled);
	mLabelEditD->setEnabled(pEnabled);
	mEditA->setEnabled(pEnabled);
	mEditB->setEnabled(pEnabled);
	mEditC->setEnabled(pEnabled);
	mEditD->setEnabled(pEnabled);
	mEditPhantom->setEnabled(pEnabled);
	mApplyEdits->setEnabled(pEnabled);
}

LLPathfindingLinkset::EPathState LLFloaterPathfindingLinksets::getPathState() const
{
	LLPathfindingLinkset::EPathState pathState;
	
	switch (mEditPathState->getValue().asInteger())
	{
	case XUI_PATH_STATE_WALKABLE :
		pathState = LLPathfindingLinkset::kWalkable;
		break;
	case XUI_PATH_STATE_OBSTACLE :
		pathState = LLPathfindingLinkset::kObstacle;
		break;
	case XUI_PATH_STATE_IGNORED :
		pathState = LLPathfindingLinkset::kIgnored;
		break;
	default :
		pathState = LLPathfindingLinkset::kIgnored;
		llassert(0);
		break;
	}

	return pathState;
}

void LLFloaterPathfindingLinksets::setPathState(LLPathfindingLinkset::EPathState pPathState)
{
	LLSD radioGroupValue;

	switch (pPathState)
	{
	case LLPathfindingLinkset::kWalkable :
		radioGroupValue = XUI_PATH_STATE_WALKABLE;
		break;
	case LLPathfindingLinkset::kObstacle :
		radioGroupValue = XUI_PATH_STATE_OBSTACLE;
		break;
	case LLPathfindingLinkset::kIgnored :
		radioGroupValue = XUI_PATH_STATE_IGNORED;
		break;
	default :
		radioGroupValue = XUI_PATH_STATE_IGNORED;
		llassert(0);
		break;
	}

	mEditPathState->setValue(radioGroupValue);
}

//---------------------------------------------------------------------------
// NavMeshDataGetResponder
//---------------------------------------------------------------------------

NavMeshDataGetResponder::NavMeshDataGetResponder(const std::string& pNavMeshDataGetURL, LLFloaterPathfindingLinksets *pLinksetsFloater)
	: mNavMeshDataGetURL(pNavMeshDataGetURL),
	mLinksetsFloater(pLinksetsFloater)
{
}

NavMeshDataGetResponder::~NavMeshDataGetResponder()
{
	mLinksetsFloater = NULL;
}

void NavMeshDataGetResponder::result(const LLSD& pContent)
{
	mLinksetsFloater->handleNavMeshDataGetReply(pContent);
}

void NavMeshDataGetResponder::error(U32 status, const std::string& reason)
{
	mLinksetsFloater->handleNavMeshDataGetError(mNavMeshDataGetURL, reason);
}

//---------------------------------------------------------------------------
// NavMeshDataPutResponder
//---------------------------------------------------------------------------

NavMeshDataPutResponder::NavMeshDataPutResponder(const std::string& pNavMeshDataPutURL, LLFloaterPathfindingLinksets *pLinksetsFloater)
	: mNavMeshDataPutURL(pNavMeshDataPutURL),
	mLinksetsFloater(pLinksetsFloater)
{
}

NavMeshDataPutResponder::~NavMeshDataPutResponder()
{
	mLinksetsFloater = NULL;
}

void NavMeshDataPutResponder::result(const LLSD& pContent)
{
	mLinksetsFloater->handleNavMeshDataPutReply(pContent);
}

void NavMeshDataPutResponder::error(U32 status, const std::string& reason)
{
	mLinksetsFloater->handleNavMeshDataPutError(mNavMeshDataPutURL, reason);
}
