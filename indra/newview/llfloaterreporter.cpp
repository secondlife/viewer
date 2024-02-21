/** 
 * @file llfloaterreporter.cpp
 * @brief Abuse reports.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

// self include
#include "llfloaterreporter.h"

#include <sstream>

// linden library includes
#include "llassetstorage.h"
#include "llavatarnamecache.h"
#include "llcachename.h"
#include "llcallbacklist.h"
#include "llcheckboxctrl.h"
#include "llfontgl.h"
#include "llimagepng.h"
#include "llimagej2c.h"
#include "llinventory.h"
#include "llnotificationsutil.h"
#include "llstring.h"
#include "llsys.h"
#include "llfilesystem.h"
#include "mean_collision_data.h"
#include "message.h"
#include "v3math.h"

// viewer project includes
#include "llagent.h"
#include "llbutton.h"
#include "llfloaterreg.h"
#include "lltexturectrl.h"
#include "lltexteditor.h"
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
#include "llviewercontrol.h"
#include "llviewernetwork.h"

#include "llagentui.h"

#include "lltrans.h"
#include "llexperiencecache.h"

#include "llcorehttputil.h"
#include "llviewerassetupload.h"

const std::string SCREEN_PREV_FILENAME = "screen_report_last.png";

//=========================================================================
//-----------------------------------------------------------------------------
// Support classes
//-----------------------------------------------------------------------------
class LLARScreenShotUploader : public LLResourceUploadInfo
{
public:
    LLARScreenShotUploader(LLSD report, LLUUID assetId, LLAssetType::EType assetType);

    virtual LLSD        prepareUpload();
    virtual LLSD        generatePostBody();
    virtual LLUUID      finishUpload(LLSD &result);

    virtual bool        showInventoryPanel() const { return false; }
    virtual std::string getDisplayName() const { return "Abuse Report"; }

private:

    LLSD    mReport;
};

LLARScreenShotUploader::LLARScreenShotUploader(LLSD report, LLUUID assetId, LLAssetType::EType assetType) :
    LLResourceUploadInfo(assetId, assetType, "Abuse Report"),
    mReport(report)
{
}

LLSD LLARScreenShotUploader::prepareUpload()
{
    return LLSD().with("success", LLSD::Boolean(true));
}

LLSD LLARScreenShotUploader::generatePostBody()
{   // The report was pregenerated and passed in the constructor.
    return mReport;
}

LLUUID LLARScreenShotUploader::finishUpload(LLSD &result)
{
    /* *TODO$: Report success or failure. Carried over from previous todo on responder*/
    return LLUUID::null;
}


//=========================================================================
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
	mDeselectOnClose( false ),
	mPicking( false), 
	mPosition(),
	mCopyrightWarningSeen( false ),
	mResourceDatap(new LLResourceData()),
	mAvatarNameCacheConnection()
{
	gIdleCallbacks.addFunction(onIdle, this);
}

