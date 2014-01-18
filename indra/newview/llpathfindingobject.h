/** 
* @file   llpathfindingobject.h
* @brief  Header file for llpathfindingobject
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
#ifndef LL_LLPATHFINDINGOBJECT_H
#define LL_LLPATHFINDINGOBJECT_H

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>

#include "llavatarname.h"
#include "llavatarnamecache.h"
#include "lluuid.h"
#include "v3math.h"

class LLPathfindingObject;
class LLSD;

typedef boost::shared_ptr<LLPathfindingObject> LLPathfindingObjectPtr;

class LLPathfindingObject
{
public:
	LLPathfindingObject();
	LLPathfindingObject(const std::string &pUUID, const LLSD &pObjectData);
	LLPathfindingObject(const LLPathfindingObject& pOther);
	virtual ~LLPathfindingObject();

	LLPathfindingObject& operator =(const LLPathfindingObject& pOther);

	inline const LLUUID&      getUUID() const        {return mUUID;};
	inline const std::string& getName() const        {return mName;};
	inline const std::string& getDescription() const {return mDescription;};
	inline BOOL               hasOwner() const       {return mOwnerUUID.notNull();};
	inline bool               hasOwnerName() const   {return mHasOwnerName;};
	std::string               getOwnerName() const;
	inline BOOL               isGroupOwned() const   {return mIsGroupOwned;};
	inline const LLVector3&   getLocation() const    {return mLocation;};

	typedef boost::function<void (const LLPathfindingObject *)>         name_callback_t;
	typedef boost::signals2::signal<void (const LLPathfindingObject *)> name_signal_t;
	typedef boost::signals2::connection                                 name_connection_t;

	name_connection_t registerOwnerNameListener(name_callback_t pOwnerNameCallback);

protected:

private:
	void parseObjectData(const LLSD &pObjectData);

	void fetchOwnerName();
	void handleAvatarNameFetch(const LLUUID &pOwnerUUID, const LLAvatarName &pAvatarName);
	void disconnectAvatarNameCacheConnection();

	LLUUID                                   mUUID;
	std::string                              mName;
	std::string                              mDescription;
	LLUUID                                   mOwnerUUID;
	bool                                     mHasOwnerName;
	LLAvatarName                             mOwnerName;
	LLAvatarNameCache::callback_connection_t mAvatarNameCacheConnection;
	BOOL                                     mIsGroupOwned;
	LLVector3                                mLocation;
	name_signal_t                            mOwnerNameSignal;
};

#endif // LL_LLPATHFINDINGOBJECT_H
