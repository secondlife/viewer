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
// PathfindingLinkset
//---------------------------------------------------------------------------

PathfindingLinkset::PathfindingLinkset(const std::string &pUUID, const LLSD& pNavMeshItem)
	: mUUID(pUUID),
	mName(),
	mDescription(),
	mLandImpact(0U),
	mLocation(),
	mPathState(kIgnored),
	mIsPhantom(false),
	mA(0),
	mB(0),
	mC(0),
	mD(0)
{
	llassert(pNavMeshItem.has("name"));
	llassert(pNavMeshItem.get("name").isString());
	mName = pNavMeshItem.get("name").asString();

	llassert(pNavMeshItem.has("description"));
	llassert(pNavMeshItem.get("description").isString());
	mDescription = pNavMeshItem.get("description").asString();

	llassert(pNavMeshItem.has("landimpact"));
	llassert(pNavMeshItem.get("landimpact").isInteger());
	llassert(pNavMeshItem.get("landimpact").asInteger() >= 0);
	mLandImpact = pNavMeshItem.get("landimpact").asInteger();

	llassert(pNavMeshItem.has("permanent"));
	llassert(pNavMeshItem.get("permanent").isBoolean());
	bool isPermanent = pNavMeshItem.get("permanent").asBoolean();

	llassert(pNavMeshItem.has("walkable"));
	llassert(pNavMeshItem.get("walkable").isBoolean());
	bool isWalkable = pNavMeshItem.get("walkable").asBoolean();

	mPathState = getPathState(isPermanent, isWalkable);

	llassert(pNavMeshItem.has("phantom"));
	//llassert(pNavMeshItem.get("phantom").isBoolean()); XXX stinson 01/10/2012: this should be a boolean but is not
	mIsPhantom = pNavMeshItem.get("phantom").asBoolean();

	llassert(pNavMeshItem.has("A"));
	llassert(pNavMeshItem.get("A").isReal());
	mA = llround(pNavMeshItem.get("A").asReal() * 100.0f);

	llassert(pNavMeshItem.has("B"));
	llassert(pNavMeshItem.get("B").isReal());
	mB = llround(pNavMeshItem.get("B").asReal() * 100.0f);

	llassert(pNavMeshItem.has("C"));
	llassert(pNavMeshItem.get("C").isReal());
	mC = llround(pNavMeshItem.get("C").asReal() * 100.0f);

	llassert(pNavMeshItem.has("D"));
	llassert(pNavMeshItem.get("D").isReal());
	mD = llround(pNavMeshItem.get("D").asReal() * 100.0f);

	llassert(pNavMeshItem.has("position"));
	llassert(pNavMeshItem.get("position").isArray());
	mLocation.setValue(pNavMeshItem.get("position"));
}

PathfindingLinkset::PathfindingLinkset(const PathfindingLinkset& pOther)
	: mUUID(pOther.mUUID),
	mName(pOther.mName),
	mDescription(pOther.mDescription),
	mLandImpact(pOther.mLandImpact),
	mLocation(pOther.mLocation),
	mPathState(pOther.mPathState),
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
	mPathState = pOther.mPathState;
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

PathfindingLinkset::EPathState PathfindingLinkset::getPathState() const
{
	return mPathState;
}

void PathfindingLinkset::setPathState(EPathState pPathState)
{
	mPathState = pPathState;
}

PathfindingLinkset::EPathState PathfindingLinkset::getPathState(bool pIsPermanent, bool pIsWalkable)
{
	return (pIsPermanent ? (pIsWalkable ? kWalkable : kObstacle) : kIgnored);
}

BOOL PathfindingLinkset::isPermanent(EPathState pPathState)
{
	BOOL retVal;

	switch (pPathState)
	{
	case kWalkable :
	case kObstacle :
		retVal = true;
		break;
	case kIgnored :
		retVal = false;
		break;
	default :
		retVal = false;
		llassert(0);
		break;
	}

	return retVal;
}

