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
#include "llagent.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llscrolllistctrl.h"
#include "llresmgr.h"
#include "llviewerregion.h"
#include "llhttpclient.h"
#include "lltextbase.h"
#include "lluuid.h"

//---------------------------------------------------------------------------
// NavmeshDataGetResponder
//---------------------------------------------------------------------------

class NavmeshDataGetResponder : public LLHTTPClient::Responder
{
public:
	NavmeshDataGetResponder(const std::string& pNavmeshDataGetURL, LLFloaterPathfindingLinksets *pLinksetsFloater);
	virtual ~NavmeshDataGetResponder();

	virtual void result(const LLSD& pContent);
	virtual void error(U32 pStatus, const std::string& pReason);

private:
	NavmeshDataGetResponder(const NavmeshDataGetResponder& pOther);

	std::string                  mNavmeshDataGetURL;
	LLFloaterPathfindingLinksets *mLinksetsFloater;
};

//---------------------------------------------------------------------------
// PathfindingLinkset
//---------------------------------------------------------------------------

PathfindingLinkset::PathfindingLinkset(const std::string &pUUID, const LLSD& pNavmeshItem)
	: mUUID(pUUID),
	mName(),
	mDescription(),
	mLandImpact(0U),
	mLocation(),
	mIsFixed(false),
	mIsWalkable(false),
	mIsPhantom(false),
	mA(0.0f),
	mB(0.0f),
	mC(0.0f),
	mD(0.0f)
{
	llassert(pNavmeshItem.has("name"));
	llassert(pNavmeshItem.get("name").isString());
	mName = pNavmeshItem.get("name").asString();

	llassert(pNavmeshItem.has("description"));
	llassert(pNavmeshItem.get("description").isString());
	mDescription = pNavmeshItem.get("description").asString();

	llassert(pNavmeshItem.has("landimpact"));
	llassert(pNavmeshItem.get("landimpact").isInteger());
	llassert(pNavmeshItem.get("landimpact").asInteger() >= 0);
	mLandImpact = pNavmeshItem.get("landimpact").asInteger();

	llassert(pNavmeshItem.has("fixed"));
	llassert(pNavmeshItem.get("fixed").isBoolean());
	mIsFixed = pNavmeshItem.get("fixed").asBoolean();

	llassert(pNavmeshItem.has("walkable"));
	llassert(pNavmeshItem.get("walkable").isBoolean());
	mIsWalkable = pNavmeshItem.get("walkable").asBoolean();

	llassert(pNavmeshItem.has("phantom"));
	//llassert(pNavmeshItem.get("phantom").isBoolean()); XXX stinson 01/10/2012: this should be a boolean but is not
	mIsPhantom = pNavmeshItem.get("phantom").asBoolean();

	llassert(pNavmeshItem.has("A"));
	llassert(pNavmeshItem.get("A").isReal());
	mA = pNavmeshItem.get("A").asReal();

	llassert(pNavmeshItem.has("B"));
	llassert(pNavmeshItem.get("B").isReal());
	mB = pNavmeshItem.get("B").asReal();

	llassert(pNavmeshItem.has("C"));
	llassert(pNavmeshItem.get("C").isReal());
	mC = pNavmeshItem.get("C").asReal();

	llassert(pNavmeshItem.has("D"));
	llassert(pNavmeshItem.get("D").isReal());
	mD = pNavmeshItem.get("D").asReal();

	llassert(pNavmeshItem.has("position"));
	llassert(pNavmeshItem.get("position").isArray());
	mLocation.setValue(pNavmeshItem.get("position"));
}

PathfindingLinkset::PathfindingLinkset(const PathfindingLinkset& pOther)
	: mUUID(pOther.mUUID),
	mName(pOther.mName),
	mDescription(pOther.mDescription),
	mLandImpact(pOther.mLandImpact),
	mLocation(pOther.mLocation),
	mIsFixed(pOther.mIsFixed),
	mIsWalkable(pOther.mIsWalkable),
	mIsPhantom(pOther.mIsPhantom),
	mA(pOther.mA),
	mB(pOther.mB),
	mC(pOther.mC),
	mD(pOther.mD)
{
}

PathfindingLinkset::~PathfindingLinkset()
{
}

