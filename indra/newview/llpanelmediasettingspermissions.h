/**
 * @file llpanelmediasettingspermissions.h
 * @brief LLPanelMediaSettingsPermissions class definition
 *
 * note that "permissions" tab is really "Controls" tab - refs to 'perms' and
 * 'permissions' not changed to 'controls' since we don't want to change 
 * shared files in server code and keeping everything the same seemed best.
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

#ifndef LL_LLPANELMEDIAMEDIASETTINGSPERMISSIONS_H
#define LL_LLPANELMEDIAMEDIASETTINGSPERMISSIONS_H

#include "llpanel.h"
#include "lluuid.h"

class LLComboBox;
class LLCheckBoxCtrl;
class LLNameBox;

class LLPanelMediaSettingsPermissions : public LLPanel
{
public:
    LLPanelMediaSettingsPermissions();
    ~LLPanelMediaSettingsPermissions();
    
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
    
    static void initValues( void* userdata, const LLSD& media_settings, bool editable );
    static void clearValues( void* userdata,  bool editable);
    
private:
    LLComboBox* mControls;
    LLCheckBoxCtrl* mPermsOwnerInteract;
    LLCheckBoxCtrl* mPermsOwnerControl;
    LLNameBox* mPermsGroupName;
    LLCheckBoxCtrl* mPermsGroupInteract;
    LLCheckBoxCtrl* mPermsGroupControl;
    LLCheckBoxCtrl* mPermsWorldInteract;
    LLCheckBoxCtrl* mPermsWorldControl;
};

#endif  // LL_LLPANELMEDIAMEDIASETTINGSPERMISSIONS_H
