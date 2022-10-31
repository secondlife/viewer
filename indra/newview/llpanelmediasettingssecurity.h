/**
 * @file llpanelmediasettingssecurity.h
 * @brief LLPanelMediaSettingsSecurity class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLPANELMEDIAMEDIASETTINGSSECURITY_H
#define LL_LLPANELMEDIAMEDIASETTINGSSECURITY_H

#include "llpanel.h"

class LLCheckBoxCtrl;
class LLScrollListCtrl;
class LLTextBox;
class LLFloaterMediaSettings;

class LLPanelMediaSettingsSecurity : public LLPanel
{
public:
    LLPanelMediaSettingsSecurity();
    ~LLPanelMediaSettingsSecurity();
    
    BOOL postBuild();
    virtual void draw();    
    
    // XXX TODO: put these into a common parent class?
    // Hook that the floater calls before applying changes from the panel
    void preApply();
    // Function that asks the panel to fill in values associated with the panel
    // 'include_tentative' means fill in tentative values as well, otherwise do not
    void getValues(LLSD &fill_me_in, bool include_tentative = true);
    // Hook that the floater calls after applying changes to the panel
    void postApply();
    
    static void initValues( void* userdata, const LLSD& media_settings, bool editable);
    static void clearValues( void* userdata, bool editable);
    void addWhiteListEntry( const std::string& url );
    void setParent( LLFloaterMediaSettings* parent );
    bool urlPassesWhiteList( const std::string& test_url );
    const std::string makeValidUrl( const std::string& src_url );

    void updateWhitelistEnableStatus(); 

protected:
    LLFloaterMediaSettings* mParent;
    
private:
    enum ColumnIndex 
    {
        ICON_COLUMN = 0,
        ENTRY_COLUMN = 1,
    };

    LLCheckBoxCtrl* mEnableWhiteList;
    LLScrollListCtrl* mWhiteListList;
    LLTextBox* mHomeUrlFailsWhiteListText;

    static void onBtnAdd(void*);
    static void onBtnDel(void*);
};

#endif  // LL_LLPANELMEDIAMEDIASETTINGSSECURITY_H