PathfindingLinkset& PathfindingLinkset::operator =(const PathfindingLinkset& pOther)
{
	mUUID = pOther.mUUID;
	mName = pOther.mName;
	mDescription = pOther.mDescription;
	mLandImpact = pOther.mLandImpact;
	mLocation = pOther.mLocation;
	mIsFixed = pOther.mIsFixed;
	mIsWalkable = pOther.mIsWalkable;
	mIsPhantom = pOther.mIsPhantom;
	mA = pOther.mA;
	mB = pOther.mB;
	mC = pOther.mC;
	mD = pOther.mD;

	return *this;
}

const LLUUID& PathfindingLinkset::getUUID() const
{
	return mUUID;
}

const std::string& PathfindingLinkset::getName() const
{
	return mName;
}

const std::string& PathfindingLinkset::getDescription() const
{
	return mDescription;
}

U32 PathfindingLinkset::getLandImpact() const
{
	return mLandImpact;
}

const LLVector3& PathfindingLinkset::getPositionAgent() const
{
	return mLocation;
}

BOOL PathfindingLinkset::isFixed() const
{
	return mIsFixed;
}

void PathfindingLinkset::setFixed(BOOL pIsFixed)
{
	mIsFixed = pIsFixed;
}

BOOL PathfindingLinkset::isWalkable() const
{
	return mIsWalkable;
}

void PathfindingLinkset::setWalkable(BOOL pIsWalkable)
{
	mIsWalkable = pIsWalkable;
}

BOOL PathfindingLinkset::isPhantom() const
{
	return mIsPhantom;
}

void PathfindingLinkset::setPhantom(BOOL pIsPhantom)
{
	mIsPhantom = pIsPhantom;
}

F32 PathfindingLinkset::getA() const
{
	return mA;
}

void PathfindingLinkset::setA(F32 pA)
{
	mA = pA;
}

F32 PathfindingLinkset::getB() const
{
	return mB;
}

void PathfindingLinkset::setB(F32 pB)
{
	mB = pB;
}

F32 PathfindingLinkset::getC() const
{
	return mC;
}

void PathfindingLinkset::setC(F32 pC)
{
	mC = pC;
}

F32 PathfindingLinkset::getD() const
{
	return mD;
}

void PathfindingLinkset::setD(F32 pD)
{
	mD = pD;
}

//---------------------------------------------------------------------------
// PathfindingLinksets
//---------------------------------------------------------------------------

PathfindingLinksets::PathfindingLinksets()
	: mAllLinksets(),
	mFilteredLinksets(),
	mIsFilterDirty(false),
	mNameFilter()
{
}

PathfindingLinksets::PathfindingLinksets(const LLSD& pNavmeshData)
	: mAllLinksets(),
	mFilteredLinksets(),
	mIsFilterDirty(false),
	mNameFilter()
{
	parseNavmeshData(pNavmeshData);
}

PathfindingLinksets::PathfindingLinksets(const PathfindingLinksets& pOther)
	: mAllLinksets(pOther.mAllLinksets),
	mFilteredLinksets(pOther.mFilteredLinksets),
	mIsFilterDirty(pOther.mIsFilterDirty),
	mNameFilter(pOther.mNameFilter)
{
}

PathfindingLinksets::~PathfindingLinksets()
{
	clearLinksets();
}

void PathfindingLinksets::parseNavmeshData(const LLSD& pNavmeshData)
{
	clearLinksets();

	for (LLSD::map_const_iterator linksetIter = pNavmeshData.beginMap();
		linksetIter != pNavmeshData.endMap(); ++linksetIter)
	{
		const std::string& uuid(linksetIter->first);
		const LLSD& linksetData = linksetIter->second;
		PathfindingLinkset linkset(uuid, linksetData);

		mAllLinksets.insert(std::pair<std::string, PathfindingLinkset>(uuid, linkset));
	}

	mIsFilterDirty = true;
}

void PathfindingLinksets::clearLinksets()
{
	mAllLinksets.clear();
	mFilteredLinksets.clear();
	mIsFilterDirty = false;
}

const PathfindingLinksets::PathfindingLinksetMap& PathfindingLinksets::getAllLinksets() const
{
	return mAllLinksets;
}

const PathfindingLinksets::PathfindingLinksetMap& PathfindingLinksets::getFilteredLinksets()
{
#if 0
	if (!isFilterActive())
	{
		return mAllLinksets;
	}
	else
	{
		if (mIsFilterDirty)
		{
			mFilteredLinksets.clear();
			for (PathfindingLinksetMap::const_iterator linksetIter = mAllLinksets.begin();
				linksetIter != mAllLinksets.end(); ++linksetIter)
			{

			}
			mIsFilterDirty = false;
		}
		return mFilteredLinksets;
	}
#else
	return mAllLinksets;
#endif
}