BOOL PathfindingLinkset::isWalkable(EPathState pPathState)
{
	BOOL retVal;

	switch (pPathState)
	{
	case kWalkable :
		retVal = true;
		break;
	case kObstacle :
	case kIgnored :
		retVal = false;
		break;
	default :
		retVal = false;
		llassert(0);
		break;
	}

	return retVal;
}

BOOL PathfindingLinkset::isPhantom() const
{
	return mIsPhantom;
}

void PathfindingLinkset::setPhantom(BOOL pIsPhantom)
{
	mIsPhantom = pIsPhantom;
}

S32 PathfindingLinkset::getA() const
{
	return mA;
}

void PathfindingLinkset::setA(S32 pA)
{
	mA = pA;
}

S32 PathfindingLinkset::getB() const
{
	return mB;
}

void PathfindingLinkset::setB(S32 pB)
{
	mB = pB;
}

S32 PathfindingLinkset::getC() const
{
	return mC;
}

void PathfindingLinkset::setC(S32 pC)
{
	mC = pC;
}

S32 PathfindingLinkset::getD() const
{
	return mD;
}

void PathfindingLinkset::setD(S32 pD)
{
	mD = pD;
}

//---------------------------------------------------------------------------
// FilterString
//---------------------------------------------------------------------------

FilterString::FilterString()
	: mFilter(),
	mUpperFilter()
{
}

FilterString::FilterString(const std::string& pFilter)
	: mFilter(pFilter),
	mUpperFilter()
{
	LLStringUtil::trim(mFilter);
	mUpperFilter = mFilter;
	if (!mUpperFilter.empty())
	{
		LLStringUtil::toUpper(mUpperFilter);
	}
}

FilterString::FilterString(const FilterString& pOther)
	: mFilter(pOther.mFilter),
	mUpperFilter(pOther.mUpperFilter)
{
}

FilterString::~FilterString()
{
}

const std::string& FilterString::get() const
{
	return mFilter;
}

bool FilterString::set(const std::string& pFilter)
{
	std::string newFilter(pFilter);
	LLStringUtil::trim(newFilter);
	bool didFilterChange = (mFilter.compare(newFilter) != 0);
	if (didFilterChange)
	{
		mFilter = newFilter;
		mUpperFilter = newFilter;
		LLStringUtil::toUpper(mUpperFilter);
	}

	return didFilterChange;
}

void FilterString::clear()
{
	mFilter.clear();
	mUpperFilter.clear();
}

bool FilterString::isActive() const
{
	return !mFilter.empty();
}

bool FilterString::doesMatch(const std::string& pTestString) const
{
	bool doesMatch = true;

	if (isActive())
	{
		std::string upperTestString(pTestString);
		LLStringUtil::toUpper(upperTestString);
		doesMatch = (upperTestString.find(mUpperFilter) != std::string::npos);
	}

	return doesMatch;
}

//---------------------------------------------------------------------------
// PathfindingLinksets
//---------------------------------------------------------------------------

PathfindingLinksets::PathfindingLinksets()
	: mAllLinksets(),
	mFilteredLinksets(),
	mIsFiltersDirty(false),
	mNameFilter(),
	mDescriptionFilter(),
	mIsWalkableFilter(true),
	mIsObstacleFilter(true),
	mIsIgnoredFilter(true)
{
}

PathfindingLinksets::PathfindingLinksets(const LLSD& pNavMeshData)
	: mAllLinksets(),
	mFilteredLinksets(),
	mIsFiltersDirty(false),
	mNameFilter(),
	mDescriptionFilter(),
	mIsWalkableFilter(true),
	mIsObstacleFilter(true),
	mIsIgnoredFilter(true)
{
	setNavMeshData(pNavMeshData);
}

