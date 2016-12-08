/** 
 * @file llfloaterperms.cpp
 * @brief Asset creation permission preferences.
 * @author Jonathan Yap
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

#include "llviewerprecompiledheaders.h"
#include "llcheckboxctrl.h"
#include "llfloaterperms.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "lluictrlfactory.h"
#include "llpermissions.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "llnotificationsutil.h"
#include "llsdserialize.h"
#include "llvoavatar.h"
#include "llcorehttputil.h"
#include "lleventfilter.h"
#include "lleventcoro.h"

LLFloaterPerms::LLFloaterPerms(const LLSD& seed)
: LLFloater(seed)
{
}

BOOL LLFloaterPerms::postBuild()
{
	return TRUE;
}

//static 
U32 LLFloaterPerms::getGroupPerms(std::string prefix)
{	
	return gSavedSettings.getBOOL(prefix+"ShareWithGroup") ? PERM_COPY | PERM_MOVE | PERM_MODIFY : PERM_NONE;
}

//static 
U32 LLFloaterPerms::getEveryonePerms(std::string prefix)
{
	return gSavedSettings.getBOOL(prefix+"EveryoneCopy") ? PERM_COPY : PERM_NONE;
}

//static 
U32 LLFloaterPerms::getNextOwnerPerms(std::string prefix)
{
	U32 flags = PERM_MOVE;
	if ( gSavedSettings.getBOOL(prefix+"NextOwnerCopy") )
	{
		flags |= PERM_COPY;
	}
	if ( gSavedSettings.getBOOL(prefix+"NextOwnerModify") )
	{
		flags |= PERM_MODIFY;
	}
	if ( gSavedSettings.getBOOL(prefix+"NextOwnerTransfer") )
	{
		flags |= PERM_TRANSFER;
	}
	return flags;
}

//static 
U32 LLFloaterPerms::getNextOwnerPermsInverted(std::string prefix)
{
	// Sets bits for permissions that are off
	U32 flags = PERM_MOVE;
	if ( !gSavedSettings.getBOOL(prefix+"NextOwnerCopy") )
	{
		flags |= PERM_COPY;
	}
	if ( !gSavedSettings.getBOOL(prefix+"NextOwnerModify") )
	{
		flags |= PERM_MODIFY;
	}
	if ( !gSavedSettings.getBOOL(prefix+"NextOwnerTransfer") )
	{
		flags |= PERM_TRANSFER;
	}
	return flags;
}

static bool mCapSent = false;

LLFloaterPermsDefault::LLFloaterPermsDefault(const LLSD& seed)
	: LLFloater(seed)
{
	mCommitCallbackRegistrar.add("PermsDefault.Copy", boost::bind(&LLFloaterPermsDefault::onCommitCopy, this, _2));
	mCommitCallbackRegistrar.add("PermsDefault.OK", boost::bind(&LLFloaterPermsDefault::onClickOK, this));
	mCommitCallbackRegistrar.add("PermsDefault.Cancel", boost::bind(&LLFloaterPermsDefault::onClickCancel, this));
}

 
// String equivalents of enum Categories - initialization order must match enum order!
const std::string LLFloaterPermsDefault::sCategoryNames[CAT_LAST] =
{
	"Objects",
	"Uploads",
	"Scripts",
	"Notecards",
	"Gestures",
	"Wearables"
};

BOOL LLFloaterPermsDefault::postBuild()
{
	if(!gSavedSettings.getBOOL("DefaultUploadPermissionsConverted"))
	{
		gSavedSettings.setBOOL("UploadsEveryoneCopy", gSavedSettings.getBOOL("EveryoneCopy"));
		gSavedSettings.setBOOL("UploadsNextOwnerCopy", gSavedSettings.getBOOL("NextOwnerCopy"));
		gSavedSettings.setBOOL("UploadsNextOwnerModify", gSavedSettings.getBOOL("NextOwnerModify"));
		gSavedSettings.setBOOL("UploadsNextOwnerTransfer", gSavedSettings.getBOOL("NextOwnerTransfer"));
		gSavedSettings.setBOOL("UploadsShareWithGroup", gSavedSettings.getBOOL("ShareWithGroup"));
		gSavedSettings.setBOOL("DefaultUploadPermissionsConverted", true);
	}

	mCloseSignal.connect(boost::bind(&LLFloaterPermsDefault::cancel, this));

	refresh();
	
	return true;
}

void LLFloaterPermsDefault::onClickOK()
{
	ok();
	closeFloater();
}

void LLFloaterPermsDefault::onClickCancel()
{
	cancel();
	closeFloater();
}

void LLFloaterPermsDefault::onCommitCopy(const LLSD& user_data)
{
	// Implements fair use
	std::string prefix = user_data.asString();

	BOOL copyable = gSavedSettings.getBOOL(prefix+"NextOwnerCopy");
	if(!copyable)
	{
		gSavedSettings.setBOOL(prefix+"NextOwnerTransfer", TRUE);
	}
	LLCheckBoxCtrl* xfer = getChild<LLCheckBoxCtrl>(prefix+"_transfer");
	xfer->setEnabled(copyable);
}

const int MAX_HTTP_RETRIES = 5;
const float RETRY_TIMEOUT = 5.0;

void LLFloaterPermsDefault::sendInitialPerms()
{
	if(!mCapSent)
	{
		updateCap();
		setCapSent(true);
	}
}

void LLFloaterPermsDefault::updateCap()
{
    if (!gAgent.getRegion())
    {
        LL_WARNS("Avatar") << "Region not set, cannot request capability update" << LL_ENDL;
        return;
    }
	std::string object_url = gAgent.getRegion()->getCapability("AgentPreferences");

	if(!object_url.empty())
	{
        LLCoros::instance().launch("LLFloaterPermsDefault::updateCapCoro",
            boost::bind(&LLFloaterPermsDefault::updateCapCoro, object_url));
	}
    else
    {
        LL_DEBUGS("ObjectPermissionsFloater") << "AgentPreferences cap not available." << LL_ENDL;
    }
}

/*static*/
void LLFloaterPermsDefault::updateCapCoro(std::string url)
{
    int retryCount = 0;
    std::string previousReason;
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericPostCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD postData = LLSD::emptyMap();
    postData["default_object_perm_masks"]["Group"] =
        (LLSD::Integer)LLFloaterPerms::getGroupPerms(sCategoryNames[CAT_OBJECTS]);
    postData["default_object_perm_masks"]["Everyone"] =
        (LLSD::Integer)LLFloaterPerms::getEveryonePerms(sCategoryNames[CAT_OBJECTS]);
    postData["default_object_perm_masks"]["NextOwner"] =
        (LLSD::Integer)LLFloaterPerms::getNextOwnerPerms(sCategoryNames[CAT_OBJECTS]);

    {
        LL_DEBUGS("ObjectPermissionsFloater") << "Sending default permissions to '"
            << url << "'\n";
        std::ostringstream sent_perms_log;
        LLSDSerialize::toPrettyXML(postData, sent_perms_log);
        LL_CONT << sent_perms_log.str() << LL_ENDL;
    }

    while (true)
    {
        ++retryCount;
        LLSD result = httpAdapter->postAndSuspend(httpRequest, url, postData);

        LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
        LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

        if (!status)
        {
            const std::string& reason = status.toString();
            // Do not display the same error more than once in a row
            if (reason != previousReason)
            {
                previousReason = reason;
                LLSD args;
                args["REASON"] = reason;
                LLNotificationsUtil::add("DefaultObjectPermissions", args);
            }

            llcoro::suspendUntilTimeout(RETRY_TIMEOUT);
            if (retryCount < MAX_HTTP_RETRIES)
                continue;

            LL_WARNS("ObjectPermissionsFloater") << "Unable to send default permissions.  Giving up for now." << LL_ENDL;
            return;
        }
        break;
    }

    // Since we have had a successful POST call be sure to display the next error message
    // even if it is the same as a previous one.
    previousReason.clear();
    LLFloaterPermsDefault::setCapSent(true);
    LL_INFOS("ObjectPermissionsFloater") << "Default permissions successfully sent to simulator" << LL_ENDL;
}

