/** 
 * @file llmediaentry.h
 * @brief This is a single instance of media data related to the face of a prim
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLMEDIAENTRY_H
#define LL_LLMEDIAENTRY_H

#include "llsd.h"
#include "llstring.h"

// For return values of set*
#include "lllslconstants.h"

class LLMediaEntry
{
public: 
    enum MediaControls {
        STANDARD = 0,
        MINI
    };
    
    // Constructors
    LLMediaEntry();
    LLMediaEntry(const LLMediaEntry &rhs);

    LLMediaEntry &operator=(const LLMediaEntry &rhs);
    virtual ~LLMediaEntry();

    bool operator==(const LLMediaEntry &rhs) const;
    bool operator!=(const LLMediaEntry &rhs) const;
    
    // Render as LLSD
    LLSD asLLSD() const;
    void asLLSD(LLSD& sd) const;
    operator LLSD() const { return asLLSD(); }
    // Returns false iff the given LLSD contains fields that violate any bounds
    // limits.
    static bool checkLLSD(const LLSD& sd);
    // This doesn't merge, it overwrites the data, so will use
    // LLSD defaults if need be.  Note: does not check limits!
    // Use checkLLSD() above first to ensure the LLSD is valid.
    void fromLLSD(const LLSD& sd);  
    // This merges data from the incoming LLSD into our fields.
    // Note that it also does NOT check limits!  Use checkLLSD() above first.
    void mergeFromLLSD(const LLSD& sd);

    // "general" fields
    bool getAltImageEnable() const { return mAltImageEnable; }
    MediaControls getControls() const { return mControls; }
    std::string getCurrentURL() const { return mCurrentURL; }
    std::string getHomeURL() const { return mHomeURL; }
    bool getAutoLoop() const { return mAutoLoop; }
    bool getAutoPlay() const { return mAutoPlay; }
    bool getAutoScale() const { return mAutoScale; }
    bool getAutoZoom() const { return mAutoZoom; }
    bool getFirstClickInteract() const { return mFirstClickInteract; }
    U16 getWidthPixels() const { return mWidthPixels; }
    U16 getHeightPixels() const { return mHeightPixels; }

    // "security" fields
    bool getWhiteListEnable() const { return mWhiteListEnable; }
    const std::vector<std::string> &getWhiteList() const { return mWhiteList; }

    // "permissions" fields
    U8 getPermsInteract() const { return mPermsInteract; }
    U8 getPermsControl() const { return mPermsControl; }

    // Setters.  Those that return a U32 return a status error code
    // See lllslconstants.h
    
    // "general" fields
    U32 setAltImageEnable(bool alt_image_enable) { mAltImageEnable = alt_image_enable; return LSL_STATUS_OK; }
    U32 setControls(MediaControls controls);
    U32 setCurrentURL(const std::string& current_url);
    U32 setHomeURL(const std::string& home_url);
    U32 setAutoLoop(bool auto_loop) { mAutoLoop = auto_loop; return LSL_STATUS_OK; }
    U32 setAutoPlay(bool auto_play) { mAutoPlay = auto_play; return LSL_STATUS_OK; }
    U32 setAutoScale(bool auto_scale) { mAutoScale = auto_scale; return LSL_STATUS_OK; }
    U32 setAutoZoom(bool auto_zoom) { mAutoZoom = auto_zoom; return LSL_STATUS_OK; }
    U32 setFirstClickInteract(bool first_click) { mFirstClickInteract = first_click; return LSL_STATUS_OK; }
    U32 setWidthPixels(U16 width);
    U32 setHeightPixels(U16 height);

    // "security" fields
    U32 setWhiteListEnable( bool whitelist_enable ) { mWhiteListEnable = whitelist_enable; return LSL_STATUS_OK; }
    U32 setWhiteList( const std::vector<std::string> &whitelist );
    U32 setWhiteList( const LLSD &whitelist );  // takes an LLSD array

    // "permissions" fields
    U32 setPermsInteract( U8 val );
    U32 setPermsControl( U8 val );

    const LLUUID& getMediaID() const;

    // Helper function to check a candidate URL against the whitelist
    // Returns true iff candidate URL passes (or if there is no whitelist), false otherwise
    bool checkCandidateUrl(const std::string& url) const;

public:
    // Static function to check a URL against a whitelist
    // Returns true iff url passes the given whitelist
    static bool checkUrlAgainstWhitelist(const std::string &url, 
                                         const std::vector<std::string> &whitelist);
    
public:
        // LLSD key defines
    // "general" fields
    static const char*  ALT_IMAGE_ENABLE_KEY;
    static const char*  CONTROLS_KEY;
    static const char*  CURRENT_URL_KEY;
    static const char*  HOME_URL_KEY;
    static const char*  AUTO_LOOP_KEY;
    static const char*  AUTO_PLAY_KEY;
    static const char*  AUTO_SCALE_KEY;
    static const char*  AUTO_ZOOM_KEY;
    static const char*  FIRST_CLICK_INTERACT_KEY;
    static const char*  WIDTH_PIXELS_KEY;
    static const char*  HEIGHT_PIXELS_KEY;

    // "security" fields
    static const char*  WHITELIST_ENABLE_KEY;
    static const char*  WHITELIST_KEY;

    // "permissions" fields
    static const char*  PERMS_INTERACT_KEY;
    static const char*  PERMS_CONTROL_KEY;

    // Field enumerations & constants

    // *NOTE: DO NOT change the order of these, and do not insert values
    // in the middle!
    // Add values to the end, and make sure to change PARAM_MAX_ID!
    enum Fields {
         ALT_IMAGE_ENABLE_ID = 0,
         CONTROLS_ID = 1,
         CURRENT_URL_ID = 2,
         HOME_URL_ID = 3,
         AUTO_LOOP_ID = 4,
         AUTO_PLAY_ID = 5,
         AUTO_SCALE_ID = 6,
         AUTO_ZOOM_ID = 7,
         FIRST_CLICK_INTERACT_ID = 8,
         WIDTH_PIXELS_ID = 9,
         HEIGHT_PIXELS_ID = 10,
         WHITELIST_ENABLE_ID = 11,
         WHITELIST_ID = 12,
         PERMS_INTERACT_ID = 13,
         PERMS_CONTROL_ID = 14,
         PARAM_MAX_ID = PERMS_CONTROL_ID
    };

    // "permissions" values
    // (e.g. (PERM_OWNER | PERM_GROUP) sets permissions on for OWNER and GROUP
    static const U8    PERM_NONE             = 0x0;
    static const U8    PERM_OWNER            = 0x1;
    static const U8    PERM_GROUP            = 0x2;
    static const U8    PERM_ANYONE           = 0x4;
    static const U8    PERM_ALL              = PERM_OWNER|PERM_GROUP|PERM_ANYONE;
    static const U8    PERM_MASK             = PERM_OWNER|PERM_GROUP|PERM_ANYONE;

    // Limits (in bytes)
    static const U32   MAX_URL_LENGTH        = 1024;
    static const U32   MAX_WHITELIST_SIZE    = 1024;
    static const U32   MAX_WHITELIST_COUNT   = 64;
    static const U16   MAX_WIDTH_PIXELS      = 2048;
    static const U16   MAX_HEIGHT_PIXELS     = 2048;

private:

    U32 setStringFieldWithLimit( std::string &field, const std::string &value, U32 limit );
    U32 setCurrentURLInternal( const std::string &url, bool check_whitelist);
    bool fromLLSDInternal(const LLSD &sd, bool overwrite);

private:
     // "general" fields
    bool mAltImageEnable;
    MediaControls mControls;
    std::string mCurrentURL;
    std::string mHomeURL;
    bool mAutoLoop;
    bool mAutoPlay;
    bool mAutoScale;
    bool mAutoZoom;
    bool mFirstClickInteract;
    U16 mWidthPixels;
    U16 mHeightPixels;

    // "security" fields
    bool mWhiteListEnable;
    std::vector<std::string> mWhiteList;

    // "permissions" fields
    U8 mPermsInteract;
    U8 mPermsControl;
    
    mutable LLUUID *mMediaIDp;            // temporary id assigned to media on the viewer
};

#endif

