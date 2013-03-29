/** 
* @file llpathfindingnavmesh.cpp
* @brief Implementation of llpathfindingnavmesh
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

#include "llpathfindingnavmesh.h"

#include <string>

#include "llpathfindingnavmeshstatus.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "lluuid.h"

#define NAVMESH_VERSION_FIELD "navmesh_version"
#define NAVMESH_DATA_FIELD    "navmesh_data"

//---------------------------------------------------------------------------
// LLPathfindingNavMesh
//---------------------------------------------------------------------------

LLPathfindingNavMesh::LLPathfindingNavMesh(const LLUUID &pRegionUUID)
	: mNavMeshStatus(pRegionUUID),
	mNavMeshRequestStatus(kNavMeshRequestUnknown),
	mNavMeshSignal(),
	mNavMeshData()

{
}

LLPathfindingNavMesh::~LLPathfindingNavMesh()
{
}

LLPathfindingNavMesh::navmesh_slot_t LLPathfindingNavMesh::registerNavMeshListener(navmesh_callback_t pNavMeshCallback)
{
	return mNavMeshSignal.connect(pNavMeshCallback);
}

bool LLPathfindingNavMesh::hasNavMeshVersion(const LLPathfindingNavMeshStatus &pNavMeshStatus) const
{
	return ((mNavMeshStatus.getVersion() == pNavMeshStatus.getVersion()) &&
		((mNavMeshRequestStatus == kNavMeshRequestStarted) || (mNavMeshRequestStatus == kNavMeshRequestCompleted) ||
		((mNavMeshRequestStatus == kNavMeshRequestChecking) && !mNavMeshData.empty())));
}

void LLPathfindingNavMesh::handleNavMeshWaitForRegionLoad()
{
	setRequestStatus(kNavMeshRequestWaiting);
}

void LLPathfindingNavMesh::handleNavMeshCheckVersion()
{
	setRequestStatus(kNavMeshRequestChecking);
}

void LLPathfindingNavMesh::handleRefresh(const LLPathfindingNavMeshStatus &pNavMeshStatus)
{
	llassert(mNavMeshStatus.getRegionUUID() == pNavMeshStatus.getRegionUUID());
	llassert(mNavMeshStatus.getVersion() == pNavMeshStatus.getVersion());
	mNavMeshStatus = pNavMeshStatus;
	if (mNavMeshRequestStatus == kNavMeshRequestChecking)
	{
		llassert(!mNavMeshData.empty());
		setRequestStatus(kNavMeshRequestCompleted);
	}
	else
	{
		sendStatus();
	}
}

void LLPathfindingNavMesh::handleNavMeshNewVersion(const LLPathfindingNavMeshStatus &pNavMeshStatus)
{
	llassert(mNavMeshStatus.getRegionUUID() == pNavMeshStatus.getRegionUUID());
	if (mNavMeshStatus.getVersion() == pNavMeshStatus.getVersion())
	{
		mNavMeshStatus = pNavMeshStatus;
		sendStatus();
	}
	else
	{
		mNavMeshData.clear();
		mNavMeshStatus = pNavMeshStatus;
		setRequestStatus(kNavMeshRequestNeedsUpdate);
	}
}

void LLPathfindingNavMesh::handleNavMeshStart(const LLPathfindingNavMeshStatus &pNavMeshStatus)
{
	llassert(mNavMeshStatus.getRegionUUID() == pNavMeshStatus.getRegionUUID());
	mNavMeshStatus = pNavMeshStatus;
	setRequestStatus(kNavMeshRequestStarted);
}

void LLPathfindingNavMesh::handleNavMeshResult(const LLSD &pContent, U32 pNavMeshVersion)
{
	llassert(pContent.has(NAVMESH_VERSION_FIELD));
	if (pContent.has(NAVMESH_VERSION_FIELD))
	{
		llassert(pContent.get(NAVMESH_VERSION_FIELD).isInteger());
		llassert(pContent.get(NAVMESH_VERSION_FIELD).asInteger() >= 0);
		U32 embeddedNavMeshVersion = static_cast<U32>(pContent.get(NAVMESH_VERSION_FIELD).asInteger());
		llassert(embeddedNavMeshVersion == pNavMeshVersion); // stinson 03/13/2012 : does this ever occur?
		if (embeddedNavMeshVersion != pNavMeshVersion)
		{
			llwarns << "Mismatch between expected and embedded navmesh versions occurred" << llendl;
			pNavMeshVersion = embeddedNavMeshVersion;
		}
	}

	if (mNavMeshStatus.getVersion() == pNavMeshVersion)
	{
		ENavMeshRequestStatus status;
		if ( pContent.has(NAVMESH_DATA_FIELD) )
		{
			const LLSD::Binary &value = pContent.get(NAVMESH_DATA_FIELD).asBinary();
			unsigned int binSize = value.size();
			std::string newStr(reinterpret_cast<const char *>(&value[0]), binSize);
			std::istringstream streamdecomp( newStr );
			unsigned int decompBinSize = 0;
			bool valid = false;
			U8* pUncompressedNavMeshContainer = unzip_llsdNavMesh( valid, decompBinSize, streamdecomp, binSize ) ;
			if ( !valid )
			{
				llwarns << "Unable to decompress the navmesh llsd." << llendl;
				status = kNavMeshRequestError;
			}
			else
			{
				llassert(pUncompressedNavMeshContainer);
				mNavMeshData.resize( decompBinSize );
				memcpy( &mNavMeshData[0], &pUncompressedNavMeshContainer[0], decompBinSize );
				status = kNavMeshRequestCompleted;
			}
			if ( pUncompressedNavMeshContainer )
			{
				free( pUncompressedNavMeshContainer );
			}
		}
		else
		{
			llwarns << "No mesh data received" << llendl;
			status = kNavMeshRequestError;
		}
		setRequestStatus(status);
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
	if (mNavMeshStatus.getVersion() == pNavMeshVersion)
	{
		handleNavMeshError();
	}
}

void LLPathfindingNavMesh::setRequestStatus(ENavMeshRequestStatus pNavMeshRequestStatus)
{
	mNavMeshRequestStatus = pNavMeshRequestStatus;
	sendStatus();
}

void LLPathfindingNavMesh::sendStatus()
{
	mNavMeshSignal(mNavMeshRequestStatus, mNavMeshStatus, mNavMeshData);
}
