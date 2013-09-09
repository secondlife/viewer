/** 
 * @file llfloaterexperienceprofile.cpp
 * @brief llfloaterexperienceprofile and related class definitions
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

#include "llagent.h"
#include "llexpandabletextbox.h"
#include "llexperiencecache.h"
#include "llfloaterexperienceprofile.h"
#include "llfloaterreg.h"
#include "llhttpclient.h"
#include "lllayoutstack.h"
#include "llsdserialize.h"
#include "llslurl.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewerregion.h"
#include "lllineeditor.h"
#include "llcombobox.h"
#include "llcheckboxctrl.h"
#include "llnotificationsutil.h"
#include "llappviewer.h"

#define XML_PANEL_EXPERIENCE_PROFILE "floater_experienceprofile.xml"
#define TF_NAME "experience_title"
#define TF_DESC "experience_description"
#define TF_SLURL "LocationTextText"
#define TF_MRKT "marketplace"
#define TF_MATURITY "ContentRatingText"
#define TF_OWNER "OwnerText"
#define EDIT "edit_"

#define IMG_LOGO "logo"

#define PNL_IMAGE "image_panel"
#define PNL_DESC "description panel"
#define PNL_LOC "location panel"
#define PNL_MRKT "marketplace panel"

#define BTN_EDIT "edit_btn"
#define BTN_ALLOW "allow_btn"
#define BTN_FORGET "forget_btn"
#define BTN_BLOCK "block_btn"
#define BTN_CANCEL "cancel_btn"
#define BTN_SAVE "save_btn"
#define BTN_ENABLE "enable_btn"
#define BTN_PRIVATE "private_btn"


LLFloaterExperienceProfile::LLFloaterExperienceProfile(const LLSD& data)
    : LLFloater(data)
    , mExperienceId(data.asUUID())
    , mImagePanel(NULL)
    , mDescriptionPanel(NULL)
    , mLocationPanel(NULL)
    , mMarketplacePanel(NULL)
    , mSaveCompleteAction(NOTHING)
    , mDirty(false)
    , mForceClose(false)
{

}


LLFloaterExperienceProfile::~LLFloaterExperienceProfile()
{

}

template<class T> 
class HandleResponder : public LLHTTPClient::Responder
{
public:
    HandleResponder(const LLHandle<T>& parent):mParent(parent){}
    LLHandle<T> mParent;

    virtual void error(U32 status, const std::string& reason)
    {
        llwarns << "HandleResponder failed with code: " << status<< ", reason: " << reason << llendl;
    }
};

class ExperienceUpdateResponder : public HandleResponder<LLFloaterExperienceProfile>
{
public:
    ExperienceUpdateResponder(const LLHandle<LLFloaterExperienceProfile>& parent):HandleResponder<LLFloaterExperienceProfile>(parent)
    {
    }

    virtual void result(const LLSD& content)
    {
        LLFloaterExperienceProfile* parent=mParent.get();
        if(parent)
        {
            parent->onSaveComplete(content);
        }
    }
};



class ExperiencePreferencesResponder : public HandleResponder<LLFloaterExperienceProfile>
{
public:
    ExperiencePreferencesResponder(const LLHandle<LLFloaterExperienceProfile>& parent):HandleResponder<LLFloaterExperienceProfile>(parent)
    {
    }


    virtual void result(const LLSD& content)
    {
        LLFloaterExperienceProfile* parent=mParent.get();
        if(parent)
        {
            parent->setPreferences(content);
        }
    }
};


class IsAdminResponder : public HandleResponder<LLFloaterExperienceProfile>
{
public:
    IsAdminResponder(const LLHandle<LLFloaterExperienceProfile>& parent):HandleResponder<LLFloaterExperienceProfile>(parent)
    {
    }
    
    virtual void result(const LLSD& content)
    {
        LLFloaterExperienceProfile* parent = mParent.get();
        if(!parent)
            return;

        bool enabled = true;
        LLViewerRegion* region = gAgent.getRegion();
        if (!region)
        {
            enabled = false;
        }
        else
        {
            std::string url=region->getCapability("UpdateExperience"); 
            if(url.empty())
                enabled = false;
        }
        
        parent->getChild<LLButton>(BTN_EDIT)->setVisible(enabled && content["status"].asBoolean());
    }
};

BOOL LLFloaterExperienceProfile::postBuild()
{
    mImagePanel = getChild<LLLayoutPanel>(PNL_IMAGE);
    mDescriptionPanel = getChild<LLLayoutPanel>(PNL_DESC);
    mLocationPanel = getChild<LLLayoutPanel>(PNL_LOC);
    mMarketplacePanel = getChild<LLLayoutPanel>(PNL_MRKT);  

    if (mExperienceId.notNull())
    {
        LLExperienceCache::fetch(mExperienceId, true);
        LLExperienceCache::get(mExperienceId, boost::bind(&LLFloaterExperienceProfile::experienceCallback, 
            getDerivedHandle<LLFloaterExperienceProfile>(), _1)); 
        
        LLViewerRegion* region = gAgent.getRegion();
        if (region)
        {
            std::string lookup_url=region->getCapability("IsExperienceAdmin"); 
            if(!lookup_url.empty())
            {
                LLHTTPClient::get(lookup_url+"?experience_id="+mExperienceId.asString(), new IsAdminResponder(getDerivedHandle<LLFloaterExperienceProfile>()));
            }

            lookup_url=region->getCapability("ExperiencePreferences"); 
            if(!lookup_url.empty())
            {
                LLHTTPClient::get(lookup_url+"?"+mExperienceId.asString(), new ExperiencePreferencesResponder(getDerivedHandle<LLFloaterExperienceProfile>()));
            }
        }
    }

    childSetAction(BTN_EDIT, boost::bind(&LLFloaterExperienceProfile::onClickEdit, this));
    childSetAction(BTN_ALLOW, boost::bind(&LLFloaterExperienceProfile::onClickPermission, this, "Allow"));
    childSetAction(BTN_FORGET, boost::bind(&LLFloaterExperienceProfile::onClickForget, this));
    childSetAction(BTN_BLOCK, boost::bind(&LLFloaterExperienceProfile::onClickPermission, this, "Block"));
    childSetAction(BTN_CANCEL, boost::bind(&LLFloaterExperienceProfile::onClickCancel, this));
    childSetAction(BTN_SAVE, boost::bind(&LLFloaterExperienceProfile::onClickSave, this));


    getChild<LLUICtrl>(EDIT TF_NAME)->setCommitCallback(boost::bind(&LLFloaterExperienceProfile::onFieldChanged, this));
    getChild<LLUICtrl>(EDIT TF_DESC)->setCommitCallback(boost::bind(&LLFloaterExperienceProfile::onFieldChanged, this));
    getChild<LLUICtrl>(EDIT TF_SLURL)->setCommitCallback(boost::bind(&LLFloaterExperienceProfile::onFieldChanged, this));
    getChild<LLUICtrl>(EDIT TF_MATURITY)->setCommitCallback(boost::bind(&LLFloaterExperienceProfile::onFieldChanged, this));
    getChild<LLUICtrl>(EDIT TF_MRKT)->setCommitCallback(boost::bind(&LLFloaterExperienceProfile::onFieldChanged, this));
    getChild<LLLineEditor>(EDIT TF_NAME)->setCommitCallback(boost::bind(&LLFloaterExperienceProfile::onFieldChanged, this));
    childSetAction(EDIT BTN_ENABLE, boost::bind(&LLFloaterExperienceProfile::onFieldChanged, this));
    childSetAction(EDIT BTN_PRIVATE, boost::bind(&LLFloaterExperienceProfile::onFieldChanged, this));

    getChild<LLTextEditor>(EDIT TF_DESC)->setCommitOnFocusLost(TRUE);
    
    return TRUE;
}

void LLFloaterExperienceProfile::experienceCallback(LLHandle<LLFloaterExperienceProfile> handle,  const LLSD& experience )
{
    LLFloaterExperienceProfile* pllpep = handle.get();
    if(pllpep)
    {
        pllpep->refreshExperience(experience);
    }
}

void LLFloaterExperienceProfile::onClickEdit()
{
    LLTabContainer* tabs = getChild<LLTabContainer>("tab_container");

    tabs->selectTabByName("edit_panel_experience_info");
}


void LLFloaterExperienceProfile::onClickCancel()
{
    changeToView();
}

void LLFloaterExperienceProfile::onClickSave()
{
    doSave(NOTHING);
}


void LLFloaterExperienceProfile::onClickPermission(const char* perm)
{
    LLViewerRegion* region = gAgent.getRegion();
    if (!region)
        return;
   
    std::string lookup_url=region->getCapability("ExperiencePreferences"); 
    if(lookup_url.empty())
        return;
    LLSD permission;
    LLSD data;
    permission["permission"]=perm;

    data[mExperienceId.asString()]=permission;
    LLHTTPClient::put(lookup_url, data, new ExperiencePreferencesResponder(getDerivedHandle<LLFloaterExperienceProfile>()));
   
}


void LLFloaterExperienceProfile::onClickForget()
{
    LLViewerRegion* region = gAgent.getRegion();
    if (!region)
        return;

    std::string lookup_url=region->getCapability("ExperiencePreferences"); 
    if(lookup_url.empty())
        return;

    LLHTTPClient::del(lookup_url+"?"+mExperienceId.asString(), new ExperiencePreferencesResponder(getDerivedHandle<LLFloaterExperienceProfile>()));
}

bool LLFloaterExperienceProfile::setMaturityString( U8 maturity, LLTextBox* child, LLComboBox* combo )
{
    LLStyle::Params style;
    std::string access;
    if(maturity <= SIM_ACCESS_PG)
    {
        style.image(LLUI::getUIImage(getString("maturity_icon_general")));
        access = LLTrans::getString("SIM_ACCESS_PG");
        combo->setCurrentByIndex(2);
    }
    else if(maturity <= SIM_ACCESS_MATURE)
    {
        style.image(LLUI::getUIImage(getString("maturity_icon_moderate")));
        access = LLTrans::getString("SIM_ACCESS_MATURE");
        combo->setCurrentByIndex(1);
    }
    else if(maturity <= SIM_ACCESS_ADULT)
    {
        style.image(LLUI::getUIImage(getString("maturity_icon_adult")));
        access = LLTrans::getString("SIM_ACCESS_ADULT");
        combo->setCurrentByIndex(0);
    }
    else
    {
        return false;
    }

    child->setText(LLStringUtil::null);

    child->appendImageSegment(style);

    child->appendText(access, false);
    
    return true;
}


void LLFloaterExperienceProfile::refreshExperience( const LLSD& experience )
{
    mExperienceDetails = experience;

    if(experience.has(LLExperienceCache::MISSING))
    {
        mImagePanel->setVisible(FALSE);
        mDescriptionPanel->setVisible(FALSE);
        mLocationPanel->setVisible(FALSE);
        mMarketplacePanel->setVisible(FALSE);
    }

    LLTextBox* child = getChild<LLTextBox>(TF_NAME);
    child->setText(experience[LLExperienceCache::NAME].asString());
    
    LLLineEditor* linechild = getChild<LLLineEditor>(EDIT TF_NAME);
    linechild->setText(experience[LLExperienceCache::NAME].asString());
    
    std::string value = experience[LLExperienceCache::DESCRIPTION].asString();
    LLExpandableTextBox* exchild = getChild<LLExpandableTextBox>(TF_DESC);
    exchild->setText(value);
    mDescriptionPanel->setVisible(value.length()>0);

    LLTextEditor* edit_child = getChild<LLTextEditor>(EDIT TF_DESC);
    edit_child->setText(value);
   
    value = experience[LLExperienceCache::SLURL].asString();
    child = getChild<LLTextBox>(TF_SLURL);
    child->setText(value);
    mLocationPanel->setVisible(value.length()>0);

    linechild = getChild<LLLineEditor>(EDIT TF_SLURL);
    linechild->setText(value);
    
    setMaturityString((U8)(experience[LLExperienceCache::MATURITY].asInteger()), getChild<LLTextBox>(TF_MATURITY), getChild<LLComboBox>(EDIT TF_MATURITY));

    child = getChild<LLTextBox>(TF_OWNER);

    LLUUID id = experience[LLExperienceCache::GROUP_ID].asUUID();
    if(id.notNull())
    {
        value = LLSLURL("group", id, "inspect").getSLURLString();
    }
    else
    {
        id = experience[LLExperienceCache::AGENT_ID].asUUID();
        value = LLSLURL("agent", id, "inspect").getSLURLString();
    }
    child->setText(value);
        
    value=experience[LLExperienceCache::METADATA].asString();
    if(value.empty())
        return;
    
    LLPointer<LLSDParser> parser = new LLSDXMLParser();

    LLSD data;

    std::istringstream is(value);
    if(LLSDParser::PARSE_FAILURE != parser->parse(is, data, value.size()))
    {
        value="";
        if(data.has(TF_MRKT))
        {
            value=data[TF_MRKT].asString();

            child = getChild<LLTextBox>(TF_MRKT);
            child->setText(value);
            mMarketplacePanel->setVisible(TRUE);
        }
        else
        {
            mMarketplacePanel->setVisible(FALSE);
        }
        
        linechild = getChild<LLLineEditor>(EDIT TF_MRKT);
        linechild->setText(value);

        if(data.has(IMG_LOGO))
        {
            LLTextureCtrl* logo = getChild<LLTextureCtrl>(IMG_LOGO);
            logo->setImageAssetID(data[IMG_LOGO].asUUID());
            mImagePanel->setVisible(TRUE);

            logo = getChild<LLTextureCtrl>(EDIT IMG_LOGO);
            logo->setImageAssetID(data[IMG_LOGO].asUUID());
        }
    }

    LLCheckBoxCtrl* enable = getChild<LLCheckBoxCtrl>(EDIT BTN_ENABLE);
    enable->set( 0 == (mExperienceDetails[LLExperienceCache::PROPERTIES].asInteger() & LLExperienceCache::PROPERTY_DISABLED)); 
    
    enable = getChild<LLCheckBoxCtrl>(EDIT BTN_PRIVATE);
    enable->set( 0 != (mExperienceDetails[LLExperienceCache::PROPERTIES].asInteger() & LLExperienceCache::PROPERTY_PRIVATE));  

    mDirty=false;
    setCanClose(!mDirty);

    getChild<LLButton>(BTN_SAVE)->setEnabled(mDirty);
}

void LLFloaterExperienceProfile::setPreferences( const LLSD& content )
{
    const LLSD& experiences = content["experiences"];
    const LLSD& blocked = content["blocked"];
    LLButton* button;


    for(LLSD::array_const_iterator it = experiences.beginArray(); it != experiences.endArray() ; ++it)
    {
        if(it->asUUID()==mExperienceId)
        {
            button=getChild<LLButton>(BTN_ALLOW);
            button->setEnabled(FALSE);

            button=getChild<LLButton>(BTN_FORGET);
            button->setEnabled(TRUE);

            button=getChild<LLButton>(BTN_BLOCK);
            button->setEnabled(TRUE);
            return;
        }
    }

    for(LLSD::array_const_iterator it = blocked.beginArray(); it != blocked.endArray() ; ++it)
    {
        if(it->asUUID()==mExperienceId)
        {
            button=getChild<LLButton>(BTN_ALLOW);
            button->setEnabled(TRUE);

            button=getChild<LLButton>(BTN_FORGET);
            button->setEnabled(TRUE);

            button=getChild<LLButton>(BTN_BLOCK);
            button->setEnabled(FALSE);
            return;
        }
    }


    button=getChild<LLButton>(BTN_ALLOW);
    button->setEnabled(TRUE);

    button=getChild<LLButton>(BTN_FORGET);
    button->setEnabled(FALSE);

    button=getChild<LLButton>(BTN_BLOCK);
    button->setEnabled(TRUE);
}

void LLFloaterExperienceProfile::onFieldChanged()
{
    mDirty=true;
    setCanClose(!mDirty);
    getChild<LLButton>(BTN_SAVE)->setEnabled(mDirty);
}


BOOL LLFloaterExperienceProfile::canClose()
{
    if(mForceClose || !mDirty)
    {
        return TRUE;
    }
    else
    {
        // Bring up view-modal dialog: Save changes? Yes, No, Cancel
        LLNotificationsUtil::add("SaveChanges", LLSD(), LLSD(), boost::bind(&LLFloaterExperienceProfile::handleSaveChangesDialog, this, _1, _2, CLOSE));
        return FALSE;
    }
}

bool LLFloaterExperienceProfile::handleSaveChangesDialog( const LLSD& notification, const LLSD& response, int action )
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    switch( option )
    {
    case 0:  // "Yes"
        // close after saving
        doSave( action );
        break;

    case 1:  // "No"
        if(action != NOTHING)
        {
            mForceClose = TRUE;
            if(action==CLOSE)
            {
                closeFloater();
            }
            else
            {
                changeToView();
            }
        }
        break;

    case 2: // "Cancel"
    default:
        // If we were quitting, we didn't really mean it.
        LLAppViewer::instance()->abortQuit();
        break;
    }
    return false;
}

void LLFloaterExperienceProfile::doSave( int success_action )
{
    mSaveCompleteAction=success_action;

    LLViewerRegion* region = gAgent.getRegion();
    if (!region)
        return;

    std::string url=region->getCapability("UpdateExperience"); 
    if(url.empty())
        return;

    LLSD package=mExperienceDetails;

    package[LLExperienceCache::NAME] = getChild<LLLineEditor>(EDIT TF_NAME)->getText();
    package[LLExperienceCache::DESCRIPTION] = getChild<LLTextEditor>(EDIT TF_DESC)->getText();
    package[LLExperienceCache::SLURL] = getChild<LLLineEditor>(EDIT TF_SLURL)->getText();

    package[LLExperienceCache::MATURITY] = getChild<LLComboBox>(EDIT TF_MATURITY)->getSelectedValue().asInteger();

    LLSD metadata;

    metadata[TF_MRKT] = getChild<LLLineEditor>(EDIT TF_MRKT)->getText();
    metadata[IMG_LOGO] = getChild<LLTextureCtrl>(EDIT IMG_LOGO)->getImageAssetID();

    LLPointer<LLSDXMLFormatter> formatter = new LLSDXMLFormatter();

    std::ostringstream os;
    if(formatter->format(metadata, os))
    {
        package[LLExperienceCache::METADATA]=os.str();
    }

    int properties = package[LLExperienceCache::PROPERTIES].asInteger();
    LLCheckBoxCtrl* enable = getChild<LLCheckBoxCtrl>(EDIT BTN_ENABLE);
    if(enable->get())
    {
        properties &= ~LLExperienceCache::PROPERTY_DISABLED;
    }
    else
    {
        properties |= LLExperienceCache::PROPERTY_DISABLED;
    }

    enable = getChild<LLCheckBoxCtrl>(EDIT BTN_PRIVATE);
    if(enable->get())
    {
        properties |= LLExperienceCache::PROPERTY_PRIVATE;
    }
    else
    {
        properties &= ~LLExperienceCache::PROPERTY_PRIVATE;
    }

    package[LLExperienceCache::PROPERTIES] = properties; 

    LLHTTPClient::post(url, package, new ExperienceUpdateResponder(getDerivedHandle<LLFloaterExperienceProfile>()));
}

void LLFloaterExperienceProfile::onSaveComplete( const LLSD& content )
{
    LLUUID id = getExperienceId();

    if(!content.has("experience_keys"))
    {
        llwarns << "LLFloaterExperienceProfile::onSaveComplete called with bad content" << llendl;
        return;
    }

    const LLSD& experiences = content["experience_keys"];

    LLSD::array_const_iterator it = experiences.beginArray();
    if(it == experiences.endArray())
    {
        llwarns << "LLFloaterExperienceProfile::onSaveComplete called with empty content" << llendl;
        return;
    }

    if(!it->has(LLExperienceCache::EXPERIENCE_ID) || ((*it)[LLExperienceCache::EXPERIENCE_ID].asUUID() != id))
    {
        llwarns << "LLFloaterExperienceProfile::onSaveComplete called with unexpected experience id" << llendl;
        return;
    }
 
    refreshExperience(*it);
    LLExperienceCache::fetch(id, true);

    if(mSaveCompleteAction==VIEW)
    {
        LLTabContainer* tabs = getChild<LLTabContainer>("tab_container");
        tabs->selectTabByName("panel_experience_info");
    }
    else if(mSaveCompleteAction == CLOSE)
    {
        closeFloater();
    }   
}

void LLFloaterExperienceProfile::changeToView()
{
    if(mForceClose || !mDirty)
    {
        refreshExperience(mExperienceDetails);
        LLTabContainer* tabs = getChild<LLTabContainer>("tab_container");

        tabs->selectTabByName("panel_experience_info");
    }
    else
    {
        // Bring up view-modal dialog: Save changes? Yes, No, Cancel
        LLNotificationsUtil::add("SaveChanges", LLSD(), LLSD(), boost::bind(&LLFloaterExperienceProfile::handleSaveChangesDialog, this, _1, _2, VIEW));
    }
}