PathfindingLinksets::PathfindingLinksets(const PathfindingLinksets& pOther)
	: mAllLinksets(pOther.mAllLinksets),
	mFilteredLinksets(pOther.mFilteredLinksets),
	mIsFiltersDirty(pOther.mIsFiltersDirty),
	mNameFilter(pOther.mNameFilter),
	mDescriptionFilter(pOther.mDescriptionFilter),
	mIsWalkableFilter(pOther.mIsWalkableFilter),
	mIsObstacleFilter(pOther.mIsObstacleFilter),
	mIsIgnoredFilter(pOther.mIsIgnoredFilter)
{
}

PathfindingLinksets::~PathfindingLinksets()
{
	clearLinksets();
}

void PathfindingLinksets::setNavMeshData(const LLSD& pNavMeshData)
{
	clearLinksets();

	for (LLSD::map_const_iterator navMeshDataIter = pNavMeshData.beginMap();
		navMeshDataIter != pNavMeshData.endMap(); ++navMeshDataIter)
	{
		const std::string& uuid(navMeshDataIter->first);
		const LLSD& linksetData = navMeshDataIter->second;
		PathfindingLinkset linkset(uuid, linksetData);

		mAllLinksets.insert(std::pair<std::string, PathfindingLinkset>(uuid, linkset));
	}

	mIsFiltersDirty = true;
}

void PathfindingLinksets::updateNavMeshData(const LLSD& pNavMeshData)
{
	for (LLSD::map_const_iterator navMeshDataIter = pNavMeshData.beginMap();
		navMeshDataIter != pNavMeshData.endMap(); ++navMeshDataIter)
	{
		const std::string& uuid(navMeshDataIter->first);
		const LLSD& linksetData = navMeshDataIter->second;
		PathfindingLinkset linkset(uuid, linksetData);

		PathfindingLinksetMap::iterator linksetIter = mAllLinksets.find(uuid);
		if (linksetIter == mAllLinksets.end())
		{
			mAllLinksets.insert(std::pair<std::string, PathfindingLinkset>(uuid, linkset));
		}
		else
		{
			linksetIter->second = linkset;
		}
	}

	mIsFiltersDirty = true;
}

void PathfindingLinksets::clearLinksets()
{
	mAllLinksets.clear();
	mFilteredLinksets.clear();
	mIsFiltersDirty = false;
}

const PathfindingLinksets::PathfindingLinksetMap& PathfindingLinksets::getAllLinksets() const
{
	return mAllLinksets;
}

const PathfindingLinksets::PathfindingLinksetMap& PathfindingLinksets::getFilteredLinksets()
{
	if (!isFiltersActive())
	{
		return mAllLinksets;
	}
	else
	{
		applyFilters();
		return mFilteredLinksets;
	}
}

BOOL PathfindingLinksets::isFiltersActive() const
{
	return (mNameFilter.isActive() || mDescriptionFilter.isActive() || !mIsWalkableFilter || !mIsIgnoredFilter || !mIsIgnoredFilter);
}

void PathfindingLinksets::setNameFilter(const std::string& pNameFilter)
{
	mIsFiltersDirty = (mNameFilter.set(pNameFilter) || mIsFiltersDirty);
}

const std::string& PathfindingLinksets::getNameFilter() const
{
	return mNameFilter.get();
}

void PathfindingLinksets::setDescriptionFilter(const std::string& pDescriptionFilter)
{
	mIsFiltersDirty = (mDescriptionFilter.set(pDescriptionFilter) || mIsFiltersDirty);
}

const std::string& PathfindingLinksets::getDescriptionFilter() const
{
	return mDescriptionFilter.get();
}

void PathfindingLinksets::setWalkableFilter(BOOL pWalkableFilter)
{
	mIsFiltersDirty = (mIsFiltersDirty || (mIsWalkableFilter == pWalkableFilter));
	mIsWalkableFilter = pWalkableFilter;
}

BOOL PathfindingLinksets::isWalkableFilter() const
{
	return mIsWalkableFilter;
}