// virtual
bool LLFloaterReporter::postBuild()
{
	LLSLURL slurl;
	LLAgentUI::buildSLURL(slurl);
	getChild<LLUICtrl>("abuse_location_edit")->setValue(slurl.getSLURLString());

	enableControls(true);

	// convert the position to a string
	LLVector3d pos = gAgent.getPositionGlobal();
	LLViewerRegion *regionp = gAgent.getRegion();
	if (regionp)
	{
		getChild<LLUICtrl>("sim_field")->setValue(regionp->getName());
		pos -= regionp->getOriginGlobal();
	}
	setPosBox(pos);

	// Default text to be blank
	getChild<LLUICtrl>("object_name")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("owner_name")->setValue(LLStringUtil::null);
	mOwnerName = LLStringUtil::null;

	getChild<LLUICtrl>("summary_edit")->setFocus(true);

	mDefaultSummary = getChild<LLUICtrl>("details_edit")->getValue().asString();

	// abuser name is selected from a list
	LLUICtrl* le = getChild<LLUICtrl>("abuser_name_edit");
	le->setEnabled( false );

	setPosBox((LLVector3d)mPosition.getValue());
	LLButton* pick_btn = getChild<LLButton>("pick_btn");
	pick_btn->setImages(std::string("tool_face.tga"),
						std::string("tool_face_active.tga") );
	childSetAction("pick_btn", onClickObjPicker, this);

	childSetAction("select_abuser", boost::bind(&LLFloaterReporter::onClickSelectAbuser, this));

	childSetAction("send_btn", onClickSend, this);
	childSetAction("cancel_btn", onClickCancel, this);
	
	// grab the user's name
	std::string reporter = LLSLURL("agent", gAgent.getID(), "inspect").getSLURLString();
	getChild<LLUICtrl>("reporter_field")->setValue(reporter);

	// request categories
	if (gAgent.getRegion()
		&& gAgent.getRegion()->capabilitiesReceived())
	{
		std::string cap_url = gAgent.getRegionCapability("AbuseCategories");

		if (!cap_url.empty())
		{
			std::string lang = gSavedSettings.getString("Language");
			if (lang != "default" && !lang.empty())
			{
				cap_url += "?lc=";
				cap_url += lang;
			}
			LLCoros::instance().launch("LLFloaterReporter::requestAbuseCategoriesCoro",
				boost::bind(LLFloaterReporter::requestAbuseCategoriesCoro, cap_url, this->getHandle()));
		}
	}

	center();

	return true;
}

// virtual
LLFloaterReporter::~LLFloaterReporter()
{
	if (mAvatarNameCacheConnection.connected())
	{
		mAvatarNameCacheConnection.disconnect();
	}
	gIdleCallbacks.deleteFunction(onIdle, this);

	// child views automatically deleted
	mObjectID 		= LLUUID::null;

	if (mPicking)
	{
		closePickTool(this);
	}

	mPosition.setVec(0.0f, 0.0f, 0.0f);

	delete mResourceDatap;
}

void LLFloaterReporter::onIdle(void* user_data)
{
	LLFloaterReporter* floater_reporter = (LLFloaterReporter*)user_data;
	if (floater_reporter)
	{
		static LLCachedControl<F32> screenshot_delay(gSavedSettings, "AbuseReportScreenshotDelay");
		if (floater_reporter->mSnapshotTimer.getStarted() && floater_reporter->mSnapshotTimer.getElapsedTimeF32() > screenshot_delay)
		{
			floater_reporter->mSnapshotTimer.stop();
			floater_reporter->takeNewSnapshot();
		}
	}
}

void LLFloaterReporter::enableControls(bool enable)
{
	getChildView("category_combo")->setEnabled(enable);
	getChildView("chat_check")->setEnabled(enable);
	getChildView("screenshot")->setEnabled(false);
	getChildView("pick_btn")->setEnabled(enable);
	getChildView("summary_edit")->setEnabled(enable);
	getChildView("details_edit")->setEnabled(enable);
	getChildView("send_btn")->setEnabled(enable);
	getChildView("cancel_btn")->setEnabled(enable);
}

