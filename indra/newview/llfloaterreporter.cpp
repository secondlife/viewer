/** 
 * @file llfloaterreporter.cpp
 * @brief Bug and abuse reports.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include <sstream>

// self include
#include "llfloaterreporter.h"

// linden library includes
#include "llassetstorage.h"
#include "llcachename.h"
#include "llfontgl.h"
#include "llgl.h"			// for renderer
#include "llinventory.h"
#include "llstring.h"
#include "llsys.h"
#include "llversion.h"
#include "message.h"
#include "v3math.h"

// viewer project includes
#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llinventoryview.h"
#include "lllineeditor.h"
#include "lltexturectrl.h"
#include "llscrolllistctrl.h"
#include "llimview.h"
#include "lltextbox.h"
#include "lldispatcher.h"
#include "llviewertexteditor.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llcombobox.h"
#include "llfloaterrate.h"
#include "lltooldraganddrop.h"
#include "llfloatermap.h"
#include "lluiconstants.h"
#include "lluploaddialog.h"
#include "llcallingcard.h"
#include "llviewerobjectlist.h"
#include "llagent.h"
#include "lltoolobjpicker.h"
#include "lltoolmgr.h"
#include "llviewermenu.h"		// for LLResourceData
#include "llviewerwindow.h"
#include "llviewerimagelist.h"
#include "llworldmap.h"
#include "llfilepicker.h"
#include "llfloateravatarpicker.h"
#include "lldir.h"
#include "llselectmgr.h"
#include "llviewerbuild.h"
#include "llvieweruictrlfactory.h"
#include "viewer.h"

#include "llassetuploadresponders.h"

const U32 INCLUDE_SCREENSHOT  = 0x01 << 0;

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

// this map keeps track of current reporter instances
// there can only be one instance of each reporter type
LLMap< EReportType, LLFloaterReporter* > gReporterInstances;

// keeps track of where email is going to - global to avoid a pile
// of static/non-static access outside my control
namespace {
	static BOOL gEmailToEstateOwner = FALSE;
	static BOOL gDialogVisible = FALSE;
}

//-----------------------------------------------------------------------------
// Member functions
//-----------------------------------------------------------------------------
LLFloaterReporter::LLFloaterReporter(
	const std::string& name,
	const LLRect& rect, 
	const std::string& title, 
	EReportType report_type)
	:	
	LLFloater(name, rect, title),
	mReportType(report_type),
	mObjectID(),
	mScreenID(),
	mAbuserID(),
	mDeselectOnClose( FALSE ),
	mPicking( FALSE), 
	mPosition(),
	mCopyrightWarningSeen( FALSE ),
	mResourceDatap(new LLResourceData())
{
	if (report_type == BUG_REPORT)
	{
		gUICtrlFactory->buildFloater(this, "floater_report_bug.xml");
	}
	else
	{
		gUICtrlFactory->buildFloater(this, "floater_report_abuse.xml");
	}

	LLViewerRegion *regionp = gAgent.getRegion();
	if (regionp)
	{
		childSetText("sim_field", regionp->getName() );
		childSetText("abuse_location_edit", regionp->getName() );
	}

	LLButton* pick_btn = LLUICtrlFactory::getButtonByName(this, "pick_btn");
	if (pick_btn)
	{
		// XUI: Why aren't these in viewerart.ini?
		pick_btn->setImages( "UIImgFaceUUID",
							"UIImgFaceSelectedUUID" );
		childSetAction("pick_btn", onClickObjPicker, this);
	}

	if (report_type != BUG_REPORT)
	{
		// abuser name is selected from a list
		LLLineEditor* le = (LLLineEditor*)getCtrlByNameAndType("abuser_name_edit", WIDGET_TYPE_LINE_EDITOR);
		le->setEnabled( FALSE );
	}

	childSetAction("select_abuser", onClickSelectAbuser, this);

	childSetAction("send_btn", onClickSend, this);
	childSetAction("cancel_btn", onClickCancel, this);

	enableControls(TRUE);

	// convert the position to a string
	LLVector3d pos = gAgent.getPositionGlobal();
	if (regionp)
	{
		pos -= regionp->getOriginGlobal();
	}
	setPosBox(pos);

	gReporterInstances.addData(report_type, this);

	// Take a screenshot, but don't draw this floater.
	setVisible(FALSE);
	takeScreenshot();
	setVisible(TRUE);

	// Default text to be blank
	childSetText("object_name", "");
	childSetText("owner_name", "");

	childSetFocus("summary_edit");

	mDefaultSummary = childGetText("details_edit");

	gDialogVisible = TRUE;

	// only request details for abuse reports (not BUG reports)
	if (report_type != BUG_REPORT)
	{
		// send a message and ask for information about this region - 
		// result comes back in processRegionInfo(..)
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("RequestRegionInfo");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		gAgent.sendReliableMessage();
	};
}

// static
void LLFloaterReporter::processRegionInfo(LLMessageSystem* msg)
{
	U32 region_flags;
	msg->getU32("RegionInfo", "RegionFlags", region_flags);
	gEmailToEstateOwner = ( region_flags & REGION_FLAGS_ABUSE_EMAIL_TO_ESTATE_OWNER );

	if ( gDialogVisible )
	{
		if ( gEmailToEstateOwner )
		{
			gViewerWindow->alertXml("HelpReportAbuseEmailEO");
		}
		else
			gViewerWindow->alertXml("HelpReportAbuseEmailLL");
	};
}

// virtual
LLFloaterReporter::~LLFloaterReporter()
{
	gReporterInstances.removeData(mReportType);
	// child views automatically deleted
	mObjectID 		= LLUUID::null;

	if (mPicking)
	{
		closePickTool(this);
	}

	mPosition.setVec(0.0f, 0.0f, 0.0f);

	std::for_each(mMCDList.begin(), mMCDList.end(), DeletePointer() );
	mMCDList.clear();

	delete mResourceDatap;
	gDialogVisible = FALSE;
}

// virtual
void LLFloaterReporter::draw()
{
	// this is set by a static callback sometime after the dialog is created.
	// Only disable screenshot for abuse reports to estate owners - bug reports always
	// allow screenshots to be taken.
	if ( gEmailToEstateOwner && ( mReportType != BUG_REPORT ) )
	{
		childSetValue("screen_check", FALSE );
		childSetEnabled("screen_check", FALSE );
	}
	else
	{
		childSetEnabled("screen_check", TRUE );
	}

	LLFloater::draw();
}

void LLFloaterReporter::enableControls(BOOL enable)
{
	childSetEnabled("category_combo", enable);
	// bug reports never include the chat history
	if (mReportType != BUG_REPORT)
	{
		childSetEnabled("chat_check", enable);
	}
	childSetEnabled("screen_check",	enable);
	childDisable("screenshot");
	childSetEnabled("pick_btn",		enable);
	childSetEnabled("summary_edit",	enable);
	childSetEnabled("details_edit",	enable);
	childSetEnabled("send_btn",		enable);
	childSetEnabled("cancel_btn",	enable);
}

void LLFloaterReporter::getObjectInfo(const LLUUID& object_id)
{
	// TODO -- 
	// 1 need to send to correct simulator if object is not 
	//   in same simulator as agent
	// 2 display info in widget window that gives feedback that
	//   we have recorded the object info
	// 3 can pick avatar ==> might want to indicate when a picked 
	//   object is an avatar, attachment, or other category

	mObjectID = object_id;

	if (LLUUID::null != mObjectID)
	{
		// get object info for the user's benefit
		LLViewerObject* objectp = NULL;
		objectp = gObjectList.findObject( mObjectID );
		if (objectp)
		{
			if ( objectp->isAttachment() )
			{
				objectp = (LLViewerObject*)objectp->getRoot();
			}

			// correct the region and position information
			LLViewerRegion *regionp = objectp->getRegion();
			if (regionp)
			{
				childSetText("sim_field", regionp->getName());
				LLVector3d global_pos;
				global_pos.setVec(objectp->getPositionRegion());
				setPosBox(global_pos);
			}
	
			if (objectp->isAvatar())
			{
				// we have the information we need
				LLString object_owner;

				LLNameValue* firstname = objectp->getNVPair("FirstName");
				LLNameValue* lastname =  objectp->getNVPair("LastName");
				if (firstname && lastname)
				{
					object_owner.append(firstname->getString());
					object_owner.append(1, ' ');
					object_owner.append(lastname->getString());
				}
				else
				{
					object_owner.append("Unknown");
				}
				childSetText("object_name", object_owner);
				childSetText("owner_name", object_owner);
			}
			else
			{
				// we have to query the simulator for information 
				// about this object
				LLMessageSystem* msg = gMessageSystem;
				U32 request_flags = (mReportType == BUG_REPORT) ? BUG_REPORT_REQUEST : COMPLAINT_REPORT_REQUEST;
				msg->newMessageFast(_PREHASH_RequestObjectPropertiesFamily);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_ObjectData);
				msg->addU32Fast(_PREHASH_RequestFlags, request_flags );
				msg->addUUIDFast(_PREHASH_ObjectID, 	mObjectID);
				LLViewerRegion* regionp = objectp->getRegion();
				msg->sendReliable( regionp->getHost() );
			}
		}
	}
}


// static
void LLFloaterReporter::onClickSelectAbuser(void *userdata)
{
	LLFloaterReporter *self = (LLFloaterReporter *)userdata;

	gFloaterView->getParentFloater(self)->addDependentFloater(LLFloaterAvatarPicker::show(callbackAvatarID, userdata, FALSE, TRUE ));
}

// static
void LLFloaterReporter::callbackAvatarID(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* data)
{
	LLFloaterReporter* self = (LLFloaterReporter*) data;

	if (ids.empty() || names.empty()) return;

	// this should never be called in a bug report but here for safety.
	if ( self->mReportType != BUG_REPORT )
	{
		self->childSetText("abuser_name_edit", names[0] );
		
		self->mAbuserID = ids[0];

		self->refresh();
	};
}

// static
void LLFloaterReporter::onClickSend(void *userdata)
{
	LLFloaterReporter *self = (LLFloaterReporter *)userdata;
	
	if (self->mPicking)
	{
		closePickTool(self);
	}

	if(self->validateReport())
	{
		// only show copyright alert for abuse reports
		if ( self->mReportType != BUG_REPORT )
		{
			if ( ! self->mCopyrightWarningSeen )
			{
				LLString details_lc = self->childGetText("details_edit");
				LLString::toLower( details_lc );
				LLString summary_lc = self->childGetText("summary_edit");
				LLString::toLower( summary_lc );
				if ( details_lc.find( "copyright" ) != std::string::npos ||
					summary_lc.find( "copyright" ) != std::string::npos )
				{
					gViewerWindow->alertXml("HelpReportAbuseContainsCopyright");
					self->mCopyrightWarningSeen = TRUE;
					return;
				};
			};
		};

		LLUploadDialog::modalUploadDialog("Uploading...\n\nReport");
		// *TODO don't upload image if checkbox isn't checked
		std::string url = gAgent.getRegion()->getCapability("SendUserReport");
		std::string sshot_url = gAgent.getRegion()->getCapability("SendUserReportWithScreenshot");
		if(!url.empty() || !sshot_url.empty())
		{
			self->sendReportViaCaps(url, sshot_url, self->gatherReport());
			self->close();
		}
		else
		{
			if(self->childGetValue("screen_check"))
			{
				self->childDisable("send_btn");
				self->childDisable("cancel_btn");
				// the callback from uploading the image calls sendReportViaLegacy()
				self->uploadImage();
			}
			else
			{
				self->sendReportViaLegacy(self->gatherReport());
				LLUploadDialog::modalUploadFinished();
				self->close();
			}
		}
	}
}


// static
void LLFloaterReporter::onClickCancel(void *userdata)
{
	LLFloaterReporter *self = (LLFloaterReporter *)userdata;

	// reset flag in case the next report also contains this text
	self->mCopyrightWarningSeen = FALSE;

	if (self->mPicking)
	{
		closePickTool(self);
	}
	self->close();
}


// static 
void LLFloaterReporter::onClickObjPicker(void *userdata)
{
	LLFloaterReporter *self = (LLFloaterReporter *)userdata;
	gToolObjPicker->setExitCallback(LLFloaterReporter::closePickTool, self);
	gToolMgr->setTransientTool(gToolObjPicker);
	self->mPicking = TRUE;
	self->childSetText("object_name", "");
	self->childSetText("owner_name", "");
	LLButton* pick_btn = LLUICtrlFactory::getButtonByName(self, "pick_btn");
	if (pick_btn) pick_btn->setToggleState(TRUE);
}


// static
void LLFloaterReporter::closePickTool(void *userdata)
{
	LLFloaterReporter *self = (LLFloaterReporter *)userdata;

	LLUUID object_id = gToolObjPicker->getObjectID();
	self->getObjectInfo(object_id);

	gToolMgr->clearTransientTool();
	self->mPicking = FALSE;
	LLButton* pick_btn = LLUICtrlFactory::getButtonByName(self, "pick_btn");
	if (pick_btn) pick_btn->setToggleState(FALSE);
}


// static
void LLFloaterReporter::showFromMenu(EReportType report_type)
{
	if (gReporterInstances.checkData(report_type))
	{
		// ...bring that window to front
		LLFloaterReporter *f = gReporterInstances.getData(report_type);
		f->open();		/* Flawfinder: ignore */
	}
	else
	{
		LLFloaterReporter *f;
		if (BUG_REPORT == report_type)
		{
			f = LLFloaterReporter::createNewBugReporter();
		}
		else if (COMPLAINT_REPORT == report_type)
		{
			f = LLFloaterReporter::createNewAbuseReporter();
		}
		else
		{
			llwarns << "Unknown LLViewerReporter type : " << report_type << llendl;
			return;
		}

		f->center();

		if (report_type == BUG_REPORT)
		{
 			gViewerWindow->alertXml("HelpReportBug");
		}
		else
		{
			// popup for abuse reports is triggered elsewhere
		}

		// grab the user's name
		LLString fullname;
		gAgent.buildFullname(fullname);
		f->childSetText("reporter_field", fullname);
	}
}