void PathfindingLinksets::setObstacleFilter(BOOL pObstacleFilter)
{
	mIsFiltersDirty = (mIsFiltersDirty || (mIsObstacleFilter == pObstacleFilter));
	mIsObstacleFilter = pObstacleFilter;
}

BOOL PathfindingLinksets::isObstacleFilter() const
{
	return mIsObstacleFilter;
}

void PathfindingLinksets::setIgnoredFilter(BOOL pIgnoredFilter)
{
	mIsFiltersDirty = (mIsFiltersDirty || (mIsIgnoredFilter == pIgnoredFilter));
	mIsIgnoredFilter = pIgnoredFilter;
}

BOOL PathfindingLinksets::isIgnoredFilter() const
{
	return mIsIgnoredFilter;
}

void PathfindingLinksets::clearFilters()
{
	mNameFilter.clear();
	mDescriptionFilter.clear();
	mIsWalkableFilter = true;
	mIsObstacleFilter = true;
	mIsIgnoredFilter = true;
	mIsFiltersDirty = false;
}

void PathfindingLinksets::applyFilters()
{
	mFilteredLinksets.clear();

	for (PathfindingLinksetMap::const_iterator linksetIter = mAllLinksets.begin();
		linksetIter != mAllLinksets.end(); ++linksetIter)
	{
		const std::string& uuid(linksetIter->first);
		const PathfindingLinkset& linkset(linksetIter->second);
		if (doesMatchFilters(linkset))
		{
			mFilteredLinksets.insert(std::pair<std::string, PathfindingLinkset>(uuid, linkset));
		}
	}

	mIsFiltersDirty = false;
}

BOOL PathfindingLinksets::doesMatchFilters(const PathfindingLinkset& pLinkset) const
{
	return (((mIsWalkableFilter && (pLinkset.getPathState() == PathfindingLinkset::kWalkable)) ||
			 (mIsObstacleFilter && (pLinkset.getPathState() == PathfindingLinkset::kObstacle)) ||
			 (mIsIgnoredFilter && (pLinkset.getPathState() == PathfindingLinkset::kIgnored))) &&
			(!mNameFilter.isActive() || mNameFilter.doesMatch(pLinkset.getName())) &&
			(!mDescriptionFilter.isActive() || mDescriptionFilter.doesMatch(pLinkset.getDescription())));
}

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
	setFetchState(kFetchInitial);

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
	case kFetchRequestSent :
	case kFetchRequestSent_MultiRequested :
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
	mLinksetsStatus(NULL),
	mFilterByName(NULL),
	mFilterByDescription(NULL),
	mFilterByWalkable(NULL),
	mFilterByObstacle(NULL),
	mFilterByIgnored(NULL),
	mEditPathState(NULL),
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
	if (isFetchInProgress())
	{
		if (getFetchState() == kFetchRequestSent)
		{
			setFetchState(kFetchRequestSent_MultiRequested);
		}
	}
	else
	{
		setFetchState(kFetchStarting);
		mPathfindingLinksets.clearLinksets();
		updateLinksetsList();

		std::string navMeshDataURL = getCapabilityURL();
		if (navMeshDataURL.empty())
		{
			setFetchState(kFetchComplete);
			llwarns << "cannot query object navmesh properties from current region '" << getRegionName() << "'" << llendl;
		}
		else
		{
			setFetchState(kFetchRequestSent);
			LLHTTPClient::get(navMeshDataURL, new NavMeshDataGetResponder(navMeshDataURL, this));
		}
	}
}

void LLFloaterPathfindingLinksets::sendNavMeshDataPutRequest(const LLSD& pPostData)
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

void LLFloaterPathfindingLinksets::handleNavMeshDataGetReply(const LLSD& pNavMeshData)
{
	setFetchState(kFetchReceived);
	mPathfindingLinksets.setNavMeshData(pNavMeshData);
	updateLinksetsList();
	setFetchState(kFetchComplete);
}