void LLFloaterReporter::getExperienceInfo(const LLUUID& experience_id)
{
	mExperienceID = experience_id;

	if (LLUUID::null != mExperienceID)
	{
        const LLSD& experience = LLExperienceCache::instance().get(mExperienceID);
		std::stringstream desc;

		if(experience.isDefined())
		{
			setFromAvatarID(experience[LLExperienceCache::AGENT_ID]);
			desc << "Experience id: " << mExperienceID;
		}
		else
		{
			desc << "Unable to retrieve details for id: "<< mExperienceID;
		}
		
		LLUICtrl* details = getChild<LLUICtrl>("details_edit");
		details->setValue(desc.str());
	}
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
				mObjectID = objectp->getID();
			}

			// correct the region and position information
			LLViewerRegion *regionp = objectp->getRegion();
			if (regionp)
			{
				getChild<LLUICtrl>("sim_field")->setValue(regionp->getName());
				LLVector3d global_pos;
				global_pos.setVec(objectp->getPositionRegion());
				setPosBox(global_pos);
			}
	
			if (objectp->isAvatar())
			{
				setFromAvatarID(mObjectID);
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

void LLFloaterReporter::onClickSelectAbuser()
{
    LLView * button = findChild<LLButton>("select_abuser", true);

    LLFloater * root_floater = gFloaterView->getParentFloater(this);
	LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(boost::bind(&LLFloaterReporter::callbackAvatarID, this, _1, _2), false, true, false, root_floater->getName(), button);
	if (picker)
	{
		root_floater->addDependentFloater(picker);
	}
}

void LLFloaterReporter::callbackAvatarID(const uuid_vec_t& ids, const std::vector<LLAvatarName> names)
{
	if (ids.empty() || names.empty()) return;

	getChild<LLUICtrl>("abuser_name_edit")->setValue(names[0].getCompleteName());

	mAbuserID = ids[0];

	refresh();

}

void LLFloaterReporter::setFromAvatarID(const LLUUID& avatar_id)
{
	mAbuserID = mObjectID = avatar_id;
	std::string avatar_link = LLSLURL("agent", mObjectID, "inspect").getSLURLString();
	getChild<LLUICtrl>("owner_name")->setValue(avatar_link);

	if (mAvatarNameCacheConnection.connected())
	{
		mAvatarNameCacheConnection.disconnect();
	}
	mAvatarNameCacheConnection = LLAvatarNameCache::get(avatar_id, boost::bind(&LLFloaterReporter::onAvatarNameCache, this, _1, _2));
}

void LLFloaterReporter::onAvatarNameCache(const LLUUID& avatar_id, const LLAvatarName& av_name)
{
	mAvatarNameCacheConnection.disconnect();

	if (mObjectID == avatar_id)
	{
		mOwnerName = av_name.getCompleteName();
		getChild<LLUICtrl>("object_name")->setValue(av_name.getCompleteName());
		getChild<LLUICtrl>("object_name")->setToolTip(av_name.getCompleteName());
		getChild<LLUICtrl>("abuser_name_edit")->setValue(av_name.getCompleteName());
	}
}

void LLFloaterReporter::requestAbuseCategoriesCoro(std::string url, LLHandle<LLFloater> handle)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("requestAbuseCategoriesCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status || !result.has("categories")) // success = httpResults["success"].asBoolean();
    {
        LL_WARNS() << "Error requesting Abuse Categories from capability: " << url << LL_ENDL;
        return;
    }

    if (handle.isDead())
    {
        // nothing to do
        return;
    }

    LLFloater* floater = handle.get();
    LLComboBox* combo = floater->getChild<LLComboBox>("category_combo");
    if (!combo)
    {
        LL_WARNS() << "categories category_combo not found!" << LL_ENDL;
        return;
    }

    //get selection (in case capability took a while)
    S32 selection = combo->getCurrentIndex();

    // Combobox should have a "Select category" element;
    // This is a bit of workaround since there is no proper and simple way to save array of
    // localizable strings in xml along with data (value). For now combobox is initialized along
    // with placeholders, and first element is "Select category" which we want to keep, so remove
    // everything but first element.
    // Todo: once sim with capability fully releases, just remove this string and all unnecessary
    // items from combobox since they will be obsolete (or depending on situation remake this to
    // something better, for example move "Select category" to separate string)
    while (combo->remove(1));

    LLSD contents = result["categories"];

    LLSD::array_iterator i = contents.beginArray();
    LLSD::array_iterator iEnd = contents.endArray();
    for (; i != iEnd; ++i)
    {
        const LLSD &message_data(*i);
        std::string label = message_data["description_localized"];
        combo->add(label, message_data["category"]);
    }

    //restore selection
    combo->selectNthItem(selection);
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

				std::string details_lc = self->getChild<LLUICtrl>("details_edit")->getValue().asString();
				LLStringUtil::toLower( details_lc );
				std::string summary_lc = self->getChild<LLUICtrl>("summary_edit")->getValue().asString();
				LLStringUtil::toLower( summary_lc );
				if ( details_lc.find( "copyright" ) != std::string::npos ||
					summary_lc.find( "copyright" ) != std::string::npos  ||
					category_value == IP_CONTENT_REMOVAL ||
					category_value == IP_PERMISSONS_EXPLOIT)
				{
					LLNotificationsUtil::add("HelpReportAbuseContainsCopyright");
					self->mCopyrightWarningSeen = true;
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

		LLUploadDialog::modalUploadDialog(LLTrans::getString("uploading_abuse_report"));
		// *TODO don't upload image if checkbox isn't checked
		std::string url = gAgent.getRegionCapability("SendUserReport");
		std::string sshot_url = gAgent.getRegionCapability("SendUserReportWithScreenshot");
		if(!url.empty() || !sshot_url.empty())
		{
			self->sendReportViaCaps(url, sshot_url, self->gatherReport());
			LLNotificationsUtil::add("HelpReportAbuseConfirm");
			self->closeFloater();
		}
		else
		{
			self->getChildView("send_btn")->setEnabled(false);
			self->getChildView("cancel_btn")->setEnabled(false);
			// the callback from uploading the image calls sendReportViaLegacy()
			self->uploadImage();
		}
	}
}


// static
void LLFloaterReporter::onClickCancel(void *userdata)
{
	LLFloaterReporter *self = (LLFloaterReporter *)userdata;

	// reset flag in case the next report also contains this text
	self->mCopyrightWarningSeen = false;

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
	self->mPicking = true;
	self->getChild<LLUICtrl>("object_name")->setValue(LLStringUtil::null);
	self->getChild<LLUICtrl>("owner_name")->setValue(LLStringUtil::null);
	self->mOwnerName = LLStringUtil::null;
	LLButton* pick_btn = self->getChild<LLButton>("pick_btn");
	if (pick_btn) pick_btn->setToggleState(true);
}


// static
void LLFloaterReporter::closePickTool(void *userdata)
{
	LLFloaterReporter *self = (LLFloaterReporter *)userdata;

	LLUUID object_id = LLToolObjPicker::getInstance()->getObjectID();
	self->getObjectInfo(object_id);

	LLToolMgr::getInstance()->clearTransientTool();
	self->mPicking = false;
	LLButton* pick_btn = self->getChild<LLButton>("pick_btn");
	if (pick_btn) pick_btn->setToggleState(false);
}


// static
void LLFloaterReporter::showFromMenu(EReportType report_type)
{
	if (COMPLAINT_REPORT != report_type)
	{
		LL_WARNS() << "Unknown LLViewerReporter type : " << report_type << LL_ENDL;
		return;
	}
	LLFloaterReporter* reporter_floater = LLFloaterReg::findTypedInstance<LLFloaterReporter>("reporter");
	if(reporter_floater && reporter_floater->isInVisibleChain())
	{
		gSavedPerAccountSettings.setBOOL("PreviousScreenshotForReport", false);
	}
	reporter_floater = LLFloaterReg::showTypedInstance<LLFloaterReporter>("reporter", LLSD());
	if (reporter_floater)
	{
		reporter_floater->setReportType(report_type);
	}
}

// static
void LLFloaterReporter::show(const LLUUID& object_id, const std::string& avatar_name, const LLUUID& experience_id)
{
	LLFloaterReporter* reporter_floater = LLFloaterReg::findTypedInstance<LLFloaterReporter>("reporter");
	if(reporter_floater && reporter_floater->isInVisibleChain())
	{
		gSavedPerAccountSettings.setBOOL("PreviousScreenshotForReport", false);
	}
	reporter_floater = LLFloaterReg::showTypedInstance<LLFloaterReporter>("reporter");
	if (avatar_name.empty())
	{
		// Request info for this object
		reporter_floater->getObjectInfo(object_id);
	}
	else
	{
		reporter_floater->setFromAvatarID(object_id);
	}
	if(experience_id.notNull())
	{
		reporter_floater->getExperienceInfo(experience_id);
	}

	// Need to deselect on close
	reporter_floater->mDeselectOnClose = true;
}



void LLFloaterReporter::showFromExperience( const LLUUID& experience_id )
{
	LLFloaterReporter* reporter_floater = LLFloaterReg::findTypedInstance<LLFloaterReporter>("reporter");
	if(reporter_floater && reporter_floater->isInVisibleChain())
	{
		gSavedPerAccountSettings.setBOOL("PreviousScreenshotForReport", false);
	}
	reporter_floater = LLFloaterReg::showTypedInstance<LLFloaterReporter>("reporter");
	reporter_floater->getExperienceInfo(experience_id);

	// Need to deselect on close
	reporter_floater->mDeselectOnClose = true;
}


// static
void LLFloaterReporter::showFromObject(const LLUUID& object_id, const LLUUID& experience_id)
{
	show(object_id, LLStringUtil::null, experience_id);
}

// static
void LLFloaterReporter::showFromAvatar(const LLUUID& avatar_id, const std::string avatar_name)
{
	show(avatar_id, avatar_name);
}

// static
void LLFloaterReporter::showFromChat(const LLUUID& avatar_id, const std::string& avatar_name, const std::string& time, const std::string& description)
{
    show(avatar_id, avatar_name);

    LLStringUtil::format_map_t args;
    args["[MSG_TIME]"] = time;
    args["[MSG_DESCRIPTION]"] = description;

    LLFloaterReporter *self = LLFloaterReg::findTypedInstance<LLFloaterReporter>("reporter");
    if (self)
    {
        std::string description = self->getString("chat_report_format", args);
        self->getChild<LLUICtrl>("details_edit")->setValue(description);
    }
}

void LLFloaterReporter::setPickedObjectProperties(const std::string& object_name, const std::string& owner_name, const LLUUID owner_id)
{
	getChild<LLUICtrl>("object_name")->setValue(object_name);
	std::string owner_link =
		LLSLURL("agent", owner_id, "inspect").getSLURLString();
	getChild<LLUICtrl>("owner_name")->setValue(owner_link);
	getChild<LLUICtrl>("abuser_name_edit")->setValue(owner_name);
	mAbuserID = owner_id;
	mOwnerName = owner_name;
}


bool LLFloaterReporter::validateReport()
{
	// Ensure user selected a category from the list
	LLSD category_sd = getChild<LLUICtrl>("category_combo")->getValue();
	U8 category = (U8)category_sd.asInteger();
	if (category == 0)
	{
		LLNotificationsUtil::add("HelpReportAbuseSelectCategory");
		return false;
	}


	if ( getChild<LLUICtrl>("abuser_name_edit")->getValue().asString().empty() )
	{
		LLNotificationsUtil::add("HelpReportAbuseAbuserNameEmpty");
		return false;
	};

	if ( getChild<LLUICtrl>("abuse_location_edit")->getValue().asString().empty() )
	{
		LLNotificationsUtil::add("HelpReportAbuseAbuserLocationEmpty");
		return false;
	};

	if ( getChild<LLUICtrl>("abuse_location_edit")->getValue().asString().empty() )
	{
		LLNotificationsUtil::add("HelpReportAbuseAbuserLocationEmpty");
		return false;
	};


	if ( getChild<LLUICtrl>("summary_edit")->getValue().asString().empty() )
	{
		LLNotificationsUtil::add("HelpReportAbuseSummaryEmpty");
		return false;
	};

	if ( getChild<LLUICtrl>("details_edit")->getValue().asString() == mDefaultSummary )
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
	mCopyrightWarningSeen = false;

	std::ostringstream summary;
	if (!LLGridManager::getInstance()->isInProductionGrid())
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
#else
	const char* platform = "???";
#endif



	summary << ""
		<< " |" << regionp->getName() << "|"								// region reporter is currently in.
		<< " (" << getChild<LLUICtrl>("abuse_location_edit")->getValue().asString() << ")"				// region abuse occured in (freeform text - no LLRegionPicker tool)
		<< " [" << category_name << "] "									// updated category
		<< " {" << getChild<LLUICtrl>("abuser_name_edit")->getValue().asString() << "} "					// name of abuse entered in report (chosen using LLAvatarPicker)
		<< " \"" << getChild<LLUICtrl>("summary_edit")->getValue().asString() << "\"";		// summary as entered


	std::ostringstream details;

	details << "V" << LLVersionInfo::instance().getVersion() << std::endl << std::endl;	// client version moved to body of email for abuse reports

	std::string object_name = getChild<LLUICtrl>("object_name")->getValue().asString();
	if (!object_name.empty() && !mOwnerName.empty())
	{
		details << "Object: " << object_name << "\n";
		details << "Owner: " << mOwnerName << "\n";
	}


	details << "Abuser name: " << getChild<LLUICtrl>("abuser_name_edit")->getValue().asString() << " \n";
	details << "Abuser location: " << getChild<LLUICtrl>("abuse_location_edit")->getValue().asString() << " \n";

	details << getChild<LLUICtrl>("details_edit")->getValue().asString();

	std::string version_string;
	version_string = llformat(
			"%s %s %s %s %s",
			LLVersionInfo::instance().getShortVersion().c_str(),
			platform,
			gSysCPU.getFamily().c_str(),
			gGLManager.mGLRenderer.c_str(),
			gGLManager.mDriverVersionVendorString.c_str());

	// only send a screenshot ID if we're asked to and the email is 
	// going to LL - Estate Owners cannot see the screenshot asset
	LLUUID screenshot_id = LLUUID::null;
	screenshot_id = getChild<LLUICtrl>("screenshot")->getValue();

	LLSD report = LLSD::emptyMap();
	report["report-type"] = (U8) mReportType;
	report["category"] = getChild<LLUICtrl>("category_combo")->getValue();
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

void LLFloaterReporter::finishedARPost(const LLSD &)
{
    LLUploadDialog::modalUploadFinished();

}

void LLFloaterReporter::sendReportViaCaps(std::string url, std::string sshot_url, const LLSD& report)
{
	if(!sshot_url.empty())
    {
		// try to upload screenshot
        LLResourceUploadInfo::ptr_t uploadInfo(new  LLARScreenShotUploader(report, mResourceDatap->mAssetInfo.mUuid, mResourceDatap->mAssetInfo.mType));
        LLViewerAssetUpload::EnqueueInventoryUpload(sshot_url, uploadInfo);
	}
	else
	{
        LLCoreHttpUtil::HttpCoroutineAdapter::completionCallback_t proc = boost::bind(&LLFloaterReporter::finishedARPost, _1);
        LLUploadDialog::modalUploadDialog("Abuse Report");
        LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpPost(url, report, proc, proc);
	}
}

void LLFloaterReporter::takeScreenshot(bool use_prev_screenshot)
{
	gSavedPerAccountSettings.setBOOL("PreviousScreenshotForReport", true);
	if(!use_prev_screenshot)
	{
		std::string screenshot_filename(gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter() + SCREEN_PREV_FILENAME);
		LLPointer<LLImagePNG> png_image = new LLImagePNG;
		if(png_image->encode(mImageRaw, 0.0f))
		{
			png_image->save(screenshot_filename);
		}
	}
	else
	{
		mImageRaw = mPrevImageRaw;
	}

	LLPointer<LLImageJ2C> upload_data = LLViewerTextureList::convertToUploadFile(mImageRaw);

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
		LL_WARNS() << "Unknown LLFloaterReporter type" << LL_ENDL;
	}
	mResourceDatap->mAssetInfo.mCreatorID = gAgentID;
	mResourceDatap->mAssetInfo.setName("screenshot_name");
	mResourceDatap->mAssetInfo.setDescription("screenshot_descr");

	// store in cache
    LLFileSystem j2c_file(mResourceDatap->mAssetInfo.mUuid, mResourceDatap->mAssetInfo.mType, LLFileSystem::WRITE);
    j2c_file.write(upload_data->getData(), upload_data->getDataSize());

	// store in the image list so it doesn't try to fetch from the server
	LLPointer<LLViewerFetchedTexture> image_in_list = 
		LLViewerTextureManager::getFetchedTexture(mResourceDatap->mAssetInfo.mUuid);
	image_in_list->createGLTexture(0, mImageRaw, 0, true, LLGLTexture::OTHER);
	
	// the texture picker then uses that texture
	LLTextureCtrl* texture = getChild<LLTextureCtrl>("screenshot");
	if (texture)
	{
		texture->setImageAssetID(mResourceDatap->mAssetInfo.mUuid);
		texture->setDefaultImageAssetID(mResourceDatap->mAssetInfo.mUuid);
		texture->setCaption(getString("Screenshot"));
	}
}

void LLFloaterReporter::takeNewSnapshot()
{
	childSetEnabled("send_btn", true);
	mImageRaw = new LLImageRaw;
	const S32 IMAGE_WIDTH = 1024;
	const S32 IMAGE_HEIGHT = 768;

	// Take a screenshot, but don't draw this floater.
	setVisible(false);
    if (!gViewerWindow->rawSnapshot(mImageRaw,IMAGE_WIDTH, IMAGE_HEIGHT, true, false, true /*UI*/, true, false))
	{
		LL_WARNS() << "Unable to take screenshot" << LL_ENDL;
		setVisible(true);
		return;
	}
	setVisible(true);

	if(gSavedPerAccountSettings.getBOOL("PreviousScreenshotForReport"))
	{
		std::string screenshot_filename(gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter() + SCREEN_PREV_FILENAME);
		mPrevImageRaw = new LLImageRaw;
		LLPointer<LLImagePNG> start_image_png = new LLImagePNG;
		if(start_image_png->load(screenshot_filename))
		{
			if (start_image_png->decode(mPrevImageRaw, 0.0f))
			{
				LLNotificationsUtil::add("LoadPreviousReportScreenshot", LLSD(), LLSD(), boost::bind(&LLFloaterReporter::onLoadScreenshotDialog,this, _1, _2));
				return;
			}
		}
	}
	takeScreenshot();
}


void LLFloaterReporter::onOpen(const LLSD& key)
{
	childSetEnabled("send_btn", false);
	//Time delay to avoid UI artifacts. MAINT-7067
	mSnapshotTimer.start();
}

void LLFloaterReporter::onLoadScreenshotDialog(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	takeScreenshot(option == 0);
}

void LLFloaterReporter::uploadImage()
{
	LL_INFOS() << "*** Uploading: " << LL_ENDL;
	LL_INFOS() << "Type: " << LLAssetType::lookup(mResourceDatap->mAssetInfo.mType) << LL_ENDL;
	LL_INFOS() << "UUID: " << mResourceDatap->mAssetInfo.mUuid << LL_ENDL;
	LL_INFOS() << "Name: " << mResourceDatap->mAssetInfo.getName() << LL_ENDL;
	LL_INFOS() << "Desc: " << mResourceDatap->mAssetInfo.getDescription() << LL_ENDL;

	gAssetStorage->storeAssetData(mResourceDatap->mAssetInfo.mTransactionID,
									mResourceDatap->mAssetInfo.mType,
									LLFloaterReporter::uploadDoneCallback,
									(void*)mResourceDatap, true);
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
		LL_WARNS() << err_msg << LL_ENDL;
		return;
	}

	if (data->mPreferredLocation != LLResourceData::INVALID_LOCATION)
	{
		LL_WARNS() << "Unknown report type : " << data->mPreferredLocation << LL_ENDL;
	}

	LLFloaterReporter *self = LLFloaterReg::findTypedInstance<LLFloaterReporter>("reporter");
	if (self)
	{
		self->mScreenID = uuid;
		LL_INFOS() << "Got screen shot " << uuid << LL_ENDL;
		self->sendReportViaLegacy(self->gatherReport());
		LLNotificationsUtil::add("HelpReportAbuseConfirm");
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
	getChild<LLUICtrl>("pos_field")->setValue(pos_string);
}

void LLFloaterReporter::onClose(bool app_quitting)
{
	mSnapshotTimer.stop();
	gSavedPerAccountSettings.setBOOL("PreviousScreenshotForReport", app_quitting);
}