// static
void LLFloaterReporter::showFromObject(const LLUUID& object_id)
{
	LLFloaterReporter* f = createNewAbuseReporter();
	f->center();
	f->setFocus(TRUE);

	// grab the user's name
	LLString fullname;
	gAgent.buildFullname(fullname);
	f->childSetText("reporter_field", fullname);

	// Request info for this object
	f->getObjectInfo(object_id);

	// Need to deselect on close
	f->mDeselectOnClose = TRUE;

	f->open();		/* Flawfinder: ignore */
}


// static 
LLFloaterReporter* LLFloaterReporter::getReporter(EReportType report_type)
{
	LLFloaterReporter *self = NULL;
	if (gReporterInstances.checkData(report_type))
	{
		// ...bring that window to front
		self = gReporterInstances.getData(report_type);
	}
	return self;
}

LLFloaterReporter* LLFloaterReporter::createNewAbuseReporter()
{
	return new LLFloaterReporter("complaint_reporter",
						         LLRect(),
								 "Report Abuse",
								 COMPLAINT_REPORT);
}

//static
LLFloaterReporter* LLFloaterReporter::createNewBugReporter()
{
	return new LLFloaterReporter("bug_reporter",
				                 LLRect(),
 					             "Report Bug",
                     			 BUG_REPORT);
}
	


