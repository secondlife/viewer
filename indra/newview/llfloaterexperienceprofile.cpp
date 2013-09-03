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

#include "llexpandabletextbox.h"
#include "llexperiencecache.h"
#include "llfloaterexperienceprofile.h"
#include "llfloaterreg.h"
#include "lllayoutstack.h"
#include "llsdserialize.h"
#include "llslurl.h"
#include "lltextbox.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewerregion.h"
#include "llagent.h"
#include "llhttpclient.h"

#define XML_PANEL_EXPERIENCE_PROFILE "floater_experienceprofile.xml"
#define TF_NAME "experience_title"
#define TF_DESC "experience_description"
#define TF_SLURL "LocationTextText"
#define TF_MRKT "marketplace"
#define TF_MATURITY "ContentRatingText"
#define TF_OWNER "OwnerText"

#define IMG_LOGO "logo"

#define PNL_IMAGE "image_panel"
#define PNL_DESC "description panel"
#define PNL_LOC "location panel"
#define PNL_MRKT "marketplace panel"

#define BTN_EDIT "edit_btn"
#define BTN_ALLOW "allow_btn"
#define BTN_FORGET "forget_btn"
#define BTN_BLOCK "block_btn"


LLFloaterExperienceProfile::LLFloaterExperienceProfile(const LLSD& data)
    : LLFloater(data)
    , mExperienceId(data.asUUID())
    , mImagePanel(NULL)
    , mDescriptionPanel(NULL)
    , mLocationPanel(NULL)
    , mMarketplacePanel(NULL)
{

}

LLFloaterExperienceProfile::~LLFloaterExperienceProfile()
{

}

class ExperiencePreferencesResponder : public LLHTTPClient::Responder
{
public:
    ExperiencePreferencesResponder(const LLHandle<LLFloaterExperienceProfile>& parent):mParent(parent)
    {
    }

    LLHandle<LLFloaterExperienceProfile> mParent;

    virtual void result(const LLSD& content)
    {
        LLFloaterExperienceProfile* parent=mParent.get();
        if(parent)
        {
            parent->setPreferences(content);
        }
    }
    virtual void error(U32 status, const std::string& reason)
    {
        lldebugs << "ExperiencePreferencesResponder failed with code: " << status<< ", reason: " << reason << llendl;
    }
};


class IsAdminResponder : public LLHTTPClient::Responder
{
public:
    IsAdminResponder(const LLHandle<LLFloaterExperienceProfile>& parent):mParent(parent)
    {
    }

    LLHandle<LLFloaterExperienceProfile> mParent;

    virtual void result(const LLSD& content)
    {
        LLFloaterExperienceProfile* parent = mParent.get();
        if(!parent)
            return;
        
        LLButton* edit=parent->getChild<LLButton>(BTN_EDIT);

        if(content.has("experience_ids"))
        {
            LLUUID id=parent->getKey().asUUID();
            const LLSD& xp_ids = content["experience_ids"];
            LLSD::array_const_iterator it = xp_ids.beginArray();
            while(it != xp_ids.endArray())
            {
                if(it->asUUID() == id)
                {
                    edit->setVisible(TRUE);
                    return;
                }
                ++it;
            }
        }
        edit->setVisible(FALSE);

        //parent->getChild<LLButton>(BTN_EDIT)->setVisible(content["status"].asBoolean());
    }
    virtual void error(U32 status, const std::string& reason)
    {
            lldebugs << "IsAdminResponder failed with code: " << status<< ", reason: " << reason << llendl;
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
            //             std::string lookup_url=region->getCapability("IsExperienceAdmin"); 
            //             if(!lookup_url.empty())
            //             {
            //                 LLHTTPClient::get(lookup_url+"/"+mExperienceId.asString(), new IsAdminResponder(getDerivedHandle<LLFloaterExperienceProfile>()));
            //             }
            
            std::string lookup_url=region->getCapability("GetAdminExperiences"); 
            if(!lookup_url.empty())
            {
                LLHTTPClient::get(lookup_url, new IsAdminResponder(getDerivedHandle<LLFloaterExperienceProfile>()));
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

bool LLFloaterExperienceProfile::setMaturityString( U8 maturity, LLTextBox* child )
{
    LLStyle::Params style;
    std::string access;
    if(maturity <= SIM_ACCESS_PG)
    {
        style.image(LLUI::getUIImage(getString("maturity_icon_general")));
        access = LLTrans::getString("SIM_ACCESS_PG");
    }
    else if(maturity <= SIM_ACCESS_MATURE)
    {
        style.image(LLUI::getUIImage(getString("maturity_icon_moderate")));
        access = LLTrans::getString("SIM_ACCESS_MATURE");
    }
    else if(maturity <= SIM_ACCESS_ADULT)
    {
        style.image(LLUI::getUIImage(getString("maturity_icon_adult")));
        access = LLTrans::getString("SIM_ACCESS_ADULT");
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

    std::string value = experience[LLExperienceCache::DESCRIPTION].asString();
    LLExpandableTextBox* exchild = getChild<LLExpandableTextBox>(TF_DESC);
    exchild->setText(value);
    mDescriptionPanel->setVisible(value.length()>0);
   
    value = experience[LLExperienceCache::SLURL].asString();
    child = getChild<LLTextBox>(TF_SLURL);
    child->setText(value);
    mLocationPanel->setVisible(value.length()>0);
    
    child = getChild<LLTextBox>(TF_MATURITY);
    setMaturityString((U8)(experience[LLExperienceCache::MATURITY].asInteger()), child);

    child = getChild<LLTextBox>(TF_OWNER);

    LLUUID id = experience[LLExperienceCache::OWNER_ID].asUUID();
    if(experience[LLExperienceCache::GROUP_ID].asUUID() == id)
    {
        value = LLSLURL("group", id, "inspect").getSLURLString();
    }
    else
    {
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

        if(data.has(IMG_LOGO))
        {
            LLTextureCtrl* logo = getChild<LLTextureCtrl>(IMG_LOGO);
            logo->setImageAssetID(data[IMG_LOGO].asUUID());
            mImagePanel->setVisible(TRUE);
        }
    }




    

    
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
