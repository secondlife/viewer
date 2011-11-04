/** 
 * @file llnavmeshstation.h
 * @brief Client-side navmesh support
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_NAV_MESH_STATION_H
#define LL_NAV_MESH_STATION_H

//===============================================================================
#include "llhandle.h"
//===============================================================================
class LLCurlRequest;
//===============================================================================
class LLNavMeshObserver
{
public:
	//Ctor
	LLNavMeshObserver() { mObserverHandle.bind(this); }
	//Dtor
	virtual ~LLNavMeshObserver() {}
	//Accessor for the observers handle
	const LLHandle<LLNavMeshObserver>& getObserverHandle() const { return mObserverHandle; }

protected:
	LLRootHandle<LLNavMeshObserver> mObserverHandle;	
};
//===============================================================================
class LLNavMeshStation : public LLSingleton<LLNavMeshStation>
{
public:
	//Ctor
	LLNavMeshStation();
	//Facilitates the posting of a prepopulated llsd block to an existing url
	bool postNavMeshToServer( LLSD& data, const LLHandle<LLNavMeshObserver>& observerHandle );
	//Setter for the navmesh upload url
	void setNavMeshUploadURL( std::string& url ) { mNavMeshUploadURL = url; }

protected:	
	//Curl object to facilitate posts to server
	LLCurlRequest*	mCurlRequest;
	//Maximum time in seconds to execute an uploading request.
	S32				mMeshUploadTimeOut ; 
	//URL used for uploading viewer generated navmesh
	std::string		mNavMeshUploadURL;

};
//===============================================================================
#endif LL_NAV_MESH_STATION_H