void LLFloaterReporter::setPickedObjectProperties(const char *object_name, const char *owner_name)
{
	childSetText("object_name", object_name);
	childSetText("owner_name", owner_name);
}


bool LLFloaterReporter::validateReport()
{
	// Ensure user selected a category from the list
	LLSD category_sd = childGetValue("category_combo");
	U8 category = (U8)category_sd.asInteger();
	if (category == 0)
	{
		if ( mReportType != BUG_REPORT )
		{
			gViewerWindow->alertXml("HelpReportAbuseSelectCategory");
		}
		else
		{
			gViewerWindow->alertXml("HelpReportBugSelectCategory");
		}
		return false;
	}

	if ( mReportType != BUG_REPORT )
	{
	  if ( childGetText("abuser_name_edit").empty() )
	  {
		  gViewerWindow->alertXml("HelpReportAbuseAbuserNameEmpty");
		  return false;
	  };
  
	  if ( childGetText("abuse_location_edit").empty() )
	  {
		  gViewerWindow->alertXml("HelpReportAbuseAbuserLocationEmpty");
		  return false;
	  };
	};

	if ( childGetText("summary_edit").empty() )
	{
		if ( mReportType != BUG_REPORT )
		{
			gViewerWindow->alertXml("HelpReportAbuseSummaryEmpty");
		}
		else
		{
			gViewerWindow->alertXml("HelpReportBugSummaryEmpty");
		}
		return false;
	};

	if ( childGetText("details_edit") == mDefaultSummary )
	{
		if ( mReportType != BUG_REPORT )
		{
			gViewerWindow->alertXml("HelpReportAbuseDetailsEmpty");
		}
		else
		{
			gViewerWindow->alertXml("HelpReportBugDetailsEmpty");
		}
		return false;
	};
	return true;
}

