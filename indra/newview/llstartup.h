/** 
 * @file llstartup.h
 * @brief startup routines and logic declaration
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLSTARTUP_H
#define LL_LLSTARTUP_H

#include "llimagegl.h"

// functions
BOOL idle_startup();
void cleanup_app();
LLString load_password_from_disk();
void release_start_screen();

// constants, variables,  & enumerations
extern const char* SCREEN_HOME_FILENAME;
extern const char* SCREEN_LAST_FILENAME;

enum EStartupState{
	STATE_FIRST,					// Initial startup
	STATE_LOGIN_SHOW,				// Show login screen
	STATE_LOGIN_WAIT,				// Wait for user input at login screen
	STATE_LOGIN_CLEANUP,			// Get rid of login screen and start login
	STATE_UPDATE_CHECK,				// Wait for user at a dialog box (updates, term-of-service, etc)
	STATE_LOGIN_AUTH_INIT,			// Start login to SL servers
	STATE_LOGIN_AUTHENTICATE,		// Do authentication voodoo
	STATE_LOGIN_NO_DATA_YET,		// Waiting for authentication replies to start
	STATE_LOGIN_DOWNLOADING,		// Waiting for authentication replies to download
	STATE_LOGIN_PROCESS_RESPONSE,	// Check authentication reply
	STATE_WORLD_INIT,				// Start building the world
	STATE_SEED_GRANTED_WAIT,		// Wait for seed cap grant
	STATE_SEED_CAP_GRANTED,			// Have seed cap grant 
	STATE_QUICKTIME_INIT,			// Initialzie QT
	STATE_WORLD_WAIT,				// Waiting for simulator
	STATE_AGENT_SEND,				// Connect to a region
	STATE_AGENT_WAIT,				// Wait for region
	STATE_INVENTORY_SEND,			// Do inventory transfer
	STATE_MISC,						// Do more things (set bandwidth, start audio, save location, etc)
	STATE_PRECACHE,					// Wait a bit for textures to download
	STATE_WEARABLES_WAIT,			// Wait for clothing to download
	STATE_CLEANUP,					// Final cleanup
	STATE_STARTED					// Up and running in-world
};

// exported symbols
extern BOOL gAgentMovementCompleted;
extern bool gUseQuickTime;
extern bool gQuickTimeInitialized;
extern LLPointer<LLImageGL> gStartImageGL;

class LLStartUp
{
public:
	static bool canGoFullscreen();
		// returns true if we are far enough along in startup to allow
		// going full screen

	// Always use this to set gStartupState so changes are logged
	static void	setStartupState( S32 state );
	static S32	getStartupState()				{ return gStartupState;		};

	static bool dispatchURL();
		// if we have a SLURL or sim string ("Ahern/123/45") that started
		// the viewer, dispatch it

	static std::string sSLURLCommand;
		// *HACK: On startup, if we were passed a secondlife://app/do/foo
		// command URL, store it for later processing.

protected:
	static S32 gStartupState;			// Do not set directly, use LLStartup::setStartupState
};


#endif // LL_LLSTARTUP_H
