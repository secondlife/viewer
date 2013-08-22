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

#include "llfloaterexperienceprofile.h"
#include "llexperiencecache.h"
#include "llfloaterworldmap.h"
#include "llfloaterreg.h"
#include "lllayoutstack.h"
#include "lltextbox.h"
#include "llsdserialize.h"
#include "llexpandabletextbox.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include <sstream>

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

#define BTN_TP "teleport_btn"
#define BTN_MAP "show_on_map_btn"
#define BTN_EDIT "edit_btn"


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
    }


    childSetAction(BTN_TP, boost::bind(&LLFloaterExperienceProfile::onClickTeleport, this));
    childSetAction(BTN_MAP, boost::bind(&LLFloaterExperienceProfile::onClickMap, this));
    childSetAction(BTN_EDIT, boost::bind(&LLFloaterExperienceProfile::onClickEdit, this));

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

void LLFloaterExperienceProfile::onClickMap()
{
//     LLFloaterWorldMap::getInstance()->trackLocation(getPosGlobal());
//     LLFloaterReg::showInstance("world_map", "center");

}

void LLFloaterExperienceProfile::onClickTeleport()
{
//     if (!getPosGlobal().isExactlyZero())
//     {
//         gAgent.teleportViaLocation(getPosGlobal());
//         LLFloaterWorldMap::getInstance()->trackLocation(getPosGlobal());
//     }

}

void LLFloaterExperienceProfile::onClickEdit()
{

}

std::string LLFloaterExperienceProfile::getMaturityString(U8 maturity)
{
    if(maturity <= SIM_ACCESS_MIN)
        return LLTrans::getString("SIM_ACCESS_MIN");
    if(maturity <= SIM_ACCESS_PG)
        return LLTrans::getString("SIM_ACCESS_PG");
    if(maturity <= SIM_ACCESS_MATURE)
        return LLTrans::getString("SIM_ACCESS_MATURE");
    if(maturity <= SIM_ACCESS_ADULT)
        return LLTrans::getString("SIM_ACCESS_ADULT");
    
    return LLStringUtil::null;
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
    child->setText(getMaturityString((U8)(experience[LLExperienceCache::MATURITY].asInteger())));

    child = getChild<LLTextBox>(TF_OWNER);
    child->setText(experience[LLExperienceCache::OWNER_ID].asString()); 
    
    value=experience[LLExperienceCache::METADATA].asString();
    if(value.empty())
        return;
    
    LLPointer<LLSDParser> parser = new LLSDXMLParser();

    LLSD data;

    std::istringstream is = std::istringstream(value);
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
