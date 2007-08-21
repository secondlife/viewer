/** 
 * @file llstartup.h
 * @brief startup routines and logic declaration
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
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

protected:
	static S32 gStartupState;			// Do not set directly, use LLStartup::setStartupState
};


#endif // LL_LLSTARTUP_H