void LLFloaterPermsDefault::setCapSent(bool cap_sent)
{
	mCapSent = cap_sent;
}

void LLFloaterPermsDefault::ok()
{
//	Changes were already applied automatically to saved settings.
//	Refreshing internal values makes it official.
	refresh();

// We know some setting has changed but not which one.  Just in case it was a setting for
// object permissions tell the server what the values are.
	updateCap();
}

void LLFloaterPermsDefault::cancel()
{
	for (U32 iter = CAT_OBJECTS; iter < CAT_LAST; iter++)
	{
		gSavedSettings.setBOOL(sCategoryNames[iter]+"NextOwnerCopy",		mNextOwnerCopy[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"NextOwnerModify",		mNextOwnerModify[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"NextOwnerTransfer",	mNextOwnerTransfer[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"ShareWithGroup",		mShareWithGroup[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"EveryoneCopy",			mEveryoneCopy[iter]);
	}
}

void LLFloaterPermsDefault::refresh()
{
	for (U32 iter = CAT_OBJECTS; iter < CAT_LAST; iter++)
	{
		mShareWithGroup[iter]    = gSavedSettings.getBOOL(sCategoryNames[iter]+"ShareWithGroup");
		mEveryoneCopy[iter]      = gSavedSettings.getBOOL(sCategoryNames[iter]+"EveryoneCopy");
		mNextOwnerCopy[iter]     = gSavedSettings.getBOOL(sCategoryNames[iter]+"NextOwnerCopy");
		mNextOwnerModify[iter]   = gSavedSettings.getBOOL(sCategoryNames[iter]+"NextOwnerModify");
		mNextOwnerTransfer[iter] = gSavedSettings.getBOOL(sCategoryNames[iter]+"NextOwnerTransfer");
	}
}
