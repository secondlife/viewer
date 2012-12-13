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

#include "llerror.h"
#include "llfile.h"
#include "llpointer.h"
#include "llsd.h"
#include "llsdserialize.h"


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