LLSD LLFloaterReporter::gatherReport()
{	
	LLViewerRegion *regionp = gAgent.getRegion();
	if (!regionp) return LLSD(); // *TODO handle this failure case more gracefully

	// reset flag in case the next report also contains this text
	mCopyrightWarningSeen = FALSE;

	std::ostringstream summary;
	if (!gInProductionGrid)
	{
		summary << "Preview ";
	}

	LLString category_name;
	LLComboBox* combo = LLUICtrlFactory::getComboBoxByName(this, "category_combo");
	if (combo)
	{
		category_name = combo->getSimpleSelectedItem(); // want label, not value
	}

#if LL_WINDOWS
	const char* platform = "Win";
	const char* short_platform = "O:W";
#elif LL_DARWIN
	const char* platform = "Mac";
	const char* short_platform = "O:M";
#elif LL_LINUX
	const char* platform = "Lnx";
	const char* short_platform = "O:L";
#else
	const char* platform = "???";
	const char* short_platform = "O:?";
#endif


	if ( mReportType == BUG_REPORT)
	{
		summary << short_platform << " V" << LL_VERSION_MAJOR << "."
			<< LL_VERSION_MINOR << "."
			<< LL_VERSION_PATCH << "."
			<< LL_VIEWER_BUILD
			<< " (" << regionp->getName() << ")"
			<< "[" << category_name << "] "
			<< "\"" << childGetValue("summary_edit").asString() << "\"";
	}
	else
	{
		summary << ""
			<< " |" << regionp->getName() << "|"								// region reporter is currently in.
			<< " (" << childGetText("abuse_location_edit") << ")"				// region abuse occured in (freeform text - no LLRegionPicker tool)
			<< " [" << category_name << "] "									// updated category
			<< " {" << childGetText("abuser_name_edit") << "} "					// name of abuse entered in report (chosen using LLAvatarPicker)
			<< " \"" << childGetValue("summary_edit").asString() << "\"";		// summary as entered
	};

	std::ostringstream details;
	if (mReportType != BUG_REPORT)
	{
		details << "V" << LL_VERSION_MAJOR << "."								// client version moved to body of email for abuse reports
			<< LL_VERSION_MINOR << "."
			<< LL_VERSION_PATCH << "."
			<< LL_VIEWER_BUILD << std::endl << std::endl;
	}
	LLString object_name = childGetText("object_name");
	LLString owner_name = childGetText("owner_name");
	if (!object_name.empty() && !owner_name.empty())
	{
		details << "Object: " << object_name << "\n";
		details << "Owner: " << owner_name << "\n";
	}

	if ( mReportType != BUG_REPORT )
	{
		details << "Abuser name: " << childGetText("abuser_name_edit") << " \n";
		details << "Abuser location: " << childGetText("abuse_location_edit") << " \n";
	};

	details << childGetValue("details_edit").asString();

	char version_string[MAX_STRING];		/* Flawfinder: ignore */
	snprintf(version_string,				/* Flawfinder: ignore */
			MAX_STRING,
			"%d.%d.%d %s %s %s %s",
			LL_VERSION_MAJOR,
			LL_VERSION_MINOR,
			LL_VERSION_PATCH,
			platform,
			gSysCPU.getFamily().c_str(),
			gGLManager.mGLRenderer.c_str(),
			gGLManager.mDriverVersionVendorString.c_str());

	// only send a screenshot ID if we're asked to and the email is 
	// going to LL - Estate Owners cannot see the screenshot asset
	LLUUID screenshot_id = LLUUID::null;
	if (childGetValue("screen_check"))
	{
		if ( mReportType != BUG_REPORT )
		{
			if ( gEmailToEstateOwner == FALSE )
			{
				screenshot_id = childGetValue("screenshot");
			}
		}
		else
		{
			screenshot_id = childGetValue("screenshot");
		};
	};

	LLSD report = LLSD::emptyMap();
	report["report-type"] = (U8) mReportType;
	report["category"] = childGetValue("category_combo");
	report["position"] = mPosition.getValue();
	report["check-flags"] = (U8)0; // this is not used
	report["screenshot-id"] = screenshot_id;
	report["object-id"] = mObjectID;
	report["abuser-id"] = mAbuserID;
	report["abuse-region-name"] = "";
	report["abuse-region-id"] = LLUUID::null;
	report["summary"] = summary.str();
	report["version-string"] = version_string;
	report["details"] = details.str();
	return report;
}