//---------------------------------------------------------------------------
// LLFloaterPathfindingLinksets
//---------------------------------------------------------------------------

BOOL LLFloaterPathfindingLinksets::postBuild()
{
	childSetAction("refresh_linksets_list", boost::bind(&LLFloaterPathfindingLinksets::onRefreshLinksetsClicked, this));
	childSetAction("select_all_linksets", boost::bind(&LLFloaterPathfindingLinksets::onSelectAllLinksetsClicked, this));
	childSetAction("select_none_linksets", boost::bind(&LLFloaterPathfindingLinksets::onSelectNoneLinksetsClicked, this));

	mLinksetsScrollList = findChild<LLScrollListCtrl>("pathfinding_linksets");
	llassert(mLinksetsScrollList != NULL);
	mLinksetsScrollList->setCommitCallback(boost::bind(&LLFloaterPathfindingLinksets::onLinksetsSelectionChange, this));
	mLinksetsScrollList->setCommitOnSelectionChange(true);

	mLinksetsStatus = findChild<LLTextBase>("linksets_status");
	llassert(mLinksetsStatus != NULL);

	setFetchState(kFetchInitial);

	return LLFloater::postBuild();
}

void LLFloaterPathfindingLinksets::onOpen(const LLSD& pKey)
{
	sendNavmeshDataGetRequest();
}

void LLFloaterPathfindingLinksets::openLinksetsEditor()
{
	LLFloaterReg::toggleInstanceOrBringToFront("pathfinding_linksets");
}

LLFloaterPathfindingLinksets::EFetchState LLFloaterPathfindingLinksets::getFetchState() const
{
	return mFetchState;
}

