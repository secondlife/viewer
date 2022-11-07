/**
* @file llnotificationstorage.h
* @brief LLNotificationStorage class declaration
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

#ifndef LL_NOTIFICATIONSTORAGE_H
#define LL_NOTIFICATIONSTORAGE_H

#include <string>

#include "llerror.h"

class LLNotificationResponderInterface;
class LLSD;

class LLNotificationStorage
{
    LOG_CLASS(LLNotificationStorage);
public:
    LLNotificationStorage(std::string pFileName);
    ~LLNotificationStorage();

protected:
    bool writeNotifications(const LLSD& pNotificationData) const;
    bool readNotifications(LLSD& pNotificationData, bool is_new_filename = true) const;
    void setFileName(std::string pFileName) {mFileName = pFileName;}
    void setOldFileName(std::string pFileName) {mOldFileName = pFileName;}

    LLNotificationResponderInterface* createResponder(const std::string& pNotificationName, const LLSD& pParams) const;

private:
    std::string mFileName;
    std::string mOldFileName;
};

#endif // LL_NOTIFICATIONSTORAGE_H