void LLFloaterReporter::sendReportViaLegacy(const LLSD & report)
{
	LLViewerRegion *regionp = gAgent.getRegion();
	if (!regionp) return;
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_UserReport);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	
	msg->nextBlockFast(_PREHASH_ReportData);
	msg->addU8Fast(_PREHASH_ReportType, report["report-type"].asInteger());
	msg->addU8(_PREHASH_Category, report["category"].asInteger());
	msg->addVector3Fast(_PREHASH_Position, 	LLVector3(report["position"]));
	msg->addU8Fast(_PREHASH_CheckFlags, report["check-flags"].asInteger());
	msg->addUUIDFast(_PREHASH_ScreenshotID, report["screenshot-id"].asUUID());
	msg->addUUIDFast(_PREHASH_ObjectID, report["object-id"].asUUID());
	msg->addUUID("AbuserID", report["abuser-id"].asUUID());
	msg->addString("AbuseRegionName", report["abuse-region-name"].asString());
	msg->addUUID("AbuseRegionID", report["abuse-region-id"].asUUID());

	msg->addStringFast(_PREHASH_Summary, report["summary"].asString().c_str());
	msg->addString("VersionString", report["version-string"]);
	msg->addStringFast(_PREHASH_Details, report["details"] );
	
	msg->sendReliable(regionp->getHost());
}

