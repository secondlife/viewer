/** 
 * @file llpathfindingnavmesh.cpp
 * @author William Todd Stinson
 * @brief A class for representing the navmesh of a pathfinding region.
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
#include "lluuid.h"
#include "llpathfindingnavmesh.h"
#include "llsdserialize.h"

#include <string>

//---------------------------------------------------------------------------
// LLPathfindingNavMesh
//---------------------------------------------------------------------------

LLPathfindingNavMesh::LLPathfindingNavMesh(const LLUUID &pRegionUUID)
	: mRegionUUID(pRegionUUID),
	mNavMeshRequestStatus(kNavMeshRequestUnknown),
	mNavMeshSignal(),
	mNavMeshData(),
	mNavMeshVersion(0U)
{
}

LLPathfindingNavMesh::~LLPathfindingNavMesh()
{
}

LLPathfindingNavMesh::navmesh_slot_t LLPathfindingNavMesh::registerNavMeshListener(navmesh_callback_t pNavMeshCallback)
{
	return mNavMeshSignal.connect(pNavMeshCallback);
}

bool LLPathfindingNavMesh::hasNavMeshVersion(U32 pNavMeshVersion) const
{
	return ((mNavMeshVersion == pNavMeshVersion) && 
		((mNavMeshRequestStatus == kNavMeshRequestStarted) || (mNavMeshRequestStatus == kNavMeshRequestCompleted) ||
		((mNavMeshRequestStatus == kNavMeshRequestChecking) && !mNavMeshData.empty())));
}

void LLPathfindingNavMesh::handleRefresh(U32 pNavMeshVersion)
{
	llassert(pNavMeshVersion == mNavMeshVersion);
	if (mNavMeshRequestStatus == kNavMeshRequestChecking)
	{
		llassert(!mNavMeshData.empty());
		setRequestStatus(kNavMeshRequestCompleted);
	}
	else
	{
		mNavMeshSignal(mNavMeshRequestStatus, mRegionUUID, mNavMeshVersion, mNavMeshData);
	}
}

void LLPathfindingNavMesh::handleNavMeshCheckVersion()
{
	setRequestStatus(kNavMeshRequestChecking);
}

void LLPathfindingNavMesh::handleNavMeshNewVersion(U32 pNavMeshVersion)
{
	if (mNavMeshVersion != pNavMeshVersion)
	{
		mNavMeshData.clear();
		mNavMeshVersion = pNavMeshVersion;
		setRequestStatus(kNavMeshRequestNeedsUpdate);
	}
}

void LLPathfindingNavMesh::handleNavMeshStart(U32 pNavMeshVersion)
{
	mNavMeshVersion = pNavMeshVersion;
	setRequestStatus(kNavMeshRequestStarted);
}

void LLPathfindingNavMesh::handleNavMeshResult(const LLSD &pContent, U32 pNavMeshVersion)
{
	if (mNavMeshVersion == pNavMeshVersion)
	{
		if ( pContent.has("navmesh_data") )
		{
			const LLSD::Binary &value = pContent["navmesh_data"].asBinary();
			unsigned int binSize = value.size();
			std::string newStr(reinterpret_cast<const char *>(&value[0]), binSize);
			std::istringstream streamdecomp( newStr );                 
			unsigned int decompBinSize = 0;
			bool valid = false;
			U8* pUncompressedNavMeshContainer = unzip_llsdNavMesh( valid, decompBinSize, streamdecomp, binSize ) ;
			if ( !valid )
			{
				llwarns << "Unable to decompress the navmesh llsd." << llendl;
				setRequestStatus(kNavMeshRequestError);
			}
			else
			{
				llassert(pUncompressedNavMeshContainer);
				mNavMeshData.resize( decompBinSize );
				memcpy( &mNavMeshData[0], &pUncompressedNavMeshContainer[0], decompBinSize );
				setRequestStatus(kNavMeshRequestCompleted);
			}					
			if ( pUncompressedNavMeshContainer )
			{
				free( pUncompressedNavMeshContainer );
			}
		}
		else
		{
			llwarns << "No mesh data received" << llendl;
			setRequestStatus(kNavMeshRequestError);
		}
	}
}

void LLPathfindingNavMesh::handleNavMeshNotEnabled()
{
	mNavMeshData.clear();
	setRequestStatus(kNavMeshRequestNotEnabled);
}

void LLPathfindingNavMesh::handleNavMeshError()
{
	mNavMeshData.clear();
	setRequestStatus(kNavMeshRequestError);
}

void LLPathfindingNavMesh::handleNavMeshError(U32 pStatus, const std::string &pReason, const std::string &pURL, U32 pNavMeshVersion)
{
	llwarns << "error with request to URL '" << pURL << "' because " << pReason << " (statusCode:" << pStatus << ")" << llendl;
	if (mNavMeshVersion == pNavMeshVersion)
	{
		handleNavMeshError();
	}
}

void LLPathfindingNavMesh::setRequestStatus(ENavMeshRequestStatus pNavMeshRequestStatus)
{
	mNavMeshRequestStatus = pNavMeshRequestStatus;
	mNavMeshSignal(mNavMeshRequestStatus, mRegionUUID, mNavMeshVersion, mNavMeshData);
}
