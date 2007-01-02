/** 
 * @file llfloaterreporter.h
 * @author Andrew Meadows
 * @brief Bug and abuse reports.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERREPORTER_H
#define LL_LLFLOATERREPORTER_H

#include "llfloater.h"
#include "lluuid.h"
#include "v3math.h"

class LLMessageSystem;
class LLViewerImage;
class LLInventoryItem;
class LLViewerObject;
class LLAgent;
class LLToolObjPicker;
class LLMeanCollisionData;

// these flags are used to label info requests to the server
const U32 BUG_REPORT_REQUEST 		= 0x01 << 0;
const U32 COMPLAINT_REPORT_REQUEST 	= 0x01 << 1;

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
	BUG_REPORT = 2,
	COMPLAINT_REPORT = 3,
	CS_REQUEST_REPORT = 4
};


class LLFloaterReporter
:	public LLFloater
{
public:
	LLFloaterReporter(const std::string& name, 
					  const LLRect &rect, 
					  const std::string& title, 
					  EReportType = UNKNOWN_REPORT);
	/*virtual*/ ~LLFloaterReporter();

	// Enables all buttons
	static void showFromMenu(EReportType report_type);

	static void showFromObject(const LLUUID& object_id);

	static void onClickSend			(void *userdata);
	static void onClickCancel		(void *userdata);
	static void onClickObjPicker	(void *userdata);
	static void onClickSelectAbuser (void *userdata);
	static void closePickTool	(void *userdata);
	static void uploadDoneCallback(const LLUUID &uuid, void* user_data, S32 result);
	static void addDescription(const LLString& description, LLMeanCollisionData *mcd = NULL);
	static void setDescription(const LLString& description, LLMeanCollisionData *mcd = NULL);
	
	// returns a pointer to reporter of report_type
	static LLFloaterReporter* getReporter(EReportType report_type);
	static LLFloaterReporter* createNewAbuseReporter();
	static LLFloaterReporter* createNewBugReporter();

	void setPickedObjectProperties(const char *object_name, const char *owner_name);
	void uploadScreenshot();

private:
	void setReporterID();
	void sendReport();
	void setPosBox(const LLVector3d &pos);
	void enableControls(BOOL own_avatar);
	void getObjectInfo(const LLUUID& object_id);
	static void callbackAvatarID(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* data);

private:
	EReportType		mReportType;
	LLUUID 			mObjectID;
	LLUUID			mScreenID;
	BOOL			mDeselectOnClose;
	BOOL 			mPicking;
	LLVector3		mPosition;
	BOOL			mCopyrightWarningSeen;
	std::list<LLMeanCollisionData*> mMCDList;
	LLString		mDefaultSummary;
};

#endif