class LLUserReportScreenshotResponder : public LLAssetUploadResponder
{
public:
	LLUserReportScreenshotResponder(const LLSD & post_data, 
									const LLUUID & vfile_id, 
									LLAssetType::EType asset_type):
	  LLAssetUploadResponder(post_data, vfile_id, asset_type)
	{
	}
	void uploadFailed(const LLSD& content)
	{
		// *TODO pop up a dialog so the user knows their report screenshot didn't make it
		LLUploadDialog::modalUploadFinished();
	}
	void uploadComplete(const LLSD& content)
	{
		// we don't care about what the server returns from this post, just clean up the UI
		LLUploadDialog::modalUploadFinished();
	}
};

class LLUserReportResponder : public LLHTTPClient::Responder
{
public:
	LLUserReportResponder(): LLHTTPClient::Responder()  {}

	void error(U32 status, const std::string& reason)
	{
		// *TODO do some user messaging here
		LLUploadDialog::modalUploadFinished();
	}
	void result(const LLSD& content)
	{
		// we don't care about what the server returns
		LLUploadDialog::modalUploadFinished();
	}
};

void LLFloaterReporter::sendReportViaCaps(std::string url, std::string sshot_url, const LLSD& report)
{
	if(childGetValue("screen_check").asBoolean() && !sshot_url.empty())
	{
		// try to upload screenshot
		LLHTTPClient::post(sshot_url, report, new LLUserReportScreenshotResponder(report, 
															mResourceDatap->mAssetInfo.mUuid, 
															mResourceDatap->mAssetInfo.mType));			
	}
	else
	{
		// screenshot not wanted or we don't have screenshot cap
		LLHTTPClient::post(url, report, new LLUserReportResponder());			
	}
}