void LLFloaterPathfindingLinksets::handleNavMeshDataGetError(const std::string& pURL, const std::string& pErrorReason)
{
	setFetchState(kFetchError);
	mPathfindingLinksets.clearLinksets();
	updateLinksetsList();
	llwarns << "Error fetching object navmesh properties from URL '" << pURL << "' because " << pErrorReason << llendl;
}

void LLFloaterPathfindingLinksets::handleNavMeshDataPutReply(const LLSD& pModifiedData)
{
	setFetchState(kFetchReceived);
	mPathfindingLinksets.updateNavMeshData(pModifiedData);
	updateLinksetsList();
	setFetchState(kFetchComplete);
}

void LLFloaterPathfindingLinksets::handleNavMeshDataPutError(const std::string& pURL, const std::string& pErrorReason)
{
	setFetchState(kFetchError);
	mPathfindingLinksets.clearLinksets();
	updateLinksetsList();
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
#ifdef XXX_STINSON_REGION_CAP_RENAME
	std::string navMeshDataURL("");

	LLViewerRegion* region = gAgent.getRegion();
	if (region != NULL)
	{
		navMeshDataURL = region->getCapability("ObjectNavMeshProperties");
		if (navMeshDataURL.empty())
		{
			navMeshDataURL = region->getCapability("ObjectNavmesh");
		}
	}

	return navMeshDataURL;
#else // XXX_STINSON_REGION_CAP_RENAME
	std::string navMeshDataURL("");

	LLViewerRegion* region = gAgent.getRegion();
	if (region != NULL)
	{
		navMeshDataURL = region->getCapability("ObjectNavMeshProperties");
	}

	return navMeshDataURL;
#endif // XXX_STINSON_REGION_CAP_RENAME
}

