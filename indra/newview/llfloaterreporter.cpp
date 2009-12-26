/** 
 * @file llfloaterreporter.cpp
 * @brief Abuse reports.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

// self include
#include "llfloaterreporter.h"

#include <sstream>

// linden library includes
#include "llassetstorage.h"
#include "llfontgl.h"
#include "llimagej2c.h"
#include "llinventory.h"
#include "llnotificationsutil.h"
#include "llstring.h"
#include "llsys.h"
#include "llvfile.h"
#include "llvfs.h"
#include "mean_collision_data.h"
#include "message.h"
#include "v3math.h"

// viewer project includes
#include "llagent.h"
#include "llbutton.h"
#include "llfloaterreg.h"
#include "lltexturectrl.h"
#include "llscrolllistctrl.h"
#include "lldispatcher.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llcombobox.h"
#include "lltooldraganddrop.h"
#include "lluiconstants.h"
#include "lluploaddialog.h"
#include "llcallingcard.h"
#include "llviewerobjectlist.h"
#include "lltoolobjpicker.h"
#include "lltoolmgr.h"
#include "llresourcedata.h"		// for LLResourceData
#include "llslurl.h"
#include "llviewerwindow.h"
#include "llviewertexturelist.h"
#include "llworldmap.h"
#include "llfilepicker.h"
#include "llfloateravatarpicker.h"
#include "lldir.h"
#include "llselectmgr.h"
#include "llversioninfo.h"
#include "lluictrlfactory.h"
#include "llviewernetwork.h"

#include "llassetuploadresponders.h"
#include "llagentui.h"

const U32 INCLUDE_SCREENSHOT  = 0x01 << 0;

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Member functions
//-----------------------------------------------------------------------------
								 
LLFloaterReporter::LLFloaterReporter(const LLSD& key)
:	LLFloater(key),
	mReportType(COMPLAINT_REPORT),
	mObjectID(),
	mScreenID(),
	mAbuserID(),
	mOwnerName(),
	mDeselectOnClose( FALSE ),
	mPicking( FALSE), 
	mPosition(),
	mCopyrightWarningSeen( FALSE ),
	mResourceDatap(new LLResourceData())
{
	//LLUICtrlFactory::getInstance()->buildFloater(this, "floater_report_abuse.xml");
}

// static
void LLFloaterReporter::processRegionInfo(LLMessageSystem* msg)
{
	U32 region_flags;
	msg->getU32("RegionInfo", "RegionFlags", region_flags);
	
	if ( LLFloaterReg::instanceVisible("reporter") )
	{
		LLNotificationsUtil::add("HelpReportAbuseEmailLL");
	};
}
// virtual
BOOL LLFloaterReporter::postBuild()
{
	childSetText("abuse_location_edit", LLAgentUI::buildSLURL());

	enableControls(TRUE);

	// convert the position to a string
	LLVector3d pos = gAgent.getPositionGlobal();
	LLViewerRegion *regionp = gAgent.getRegion();
	if (regionp)
	{
		childSetText("sim_field", regionp->getName());
		pos -= regionp->getOriginGlobal();
	}
	setPosBox(pos);

	// Take a screenshot, but don't draw this floater.
	setVisible(FALSE);
	takeScreenshot();
	setVisible(TRUE);

	// Default text to be blank
	childSetText("object_name", LLStringUtil::null);
	childSetText("owner_name", LLStringUtil::null);
	mOwnerName = LLStringUtil::null;

	childSetFocus("summary_edit");

	mDefaultSummary = childGetText("details_edit");

	// send a message and ask for information about this region - 
	// result comes back in processRegionInfo(..)
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("RequestRegionInfo");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	gAgent.sendReliableMessage();
	
	
	// abuser name is selected from a list
	LLUICtrl* le = getChild<LLUICtrl>("abuser_name_edit");
	le->setEnabled( false );

	setPosBox((LLVector3d)mPosition.getValue());
	LLButton* pick_btn = getChild<LLButton>("pick_btn");
	pick_btn->setImages(std::string("tool_face.tga"),
						std::string("tool_face_active.tga") );
	childSetAction("pick_btn", onClickObjPicker, this);

	childSetAction("select_abuser", onClickSelectAbuser, this);

	childSetAction("send_btn", onClickSend, this);
	childSetAction("cancel_btn", onClickCancel, this);
	
	// grab the user's name
	std::string fullname;
	LLAgentUI::buildFullname(fullname);
	childSetText("reporter_field", fullname);
	
	center();

	return TRUE;
}
// virtual
LLFloaterReporter::~LLFloaterReporter()
{
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
}

// virtual
void LLFloaterReporter::draw()
{
	childSetEnabled("screen_check", TRUE );

	LLFloater::draw();
}

void LLFloaterReporter::enableControls(BOOL enable)
{
	childSetEnabled("category_combo", enable);
	childSetEnabled("chat_check", enable);
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
				std::string object_owner;

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
				std::string owner_link =
					LLSLURL::buildCommand("agent", mObjectID, "inspect");
				childSetText("owner_name", owner_link);
				childSetText("abuser_name_edit", object_owner);
				mAbuserID = object_id;
				mOwnerName = object_owner;
			}
			else
			{
				// we have to query the simulator for information 
				// about this object
				LLMessageSystem* msg = gMessageSystem;
				U32 request_flags = COMPLAINT_REPORT_REQUEST;
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

	self->childSetText("abuser_name_edit", names[0] );

	self->mAbuserID = ids[0];

	self->refresh();

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

			const int IP_CONTENT_REMOVAL = 66;
			const int IP_PERMISSONS_EXPLOIT = 37;
			LLComboBox* combo = self->getChild<LLComboBox>( "category_combo");
			int category_value = combo->getSelectedValue().asInteger(); 

			if ( ! self->mCopyrightWarningSeen )
			{

				std::string details_lc = self->childGetText("details_edit");
				LLStringUtil::toLower( details_lc );
				std::string summary_lc = self->childGetText("summary_edit");
				LLStringUtil::toLower( summary_lc );
				if ( details_lc.find( "copyright" ) != std::string::npos ||
					summary_lc.find( "copyright" ) != std::string::npos  ||
					category_value == IP_CONTENT_REMOVAL ||
					category_value == IP_PERMISSONS_EXPLOIT)
				{
					LLNotificationsUtil::add("HelpReportAbuseContainsCopyright");
					self->mCopyrightWarningSeen = TRUE;
					return;
				}
			}
			else if (category_value == IP_CONTENT_REMOVAL)
			{
				// IP_CONTENT_REMOVAL *always* shows the dialog - 
				// ergo you can never send that abuse report type.
				LLNotificationsUtil::add("HelpReportAbuseContainsCopyright");
				return;
			}


		LLUploadDialog::modalUploadDialog("Uploading...\n\nReport");
		// *TODO don't upload image if checkbox isn't checked
		std::string url = gAgent.getRegion()->getCapability("SendUserReport");
		std::string sshot_url = gAgent.getRegion()->getCapability("SendUserReportWithScreenshot");
		if(!url.empty() || !sshot_url.empty())
		{
			self->sendReportViaCaps(url, sshot_url, self->gatherReport());
			self->closeFloater();
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
				self->closeFloater();
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
	self->closeFloater();
}


// static 
void LLFloaterReporter::onClickObjPicker(void *userdata)
{
	LLFloaterReporter *self = (LLFloaterReporter *)userdata;
	LLToolObjPicker::getInstance()->setExitCallback(LLFloaterReporter::closePickTool, self);
	LLToolMgr::getInstance()->setTransientTool(LLToolObjPicker::getInstance());
	self->mPicking = TRUE;
	self->childSetText("object_name", LLStringUtil::null);
	self->childSetText("owner_name", LLStringUtil::null);
	self->mOwnerName = LLStringUtil::null;
	LLButton* pick_btn = self->getChild<LLButton>("pick_btn");
	if (pick_btn) pick_btn->setToggleState(TRUE);
}


// static
void LLFloaterReporter::closePickTool(void *userdata)
{
	LLFloaterReporter *self = (LLFloaterReporter *)userdata;

	LLUUID object_id = LLToolObjPicker::getInstance()->getObjectID();
	self->getObjectInfo(object_id);

	LLToolMgr::getInstance()->clearTransientTool();
	self->mPicking = FALSE;
	LLButton* pick_btn = self->getChild<LLButton>("pick_btn");
	if (pick_btn) pick_btn->setToggleState(FALSE);
}


// static
void LLFloaterReporter::showFromMenu(EReportType report_type)
{
	if (COMPLAINT_REPORT != report_type)
	{
		llwarns << "Unknown LLViewerReporter type : " << report_type << llendl;
		return;
	}
	
	LLFloaterReporter* f = LLFloaterReg::showTypedInstance<LLFloaterReporter>("reporter", LLSD());
	if (f)
	{
		f->setReportType(report_type);
	}
}


// static
void LLFloaterReporter::showFromObject(const LLUUID& object_id)
{
	LLFloaterReporter* f = LLFloaterReg::showTypedInstance<LLFloaterReporter>("reporter");

	// grab the user's name
	std::string fullname;
	LLAgentUI::buildFullname(fullname);
	f->childSetText("reporter_field", fullname);

	// Request info for this object
	f->getObjectInfo(object_id);

	// Need to deselect on close
	f->mDeselectOnClose = TRUE;

	f->openFloater();
}


void LLFloaterReporter::setPickedObjectProperties(const std::string& object_name, const std::string& owner_name, const LLUUID owner_id)
{
	childSetText("object_name", object_name);
	std::string owner_link =
		LLSLURL::buildCommand("agent", owner_id, "inspect");
	childSetText("owner_name", owner_link);
	childSetText("abuser_name_edit", owner_name);
	mAbuserID = owner_id;
	mOwnerName = owner_name;
}


bool LLFloaterReporter::validateReport()
{
	// Ensure user selected a category from the list
	LLSD category_sd = childGetValue("category_combo");
	U8 category = (U8)category_sd.asInteger();
	if (category == 0)
	{
		LLNotificationsUtil::add("HelpReportAbuseSelectCategory");
		return false;
	}


	if ( childGetText("abuser_name_edit").empty() )
	{
		LLNotificationsUtil::add("HelpReportAbuseAbuserNameEmpty");
		return false;
	};

	if ( childGetText("abuse_location_edit").empty() )
	{
		LLNotificationsUtil::add("HelpReportAbuseAbuserLocationEmpty");
		return false;
	};

	if ( childGetText("abuse_location_edit").empty() )
	{
		LLNotificationsUtil::add("HelpReportAbuseAbuserLocationEmpty");
		return false;
	};


	if ( childGetText("summary_edit").empty() )
	{
		LLNotificationsUtil::add("HelpReportAbuseSummaryEmpty");
		return false;
	};

	if ( childGetText("details_edit") == mDefaultSummary )
	{
		LLNotificationsUtil::add("HelpReportAbuseDetailsEmpty");
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
	if (!LLViewerLogin::getInstance()->isInProductionGrid())
	{
		summary << "Preview ";
	}

	std::string category_name;
	LLComboBox* combo = getChild<LLComboBox>( "category_combo");
	if (combo)
	{
		category_name = combo->getSelectedItemLabel(); // want label, not value
	}

#if LL_WINDOWS
	const char* platform = "Win";
#elif LL_DARWIN
	const char* platform = "Mac";
#elif LL_LINUX
	const char* platform = "Lnx";
#elif LL_SOLARIS
	const char* platform = "Sol";
	const char* short_platform = "O:S";
#else
	const char* platform = "???";
#endif



	summary << ""
		<< " |" << regionp->getName() << "|"								// region reporter is currently in.
		<< " (" << childGetText("abuse_location_edit") << ")"				// region abuse occured in (freeform text - no LLRegionPicker tool)
		<< " [" << category_name << "] "									// updated category
		<< " {" << childGetText("abuser_name_edit") << "} "					// name of abuse entered in report (chosen using LLAvatarPicker)
		<< " \"" << childGetValue("summary_edit").asString() << "\"";		// summary as entered


	std::ostringstream details;

	details << "V" << LLVersionInfo::getVersion() << std::endl << std::endl;	// client version moved to body of email for abuse reports

	std::string object_name = childGetText("object_name");
	if (!object_name.empty() && !mOwnerName.empty())
	{
		details << "Object: " << object_name << "\n";
		details << "Owner: " << mOwnerName << "\n";
	}


	details << "Abuser name: " << childGetText("abuser_name_edit") << " \n";
	details << "Abuser location: " << childGetText("abuse_location_edit") << " \n";

	details << childGetValue("details_edit").asString();

	std::string version_string;
	version_string = llformat(
			"%s %s %s %s %s",
			LLVersionInfo::getShortVersion().c_str(),
			platform,
			gSysCPU.getFamily().c_str(),
			gGLManager.mGLRenderer.c_str(),
			gGLManager.mDriverVersionVendorString.c_str());

	// only send a screenshot ID if we're asked to and the email is 
	// going to LL - Estate Owners cannot see the screenshot asset
	LLUUID screenshot_id = LLUUID::null;
	if (childGetValue("screen_check"))
	{
		screenshot_id = childGetValue("screenshot");
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

	msg->addStringFast(_PREHASH_Summary, report["summary"].asString());
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
	if( !gViewerWindow->rawSnapshot(raw, IMAGE_WIDTH, IMAGE_HEIGHT, TRUE, FALSE, TRUE, FALSE))
	{
		llwarns << "Unable to take screenshot" << llendl;
		return;
	}
	LLPointer<LLImageJ2C> upload_data = LLViewerTextureList::convertToUploadFile(raw);

	// create a resource data
	mResourceDatap->mInventoryType = LLInventoryType::IT_NONE;
	mResourceDatap->mNextOwnerPerm = 0; // not used
	mResourceDatap->mExpectedUploadCost = 0; // we expect that abuse screenshots are free
	mResourceDatap->mAssetInfo.mTransactionID.generate();
	mResourceDatap->mAssetInfo.mUuid = mResourceDatap->mAssetInfo.mTransactionID.makeAssetID(gAgent.getSecureSessionID());

	if (COMPLAINT_REPORT == mReportType)
	{
		mResourceDatap->mAssetInfo.mType = LLAssetType::AT_TEXTURE;
		mResourceDatap->mPreferredLocation = LLFolderType::EType(LLResourceData::INVALID_LOCATION);
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
	LLPointer<LLViewerFetchedTexture> image_in_list = 
		LLViewerTextureManager::getFetchedTexture(mResourceDatap->mAssetInfo.mUuid, TRUE, LLViewerTexture::BOOST_NONE, LLViewerTexture::FETCHED_TEXTURE);
	image_in_list->createGLTexture(0, raw, 0, TRUE, LLViewerTexture::OTHER);
	
	// the texture picker then uses that texture
	LLTexturePicker* texture = getChild<LLTextureCtrl>("screenshot");
	if (texture)
	{
		texture->setImageAssetID(mResourceDatap->mAssetInfo.mUuid);
		texture->setDefaultImageAssetID(mResourceDatap->mAssetInfo.mUuid);
		texture->setCaption(getString("Screenshot"));
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
void LLFloaterReporter::uploadDoneCallback(const LLUUID &uuid, void *user_data, S32 result, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLUploadDialog::modalUploadFinished();

	LLResourceData* data = (LLResourceData*)user_data;

	if(result < 0)
	{
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(result));
		LLNotificationsUtil::add("ErrorUploadingReportScreenshot", args);

		std::string err_msg("There was a problem uploading a report screenshot");
		err_msg += " due to the following reason: " + args["REASON"].asString();
		llwarns << err_msg << llendl;
		return;
	}

	EReportType report_type = UNKNOWN_REPORT;
	if (data->mPreferredLocation == LLResourceData::INVALID_LOCATION)
	{
		report_type = COMPLAINT_REPORT;
	}
	else 
	{
		llwarns << "Unknown report type : " << data->mPreferredLocation << llendl;
	}

	LLFloaterReporter *self = LLFloaterReg::findTypedInstance<LLFloaterReporter>("reporter");
	if (self)
	{
		self->mScreenID = uuid;
		llinfos << "Got screen shot " << uuid << llendl;
		self->sendReportViaLegacy(self->gatherReport());
		self->closeFloater();
	}
}


void LLFloaterReporter::setPosBox(const LLVector3d &pos)
{
	mPosition.setVec(pos);
	std::string pos_string = llformat("{%.1f, %.1f, %.1f}",
		mPosition.mV[VX],
		mPosition.mV[VY],
		mPosition.mV[VZ]);
	childSetText("pos_field", pos_string);
}

// void LLFloaterReporter::setDescription(const std::string& description, LLMeanCollisionData *mcd)
// {
// 	LLFloaterReporter *self = LLFloaterReg::findTypedInstance<LLFloaterReporter>("reporter");
// 	if (self)
// 	{
// 		self->childSetText("details_edit", description);

// 		for_each(self->mMCDList.begin(), self->mMCDList.end(), DeletePointer());
// 		self->mMCDList.clear();
// 		if (mcd)
// 		{
// 			self->mMCDList.push_back(new LLMeanCollisionData(mcd));
// 		}
// 	}
// }

// void LLFloaterReporter::addDescription(const std::string& description, LLMeanCollisionData *mcd)
// {
// 	LLFloaterReporter *self = LLFloaterReg::findTypedInstance<LLFloaterReporter>("reporter");
// 	if (self)
// 	{
// 		LLTextEditor* text = self->getChild<LLTextEditor>("details_edit");
// 		if (text)
// 		{	
// 			text->insertText(description);
// 		}
// 		if (mcd)
// 		{
// 			self->mMCDList.push_back(new LLMeanCollisionData(mcd));
// 		}
// 	}
// }
