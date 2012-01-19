/** 
 * @file llfloaterpathfindinglinksets.h
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

#ifndef LL_LLFLOATERPATHFINDINGLINKSETS_H
#define LL_LLFLOATERPATHFINDINGLINKSETS_H

#include "llsd.h"
#include "v3math.h"
#include "llfloater.h"
#include "lluuid.h"

class LLTextBase;
class LLScrollListCtrl;
class LLLineEditor;
class LLCheckBoxCtrl;
class LLButton;

class PathfindingLinkset
{
public:
	PathfindingLinkset(const std::string &pUUID, const LLSD &pNavMeshItem);
	PathfindingLinkset(const PathfindingLinkset& pOther);
	virtual ~PathfindingLinkset();

	PathfindingLinkset& operator = (const PathfindingLinkset& pOther);

	const LLUUID&      getUUID() const;
	const std::string& getName() const;
	const std::string& getDescription() const;
	U32                getLandImpact() const;
	const LLVector3&   getPositionAgent() const;

	BOOL               isFixed() const;
	void               setFixed(BOOL pIsFixed);

	BOOL               isWalkable() const;
	void               setWalkable(BOOL pIsWalkable);

	BOOL               isPhantom() const;
	void               setPhantom(BOOL pIsPhantom);

	S32                getA() const;
	void               setA(S32 pA);

	S32                getB() const;
	void               setB(S32 pB);

	S32                getC() const;
	void               setC(S32 pC);

	S32                getD() const;
	void               setD(S32 pD);

protected:

private:
	LLUUID      mUUID;
	std::string mName;
	std::string mDescription;
	U32         mLandImpact;
	LLVector3   mLocation;
	BOOL        mIsFixed;
	BOOL        mIsWalkable;
	BOOL        mIsPhantom;
	S32         mA;
	S32         mB;
	S32         mC;
	S32         mD;
};

class FilterString
{
public:
	FilterString();
	FilterString(const std::string& pFilter);
	FilterString(const FilterString& pOther);
	virtual ~FilterString();

	const std::string& get() const;
	bool               set(const std::string& pFilter);
	void               clear();

	bool isActive() const;
	bool doesMatch(const std::string& pTestString) const;

protected:

private:
	std::string mFilter;
	std::string mUpperFilter;
};

class PathfindingLinksets
{
public:
	typedef std::map<std::string, PathfindingLinkset> PathfindingLinksetMap;

	PathfindingLinksets();
	PathfindingLinksets(const LLSD& pNavMeshData);
	PathfindingLinksets(const PathfindingLinksets& pOther);
	virtual ~PathfindingLinksets();

	void setNavMeshData(const LLSD& pNavMeshData);
	void updateNavMeshData(const LLSD& pNavMeshData);
	void clearLinksets();

	const PathfindingLinksetMap& getAllLinksets() const;
	const PathfindingLinksetMap& getFilteredLinksets();

	BOOL               isFiltersActive() const;
	void               setNameFilter(const std::string& pNameFilter);
	const std::string& getNameFilter() const;
	void               setDescriptionFilter(const std::string& pDescriptionFilter);
	const std::string& getDescriptionFilter() const;
	void               setFixedFilter(BOOL pFixedFilter);
	BOOL               isFixedFilter() const;
	void               setWalkableFilter(BOOL pWalkableFilter);
	BOOL               isWalkableFilter() const;
	void               clearFilters();

protected:

private:
	PathfindingLinksetMap mAllLinksets;
	PathfindingLinksetMap mFilteredLinksets;

	bool         mIsFiltersDirty;
	FilterString mNameFilter;
	FilterString mDescriptionFilter;
	BOOL         mIsFixedFilter;
	BOOL         mIsWalkableFilter;

	void applyFilters();
	BOOL doesMatchFilters(const PathfindingLinkset& pLinkset) const;
};

class LLFloaterPathfindingLinksets
:	public LLFloater
{
	friend class LLFloaterReg;
	friend class NavMeshDataGetResponder;
	friend class NavMeshDataPutResponder;

public:
	typedef enum
	{
		kFetchInitial,
		kFetchStarting,
		kFetchRequestSent,
		kFetchRequestSent_MultiRequested,
		kFetchReceived,
		kFetchError,
		kFetchComplete
	} EFetchState;

	virtual BOOL postBuild();
	virtual void onOpen(const LLSD& pKey);

	static void openLinksetsEditor();

	EFetchState getFetchState() const;
	BOOL        isFetchInProgress() const;

protected:

private:
	PathfindingLinksets mPathfindingLinksets;
	EFetchState         mFetchState;
	LLScrollListCtrl    *mLinksetsScrollList;
	LLTextBase          *mLinksetsStatus;
	LLLineEditor        *mFilterByName;
	LLLineEditor        *mFilterByDescription;
	LLCheckBoxCtrl      *mFilterByFixed;
	LLCheckBoxCtrl      *mFilterByWalkable;
	LLCheckBoxCtrl      *mEditFixed;
	LLCheckBoxCtrl      *mEditWalkable;
	LLCheckBoxCtrl      *mEditPhantom;
	LLTextBase          *mLabelWalkabilityCoefficients;
	LLTextBase          *mLabelEditA;
	LLTextBase          *mLabelEditB;
	LLTextBase          *mLabelEditC;
	LLTextBase          *mLabelEditD;
	LLLineEditor        *mEditA;
	LLLineEditor        *mEditB;
	LLLineEditor        *mEditC;
	LLLineEditor        *mEditD;
	LLButton            *mApplyEdits;

	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterPathfindingLinksets(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingLinksets();

	void sendNavMeshDataGetRequest();
	void sendNavMeshDataPutRequest(const LLSD& pPostData);
	void handleNavMeshDataGetReply(const LLSD& pNavMeshData);
	void handleNavMeshDataGetError(const std::string& pURL, const std::string& pErrorReason);
	void handleNavMeshDataPutReply(const LLSD& pModifiedData);
	void handleNavMeshDataPutError(const std::string& pURL, const std::string& pErrorReason);

	std::string getRegionName() const;
	std::string getCapabilityURL() const;

	void setFetchState(EFetchState pFetchState);

	void onApplyFiltersClicked();
	void onClearFiltersClicked();
	void onLinksetsSelectionChange();
	void onRefreshLinksetsClicked();
	void onSelectAllLinksetsClicked();
	void onSelectNoneLinksetsClicked();
	void onApplyChangesClicked();

	void applyFilters();
	void clearFilters();

	void updateLinksetsList();
	void selectAllLinksets();
	void selectNoneLinksets();

	void updateLinksetsStatusMessage();

	void updateEditFields();
	void applyEditFields();
	void setEnableEditFields(BOOL pEnabled);
};

#endif // LL_LLFLOATERPATHFINDINGLINKSETS_H
