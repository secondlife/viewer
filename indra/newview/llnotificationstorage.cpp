/**
* @file llnotificationstorage.cpp
* @brief LLPersistentNotificationStorage class implementation
*
* $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h" // must be first include

#include "llnotificationstorage.h"

#include <string>
#include <map>

#include "llerror.h"
#include "llfile.h"
#include "llnotifications.h"
#include "llpointer.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llsingleton.h"
#include "llviewermessage.h"


class LLResponderRegistry : public LLSingleton<LLResponderRegistry>
{
public:
	LLResponderRegistry();
	~LLResponderRegistry();
	
	LLNotificationResponderInterface* createResponder(const std::string& pNotificationName, const LLSD& pParams);
	
protected:
	
private:
	template<typename RESPONDER_TYPE> static LLNotificationResponderInterface* create(const LLSD& pParams);
	
	typedef boost::function<LLNotificationResponderInterface* (const LLSD& params)> responder_constructor_t;
	
	void add(const std::string& pNotificationName, const responder_constructor_t& pConstructor);
	
	typedef std::map<std::string, responder_constructor_t> build_map_t;
	build_map_t mBuildMap;
};

LLNotificationStorage::LLNotificationStorage(std::string pFileName)
	: mFileName(pFileName)
{
}

LLNotificationStorage::~LLNotificationStorage()
{
}

bool LLNotificationStorage::writeNotifications(const LLSD& pNotificationData) const
{

	llofstream notifyFile(mFileName.c_str());
	bool didFileOpen = notifyFile.is_open();

	if (!didFileOpen)
	{
		LL_WARNS("LLNotificationStorage") << "Failed to open file '" << mFileName << "'" << LL_ENDL;
	}
	else
	{
		LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();
		formatter->format(pNotificationData, notifyFile, LLSDFormatter::OPTIONS_PRETTY);
	}

	return didFileOpen;
}

bool LLNotificationStorage::readNotifications(LLSD& pNotificationData) const
{
	bool didFileRead;

	pNotificationData.clear();

	llifstream notifyFile(mFileName.c_str());
	didFileRead = notifyFile.is_open();
	if (!didFileRead)
	{
		LL_WARNS("LLNotificationStorage") << "Failed to open file '" << mFileName << "'" << LL_ENDL;
	}
	else
	{
		LLPointer<LLSDParser> parser = new LLSDXMLParser();
		didFileRead = (parser->parse(notifyFile, pNotificationData, LLSDSerialize::SIZE_UNLIMITED) >= 0);
		if (!didFileRead)
		{
			LL_WARNS("LLNotificationStorage") << "Failed to parse open notifications from file '" << mFileName 
				<< "'" << LL_ENDL;
		}
	}

	return didFileRead;
}

LLNotificationResponderInterface* LLNotificationStorage::createResponder(const std::string& pNotificationName, const LLSD& pParams) const
{
	return LLResponderRegistry::getInstance()->createResponder(pNotificationName, pParams);
}

LLResponderRegistry::LLResponderRegistry()
	: LLSingleton<LLResponderRegistry>()
	, mBuildMap()
{
	add("ObjectGiveItem", &create<LLOfferInfo>);
	add("OwnObjectGiveItem", &create<LLOfferInfo>);
	add("UserGiveItem", &create<LLOfferInfo>);

	add("TeleportOffered", &create<LLOfferInfo>);
	add("TeleportOffered_MaturityExceeded", &create<LLOfferInfo>);

	add("OfferFriendship", &create<LLOfferInfo>);
}

LLResponderRegistry::~LLResponderRegistry()
{
}

LLNotificationResponderInterface* LLResponderRegistry::createResponder(const std::string& pNotificationName, const LLSD& pParams)
{
	build_map_t::const_iterator it = mBuildMap.find(pNotificationName);
	if(mBuildMap.end() == it)
	{
		return NULL;
	}
	responder_constructor_t ctr = it->second;
	return ctr(pParams);
}

template<typename RESPONDER_TYPE> LLNotificationResponderInterface* LLResponderRegistry::create(const LLSD& pParams)
{
	RESPONDER_TYPE* responder = new RESPONDER_TYPE();
	responder->fromLLSD(pParams);
	return responder;
}
	
void LLResponderRegistry::add(const std::string& pNotificationName, const responder_constructor_t& pConstructor)
{
	if (mBuildMap.find(pNotificationName) != mBuildMap.end())
	{
		LL_ERRS("LLResponderRegistry") << "Responder is already registered : " << pNotificationName << LL_ENDL;
	}
	mBuildMap.insert(std::make_pair<std::string, responder_constructor_t>(pNotificationName, pConstructor));
}
