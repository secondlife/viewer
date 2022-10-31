/**
 * @file llpanellandmedia.h
 * @brief Allows configuration of "media" for a land parcel,
 *   for example movies, web pages, and audio.
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

#ifndef LLPANELLANDMEDIA_H
#define LLPANELLANDMEDIA_H

#include "lllineeditor.h"
#include "llpanel.h"
#include "llparcelselection.h"
#include "lluifwd.h"    // widget pointer types

class LLPanelLandMedia
:   public LLPanel
{
public:
    LLPanelLandMedia(LLSafeHandle<LLParcelSelection>& parcelp);
    /*virtual*/ ~LLPanelLandMedia();
    /*virtual*/ BOOL postBuild();
    void refresh();
    void setMediaType(const std::string& media_type);
    void setMediaURL(const std::string& media_type);
    std::string getMediaURL();

private:
    void populateMIMECombo();
    static void onCommitAny(LLUICtrl* ctrl, void *userdata);
    static void onCommitType(LLUICtrl* ctrl, void *userdata);
    static void onSetBtn(void* userdata);
    static void onResetBtn(void* userdata);
    
private:
    LLLineEditor*   mMediaURLEdit;
    LLLineEditor*   mMediaDescEdit;
    LLComboBox*     mMediaTypeCombo;
    LLButton*       mSetURLButton;
    LLSpinCtrl*     mMediaHeightCtrl;
    LLSpinCtrl*     mMediaWidthCtrl;
    LLTextBox*      mMediaSizeCtrlLabel;
    LLTextureCtrl*  mMediaTextureCtrl;
    LLCheckBoxCtrl* mMediaAutoScaleCheck;
    LLCheckBoxCtrl* mMediaLoopCheck;
    LLHandle<LLFloater> mURLEntryFloater;


    
    LLSafeHandle<LLParcelSelection>&    mParcel;
};

#endif
