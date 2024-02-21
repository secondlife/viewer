/** 
 * @file llfloaterreporter.h
 * @author Andrew Meadows
 * @brief Abuse reports.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLFLOATERREPORTER_H
#define LL_LLFLOATERREPORTER_H

#include "llfloater.h"
#include "lluuid.h"
#include "v3math.h"
#include "llextendedstatus.h"

class LLAvatarName;
class LLMessageSystem;
class LLViewerTexture;
class LLInventoryItem;
class LLViewerObject;
class LLAgent;
class LLToolObjPicker;
class LLMeanCollisionData;
struct LLResourceData;

// these flags are used to label info requests to the server
//const U32 BUG_REPORT_REQUEST 		= 0x01 << 0; // DEPRECATED
const U32 COMPLAINT_REPORT_REQUEST 	= 0x01 << 1;
const U32 OBJECT_PAY_REQUEST		= 0x01 << 2;


// ************************************************************
// THESE ENUMS ARE IN THE DATABASE!!!
//
// The process for adding a new report type is to:
// 1. Issue a command to the database to insert the new value:
//    insert into user_report_type (description)
//                values ('${new type name}');
// 2. Record the integer value assigned:
//    select type from user_report_type
//           where description='${new type name}';
// 3. Add it here.
//     ${NEW TYPE NAME}_REPORT = ${type_number};
//
// Failure to follow this process WILL result in incorrect
// queries on user reports.
// ************************************************************
enum EReportType
{
	NULL_REPORT = 0,		// don't use this value anywhere
	UNKNOWN_REPORT = 1,
	//BUG_REPORT = 2, // DEPRECATED
	COMPLAINT_REPORT = 3,
	CS_REQUEST_REPORT = 4
};

class LLFloaterReporter
:	public LLFloater
{
public:
	LLFloaterReporter(const LLSD& key);
	/*virtual*/ ~LLFloaterReporter();
	/*virtual*/ bool postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void onClose(bool app_quitting);

	static void onIdle(void* user_data);
	
	void setReportType(EReportType type) { mReportType = type; }
	
	// Enables all buttons
	static void showFromMenu(EReportType report_type);

	static void showFromObject(const LLUUID& object_id, const LLUUID& experience_id = LLUUID::null);
	static void showFromAvatar(const LLUUID& avatar_id, const std::string avatar_name);
    static void showFromChat(const LLUUID& avatar_id, const std::string& avatar_name, const std::string& time, const std::string& description);
	static void showFromExperience(const LLUUID& experience_id);

	static void onClickSend			(void *userdata);
	static void onClickCancel		(void *userdata);
	static void onClickObjPicker	(void *userdata);
	void onClickSelectAbuser ();
	static void closePickTool	(void *userdata);
	static void uploadDoneCallback(const LLUUID &uuid, void* user_data, S32 result, LLExtStat ext_status);

	void setPickedObjectProperties(const std::string& object_name, const std::string& owner_name, const LLUUID owner_id);

	void onLoadScreenshotDialog(const LLSD& notification, const LLSD& response);

	void takeNewSnapshot();

private:
	static void show(const LLUUID& object_id, const std::string& avatar_name = LLStringUtil::null, const LLUUID& experience_id = LLUUID::null);

	void takeScreenshot(bool use_prev_screenshot = false);
	void uploadImage();
	bool validateReport();
	LLSD gatherReport();
	void sendReportViaLegacy(const LLSD & report);
	void sendReportViaCaps(std::string url, std::string sshot_url, const LLSD & report);
	void setPosBox(const LLVector3d &pos);
	void enableControls(bool own_avatar);
	void getExperienceInfo(const LLUUID& object_id);
	void getObjectInfo(const LLUUID& object_id);
	void callbackAvatarID(const uuid_vec_t& ids, const std::vector<LLAvatarName> names);
	void setFromAvatarID(const LLUUID& avatar_id);
	void onAvatarNameCache(const LLUUID& avatar_id, const LLAvatarName& av_name);

	static void requestAbuseCategoriesCoro(std::string url, LLHandle<LLFloater> handle);
    static void finishedARPost(const LLSD &);

private:
	EReportType		mReportType;
	LLUUID 			mObjectID;
	LLUUID			mScreenID;
	LLUUID			mAbuserID;
	LLUUID			mExperienceID;
	// Store the real name, not the link, for upstream reporting
	std::string		mOwnerName;
	bool			mDeselectOnClose;
	bool 			mPicking;
	LLVector3		mPosition;
	bool			mCopyrightWarningSeen;
	std::string		mDefaultSummary;
	LLResourceData* mResourceDatap;
	boost::signals2::connection mAvatarNameCacheConnection;

	LLPointer<LLImageRaw> mImageRaw;
	LLPointer<LLImageRaw> mPrevImageRaw;
	LLFrameTimer	mSnapshotTimer;
};

#endif