void LLFloaterReporter::takeScreenshot()
{
	const S32 IMAGE_WIDTH = 1024;
	const S32 IMAGE_HEIGHT = 768;

	LLPointer<LLImageRaw> raw = new LLImageRaw;
	if( !gViewerWindow->rawSnapshot(raw, IMAGE_WIDTH, IMAGE_HEIGHT, TRUE, TRUE, FALSE))
	{
		llwarns << "Unable to take screenshot" << llendl;
		return;
	}
	LLPointer<LLImageJ2C> upload_data = LLViewerImageList::convertToUploadFile(raw);

	// create a resource data
	mResourceDatap->mInventoryType = LLInventoryType::IT_NONE;
	mResourceDatap->mAssetInfo.mTransactionID.generate();
	mResourceDatap->mAssetInfo.mUuid = mResourceDatap->mAssetInfo.mTransactionID.makeAssetID(gAgent.getSecureSessionID());
	if (BUG_REPORT == mReportType)
	{
		mResourceDatap->mAssetInfo.mType = LLAssetType::AT_TEXTURE;
		mResourceDatap->mPreferredLocation = LLAssetType::EType(-1);
	}
	else if (COMPLAINT_REPORT == mReportType)
	{
		mResourceDatap->mAssetInfo.mType = LLAssetType::AT_TEXTURE;
		mResourceDatap->mPreferredLocation = LLAssetType::EType(-2);
	}
	else
	{
		llwarns << "Unknown LLFloaterReporter type" << llendl;
	}
	mResourceDatap->mAssetInfo.mCreatorID = gAgentID;
	mResourceDatap->mAssetInfo.setName("screenshot_name");
	mResourceDatap->mAssetInfo.setDescription("screenshot_descr");

	// store in VFS
	LLVFile::writeFile(upload_data->getData(), 
						upload_data->getDataSize(), 
						gVFS, 
						mResourceDatap->mAssetInfo.mUuid, 
						mResourceDatap->mAssetInfo.mType);

	// store in the image list so it doesn't try to fetch from the server
	LLViewerImage* image_in_list = new LLViewerImage(mResourceDatap->mAssetInfo.mUuid, TRUE);
	image_in_list->createGLTexture(0, raw);
	gImageList.addImage(image_in_list); 

	// the texture picker then uses that texture
	LLTexturePicker* texture = LLUICtrlFactory::getTexturePickerByName(this, "screenshot");
	if (texture)
	{
		texture->setImageAssetID(mResourceDatap->mAssetInfo.mUuid);
		texture->setDefaultImageAssetID(mResourceDatap->mAssetInfo.mUuid);
		texture->setCaption("Screenshot");
	}

}

