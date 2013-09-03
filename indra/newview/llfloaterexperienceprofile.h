/** 
 * @file llfloaterexperienceprofile.h
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



#ifndef LL_LLFLOATEREXPERIENCEPROFILE_H
#define LL_LLFLOATEREXPERIENCEPROFILE_H

#include "llfloater.h"
#include "lluuid.h"
#include "llsd.h"

class LLLayoutPanel;
class LLTextBox;

class LLFloaterExperienceProfile : public LLFloater
{
    LOG_CLASS(LLFloaterExperienceProfile);
public:
    LLFloaterExperienceProfile(const LLSD& data);
    virtual ~LLFloaterExperienceProfile();

    void setExperienceId( const LLUUID& experience_id );
    void setPreferences( const LLSD& content );

protected:
    void onClickEdit();
    void onClickPermission(const char* permission);
    void onClickForget();


    static void experienceCallback(LLHandle<LLFloaterExperienceProfile> handle, const LLSD& experience);

    void refreshExperience(const LLSD& experience);
    BOOL postBuild();
    bool setMaturityString(U8 maturity, LLTextBox* child);
    LLUUID mExperienceId;
    LLSD mExperienceDetails;

    LLLayoutPanel* mImagePanel;
    LLLayoutPanel* mDescriptionPanel;
    LLLayoutPanel* mLocationPanel;
    LLLayoutPanel* mMarketplacePanel;
    
private:

};

#endif // LL_LLFLOATEREXPERIENCEPROFILE_H
