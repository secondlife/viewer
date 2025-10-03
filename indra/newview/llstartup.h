/**
 * @file llstartup.h
 * @brief startup routines and logic declaration
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLSTARTUP_H
#define LL_LLSTARTUP_H

#include <memory>                   // unique_ptr

class LLViewerTexture ;
class LLEventPump;
class LLStartupListener;
class LLSLURL;

#include "llviewerstats.h"

// functions
bool idle_startup();
void release_start_screen();
bool login_alert_done(const LLSD& notification, const LLSD& response);
void pump_idle_startup_network();

// start location constants
enum EStartLocation
{
    START_LOCATION_ID_LAST,
    START_LOCATION_ID_HOME,
    START_LOCATION_ID_DIRECT,
    START_LOCATION_ID_PARCEL,
    START_LOCATION_ID_TELEHUB,
    START_LOCATION_ID_URL,
    START_LOCATION_ID_COUNT
};

typedef enum {
    STATE_FIRST,                    // Initial startup
    STATE_BROWSER_INIT,             // Initialize web browser for login screen
    STATE_LOGIN_SHOW,               // Show login screen
    STATE_LOGIN_WAIT,               // Wait for user input at login screen
    STATE_LOGIN_CLEANUP,            // Get rid of login screen and start login
    STATE_LOGIN_AUTH_INIT,          // Start login to SL servers
    STATE_LOGIN_CURL_UNSTUCK,       // Update progress to remove "SL appears frozen" msg.
    STATE_LOGIN_PROCESS_RESPONSE,   // Check authentication reply
    STATE_WORLD_INIT,               // Start building the world
    STATE_MULTIMEDIA_INIT,          // Init the rest of multimedia library
    STATE_FONT_INIT,                // Load default fonts
    STATE_SEED_GRANTED_WAIT,        // Wait for seed cap grant
    STATE_SEED_CAP_GRANTED,         // Have seed cap grant
    STATE_WORLD_WAIT,               // Waiting for simulator
    STATE_AGENT_SEND,               // Connect to a region
    STATE_AGENT_WAIT,               // Wait for region
    STATE_INVENTORY_SEND,           // Do inventory transfer
    STATE_INVENTORY_CALLBACKS,      // Wait for missing system folders and register callbacks
    STATE_INVENTORY_SKEL,           // Do more inventory skeleton loading
    STATE_INVENTORY_SEND2,          // Do more inventory init after skeleton is loaded
    STATE_MISC,                     // Do more things (set bandwidth, start audio, save location, etc)
    STATE_PRECACHE,                 // Wait a bit for textures to download
    STATE_WEARABLES_WAIT,           // Wait for clothing to download
    STATE_CLEANUP,                  // Final cleanup
    STATE_STARTED                   // Up and running in-world
} EStartupState;

// exported symbols
extern bool gAgentMovementCompleted;
extern LLPointer<LLViewerTexture> gStartTexture;

class LLStartUp
{
public:

    // Always use this to set gStartupState so changes are logged
    static void setStartupState( EStartupState state );
    static EStartupState getStartupState() { return gStartupState; };
    static std::string getStartupStateString() { return startupStateToString(gStartupState); };
    static std::string getScreenLastFilename(); // screenshot taken on exit
    static std::string getScreenHomeFilename(); // screenshot taken on setting Home

    static void multimediaInit();
        // Initialize LLViewerMedia multimedia engine.

    // Load default fonts not already loaded at start screen
    static void fontInit();

    static void initNameCache();
    static void initExperiences();

    static void cleanupNameCache();

    // outfit_folder_name can be a folder anywhere in your inventory,
    // but the name must be a case-sensitive exact match.
    // gender_name is either "male" or "female"
    static void loadInitialOutfit( const std::string& outfit_folder_name,
                                   const std::string& gender_name );

    static std::string& getInitialOutfitName();
    static std::string getUserId();

    static bool dispatchURL();
        // if we have a SLURL or sim string ("Ahern/123/45") that started
        // the viewer, dispatch it

    static void postStartupState();
    static void setStartSLURL(const LLSLURL& slurl);
    static LLSLURL& getStartSLURL();

    static bool startLLProxy(); // Initialize the SOCKS 5 proxy

    static LLViewerStats::PhaseMap& getPhases() { return *sPhases; }
private:
    friend class LLStartupListener;
    static LLSLURL sStartSLURL;

    static std::string startupStateToString(EStartupState state);
    static EStartupState gStartupState; // Do not set directly, use LLStartup::setStartupState
    static std::unique_ptr<LLEventPump> sStateWatcher;
    static std::unique_ptr<LLStartupListener> sListener;
    static std::unique_ptr<LLViewerStats::PhaseMap> sPhases;
};


#endif // LL_LLSTARTUP_H