void LLFloaterReporter::uploadImage()
{
	llinfos << "*** Uploading: " << llendl;
	llinfos << "Type: " << LLAssetType::lookup(mResourceDatap->mAssetInfo.mType) << llendl;
	llinfos << "UUID: " << mResourceDatap->mAssetInfo.mUuid << llendl;
	llinfos << "Name: " << mResourceDatap->mAssetInfo.getName() << llendl;
	llinfos << "Desc: " << mResourceDatap->mAssetInfo.getDescription() << llendl;

	gAssetStorage->storeAssetData(mResourceDatap->mAssetInfo.mTransactionID,
									mResourceDatap->mAssetInfo.mType,
									LLFloaterReporter::uploadDoneCallback,
									(void*)mResourceDatap, TRUE);
}


// static
void LLFloaterReporter::uploadDoneCallback(const LLUUID &uuid, void *user_data, S32 result) // StoreAssetData callback (fixed)
{
	LLUploadDialog::modalUploadFinished();

	LLResourceData* data = (LLResourceData*)user_data;

	if(result < 0)
	{
		LLStringBase<char>::format_map_t args;
		args["[REASON]"] = std::string(LLAssetStorage::getErrorString(result));
		gViewerWindow->alertXml("ErrorUploadingReportScreenshot", args);

		std::string err_msg("There was a problem uploading a report screenshot");
		err_msg += " due to the following reason: " + args["[REASON]"];
		llwarns << err_msg << llendl;
		return;
	}

	EReportType report_type = UNKNOWN_REPORT;
	if (data->mPreferredLocation == -1)
	{
		report_type = BUG_REPORT;
	}
	else if (data->mPreferredLocation == -2)
	{
		report_type = COMPLAINT_REPORT;
	}
	else 
	{
		llwarns << "Unknown report type : " << data->mPreferredLocation << llendl;
	}

	LLFloaterReporter *self = getReporter(report_type);
	if (self)
	{
		self->mScreenID = uuid;
		llinfos << "Got screen shot " << uuid << llendl;
		self->sendReportViaLegacy(self->gatherReport());
	}
	self->close();
}


void LLFloaterReporter::setPosBox(const LLVector3d &pos)
{
	mPosition.setVec(pos);
	LLString pos_string = llformat("{%.1f, %.1f, %.1f}",
		mPosition.mV[VX],
		mPosition.mV[VY],
		mPosition.mV[VZ]);
	childSetText("pos_field", pos_string);
}

void LLFloaterReporter::setDescription(const LLString& description, LLMeanCollisionData *mcd)
{
	LLFloaterReporter *self = gReporterInstances[COMPLAINT_REPORT];
	if (self)
	{
		self->childSetText("details_edit", description);

		for_each(self->mMCDList.begin(), self->mMCDList.end(), DeletePointer());
		self->mMCDList.clear();
		if (mcd)
		{
			self->mMCDList.push_back(new LLMeanCollisionData(mcd));
		}
	}
}

void LLFloaterReporter::addDescription(const LLString& description, LLMeanCollisionData *mcd)
{
	LLFloaterReporter *self = gReporterInstances[COMPLAINT_REPORT];
	if (self)
	{
		LLTextEditor* text = LLUICtrlFactory::getTextEditorByName(self, "details_edit");
		if (text)
		{	
			text->insertText(description);
		}
		if (mcd)
		{
			self->mMCDList.push_back(new LLMeanCollisionData(mcd));
		}
	}
}