void LLFloaterPathfindingLinksets::setFetchState(EFetchState pFetchState)
{
	mFetchState = pFetchState;
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

		columns[4]["column"] = "path_state";
		switch (linkset.getPathState())
		{
		case PathfindingLinkset::kWalkable :
			columns[4]["value"] = getString("linkset_path_state_walkable");
			break;
		case PathfindingLinkset::kObstacle :
			columns[4]["value"] = getString("linkset_path_state_obstacle");
			break;
		case PathfindingLinkset::kIgnored :
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
		columns[6]["value"] = llformat("%3d", linkset.getA());
		columns[6]["font"] = "SANSSERIF";

		columns[7]["column"] = "b_percent";
		columns[7]["value"] = llformat("%3d", linkset.getB());
		columns[7]["font"] = "SANSSERIF";

		columns[8]["column"] = "c_percent";
		columns[8]["value"] = llformat("%3d", linkset.getC());
		columns[8]["font"] = "SANSSERIF";

		columns[9]["column"] = "d_percent";
		columns[9]["value"] = llformat("%3d", linkset.getD());
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

	switch (getFetchState())
	{
	case kFetchStarting :
		statusText = getString("linksets_fetching_starting");
		break;
	case kFetchRequestSent :
		statusText = getString("linksets_fetching_inprogress");
		break;
	case kFetchRequestSent_MultiRequested :
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

		const PathfindingLinksets::PathfindingLinksetMap &linksetsMap = mPathfindingLinksets.getAllLinksets();
		PathfindingLinksets::PathfindingLinksetMap::const_iterator linksetIter = linksetsMap.find(firstItem->getUUID().asString());
		const PathfindingLinkset &linkset(linksetIter->second);

		setPathState(linkset.getPathState());
		mEditA->setValue(LLSD(linkset.getA()));
		mEditB->setValue(LLSD(linkset.getB()));
		mEditC->setValue(LLSD(linkset.getC()));
		mEditD->setValue(LLSD(linkset.getD()));
		mEditPhantom->set(linkset.isPhantom());

		setEnableEditFields(true);
	}
}

void LLFloaterPathfindingLinksets::applyEditFields()
{
	std::vector<LLScrollListItem*> selectedItems = mLinksetsScrollList->getAllSelected();
	if (!selectedItems.empty())
	{
		PathfindingLinkset::EPathState pathState = getPathState();
		BOOL isPermanentBool = PathfindingLinkset::isPermanent(pathState);
		BOOL isWalkableBool = PathfindingLinkset::isWalkable(pathState);
		const std::string &aString = mEditA->getText();
		const std::string &bString = mEditB->getText();
		const std::string &cString = mEditC->getText();
		const std::string &dString = mEditD->getText();
		S32 aValue = static_cast<S32>(atoi(aString.c_str()));
		S32 bValue = static_cast<S32>(atoi(bString.c_str()));
		S32 cValue = static_cast<S32>(atoi(cString.c_str()));
		S32 dValue = static_cast<S32>(atoi(dString.c_str()));
		BOOL isPhantomBool = mEditPhantom->getValue();

		LLSD isPermanent = (bool)isPermanentBool;
		LLSD isWalkable = (bool)isWalkableBool;
		LLSD a = static_cast<F32>(aValue) / 100.0f;
		LLSD b = static_cast<F32>(bValue) / 100.0f;
		LLSD c = static_cast<F32>(cValue) / 100.0f;
		LLSD d = static_cast<F32>(dValue) / 100.0f;
		LLSD isPhantom = (bool)isPhantomBool;

		const PathfindingLinksets::PathfindingLinksetMap &linksetsMap = mPathfindingLinksets.getAllLinksets();

		LLSD editData;
		for (std::vector<LLScrollListItem*>::const_iterator itemIter = selectedItems.begin();
			itemIter != selectedItems.end(); ++itemIter)
		{
			const LLScrollListItem *listItem = *itemIter;
			LLUUID uuid = listItem->getUUID();

			const PathfindingLinksets::PathfindingLinksetMap::const_iterator linksetIter = linksetsMap.find(uuid.asString());
			const PathfindingLinkset &linkset = linksetIter->second;

			LLSD itemData;
			if (linkset.getPathState() != pathState)
			{
				itemData["permanent"] = isPermanent;
				itemData["walkable"] = isWalkable;
			}
			if (linkset.getA() != aValue)
			{
				itemData["A"] = a;
			}
			if (linkset.getB() != bValue)
			{
				itemData["B"] = b;
			}
			if (linkset.getC() != cValue)
			{
				itemData["C"] = c;
			}
			if (linkset.getD() != dValue)
			{
				itemData["D"] = d;
			}
			if (linkset.isPhantom() != isPhantomBool)
			{
				itemData["phantom"] = isPhantom;
			}

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

PathfindingLinkset::EPathState LLFloaterPathfindingLinksets::getPathState() const
{
	PathfindingLinkset::EPathState pathState;
	
	switch (mEditPathState->getValue().asInteger())
	{
	case XUI_PATH_STATE_WALKABLE :
		pathState = PathfindingLinkset::kWalkable;
		break;
	case XUI_PATH_STATE_OBSTACLE :
		pathState = PathfindingLinkset::kObstacle;
		break;
	case XUI_PATH_STATE_IGNORED :
		pathState = PathfindingLinkset::kIgnored;
		break;
	default :
		pathState = PathfindingLinkset::kIgnored;
		llassert(0);
		break;
	}

	return pathState;
}

void LLFloaterPathfindingLinksets::setPathState(PathfindingLinkset::EPathState pPathState)
{
	LLSD radioGroupValue;

	switch (pPathState)
	{
	case PathfindingLinkset::kWalkable :
		radioGroupValue = XUI_PATH_STATE_WALKABLE;
		break;
	case PathfindingLinkset::kObstacle :
		radioGroupValue = XUI_PATH_STATE_OBSTACLE;
		break;
	case PathfindingLinkset::kIgnored :
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
