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
#include "llregistry.h"
#include "llviewermessage.h" 

typedef boost::function<LLNotificationResponderInterface * (const LLSD& pParams)> responder_constructor_t;

class LLResponderRegistry : public LLRegistrySingleton<std::string, responder_constructor_t, LLResponderRegistry>
{
    LLSINGLETON_EMPTY_CTOR(LLResponderRegistry);
public:
    template<typename RESPONDER_TYPE> static LLNotificationResponderInterface * create(const LLSD& pParams);
    LLNotificationResponderInterface * createResponder(const std::string& pNotificationName, const LLSD& pParams);
};

template<typename RESPONDER_TYPE> LLNotificationResponderInterface * LLResponderRegistry::create(const LLSD& pParams)
{
    RESPONDER_TYPE* responder = new RESPONDER_TYPE();
    responder->fromLLSD(pParams);
    return responder;
}


LLNotificationResponderInterface * LLResponderRegistry::createResponder(const std::string& pNotificationName, const LLSD& pParams)
{
    responder_constructor_t * factoryFunc = (LLResponderRegistry::getValue(pNotificationName));

    if(factoryFunc)
    {
        return (*factoryFunc)(pParams);
    }
    
    return NULL;
}

LLResponderRegistry::StaticRegistrar sRegisterObjectGiveItem("ObjectGiveItem", &LLResponderRegistry::create<LLOfferInfo>);
LLResponderRegistry::StaticRegistrar sRegisterUserGiveItem("UserGiveItem", &LLResponderRegistry::create<LLOfferInfo>);
LLResponderRegistry::StaticRegistrar sRegisterOfferInfo("offer_info", &LLResponderRegistry::create<LLOfferInfo>);


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

bool LLNotificationStorage::readNotifications(LLSD& pNotificationData, bool is_new_filename) const
{
    std::string filename = is_new_filename? mFileName : mOldFileName;

    LL_INFOS("LLNotificationStorage") << "starting read '" << filename << "'" << LL_ENDL;

    bool didFileRead;

    pNotificationData.clear();

    llifstream notifyFile(filename.c_str());
    didFileRead = notifyFile.is_open();
    if (!didFileRead)
    {
        LL_WARNS("LLNotificationStorage") << "Failed to open file '" << filename << "'" << LL_ENDL;
    }
    else
    {
        LLPointer<LLSDParser> parser = new LLSDXMLParser();
        didFileRead = (parser->parse(notifyFile, pNotificationData, LLSDSerialize::SIZE_UNLIMITED) >= 0);
        notifyFile.close();

        if (!didFileRead)
        {
            LL_WARNS("LLNotificationStorage") << "Failed to parse open notifications from file '" << mFileName 
                                              << "'" << LL_ENDL;
            LLFile::remove(filename);
            LL_WARNS("LLNotificationStorage") << "Removed invalid open notifications file '" << mFileName 
                                              << "'" << LL_ENDL;
        }
    }
    
    if (!didFileRead)
    {
        if(is_new_filename)
        {
            didFileRead = readNotifications(pNotificationData, false);
            if(didFileRead)
            {
                writeNotifications(pNotificationData);
                LLFile::remove(mOldFileName);
            }
        }
    }

    return didFileRead;
}

LLNotificationResponderInterface * LLNotificationStorage::createResponder(const std::string& pNotificationName, const LLSD& pParams) const
{
    return LLResponderRegistry::getInstance()->createResponder(pNotificationName, pParams);
}