BOOL LLFloaterPathfindingLinksets::isFetchInProgress() const
{
	BOOL retVal;
	switch (getFetchState())
	{
	case kFetchStarting :
	case kFetchInProgress :
	case kFetchInProgress_MultiRequested :
	case kFetchReceived :
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
	mFetchState(kFetchInitial),
	mLinksetsScrollList(NULL),
	mLinksetsStatus(NULL)
{
}

LLFloaterPathfindingLinksets::~LLFloaterPathfindingLinksets()
{
}

void LLFloaterPathfindingLinksets::sendNavmeshDataGetRequest()
{
	if (isFetchInProgress())
	{
		if (getFetchState() == kFetchInProgress)
		{
			setFetchState(kFetchInProgress_MultiRequested);
		}
	}
	else
	{
		setFetchState(kFetchStarting);
		clearLinksetsList();

		LLViewerRegion* region = gAgent.getRegion();
		if (region != NULL)
		{
			std::string navmeshDataURL = region->getCapability("ObjectNavmesh");
			if (navmeshDataURL.empty())
			{
				setFetchState(kFetchComplete);
				llwarns << "cannot query navmesh data from current region '" << region->getName() << "'" << llendl;
			}
			else
			{
				setFetchState(kFetchInProgress);
				LLHTTPClient::get(navmeshDataURL, new NavmeshDataGetResponder(navmeshDataURL, this));
			}
		}
	}
}

void LLFloaterPathfindingLinksets::handleNavmeshDataGetReply(const LLSD& pNavmeshData)
{
	setFetchState(kFetchReceived);
	clearLinksetsList();

	mPathfindingLinksets.parseNavmeshData(pNavmeshData);

	const LLVector3& avatarPosition = gAgent.getPositionAgent();
	const PathfindingLinksets::PathfindingLinksetMap& linksetMap = mPathfindingLinksets.getFilteredLinksets();

	for (PathfindingLinksets::PathfindingLinksetMap::const_iterator linksetIter = linksetMap.begin();
		linksetIter != linksetMap.end(); ++linksetIter)
	{
		const PathfindingLinkset& linkset(linksetIter->second);

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
		columns[3]["value"] = llformat("%1.0f m", dist_vec(avatarPosition, linkset.getPositionAgent()));
		columns[3]["font"] = "SANSSERIF";

		columns[4]["column"] = "is_fixed";
		columns[4]["value"] = getString(linkset.isFixed() ? "linkset_is_fixed" : "linkset_is_not_fixed");
		columns[4]["font"] = "SANSSERIF";

		columns[5]["column"] = "is_walkable";
		columns[5]["value"] = getString(linkset.isWalkable() ? "linkset_is_walkable" : "linkset_is_not_walkable");
		columns[5]["font"] = "SANSSERIF";

		columns[6]["column"] = "is_phantom";
		columns[6]["value"] = getString(linkset.isPhantom() ? "linkset_is_phantom" : "linkset_is_not_phantom");
		columns[6]["font"] = "SANSSERIF";

		columns[7]["column"] = "a_percent";
		columns[7]["value"] = llformat("%2.0f", linkset.getA());
		columns[7]["font"] = "SANSSERIF";

		columns[8]["column"] = "b_percent";
		columns[8]["value"] = llformat("%2.0f", linkset.getB());
		columns[8]["font"] = "SANSSERIF";

		columns[9]["column"] = "c_percent";
		columns[9]["value"] = llformat("%2.0f", linkset.getC());
		columns[9]["font"] = "SANSSERIF";

		columns[10]["column"] = "d_percent";
		columns[10]["value"] = llformat("%2.0f", linkset.getD());
		columns[10]["font"] = "SANSSERIF";

		LLSD element;
		element["id"] = linkset.getUUID().asString();
		element["column"] = columns;

		mLinksetsScrollList->addElement(element);
	}

	setFetchState(kFetchComplete);
}

void LLFloaterPathfindingLinksets::handleNavmeshDataGetError(const std::string& pURL, const std::string& pErrorReason)
{
	setFetchState(kFetchError);
	clearLinksetsList();
	llwarns << "Error fetching navmesh data from URL '" << pURL << "' because " << pErrorReason << llendl;
}

void LLFloaterPathfindingLinksets::setFetchState(EFetchState pFetchState)
{
	mFetchState = pFetchState;
	updateLinksetsStatusMessage();
}

void LLFloaterPathfindingLinksets::onLinksetsSelectionChange()
{
	updateLinksetsStatusMessage();
}

void LLFloaterPathfindingLinksets::onRefreshLinksetsClicked()
{
	sendNavmeshDataGetRequest();
}

void LLFloaterPathfindingLinksets::onSelectAllLinksetsClicked()
{
	selectAllLinksets();
}

void LLFloaterPathfindingLinksets::onSelectNoneLinksetsClicked()
{
	selectNoneLinksets();
}

void LLFloaterPathfindingLinksets::clearLinksetsList()
{
	mPathfindingLinksets.clearLinksets();
	mLinksetsScrollList->deleteAllItems();
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

	switch (getFetchState())
	{
	case kFetchStarting :
		statusText = getString("linksets_fetching_starting");
		break;
	case kFetchInProgress :
		statusText = getString("linksets_fetching_inprogress");
		break;
	case kFetchInProgress_MultiRequested :
		statusText = getString("linksets_fetching_inprogress_multi_request");
		break;
	case kFetchReceived :
		statusText = getString("linksets_fetching_received");
		break;
	case kFetchError :
		statusText = getString("linksets_fetching_error");
		styleParams.color = warningColor;
		break;
	case kFetchComplete :
		if (mLinksetsScrollList->isEmpty())
		{
			statusText = getString("linksets_fetching_done_none_found");
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
			statusText = getString("linksets_fetching_done_available", string_args);
		}
		break;
	case kFetchInitial:
	default:
		statusText = getString("linksets_fetching_initial");
		break;
	}

	mLinksetsStatus->setText((LLStringExplicit)statusText, styleParams);
}

NavmeshDataGetResponder::NavmeshDataGetResponder(const std::string& pNavmeshDataGetURL, LLFloaterPathfindingLinksets *pLinksetsFloater)
	: mNavmeshDataGetURL(pNavmeshDataGetURL),
	mLinksetsFloater(pLinksetsFloater)
{
}

NavmeshDataGetResponder::~NavmeshDataGetResponder()
{
	mLinksetsFloater = NULL;
}

void NavmeshDataGetResponder::result(const LLSD& pContent)
{
	mLinksetsFloater->handleNavmeshDataGetReply(pContent);
}

void NavmeshDataGetResponder::error(U32 status, const std::string& reason)
{
	mLinksetsFloater->handleNavmeshDataGetError(mNavmeshDataGetURL, reason);
}
