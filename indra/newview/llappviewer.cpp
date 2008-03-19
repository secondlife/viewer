/** 
 * @file llappviewer.cpp
 * @brief The LLAppViewer class definitions
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
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


#include "llviewerprecompiledheaders.h"
#include "llappviewer.h"

#include "llversionviewer.h"
#include "llfeaturemanager.h"
#include "llvieweruictrlfactory.h"
#include "llalertdialog.h"
#include "llerrorcontrol.h"
#include "llviewerimagelist.h"
#include "llgroupmgr.h"
#include "llagent.h"
#include "llwindow.h"
#include "llviewerstats.h"
#include "llmd5.h"
#include "llpumpio.h"
#include "llfloateractivespeakers.h"
#include "llimpanel.h"
#include "llmimetypes.h"
#include "llstartup.h"
#include "llfocusmgr.h"
#include "llviewerjoystick.h"
#include "llares.h" 
#include "llcurl.h"
#include "llfloatersnapshot.h"
#include "llviewerwindow.h"
#include "llviewerdisplay.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llworldmap.h"
#include "llmutelist.h"
#include "llurldispatcher.h"
#include "llurlhistory.h"

#include "llweb.h"
#include "llsecondlifeurls.h"

#if LL_WINDOWS
	#include "llwindebug.h"
#endif

#if LL_WINDOWS
#	include <share.h> // For _SH_DENYWR in initMarkerFile
#else
#   include <sys/file.h> // For initMarkerFile support
#endif



#include "llnotify.h"
#include "llviewerkeyboard.h"
#include "lllfsthread.h"
#include "llworkerthread.h"
#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "llimageworker.h"

// The files below handle dependencies from cleanup.
#include "llkeyframemotion.h"
#include "llworldmap.h"
#include "llhudmanager.h"
#include "lltoolmgr.h"
#include "llassetstorage.h"
#include "llpolymesh.h"
#include "lleconomy.h"
#include "llcachename.h"
#include "audioengine.h"
#include "llviewermenu.h"
#include "llselectmgr.h"
#include "lltracker.h"
#include "llviewerparcelmgr.h"
#include "llworldmapview.h"
#include "llpostprocess.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"

#include "lldebugview.h"
#include "llconsole.h"
#include "llcontainerview.h"
#include "llfloaterstats.h"
#include "llhoverview.h"

#include "llsdserialize.h"

#include "llworld.h"
#include "llhudeffecttrail.h"
#include "llvectorperfoptions.h"
#include "llurlsimstring.h"

// Included so that constants/settings might be initialized
// in save_settings_to_globals()
#include "llbutton.h"
#include "llcombobox.h"
#include "llstatusbar.h"
#include "llsurface.h"
#include "llvosky.h"
#include "llvotree.h"
#include "llvoavatar.h"
#include "llfolderview.h"
#include "lltoolbar.h"
#include "llframestats.h"
#include "llagentpilot.h"
#include "llsrv.h"
#include "llvovolume.h"
#include "llflexibleobject.h" 
#include "llvosurfacepatch.h"

// includes for idle() idleShutdown()
#include "llviewercontrol.h"
#include "lleventnotifier.h"
#include "llcallbacklist.h"
#include "pipeline.h"
#include "llgesturemgr.h"
#include "llsky.h"
#include "llvlmanager.h"
#include "llviewercamera.h"
#include "lldrawpoolbump.h"
#include "llvieweraudio.h"
#include "llimview.h"
#include "llviewerthrottle.h"
// 

#include "llinventoryview.h"

// *FIX: Remove these once the command line params thing is figured out.
// Yuck!
static int gTempArgC = 0;
static char** gTempArgV;

// *FIX: These extern globals should be cleaned up.
// The globals either represent state/config/resource-storage of either 
// this app, or another 'component' of the viewer. App globals should be 
// moved into the app class, where as the other globals should be 
// moved out of here.
// If a global symbol reference seems valid, it will be included
// via header files above.

//----------------------------------------------------------------------------
// llviewernetwork.h
#include "llviewernetwork.h"
// extern EGridInfo gGridChoice;


////// Windows-specific includes to the bottom - nasty defines in these pollute the preprocessor
//
#if LL_WINDOWS && LL_LCD_COMPILE
	#include "lllcd.h"
#endif

//----------------------------------------------------------------------------
// viewer.cpp - these are only used in viewer, should be easily moved.
extern void disable_win_error_reporting();

//#define APPLE_PREVIEW // Define this if you're doing a preview build on the Mac
#if LL_RELEASE_FOR_DOWNLOAD
// Default userserver for production builds is agni
#ifndef APPLE_PREVIEW
static EGridInfo GridDefaultChoice = GRID_INFO_AGNI;
#else
static EGridInfo GridDefaultChoice = GRID_INFO_ADITI;
#endif
#else
// Default userserver for development builds is none
static EGridInfo GridDefaultChoice = GRID_INFO_NONE;
#endif

#if LL_WINDOWS
extern void create_console();
#endif


#if LL_DARWIN
#include <Carbon/Carbon.h>
extern void init_apple_menu(const char* product);
extern OSErr AEGURLHandler(const AppleEvent *messagein, AppleEvent *reply, long refIn);
extern OSErr AEQuitHandler(const AppleEvent *messagein, AppleEvent *reply, long refIn);
extern OSStatus simpleDialogHandler(EventHandlerCallRef handler, EventRef event, void *userdata);
extern OSStatus DisplayReleaseNotes(void);
#include <boost/tokenizer.hpp>
#endif // LL_DARWIN


extern BOOL gRandomizeFramerate;
extern BOOL gPeriodicSlowFrame;

////////////////////////////////////////////////////////////
// All from the last globals push...
bool gVerifySSLCert = true;
BOOL gHandleKeysAsync = FALSE;

BOOL gProbeHardware = TRUE; // Use DirectX 9 to probe for hardware

S32 gYieldMS = 0; // set in parse_args, used in mainLoop
BOOL gYieldTime = FALSE;

const F32 DEFAULT_AFK_TIMEOUT = 5.f * 60.f; // time with no input before user flagged as Away From Keyboard

F32 gSimLastTime; // Used in LLAppViewer::init and send_stats()
F32 gSimFrames;

LLString gDisabledMessage; // Set in LLAppViewer::initConfiguration used in idle_startup

BOOL gHideLinks = FALSE; // Set in LLAppViewer::initConfiguration, used externally

BOOL				gAllowIdleAFK = TRUE;
F32					gAFKTimeout = DEFAULT_AFK_TIMEOUT;
BOOL				gShowObjectUpdates = FALSE;
BOOL gLogMessages = FALSE;
std::string gChannelName = LL_CHANNEL;
BOOL gUseAudio = TRUE;
BOOL gUseQuickTime = TRUE;
LLString gCmdLineFirstName;
LLString gCmdLineLastName;
LLString gCmdLinePassword;

BOOL				gAutoLogin = FALSE;

const char*			DEFAULT_SETTINGS_FILE = "settings.xml";
BOOL gRequestInventoryLibrary = TRUE;
BOOL gGodConnect = FALSE;
BOOL gAcceptTOS = FALSE;
BOOL gAcceptCriticalMessage = FALSE;

eLastExecEvent gLastExecEvent = LAST_EXEC_NORMAL;

LLSD gDebugInfo;

U32	gFrameCount = 0;
U32 gForegroundFrameCount = 0; // number of frames that app window was in foreground
LLPumpIO*			gServicePump = NULL;

BOOL gPacificDaylightTime = FALSE;

U64 gFrameTime = 0;
F32 gFrameTimeSeconds = 0.f;
F32 gFrameIntervalSeconds = 0.f;
F32		gFPSClamped = 10.f;						// Pretend we start at target rate.
F32		gFrameDTClamped = 0.f;					// Time between adjacent checks to network for packets
U64	gStartTime = 0; // gStartTime is "private", used only to calculate gFrameTimeSeconds

LLTimer gRenderStartTime;
LLFrameTimer gForegroundTime;
LLTimer				gLogoutTimer;
static const F32			LOGOUT_REQUEST_TIME = 6.f;  // this will be cut short by the LogoutReply msg.
F32					gLogoutMaxTime = LOGOUT_REQUEST_TIME;

LLUUID gInventoryLibraryOwner;
LLUUID gInventoryLibraryRoot;

BOOL				gDisableVoice = FALSE;
BOOL				gDisconnected = FALSE;

// Map scale in pixels per region
F32 				gMapScale = 128.f;
F32 				gMiniMapScale = 128.f;

// used to restore texture state after a mode switch
LLFrameTimer	gRestoreGLTimer;
BOOL			gRestoreGL = FALSE;
BOOL				gUseWireframe = FALSE;

F32					gMouseSensitivity = 3.f;
BOOL				gInvertMouse = FALSE;

// VFS globals - see llappviewer.h
LLVFS* gStaticVFS = NULL;

LLMemoryInfo gSysMemory;

bool gPreloadImages = true;
bool gPreloadSounds = true;

LLString gLastVersionChannel;

LLVector3			gWindVec(3.0, 3.0, 0.0);
LLVector3			gRelativeWindVec(0.0, 0.0, 0.0);

U32		gPacketsIn = 0;

BOOL				gPrintMessagesThisFrame = FALSE;

BOOL gUseConsole = TRUE;

BOOL gRandomizeFramerate = FALSE;
BOOL gPeriodicSlowFrame = FALSE;

BOOL gQAMode = FALSE;
BOOL gLLErrorActivated = FALSE;

////////////////////////////////////////////////////////////
// Internal globals... that should be removed.
static F32 gQuitAfterSeconds = 0.f;
static BOOL gRotateRight = FALSE;
static BOOL gIgnorePixelDepth = FALSE;

// Allow multiple viewers in ReleaseForDownload
#if LL_RELEASE_FOR_DOWNLOAD
static BOOL gMultipleViewersOK = FALSE;
#else
static BOOL gMultipleViewersOK = TRUE;
#endif

static std::map<std::string, std::string> gCommandLineSettings;
static std::map<std::string, std::string> gCommandLineForcedSettings;

static LLString gArgs;

static LLString gOldSettingsFileName;
static const char* LEGACY_DEFAULT_SETTINGS_FILE = "settings.ini";
const char* MARKER_FILE_NAME = "SecondLife.exec_marker";
const char* ERROR_MARKER_FILE_NAME = "SecondLife.error_marker";
const char* LLERROR_MARKER_FILE_NAME = "SecondLife.llerror_marker";
static BOOL gDoDisconnect = FALSE;
static LLString gLaunchFileOnQuit;

//----------------------------------------------------------------------------
// File scope definitons
const char *VFS_DATA_FILE_BASE = "data.db2.x.";
const char *VFS_INDEX_FILE_BASE = "index.db2.x.";

static LLString gSecondLife;
static LLString gWindowTitle;
#ifdef LL_WINDOWS
	static char sWindowClass[] = "Second Life";
#endif

std::string gLoginPage;
std::vector<std::string> gLoginURIs;
static std::string gHelperURI;

static const char USAGE[] = "\n"
"usage:\tviewer [options]\n"
"options:\n"
" -login <first> <last> <password>     log in as a user\n"
" -autologin                           log in as last saved user\n"
" -loginpage <URL>                     login authentication page to use\n"
" -loginuri <URI>                      login server and CGI script to use\n"
" -helperuri <URI>                     helper web CGI prefix to use\n"
" -settings <filename>                 specify the filename of a\n"
"                                        configuration file\n"
"                                        default is settings.xml\n"
" -setdefault <variable> <value>       specify the value of a particular\n"
"                                        configuration variable which can be\n"
"                                        overridden by settings.xml\n"
" -set <variable> <value>              specify the value of a particular\n"
"                                        configuration variable that\n"
"                                        overrides all other settings\n"
#if !LL_RELEASE_FOR_DOWNLOAD
" -sim <simulator_ip>                  specify the simulator ip address\n"
#endif
" -god		                           log in as god if you have god access\n"
" -purge                               delete files in cache\n"
" -safe                                reset preferences, run in safe mode\n"
" -noutc                               logs in local time, not UTC\n"
" -nothread                            run vfs in single thread\n"
" -noinvlib                            Do not request inventory library\n"
" -multiple                            allow multiple viewers\n"
" -nomultiple                          block multiple viewers\n"
" -novoice                             disable voice\n"
" -ignorepixeldepth                    ignore pixel depth settings\n"
" -cooperative [ms]                    yield some idle time to local host\n"
" -skin                                ui/branding skin folder to use\n"
#if LL_WINDOWS
" -noprobe                             disable hardware probe\n"
#endif
" -noquicktime                         disable QuickTime movies, speeds startup\n"
" -nopreload                           don't preload UI images or sounds, speeds startup\n"
// these seem to be unused
//" -noenv                               turn off environmental effects\n"
//" -proxy <proxy_ip>                    specify the proxy ip address\n"
"\n";

void idle_afk_check()
{
	// check idle timers
	if (gAllowIdleAFK && (gAwayTriggerTimer.getElapsedTimeF32() > gAFKTimeout))
	{
		gAgent.setAFK();
	}
}

// A callback set in LLAppViewer::init()
static void ui_audio_callback(const LLUUID& uuid)
{
	if (gAudiop)
	{
		F32 volume = gSavedSettings.getBOOL("MuteUI") ? 0.f : gSavedSettings.getF32("AudioLevelUI");
		gAudiop->triggerSound(uuid, gAgent.getID(), volume);
	}
}

void request_initial_instant_messages()
{
	static BOOL requested = FALSE;
	if (!requested
		&& gMuteListp
		&& gMuteListp->isLoaded()
		&& gAgent.getAvatarObject())
	{
		// Auto-accepted inventory items may require the avatar object
		// to build a correct name.  Likewise, inventory offers from
		// muted avatars require the mute list to properly mute.
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_RetrieveInstantMessages);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gAgent.sendReliableMessage();
		requested = TRUE;
	}
}

// Use these strictly for things that are constructed at startup,
// or for things that are performance critical.  JC
static void saved_settings_to_globals()
{
	LLBUTTON_H_PAD		= gSavedSettings.getS32("ButtonHPad");
	LLBUTTON_V_PAD		= gSavedSettings.getS32("ButtonVPad");
	BTN_HEIGHT_SMALL	= gSavedSettings.getS32("ButtonHeightSmall");
	BTN_HEIGHT			= gSavedSettings.getS32("ButtonHeight");

	MENU_BAR_HEIGHT		= gSavedSettings.getS32("MenuBarHeight");
	MENU_BAR_WIDTH		= gSavedSettings.getS32("MenuBarWidth");
	STATUS_BAR_HEIGHT	= gSavedSettings.getS32("StatusBarHeight");

	LLCOMBOBOX_HEIGHT	= BTN_HEIGHT - 2;
	LLCOMBOBOX_WIDTH	= 128;

	LLSurface::setTextureSize(gSavedSettings.getU32("RegionTextureSize"));
	
	LLImageGL::sGlobalUseAnisotropic	= gSavedSettings.getBOOL("RenderAnisotropic");
	LLVOVolume::sLODFactor				= gSavedSettings.getF32("RenderVolumeLODFactor");
	LLVOVolume::sDistanceFactor			= 1.f-LLVOVolume::sLODFactor * 0.1f;
	LLVolumeImplFlexible::sUpdateFactor = gSavedSettings.getF32("RenderFlexTimeFactor");
	LLVOTree::sTreeFactor				= gSavedSettings.getF32("RenderTreeLODFactor");
	LLVOAvatar::sLODFactor				= gSavedSettings.getF32("RenderAvatarLODFactor");
	LLVOAvatar::sMaxVisible				= gSavedSettings.getS32("RenderAvatarMaxVisible");
	LLVOAvatar::sVisibleInFirstPerson	= gSavedSettings.getBOOL("FirstPersonAvatarVisible");
	// clamp auto-open time to some minimum usable value
	LLFolderView::sAutoOpenTime			= llmax(0.25f, gSavedSettings.getF32("FolderAutoOpenDelay"));
	LLToolBar::sInventoryAutoOpenTime	= gSavedSettings.getF32("InventoryAutoOpenDelay");
	LLSelectMgr::sRectSelectInclusive	= gSavedSettings.getBOOL("RectangleSelectInclusive");
	LLSelectMgr::sRenderHiddenSelections = gSavedSettings.getBOOL("RenderHiddenSelections");
	LLSelectMgr::sRenderLightRadius = gSavedSettings.getBOOL("RenderLightRadius");

	gFrameStats.setTrackStats(gSavedSettings.getBOOL("StatsSessionTrackFrameStats"));
	gAgentPilot.mNumRuns		= gSavedSettings.getS32("StatsNumRuns");
	gAgentPilot.mQuitAfterRuns	= gSavedSettings.getBOOL("StatsQuitAfterRuns");
	gAgent.mHideGroupTitle		= gSavedSettings.getBOOL("RenderHideGroupTitle");

	gDebugWindowProc = gSavedSettings.getBOOL("DebugWindowProc");
	gAllowIdleAFK = gSavedSettings.getBOOL("AllowIdleAFK");
	gAFKTimeout = gSavedSettings.getF32("AFKTimeout");
	gMouseSensitivity = gSavedSettings.getF32("MouseSensitivity");
	gInvertMouse = gSavedSettings.getBOOL("InvertMouse");
	gShowObjectUpdates = gSavedSettings.getBOOL("ShowObjectUpdates");
	gMapScale = gSavedSettings.getF32("MapScale");
	gMiniMapScale = gSavedSettings.getF32("MiniMapScale");
	gHandleKeysAsync = gSavedSettings.getBOOL("AsyncKeyboard");
	LLHoverView::sShowHoverTips = gSavedSettings.getBOOL("ShowHoverTips");

	LLRenderTarget::sUseFBO				= gSavedSettings.getBOOL("RenderUseFBO");
	LLVOAvatar::sUseImpostors			= gSavedSettings.getBOOL("RenderUseImpostors");
	LLVOSurfacePatch::sLODFactor		= gSavedSettings.getF32("RenderTerrainLODFactor");
	LLVOSurfacePatch::sLODFactor *= LLVOSurfacePatch::sLODFactor; //sqaure lod factor to get exponential range of [1,4]

#if LL_VECTORIZE
	if (gSysCPU.hasAltivec())
	{
		gSavedSettings.setBOOL("VectorizeEnable", TRUE );
		gSavedSettings.setU32("VectorizeProcessor", 0 );
	}
	else
	if (gSysCPU.hasSSE2())
	{
		gSavedSettings.setBOOL("VectorizeEnable", TRUE );
		gSavedSettings.setU32("VectorizeProcessor", 2 );
	}
	else
	if (gSysCPU.hasSSE())
	{
		gSavedSettings.setBOOL("VectorizeEnable", TRUE );
		gSavedSettings.setU32("VectorizeProcessor", 1 );
	}
	else
	{
		// Don't bother testing or running if CPU doesn't support it. JC
		gSavedSettings.setBOOL("VectorizePerfTest", FALSE );
		gSavedSettings.setBOOL("VectorizeEnable", FALSE );
		gSavedSettings.setU32("VectorizeProcessor", 0 );
		gSavedSettings.setBOOL("VectorizeSkin", FALSE);
	}
#else
	// This build target doesn't support SSE, don't test/run.
	gSavedSettings.setBOOL("VectorizePerfTest", FALSE );
	gSavedSettings.setBOOL("VectorizeEnable", FALSE );
	gSavedSettings.setU32("VectorizeProcessor", 0 );
	gSavedSettings.setBOOL("VectorizeSkin", FALSE);
#endif

	// propagate push to talk preference to current status
	gSavedSettings.setBOOL("PTTCurrentlyEnabled", gSavedSettings.getBOOL("EnablePushToTalk"));

	settings_setup_listeners();

	// gAgent.init() also loads from saved settings.
}

int parse_args(int argc, char **argv)
{
	// Sometimes IP addresses passed in on the command line have leading
	// or trailing white space.  Use LLString to clean that up.
	LLString ip_string;
	S32 j;

	for (j = 1; j < argc; j++) 
	{
		// Used to show first chunk of each argument passed in the 
		// window title.
		gArgs += argv[j];
		gArgs += " ";

		LLString argument = argv[j];
		if ((!strcmp(argv[j], "-port")) && (++j < argc)) 
		{
			sscanf(argv[j], "%u", &(gAgent.mViewerPort));
		}
		else if ((!strcmp(argv[j], "-drop")) && (++j < argc)) 
		{
			sscanf(argv[j], "%f", &gPacketDropPercentage);
		}
		else if ((!strcmp(argv[j], "-inbw")) && (++j < argc))
		{
			sscanf(argv[j], "%f", &gInBandwidth);
		}
		else if ((!strcmp(argv[j], "-outbw")) && (++j < argc))
		{
			sscanf(argv[j], "%f", &gOutBandwidth);
		}
		else if (!strcmp(argv[j], "--aditi"))
		{
			gGridChoice = GRID_INFO_ADITI;
			snprintf(gGridName, MAX_STRING, "%s", gGridInfo[gGridChoice].mName);		// Flawfinder: ignore
		}
		else if (!strcmp(argv[j], "--agni"))
		{
			gGridChoice = GRID_INFO_AGNI;
			snprintf(gGridName, MAX_STRING, "%s", gGridInfo[gGridChoice].mName);		// Flawfinder: ignore
		}
		else if (!strcmp(argv[j], "--aruna"))
		{
			gGridChoice = GRID_INFO_ARUNA;
			sprintf(gGridName,"%s", gGridInfo[gGridChoice].mName);
		}
		else if (!strcmp(argv[j], "--durga"))
		{
			gGridChoice = GRID_INFO_DURGA;
			snprintf(gGridName, MAX_STRING, "%s", gGridInfo[gGridChoice].mName);		// Flawfinder: ignore
		}
		else if (!strcmp(argv[j], "--ganga"))
		{
			gGridChoice = GRID_INFO_GANGA;
			sprintf(gGridName,"%s", gGridInfo[gGridChoice].mName);
		}
		else if (!strcmp(argv[j], "--mitra"))
		{
			gGridChoice = GRID_INFO_MITRA;
			sprintf(gGridName,"%s", gGridInfo[gGridChoice].mName);
		}
		else if (!strcmp(argv[j], "--mohini"))
		{
			gGridChoice = GRID_INFO_MOHINI;
			sprintf(gGridName,"%s", gGridInfo[gGridChoice].mName);
		}
		else if (!strcmp(argv[j], "--nandi"))
		{
			gGridChoice = GRID_INFO_NANDI;
			sprintf(gGridName,"%s", gGridInfo[gGridChoice].mName);
		}
		else if (!strcmp(argv[j], "--radha"))
		{
			gGridChoice = GRID_INFO_RADHA;
			sprintf(gGridName,"%s", gGridInfo[gGridChoice].mName);
		}
		else if (!strcmp(argv[j], "--ravi"))
		{
			gGridChoice = GRID_INFO_RAVI;
			sprintf(gGridName,"%s", gGridInfo[gGridChoice].mName);
		}
		else if (!strcmp(argv[j], "--siva"))
		{
			gGridChoice = GRID_INFO_SIVA;
			snprintf(gGridName, MAX_STRING, "%s", gGridInfo[gGridChoice].mName);		// Flawfinder: ignore
		}
		else if (!strcmp(argv[j], "--shakti"))
		{
			gGridChoice = GRID_INFO_SHAKTI;
			snprintf(gGridName, MAX_STRING, "%s", gGridInfo[gGridChoice].mName);		// Flawfinder: ignore
		}
		else if (!strcmp(argv[j], "--soma"))
		{
			gGridChoice = GRID_INFO_SOMA;
			snprintf(gGridName, MAX_STRING, "%s", gGridInfo[gGridChoice].mName);		// Flawfinder: ignore
		}
		else if (!strcmp(argv[j], "--uma"))
		{
			gGridChoice = GRID_INFO_UMA;
			sprintf(gGridName,"%s", gGridInfo[gGridChoice].mName);
		}
		else if (!strcmp(argv[j], "--vaak"))
		{
			gGridChoice = GRID_INFO_VAAK;
			sprintf(gGridName,"%s", gGridInfo[gGridChoice].mName);
		}
		else if (!strcmp(argv[j], "--yami"))
		{
			gGridChoice = GRID_INFO_YAMI;
			sprintf(gGridName,"%s", gGridInfo[gGridChoice].mName);
		}
		else if (!strcmp(argv[j], "-loginpage") && (++j < argc))
		{
			LLAppViewer::instance()->setLoginPage(utf8str_trim(argv[j]));
		}
		else if (!strcmp(argv[j], "-loginuri") && (++j < argc))
		{
            LLAppViewer::instance()->addLoginURI(utf8str_trim(argv[j]));
		}
		else if (!strcmp(argv[j], "-helperuri") && (++j < argc))
		{
            LLAppViewer::instance()->setHelperURI(utf8str_trim(argv[j]));
		}
		else if (!strcmp(argv[j], "-debugviews"))
		{
			LLView::sDebugRects = TRUE;
		}
		else if (!strcmp(argv[j], "-skin") && (++j < argc))
		{
			std::string folder(argv[j]);
			gDirUtilp->setSkinFolder(folder);
		}
		else if (!strcmp(argv[j], "-autologin") || !strcmp(argv[j], "--autologin")) // keep --autologin for compatibility
		{
			gAutoLogin = TRUE;
		}
		else if (!strcmp(argv[j], "-quitafter") && (++j < argc))
		{
			gQuitAfterSeconds = (F32)atof(argv[j]);
		}
		else if (!strcmp(argv[j], "-rotate"))
		{
			gRotateRight = TRUE;
		}
//		else if (!strcmp(argv[j], "-noenv")) 
//		{
			//turn OFF environmental effects for slow machines/video cards
//			gRequestParaboloidMap = FALSE;
//		}
		else if (!strcmp(argv[j], "-noaudio"))
		{
			gUseAudio = FALSE;
		}
		else if (!strcmp(argv[j], "-nosound"))  // tends to be popular cmdline on Linux.
		{
			gUseAudio = FALSE;
		}
		else if (!strcmp(argv[j], "-noprobe"))
		{
			gProbeHardware = FALSE;
		}
		else if (!strcmp(argv[j], "-noquicktime"))
		{
			// Developers can log in faster if they don't load all the
			// quicktime dlls.
			gUseQuickTime = false;
		}
		else if (!strcmp(argv[j], "-nopreload"))
		{
			// Developers can log in faster if they don't decode sounds
			// or images on startup, ~5 seconds faster.
			gPreloadSounds = false;
			gPreloadImages = false;
		}
		else if (!strcmp(argv[j], "-purge"))
		{
			LLAppViewer::instance()->purgeCache();
		}
		else if(!strcmp(argv[j], "-noinvlib"))
		{
			gRequestInventoryLibrary = FALSE;
		}
		else if (!strcmp(argv[j], "-log"))
		{
			gLogMessages = TRUE;
			continue;
		}
		else if (!strcmp(argv[j], "-logfile") && (++j < argc)) 
		{
			// *NOTE: This buffer size is hard coded into scanf() below.
			char logfile[256];	// Flawfinder: ignore
			sscanf(argv[j], "%255s", logfile);	// Flawfinder: ignore
			llinfos << "Setting log file to " << logfile << llendl;
			LLFile::remove(logfile);
			LLError::logToFile(logfile);
		}
		else if (!strcmp(argv[j], "-settings") && (++j < argc)) 
		{
			gSettingsFileName = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, argv[j]);
		}
		else if (!strcmp(argv[j], "-setdefault") && (j + 2 < argc)) 
		{
			std::string control_name;
			std::string control_value;
			
			j++;
			if (argv[j]) control_name = std::string(argv[j]);

			j++;
			if (argv[j]) control_value = std::string(argv[j]);
			
			// grab control name and value
			if (!control_name.empty())
			{
				gCommandLineSettings[control_name] = control_value;
			}
		}
		else if (!strcmp(argv[j], "-set") && (j + 2 < argc)) 
		{
			std::string control_name;
			std::string control_value;
			
			j++;
			if (argv[j]) control_name = std::string(argv[j]);

			j++;
			if (argv[j]) control_value = std::string(argv[j]);
			
			// grab control name and value
			if (!control_name.empty())
			{
				gCommandLineForcedSettings[control_name] = control_value;
			}
		}
		else if (!strcmp(argv[j], "-login"))
		{
			if (j + 3 < argc)
			{
				j++;
				gCmdLineFirstName = argv[j];
				j++;
				gCmdLineLastName = argv[j];
				j++;
				gCmdLinePassword = argv[j];
			}
			else
			{
				// only works if -login is last parameter on command line
				llerrs << "Not enough parameters to -login. Did you mean -loginuri?" << llendl;
			}
		}
		else if (!strcmp(argv[j], "-god"))
		{
			gGodConnect = TRUE;
		}
		else if (!strcmp(argv[j], "-noconsole"))
		{
			gUseConsole = FALSE;
		}
		else if (!strcmp(argv[j], "-safe"))
		{
			llinfos << "Setting viewer feature table to run in safe mode, resetting prefs" << llendl;
			gFeatureManagerp->setSafe(TRUE);
		}
		else if (!strcmp(argv[j], "-multiple"))
		{
			gMultipleViewersOK = TRUE;
		}
		else if (!strcmp(argv[j], "-nomultiple"))
		{
			gMultipleViewersOK = FALSE;
		}
		else if (!strcmp(argv[j], "-novoice"))
		{
			gDisableVoice = TRUE;
		}
		else if (!strcmp(argv[j], "-nothread"))
		{
			LLVFile::ALLOW_ASYNC = FALSE;
			llinfos << "Running VFS in nothread mode" << llendl;
		}
		// some programs don't respect the command line options in protocol handlers (I'm looking at you, Opera)
		// so this allows us to parse the URL straight off the command line without a "-url" paramater
		else if (LLURLDispatcher::isSLURL(argv[j])
				 || !strcmp(argv[j], "-url") && (++j < argc)) 
		{
			std::string slurl = argv[j];
			if (LLURLDispatcher::isSLURLCommand(slurl))
			{
				LLStartUp::sSLURLCommand = slurl;
			}
			else
			{
				LLURLSimString::setString(slurl);
			}
			// *NOTE: After setting the url, bail. What can happen is
			// that someone can use IE (or potentially other browsers)
			// and do the rough equivalent of command injection and
			// steal passwords. Phoenix. SL-55321
			return 0;
		}
		else if (!strcmp(argv[j], "-ignorepixeldepth"))
		{
			gIgnorePixelDepth = TRUE;
		}
		else if (!strcmp(argv[j], "-cooperative"))
		{
			S32 ms_to_yield = 0;
			if(++j < argc)
			{
				S32 rv = sscanf(argv[j], "%d", &ms_to_yield);
				if(0 == rv)
				{
					--j;
				}
			}
			else
			{
				--j;
			}
			gYieldMS = ms_to_yield;
			gYieldTime = TRUE;
		}
		else if (!strcmp(argv[j], "-no-verify-ssl-cert"))
		{
			gVerifySSLCert = false;
		}
		else if ( (!strcmp(argv[j], "--channel") || !strcmp(argv[j], "-channel"))  && (++j < argc)) 
		{
			gChannelName = argv[j];
		}
#if LL_DARWIN
		else if (!strncmp(argv[j], "-psn_", 5))
		{
			// this is the Finder passing the process session number
			// we ignore this
		}
#endif
		else if(!strncmp(argv[j], "-qa", 3))
		{
			gQAMode = TRUE;
		}
		else
		{

			// DBC - Mac OS X passes some stuff by default on the command line (e.g. psn).
			// Second Life URLs are passed this way as well?
			llwarns << "Possible unknown keyword " << argv[j] << llendl;

			// print usage information
			llinfos << USAGE << llendl;
			// return 1;
		}
	}
	return 0;
}

bool send_url_to_other_instance(const std::string& url)
{
#if LL_WINDOWS
	wchar_t window_class[256]; /* Flawfinder: ignore */   // Assume max length < 255 chars.
	mbstowcs(window_class, sWindowClass, 255);
	window_class[255] = 0;
	// Use the class instead of the window name.
	HWND other_window = FindWindow(window_class, NULL);
	if (other_window != NULL)
	{
		lldebugs << "Found other window with the name '" << gWindowTitle << "'" << llendl;
		COPYDATASTRUCT cds;
		const S32 SLURL_MESSAGE_TYPE = 0;
		cds.dwData = SLURL_MESSAGE_TYPE;
		cds.cbData = url.length() + 1;
		cds.lpData = (void*)url.c_str();

		LRESULT msg_result = SendMessage(other_window, WM_COPYDATA, NULL, (LPARAM)&cds);
		lldebugs << "SendMessage(WM_COPYDATA) to other window '" 
				 << gWindowTitle << "' returned " << msg_result << llendl;
		return true;
	}
#endif
	return false;
}

//----------------------------------------------------------------------------
// LLAppViewer definition

// Static members.
// The single viewer app.
LLAppViewer* LLAppViewer::sInstance = NULL;

LLTextureCache* LLAppViewer::sTextureCache = NULL; 
LLWorkerThread* LLAppViewer::sImageDecodeThread = NULL; 
LLTextureFetch* LLAppViewer::sTextureFetch = NULL; 

LLAppViewer::LLAppViewer() : 
	mMarkerFile(NULL),
	mCrashBehavior(CRASH_BEHAVIOR_ASK),
	mReportedCrash(false),
	mNumSessions(0),
	mPurgeCache(false),
    mPurgeOnExit(false),
    mSecondInstance(false),
	mSavedFinalSnapshot(false),
    mQuitRequested(false),
    mLogoutRequestSent(false)
{
	if(NULL != sInstance)
	{
		llerrs << "Oh no! An instance of LLAppViewer already exists! LLAppViewer is sort of like a singleton." << llendl;
	}

	sInstance = this;
}

LLAppViewer::~LLAppViewer()
{
	// If we got to this destructor somehow, the app didn't hang.
	removeMarkerFile();
}

bool LLAppViewer::tempStoreCommandOptions(int argc, char** argv)
{
	gTempArgC = argc;
	gTempArgV = argv;
	return true;
}

bool LLAppViewer::init()
{
    // *NOTE:Mani - LLCurl::initClass is not thread safe. 
    // Called before threads are created.
    LLCurl::initClass();

    initThreads();

	initEarlyConfiguration();

	//
	// Start of the application
	//
	// IMPORTANT! Do NOT put anything that will write
	// into the log files during normal startup until AFTER
	// we run the "program crashed last time" error handler below.
	//

	// Need to do this initialization before we do anything else, since anything
	// that touches files should really go through the lldir API
	gDirUtilp->initAppDirs("SecondLife");


	initLogging();
	
	//
	// OK to write stuff to logs now, we've now crash reported if necessary
	//

	// Set up some defaults...
	gSettingsFileName = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, DEFAULT_SETTINGS_FILE);
	gOldSettingsFileName = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, LEGACY_DEFAULT_SETTINGS_FILE);

    if (!initConfiguration())
		return false;

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	// *FIX: The following code isn't grouped into functions yet.

	//
	// Write system information into the debug log (CPU, OS, etc.)
	//
	writeSystemInfo();

	// Build a string representing the current version number.
        gCurrentVersion = llformat("%s %d.%d.%d.%d", gChannelName.c_str(), LL_VERSION_MAJOR, LL_VERSION_MINOR, LL_VERSION_PATCH, LL_VERSION_BUILD );

	//
	// Various introspection concerning the libs we're using.
	//
	llinfos << "J2C Engine is: " << LLImageJ2C::getEngineInfo() << llendl;

	// Merge with the command line overrides
	gSavedSettings.applyOverrides(gCommandLineSettings);

	// Need to do this before calling parseAlerts
	gUICtrlFactory = new LLViewerUICtrlFactory();
	
	// Pre-load alerts.xml to define the warnings settings (always loads from skins/xui/en-us/)
	// Do this *before* loading the settings file
	LLAlertDialog::parseAlerts("alerts.xml", &gSavedSettings, TRUE);
	
	// Overwrite default settings with user settings
	llinfos << "Loading configuration file " << gSettingsFileName << llendl;
	if (0 == gSavedSettings.loadFromFile(gSettingsFileName))
	{
		llinfos << "Failed to load settings from " << gSettingsFileName << llendl;
		llinfos << "Loading legacy settings from " << gOldSettingsFileName << llendl;
		gSavedSettings.loadFromFileLegacy(gOldSettingsFileName);
	}

	// need to do this here - need to have initialized global settings first
	LLString nextLoginLocation = gSavedSettings.getString( "NextLoginLocation" );
	if ( nextLoginLocation.length() )
	{
		LLURLSimString::setString( nextLoginLocation.c_str() );
	};

	// Merge with the command line overrides
	gSavedSettings.applyOverrides(gCommandLineForcedSettings);

	gLastRunVersion = gSavedSettings.getString("LastRunVersion");

	fixup_settings();
	
	// Get the single value from the crash settings file, if it exists
	std::string crash_settings_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, CRASH_SETTINGS_FILE);
	gCrashSettings.loadFromFile(crash_settings_filename.c_str());

	/////////////////////////////////////////////////
	// OS-specific login dialogs
	/////////////////////////////////////////////////
#if LL_WINDOWS
	/*
	// Display initial login screen, comes up quickly. JC
	{
		LLSplashScreen::hide();

		INT_PTR result = DialogBox(hInstance, L"CONNECTBOX", NULL, login_dialog_func);
		if (result < 0)
		{
			llwarns << "Connect dialog box failed, returned " << result << llendl;
			return 1;
		}
		// success, result contains which button user clicked
		llinfos << "Connect dialog box clicked " << result << llendl;

		LLSplashScreen::show();
	}
	*/
#endif

	// track number of times that app has run
	mNumSessions = gSavedSettings.getS32("NumSessions");
	mNumSessions++;
	gSavedSettings.setS32("NumSessions", mNumSessions);

	gSavedSettings.setString("HelpLastVisitedURL",gSavedSettings.getString("HelpHomeURL"));

	if (gSavedSettings.getBOOL("VerboseLogs"))
	{
		LLError::setPrintLocation(true);
	}

#if !LL_RELEASE_FOR_DOWNLOAD
	if (gGridChoice == GRID_INFO_NONE)
	{
		// Development version: load last server choice by default (overridden by cmd line args)
		
		S32 server = gSavedSettings.getS32("ServerChoice");
		if (server != 0)
			gGridChoice = (EGridInfo)llclamp(server, 0, (S32)GRID_INFO_COUNT - 1);
		if (server == GRID_INFO_OTHER)
		{
			LLString custom_server = gSavedSettings.getString("CustomServer");
			if (custom_server.empty())
			{
				snprintf(gGridName, MAX_STRING, "none");		/* Flawfinder: ignore */
			}
			else
			{
				snprintf(gGridName, MAX_STRING, "%s", custom_server.c_str());		/* Flawfinder: ignore */
			}
		}
	}
#endif

	if (gGridChoice == GRID_INFO_NONE)
	{
		gGridChoice = GridDefaultChoice;
	}
	
	// Load art UUID information, don't require these strings to be declared in code.
	LLString viewer_art_filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"viewerart.xml");
	llinfos << "Loading art table from " << viewer_art_filename << llendl;
	gViewerArt.loadFromFile(viewer_art_filename.c_str(), FALSE);
	LLString textures_filename = gDirUtilp->getExpandedFilename(LL_PATH_SKINS, "textures", "textures.xml");
	llinfos << "Loading art table from " << textures_filename << llendl;
	gViewerArt.loadFromFile(textures_filename.c_str(), FALSE);

	LLString colors_base_filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "colors_base.xml");
	llinfos << "Loading base colors from " << colors_base_filename << llendl;
	gColors.loadFromFile(colors_base_filename.c_str(), FALSE, TYPE_COL4U);

	// Load overrides from user colors file
	LLString user_colors_filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "colors.xml");
	llinfos << "Loading user colors from " << user_colors_filename << llendl;
	if (gColors.loadFromFile(user_colors_filename.c_str(), FALSE, TYPE_COL4U) == 0)
	{
		llinfos << "Failed to load user colors from " << user_colors_filename << llendl;
		LLString user_legacy_colors_filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "colors.ini");
		llinfos << "Loading legacy colors from " << user_legacy_colors_filename << llendl;
		gColors.loadFromFileLegacy(user_legacy_colors_filename.c_str(), FALSE, TYPE_COL4U);
	}

	// Widget construction depends on LLUI being initialized
	LLUI::initClass(&gSavedSettings, 
					&gColors, 
					&gViewerArt,
					&gImageList,
					ui_audio_callback,
					&LLUI::sGLScaleFactor);

	LLWeb::initClass();			  // do this after LLUI
	gUICtrlFactory->setupPaths(); // update paths with correct language set
	
	/////////////////////////////////////////////////
	//
	// Load settings files
	//
	//
	LLGroupMgr::parseRoleActions("role_actions.xml");

	LLAgent::parseTeleportMessages("teleport_strings.xml");

	// load MIME type -> media impl mappings
	LLMIMETypes::parseMIMETypes( "mime_types.xml" ); 

	mCrashBehavior = gCrashSettings.getS32(CRASH_BEHAVIOR_SETTING);

	LLVectorPerformanceOptions::initClass();

	// Move certain saved settings into global variables for speed
	saved_settings_to_globals();


	// Find partition serial number (Windows) or hardware serial (Mac)
	mSerialNumber = generateSerialNumber();

	if(false == initHardwareTest())
	{
		// Early out from user choice.
		return false;
	}

	// Always fetch the Ethernet MAC address, needed both for login
	// and password load.
	LLUUID::getNodeID(gMACAddress);

	// Prepare for out-of-memory situations, during which we will crash on
	// purpose and save a dump.
#if LL_WINDOWS && LL_RELEASE_FOR_DOWNLOAD && LL_USE_SMARTHEAP
	MemSetErrorHandler(first_mem_error_handler);
#endif // LL_WINDOWS && LL_RELEASE_FOR_DOWNLOAD && LL_USE_SMARTHEAP

	gViewerStats = new LLViewerStats();

	//
	// Initialize the VFS, and gracefully handle initialization errors
	//

	if (!initCache())
	{
		std::ostringstream msg;
		msg <<
			gSecondLife << " is unable to access a file that it needs.\n"
			"\n"
			"This can be because you somehow have multiple copies running, "
			"or your system incorrectly thinks a file is open. "
			"If this message persists, restart your computer and try again. "
			"If it continues to persist, you may need to completely uninstall " <<
			gSecondLife << " and reinstall it.";
		OSMessageBox(
			msg.str().c_str(),
			NULL,
			OSMB_OK);
		return 1;
	}
	
#if LL_DARWIN
	// Display the release notes for the current version
	if(!gHideLinks && gCurrentVersion != gLastRunVersion)
	{
		// Current version and last run version don't match exactly.  Display the release notes.
		DisplayReleaseNotes();
	}
#endif

	//
	// Initialize the window
	//
	initWindow();

	#if LL_WINDOWS && LL_LCD_COMPILE
		// start up an LCD window on a logitech keyboard, if there is one
		HINSTANCE hInstance = GetModuleHandle(NULL);
		gLcdScreen = new LLLCD(hInstance);
		CreateLCDDebugWindows();
	#endif

	gGLManager.getGLInfo(gDebugInfo);
	llinfos << gGLManager.getGLInfoString() << llendl;

	//load key settings
	bind_keyboard_functions();

	// Load Default bindings
	if (!gViewerKeyboard.loadBindings(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"keys.ini").c_str()))
	{
		llerrs << "Unable to open keys.ini" << llendl;
	}
	// Load Custom bindings (override defaults)
	gViewerKeyboard.loadBindings(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"custom_keys.ini").c_str());

	// If we don't have the right GL requirements, exit.
	if (!gGLManager.mHasRequirements && !gNoRender)
	{	
		// can't use an alert here since we're existing and
		// all hell breaks lose.
		OSMessageBox(
			LLAlertDialog::getTemplateMessage("UnsupportedGLRequirements").c_str(),
			NULL,
			OSMB_OK);
		return 0;
	}

	// alert the user if they are using unsupported hardware
	if(!gSavedSettings.getBOOL("AlertedUnsupportedHardware"))
	{
		bool unsupported = false;
		LLString::format_map_t args;
		LLString minSpecs;
		
		// get cpu data from xml
		std::stringstream minCPUString(LLAlertDialog::getTemplateMessage("UnsupportedCPUAmount"));
		S32 minCPU = 0;
		minCPUString >> minCPU;

		// get RAM data from XML
		std::stringstream minRAMString(LLAlertDialog::getTemplateMessage("UnsupportedRAMAmount"));
		U64 minRAM = 0;
		minRAMString >> minRAM;
		minRAM = minRAM * 1024 * 1024;

		if(!gFeatureManagerp->isGPUSupported())
		{
			minSpecs += LLAlertDialog::getTemplateMessage("UnsupportedGPU");
			minSpecs += "\n";
			unsupported = true;
		}
		if(gSysCPU.getMhz() < minCPU)
		{
			minSpecs += LLAlertDialog::getTemplateMessage("UnsupportedCPU");
			minSpecs += "\n";
			unsupported = true;
		}
		if(gSysMemory.getPhysicalMemoryClamped() < minRAM)
		{
			minSpecs += LLAlertDialog::getTemplateMessage("UnsupportedRAM");
			minSpecs += "\n";
			unsupported = true;
		}

		if(unsupported)
		{
			args["MINSPECS"] = minSpecs;
			gViewerWindow->alertXml("UnsupportedHardware", args );
			
			// turn off flag
			gSavedSettings.setBOOL("AlertedUnsupportedHardware", TRUE);
		}
	}
	
	// Save the current version to the prefs file
	gSavedSettings.setString("LastRunVersion", gCurrentVersion);

	gSimLastTime = gRenderStartTime.getElapsedTimeF32();
	gSimFrames = (F32)gFrameCount;

	return true;
}

bool LLAppViewer::mainLoop()
{
	//-------------------------------------------
	// Run main loop until time to quit
	//-------------------------------------------

	// Create IO Pump to use for HTTP Requests.
	gServicePump = new LLPumpIO(gAPRPoolp);
	LLHTTPClient::setPump(*gServicePump);
	LLCurl::setCAFile(gDirUtilp->getCAFile());
	
	// initialize voice stuff here
	gLocalSpeakerMgr = new LLLocalSpeakerMgr();
	gActiveChannelSpeakerMgr = new LLActiveSpeakerMgr();

	LLVoiceChannel::initClass();
	LLVoiceClient::init(gServicePump);
				
	LLMemType mt1(LLMemType::MTYPE_MAIN);
	LLTimer frameTimer,idleTimer;
	LLTimer debugTime;
	
	// Handle messages
	while (!LLApp::isExiting())
	{
		LLFastTimer::reset(); // Should be outside of any timer instances
		{
			LLFastTimer t(LLFastTimer::FTM_FRAME);

			{
				LLFastTimer t2(LLFastTimer::FTM_MESSAGES);
			#if LL_WINDOWS
				if (!LLWinDebug::setupExceptionHandler())
				{
					llwarns << " Someone took over my exception handler (post messagehandling)!" << llendl;
				}
			#endif

				gViewerWindow->mWindow->gatherInput();
			}

#if 1 && !LL_RELEASE_FOR_DOWNLOAD
			// once per second debug info
			if (debugTime.getElapsedTimeF32() > 1.f)
			{
				debugTime.reset();
			}
#endif

			if (!LLApp::isExiting())
			{
				// Scan keyboard for movement keys.  Command keys and typing
				// are handled by windows callbacks.  Don't do this until we're
				// done initializing.  JC
				if (gViewerWindow->mWindow->getVisible() 
					&& gViewerWindow->getActive()
					&& !gViewerWindow->mWindow->getMinimized()
					&& LLStartUp::getStartupState() == STATE_STARTED
					&& !gViewerWindow->getShowProgress()
					&& !gFocusMgr.focusLocked())
				{
					gKeyboard->scanKeyboard();
					LLViewerJoystick::scanJoystick();
				}

				// Update state based on messages, user input, object idle.
				{
					LLFastTimer t3(LLFastTimer::FTM_IDLE);
					idle();

					{
						LLFastTimer t4(LLFastTimer::FTM_PUMP);
						gAres->process();
						// this pump is necessary to make the login screen show up
						gServicePump->pump();
						gServicePump->callback();
					}
				}

				if (gDoDisconnect && (LLStartUp::getStartupState() == STATE_STARTED))
				{
					saveFinalSnapshot();
					disconnectViewer();
				}

				// Render scene.
				if (!LLApp::isExiting())
				{
					display();

					LLFloaterSnapshot::update(); // take snapshots
					
#if LL_WINDOWS && LL_LCD_COMPILE
					// update LCD Screen
					gLcdScreen->UpdateDisplay();
#endif
				}

			}

			// Sleep and run background threads
			{
				LLFastTimer t2(LLFastTimer::FTM_SLEEP);
				bool run_multiple_threads = gSavedSettings.getBOOL("RunMultipleThreads");

				// yield some time to the os based on command line option
				if(gYieldTime)
				{
					ms_sleep(gYieldMS);
				}

				// yield cooperatively when not running as foreground window
				if (   gNoRender
						|| !gViewerWindow->mWindow->getVisible()
						|| !gFocusMgr.getAppHasFocus())
				{
					// Sleep if we're not rendering, or the window is minimized.
					S32 milliseconds_to_sleep = llclamp(gSavedSettings.getS32("BackgroundYieldTime"), 0, 1000);
					// don't sleep when BackgroundYieldTime set to 0, since this will still yield to other threads
					// of equal priority on Windows
					if (milliseconds_to_sleep > 0)
					{
						ms_sleep(milliseconds_to_sleep);
						// also pause worker threads during this wait period
						LLAppViewer::getTextureCache()->pause();
						LLAppViewer::getImageDecodeThread()->pause();
					}
				}
				
				if (gRandomizeFramerate)
				{
					ms_sleep(rand() % 200);
				}

				if (gPeriodicSlowFrame
					&& (gFrameCount % 10 == 0))
				{
					llinfos << "Periodic slow frame - sleeping 500 ms" << llendl;
					ms_sleep(500);
				}


				const F64 min_frame_time = 0.0; //(.0333 - .0010); // max video frame rate = 30 fps
				const F64 min_idle_time = 0.0; //(.0010); // min idle time = 1 ms
				const F64 max_idle_time = run_multiple_threads ? min_idle_time : llmin(.005*10.0*gFrameTimeSeconds, 0.005); // 5 ms a second
				idleTimer.reset();
				while(1)
				{
					S32 work_pending = 0;
					S32 io_pending = 0;
 					work_pending += LLAppViewer::getTextureCache()->update(1); // unpauses the texture cache thread
 					work_pending += LLAppViewer::getImageDecodeThread()->update(1); // unpauses the image thread
 					work_pending += LLAppViewer::getTextureFetch()->update(1); // unpauses the texture fetch thread
					io_pending += LLVFSThread::updateClass(1);
					io_pending += LLLFSThread::updateClass(1);
					if (io_pending > 1000)
					{
						ms_sleep(llmin(io_pending/100,100)); // give the vfs some time to catch up
					}

					F64 frame_time = frameTimer.getElapsedTimeF64();
					F64 idle_time = idleTimer.getElapsedTimeF64();
					if (frame_time >= min_frame_time &&
						idle_time >= min_idle_time &&
						(!work_pending || idle_time >= max_idle_time))
					{
						break;
					}
				}
				frameTimer.reset();

				 // Prevent the worker threads from running while rendering.
				// if (LLThread::processorCount()==1) //pause() should only be required when on a single processor client...
				if (run_multiple_threads == FALSE)
				{
					LLAppViewer::getTextureCache()->pause();
					LLAppViewer::getImageDecodeThread()->pause();
					// LLAppViewer::getTextureFetch()->pause(); // Don't pause the fetch (IO) thread
				}
				//LLVFSThread::sLocal->pause(); // Prevent the VFS thread from running while rendering.
				//LLLFSThread::sLocal->pause(); // Prevent the LFS thread from running while rendering.
			}
						
		}
	}

	// Save snapshot for next time, if we made it through initialization
	if (STATE_STARTED == LLStartUp::getStartupState())
	{
		saveFinalSnapshot();
	}
	
	delete gServicePump;

	llinfos << "Exiting main_loop" << llendflush;

	return true;
}

bool LLAppViewer::cleanup()
{
	//flag all elements as needing to be destroyed immediately
	// to ensure shutdown order
	LLMortician::setZealous(TRUE);

	LLVoiceClient::terminate();
	
	disconnectViewer();

	llinfos << "Viewer disconnected" << llendflush;

	display_cleanup(); 

	release_start_screen(); // just in case

	LLError::logToFixedBuffer(NULL);

	llinfos << "Cleaning Up" << llendflush;

	LLKeyframeDataCache::clear();
	
	// Must clean up texture references before viewer window is destroyed.
	LLHUDObject::cleanupHUDObjects();
	llinfos << "HUD Objects cleaned up" << llendflush;

 	// End TransferManager before deleting systems it depends on (Audio, VFS, AssetStorage)
#if 0 // this seems to get us stuck in an infinite loop...
	gTransferManager.cleanup();
#endif
	
	// Clean up map data storage
	delete gWorldMap;
	gWorldMap = NULL;

	delete gHUDManager;
	gHUDManager = NULL;

	delete gToolMgr;
	gToolMgr = NULL;

	delete gAssetStorage;
	gAssetStorage = NULL;

	LLPolyMesh::freeAllMeshes();

	delete gCacheName;
	gCacheName = NULL;

	delete gGlobalEconomy;
	gGlobalEconomy = NULL;

	delete gActiveChannelSpeakerMgr;
	gActiveChannelSpeakerMgr = NULL;

	delete gLocalSpeakerMgr;
	gLocalSpeakerMgr = NULL;

	LLNotifyBox::cleanup();

	llinfos << "Global stuff deleted" << llendflush;

#if !LL_RELEASE_FOR_DOWNLOAD
	if (gAudiop)
	{
		gAudiop->shutdown();
	}
#else
	// This hack exists because fmod likes to occasionally hang forever
	// when shutting down for no apparent reason.
	llwarns << "Hack, skipping audio engine cleanup" << llendflush;
#endif

	llinfos << "Cleaning up feature manager" << llendflush;
	delete gFeatureManagerp;
	gFeatureManagerp = NULL;

	// Patch up settings for next time
	// Must do this before we delete the viewer window,
	// such that we can suck rectangle information out of
	// it.
	cleanupSavedSettings();
	llinfos << "Settings patched up" << llendflush;

	delete gAudiop;
	gAudiop = NULL;

	// delete some of the files left around in the cache.
	removeCacheFiles("*.wav");
	removeCacheFiles("*.tmp");
	removeCacheFiles("*.lso");
	removeCacheFiles("*.out");
	removeCacheFiles("*.dsf");
	removeCacheFiles("*.bodypart");
	removeCacheFiles("*.clothing");

	llinfos << "Cache files removed" << llendflush;


	cleanup_menus();

	// Wait for any pending VFS IO
	while (1)
	{
		S32 pending = LLVFSThread::updateClass(0);
		pending += LLLFSThread::updateClass(0);
		if (!pending)
		{
			break;
		}
		llinfos << "Waiting for pending IO to finish: " << pending << llendflush;
		ms_sleep(100);
	}
	llinfos << "Shutting down." << llendflush;
	
	// Destroy Windows(R) window, and make sure we're not fullscreen
	// This may generate window reshape and activation events.
	// Therefore must do this before destroying the message system.
	delete gViewerWindow;
	gViewerWindow = NULL;
	llinfos << "ViewerWindow deleted" << llendflush;

	// viewer UI relies on keyboard so keep it aound until viewer UI isa gone
	delete gKeyboard;
	gKeyboard = NULL;

	// Clean up selection managers after UI is destroyed, as UI
	// may be observing them.
	LLSelectMgr::cleanupGlobals();

	LLViewerObject::cleanupVOClasses();

	LLWaterParamManager::cleanupClass();
	LLWLParamManager::cleanupClass();
	LLPostProcess::cleanupClass();

	LLTracker::cleanupInstance();
	
	// *FIX: This is handled in LLAppViewerWin32::cleanup().
	// I'm keeping the comment to remember its order in cleanup,
	// in case of unforseen dependency.
	//#if LL_WINDOWS
	//	gDXHardware.cleanup();
	//#endif // LL_WINDOWS

#if LL_WINDOWS && LL_LCD_COMPILE
	// shut down the LCD window on a logitech keyboard, if there is one
	delete gLcdScreen;
	gLcdScreen = NULL;
#endif

	if (!gVolumeMgr->cleanup())
	{
		llwarns << "Remaining references in the volume manager!" << llendflush;
	}

	LLViewerParcelMgr::cleanupGlobals();

	delete gViewerStats;
	gViewerStats = NULL;

 	//end_messaging_system();

	LLFollowCamMgr::cleanupClass();
	LLVolumeMgr::cleanupClass();
	LLWorldMapView::cleanupClass();
	LLUI::cleanupClass();
	
	//
	// Shut down the VFS's AFTER the decode manager cleans up (since it cleans up vfiles).
	// Also after viewerwindow is deleted, since it may have image pointers (which have vfiles)
	// Also after shutting down the messaging system since it has VFS dependencies
	//
	LLVFile::cleanupClass();
	llinfos << "VFS cleaned up" << llendflush;

	// Store the time of our current logoff
	gSavedPerAccountSettings.setU32("LastLogoff", time_corrected());

	// Must do this after all panels have been deleted because panels that have persistent rects
	// save their rects on delete.
	gSavedSettings.saveToFile(gSettingsFileName, TRUE);
	if (!gPerAccountSettingsFileName.empty())
	{
		gSavedPerAccountSettings.saveToFile(gPerAccountSettingsFileName, TRUE);
	}
	llinfos << "Saved settings" << llendflush;

	std::string crash_settings_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, CRASH_SETTINGS_FILE);
	// save all settings, even if equals defaults
	gCrashSettings.saveToFile(crash_settings_filename.c_str(), FALSE);

	delete gUICtrlFactory;
	gUICtrlFactory = NULL;

	gSavedSettings.cleanup();
	gViewerArt.cleanup();
	gColors.cleanup();
	gCrashSettings.cleanup();

	// Save URL history file
	LLURLHistory::saveFile("url_history.xml");

	if (gMuteListp)
	{
		// save mute list
		gMuteListp->cache(gAgent.getID());

		delete gMuteListp;
		gMuteListp = NULL;
	}

	if (mPurgeOnExit)
	{
		llinfos << "Purging all cache files on exit" << llendflush;
		char mask[LL_MAX_PATH];		/* Flawfinder: ignore */
		snprintf(mask, LL_MAX_PATH, "%s*.*", gDirUtilp->getDirDelimiter().c_str());		/* Flawfinder: ignore */
		gDirUtilp->deleteFilesInDir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"").c_str(),mask);
	}

	removeMarkerFile(); // Any crashes from here on we'll just have to ignore
	
	closeDebug();

	// Let threads finish
	LLTimer idleTimer;
	idleTimer.reset();
	const F64 max_idle_time = 5.f; // 5 seconds
	while(1)
	{
		S32 pending = 0;
		pending += LLAppViewer::getTextureCache()->update(1); // unpauses the worker thread
		pending += LLAppViewer::getImageDecodeThread()->update(1); // unpauses the image thread
		pending += LLAppViewer::getTextureFetch()->update(1); // unpauses the texture fetch thread
		pending += LLVFSThread::updateClass(0);
		pending += LLLFSThread::updateClass(0);
		F64 idle_time = idleTimer.getElapsedTimeF64();
		if (!pending || idle_time >= max_idle_time)
		{
			llwarns << "Quitting with pending background tasks." << llendl;
			break;
		}
	}
	
	// Delete workers first
	// shotdown all worker threads before deleting them in case of co-dependencies
	sTextureCache->shutdown();
	sTextureFetch->shutdown();
	sImageDecodeThread->shutdown();
	delete sTextureCache;
    sTextureCache = NULL;
	delete sTextureFetch;
    sTextureFetch = NULL;
	delete sImageDecodeThread;
    sImageDecodeThread = NULL;

	gImageList.shutdown(); // shutdown again in case a callback added something
	
	// This should eventually be done in LLAppViewer
	LLImageJ2C::closeDSO();
	LLImageFormatted::cleanupClass();
	LLVFSThread::cleanupClass();
	LLLFSThread::cleanupClass();

	llinfos << "VFS Thread finished" << llendflush;

#ifndef LL_RELEASE_FOR_DOWNLOAD
	llinfos << "Auditing VFS" << llendl;
	gVFS->audit();
#endif

	// For safety, the LLVFS has to be deleted *after* LLVFSThread. This should be cleaned up.
	// (LLVFS doesn't know about LLVFSThread so can't kill pending requests) -Steve
	delete gStaticVFS;
	gStaticVFS = NULL;
	delete gVFS;
	gVFS = NULL;
	
	end_messaging_system();

    // *NOTE:Mani - The following call is not thread safe. 
    LLCurl::cleanupClass();

    // If we're exiting to launch an URL, do that here so the screen
	// is at the right resolution before we launch IE.
	if (!gLaunchFileOnQuit.empty())
	{
#if LL_WINDOWS
		// Indicate an application is starting.
		SetCursor(LoadCursor(NULL, IDC_WAIT));
#endif

		// HACK: Attempt to wait until the screen res. switch is complete.
		ms_sleep(1000);

		LLWeb::loadURLExternal( gLaunchFileOnQuit );
	}


    llinfos << "Goodbye" << llendflush;
	// return 0;
	return true;
}

bool LLAppViewer::initEarlyConfiguration()
{
	// *FIX: globals - This method sets a bunch of globals early in the init process.
	int argc = gTempArgC;
	char** argv = gTempArgV;

	// HACK! We REALLY want to know what grid they were trying to connect to if they
	// crashed hard.
	// So we walk through the command line args ONLY looking for the
	// userserver arguments first.  And we don't do ANYTHING but set
	// the gGridName (which gets passed to the crash reporter).
	// We're assuming that they're trying to log into the same grid as last
	// time, which seems fairly reasonable.
	snprintf(gGridName, MAX_STRING, "%s", gGridInfo[GridDefaultChoice].mName);		// Flawfinder: ignore
	S32 j;
	for (j = 1; j < argc; j++) 
	{
		if (!strcmp(argv[j], "--aditi"))
		{
			snprintf(gGridName, MAX_STRING, "%s", gGridInfo[GRID_INFO_ADITI].mName);		// Flawfinder: ignore
		}
		else if (!strcmp(argv[j], "--agni"))
		{
			snprintf(gGridName, MAX_STRING, "%s", gGridInfo[GRID_INFO_AGNI].mName);		// Flawfinder: ignore
		}
		else if (!strcmp(argv[j], "--siva"))
		{
			snprintf(gGridName, MAX_STRING, "%s", gGridInfo[GRID_INFO_SIVA].mName);		// Flawfinder: ignore
		}
		else if (!strcmp(argv[j], "--shakti"))
		{
			sprintf(gGridName,"%s", gGridInfo[GRID_INFO_SHAKTI].mName);
		}
		else if (!strcmp(argv[j], "--durga"))
		{
			snprintf(gGridName, MAX_STRING, "%s", gGridInfo[GRID_INFO_DURGA].mName);		// Flawfinder: ignore
		}
		else if (!strcmp(argv[j], "--soma"))
		{
			snprintf(gGridName, MAX_STRING, "%s", gGridInfo[GRID_INFO_SOMA].mName);		// Flawfinder: ignore
		}
		else if (!strcmp(argv[j], "--ganga"))
		{
			snprintf(gGridName, MAX_STRING, "%s", gGridInfo[GRID_INFO_GANGA].mName);		// Flawfinder: ignore 
		}
		else if (!strcmp(argv[j], "--vaak"))
		{
			sprintf(gGridName,"%s", gGridInfo[GRID_INFO_VAAK].mName);
		}
		else if (!strcmp(argv[j], "--uma"))
		{
			sprintf(gGridName,"%s", gGridInfo[GRID_INFO_UMA].mName);
		}
		else if (!strcmp(argv[j], "--mohini"))
		{
			sprintf(gGridName,"%s", gGridInfo[GRID_INFO_MOHINI].mName);
		}
		else if (!strcmp(argv[j], "--yami"))
		{
			sprintf(gGridName,"%s", gGridInfo[GRID_INFO_YAMI].mName);
		}
		else if (!strcmp(argv[j], "--nandi"))
		{
			sprintf(gGridName,"%s", gGridInfo[GRID_INFO_NANDI].mName);
		}
		else if (!strcmp(argv[j], "--mitra"))
		{
			sprintf(gGridName,"%s", gGridInfo[GRID_INFO_MITRA].mName);
		}
		else if (!strcmp(argv[j], "--radha"))
		{
			sprintf(gGridName,"%s", gGridInfo[GRID_INFO_RADHA].mName);
		}
		else if (!strcmp(argv[j], "--ravi"))
		{
			sprintf(gGridName,"%s", gGridInfo[GRID_INFO_RAVI].mName);
		}
		else if (!strcmp(argv[j], "--aruna"))
		{
			sprintf(gGridName,"%s", gGridInfo[GRID_INFO_ARUNA].mName);
		}
		else if (!strcmp(argv[j], "-multiple"))
		{
			// Hack to detect -multiple so we can disable the marker file check (which will always fail)
			gMultipleViewersOK = TRUE;
		}
		else if (!strcmp(argv[j], "-novoice"))
		{
			// May need to know this early also
			gDisableVoice = TRUE;
		}
		else if (!strcmp(argv[j], "-url") && (++j < argc)) 
		{
			LLURLSimString::setString(argv[j]);
		}
	}

	return true;
}

bool LLAppViewer::initThreads()
{
#if MEM_TRACK_MEM
	static const bool enable_threads = false;
#else
	static const bool enable_threads = true;
#endif
	LLVFSThread::initClass(enable_threads && true);
	LLLFSThread::initClass(enable_threads && true);

	// Image decoding
	LLAppViewer::sImageDecodeThread = new LLWorkerThread("ImageDecode", enable_threads && true);
	LLAppViewer::sTextureCache = new LLTextureCache(enable_threads && true);
	LLAppViewer::sTextureFetch = new LLTextureFetch(LLAppViewer::getTextureCache(), enable_threads && false);
	LLImageWorker::initClass(LLAppViewer::getImageDecodeThread());
	LLImageJ2C::openDSO();

	// *FIX: no error handling here!
	return true;
}

void errorCallback(const std::string &error_string)
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	OSMessageBox(error_string.c_str(), "Fatal Error", OSMB_OK);
#endif

	//Set the ErrorActivated global so we know to create a marker file
	gLLErrorActivated = true;
	
	LLError::crashAndLoop(error_string);
}

bool LLAppViewer::initLogging()
{
	//
	// Set up logging defaults for the viewer
	//
	LLError::initForApplication(
				gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, ""));
	LLError::setFatalFunction(errorCallback);
	
	// Remove the last ".old" log file.
	std::string old_log_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,
							     "SecondLife.old");
	LLFile::remove(old_log_file.c_str());

	// Rename current log file to ".old"
	std::string log_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,
							     "SecondLife.log");
	LLFile::rename(log_file.c_str(), old_log_file.c_str());

	// Set the log file to SecondLife.log

	LLError::logToFile(log_file);

	// *FIX:Mani no error handling here!
	return true;
}

bool LLAppViewer::initConfiguration()
{
	// Ye olde parse_args()...
	if(!doConfigFromCommandLine())
	{
		return false;
	}
	
	// XUI:translate
	gSecondLife = "Second Life";

	// Read skin/branding settings if specified.
	if (! gDirUtilp->getSkinDir().empty() )
	{
		std::string skin_def_file = gDirUtilp->getExpandedFilename(LL_PATH_TOP_SKIN, "skin.xml");
		LLXmlTree skin_def_tree;

		if (!skin_def_tree.parseFile(skin_def_file))
		{
			llerrs << "Failed to parse skin definition." << llendl;
		}

		LLXmlTreeNode* rootp = skin_def_tree.getRoot();
		LLXmlTreeNode* disabled_message_node = rootp->getChildByName("disabled_message");	
		if (disabled_message_node)
		{
			gDisabledMessage = disabled_message_node->getContents();
		}

		static LLStdStringHandle hide_links_string = LLXmlTree::addAttributeString("hide_links");
		rootp->getFastAttributeBOOL(hide_links_string, gHideLinks);

		// Legacy string.  This flag really meant we didn't want to expose references to "Second Life".
		// Just set gHideLinks instead.
		static LLStdStringHandle silent_string = LLXmlTree::addAttributeString("silent_update");
		BOOL silent_update;
		rootp->getFastAttributeBOOL(silent_string, silent_update);
		gHideLinks = (gHideLinks || silent_update);
	}

#if LL_DARWIN
	// Initialize apple menubar and various callbacks
	init_apple_menu(gSecondLife.c_str());

#if __ppc__
	// If the CPU doesn't have Altivec (i.e. it's not at least a G4), don't go any further.
	// Only test PowerPC - all Intel Macs have SSE.
	if(!gSysCPU.hasAltivec())
	{
		std::ostringstream msg;
		msg << gSecondLife << " requires a processor with AltiVec (G4 or later).";
		OSMessageBox(
			msg.str().c_str(),
			NULL,
			OSMB_OK);
		removeMarkerFile();
		return false;
	}
#endif
	
#endif // LL_DARWIN

	// Display splash screen.  Must be after above check for previous
	// crash as this dialog is always frontmost.
	std::ostringstream splash_msg;
	splash_msg << "Loading " << gSecondLife << "...";
	LLSplashScreen::show();
	LLSplashScreen::update(splash_msg.str().c_str());

	LLVolumeMgr::initClass();

	// Initialize the feature manager
	// The feature manager is responsible for determining what features
	// are turned on/off in the app.
	gFeatureManagerp = new LLFeatureManager;

	gStartTime = totalTime();

	////////////////////////////////////////
	//
	// Process ini files
	//

	// declare all possible setting variables
	declare_settings();

#if !LL_RELEASE_FOR_DOWNLOAD
//	only write the defaults for non-release builds!
	gSavedSettings.saveToFile(gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,"settings_default.xml").c_str(), FALSE);
#endif

	//
	// Set the name of the window
	//
#if LL_RELEASE_FOR_DOWNLOAD
	gWindowTitle = gSecondLife;
#elif LL_DEBUG
	gWindowTitle = gSecondLife + LLString(" [DEBUG] ") + gArgs;
#else
	gWindowTitle = gSecondLife + LLString(" ") + gArgs;
#endif
	LLString::truncate(gWindowTitle, 255);

	if (!gMultipleViewersOK)
	{
	    //
	    // Check for another instance of the app running
	    //
		//RN: if we received a URL, hand it off to the existing instance
		// don't call anotherInstanceRunning() when doing URL handoff, as
		// it relies on checking a marker file which will not work when running
		// out of different directories
		std::string slurl;
		if (!LLStartUp::sSLURLCommand.empty())
		{
			slurl = LLStartUp::sSLURLCommand;
		}
		else if (LLURLSimString::parse())
		{
			slurl = LLURLSimString::getURL();
		}
		if (!slurl.empty())
		{
			if (send_url_to_other_instance(slurl))
			{
				// successfully handed off URL to existing instance, exit
				return false;
			}
		}
		
		mSecondInstance = anotherInstanceRunning();
		
		if (mSecondInstance)
		{
			std::ostringstream msg;
			msg << 
				gSecondLife << " is already running.\n"
				"\n"
				"Check your task bar for a minimized copy of the program.\n"
				"If this message persists, restart your computer.",
			OSMessageBox(
				msg.str().c_str(),
				NULL,
				OSMB_OK);
			return false;
		}

		initMarkerFile();

#if LL_SEND_CRASH_REPORTS
		if (gLastExecEvent == LAST_EXEC_FROZE)
		{
			llinfos << "Last execution froze, requesting to send crash report." << llendl;
			//
			// Pop up a freeze or crash warning dialog
			//
			std::ostringstream msg;
			msg << gSecondLife
				<< " appears to have frozen or crashed on the previous run.\n"
				<< "Would you like to send a crash report?";
			std::string alert;
			alert = gSecondLife;
			alert += " Alert";
			S32 choice = OSMessageBox(msg.str().c_str(),
				alert.c_str(),
				OSMB_YESNO);
			if (OSBTN_YES == choice)
			{
				llinfos << "Sending crash report." << llendl;

#if LL_WINDOWS
				std::string exe_path = gDirUtilp->getAppRODataDir();
				exe_path += gDirUtilp->getDirDelimiter();
				exe_path += "win_crash_logger.exe";

				std::string arg_string = "-previous ";
				// Spawn crash logger.
				// NEEDS to wait until completion, otherwise log files will get smashed.
				_spawnl(_P_WAIT, exe_path.c_str(), exe_path.c_str(), arg_string.c_str(), NULL);
#elif LL_DARWIN
				std::string command_str;
				command_str = "crashreporter.app/Contents/MacOS/crashreporter ";
				command_str += "-previous";
				// XXX -- We need to exit fullscreen mode for this to work.
				// XXX -- system() also doesn't wait for completion.  Hmm...
				system(command_str.c_str());		/* Flawfinder: Ignore */
#elif LL_LINUX || LL_SOLARIS
				std::string cmd =gDirUtilp->getAppRODataDir();
				cmd += gDirUtilp->getDirDelimiter();
#if LL_LINUX
				cmd += "linux-crash-logger.bin";
#else // LL_SOLARIS
				cmd += "bin/solaris-crash-logger";
#endif
				char* const cmdargv[] =
					{(char*)cmd.c_str(),
					 (char*)"-previous",
					 NULL};
				pid_t pid = fork();
				if (pid == 0)
				{ // child
					execv(cmd.c_str(), cmdargv);		/* Flawfinder: Ignore */
					llwarns << "execv failure when trying to start " << cmd << llendl;
					_exit(1); // avoid atexit()
				} else {
					if (pid > 0)
					{
						// wait for child proc to die
						int childExitStatus;
						waitpid(pid, &childExitStatus, 0);
					} else {
						llwarns << "fork failure." << llendl;
					}
				}
#endif
			}
			else
			{
				llinfos << "Not sending crash report." << llendl;
			}
		}
#endif // #if LL_SEND_CRASH_REPORTS
	}
	else
	{
		mSecondInstance = anotherInstanceRunning();
		
		if (mSecondInstance)
		{
			gDisableVoice = TRUE;
			/* Don't start another instance if using -multiple
			//RN: if we received a URL, hand it off to the existing instance
		    if (LLURLSimString::parse())
		    {
			    LLURLSimString::send_to_other_instance();
				return 1;
			}
			*/
		}

		initMarkerFile();
	}

	return true; // Config was successful.
}

bool LLAppViewer::doConfigFromCommandLine()
{
	// *FIX: This is what parse args used to do, minus the arg reading part.
	// Now the arg parsing is handled by LLApp::parseCommandOptions() and this
	// method need only interpret settings. Perhaps some day interested parties 
	// can ask an app about a setting rather than have the app set 
	// a gazzillion globals.
	
	/////////////////////////////////////////
	//
	// Process command line arguments
	//
	S32 args_result = 0;

#if LL_DARWIN
	{
		// On the Mac, read in arguments.txt (if it exists) and process it for additional arguments.
		LLString args;
		if(_read_file_into_string(args, "arguments.txt"))		/* Flawfinder: ignore*/
		{
			// The arguments file exists.  
			// It should consist of command line arguments separated by newlines.
			// Split it into individual arguments and build a fake argv[] to pass to parse_args.
			std::vector<std::string> arglist;
			
			arglist.push_back("newview");
			
			llinfos << "Reading additional command line arguments from arguments.txt..." << llendl;
			
			typedef boost::tokenizer<boost::escaped_list_separator<char> > tokenizer;
			boost::escaped_list_separator<char> sep("\\", "\r\n ", "\"'");
			tokenizer tokens(args, sep);
			tokenizer::iterator token_iter;

			for(token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
			{
				llinfos << "argument: '" << (token_iter->c_str()) << "'" << llendl;
				
				arglist.push_back(*token_iter);
			}

			char **fakeargv = new char*[arglist.size()];
			int i;
			for(i=0; i < arglist.size(); i++)
				fakeargv[i] = const_cast<char*>(arglist[i].c_str());
				
			args_result = parse_args(arglist.size(), fakeargv);
			delete[] fakeargv;
		}
		
		// Get the user's preferred language string based on the Mac OS localization mechanism.
		// To add a new localization:
			// go to the "Resources" section of the project
			// get info on "language.txt"
			// in the "General" tab, click the "Add Localization" button
			// create a new localization for the language you're adding
			// set the contents of the new localization of the file to the string corresponding to our localization
			//   (i.e. "en-us", "ja", etc.  Use the existing ones as a guide.)
		CFURLRef url = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("language"), CFSTR("txt"), NULL);
		char path[MAX_PATH];
		if(CFURLGetFileSystemRepresentation(url, false, (UInt8 *)path, sizeof(path)))
		{
			LLString lang;
			if(_read_file_into_string(lang, path))		/* Flawfinder: ignore*/
			{
				gCommandLineForcedSettings["SystemLanguage"] = lang;
			}
		}
		CFRelease(url);
	}
#endif

	int argc = gTempArgC;
	char** argv = gTempArgV;

	//
	// Parse the command line arguments
	//
	args_result |= parse_args(argc, argv);
	if (args_result)
	{
		removeMarkerFile();
		return false;
	}

	return true;
}

bool LLAppViewer::initWindow()
{
	llinfos << "Initializing window..." << llendl;

	// store setting in a global for easy access and modification
	gNoRender = gSavedSettings.getBOOL("DisableRendering");

	// Hide the splash screen
	LLSplashScreen::hide();

	// HACK: Need a non-const char * for stupid window name (propagated deep down)
	char window_title_str[256];		/* Flawfinder: ignore */
	strncpy(window_title_str, gWindowTitle.c_str(), sizeof(window_title_str) - 1);		/* Flawfinder: ignore */
	window_title_str[sizeof(window_title_str) - 1] = '\0';

	// always start windowed
	gViewerWindow = new LLViewerWindow(window_title_str, "Second Life",
		gSavedSettings.getS32("WindowX"), gSavedSettings.getS32("WindowY"),
		gSavedSettings.getS32("WindowWidth"), gSavedSettings.getS32("WindowHeight"),
		FALSE, gIgnorePixelDepth);
		
	if (gSavedSettings.getBOOL("FullScreen"))
	{
		gViewerWindow->toggleFullscreen(FALSE);
			// request to go full screen... which will be delayed until login
	}
	
	if (gSavedSettings.getBOOL("WindowMaximized"))
	{
		gViewerWindow->mWindow->maximize();
		gViewerWindow->getWindow()->setNativeAspectRatio(gSavedSettings.getF32("FullScreenAspectRatio"));
	}

	if (!gNoRender)
	{
		//
		// Initialize GL stuff
		//

		// Set this flag in case we crash while initializing GL
		gSavedSettings.setBOOL("RenderInitError", TRUE);
		gSavedSettings.saveToFile( gSettingsFileName, TRUE );
	
		gPipeline.init();
		stop_glerror();
		gViewerWindow->initGLDefaults();

		gSavedSettings.setBOOL("RenderInitError", FALSE);
		gSavedSettings.saveToFile( gSettingsFileName, TRUE );
	}

	LLUI::sWindow = gViewerWindow->getWindow();

	LLAlertDialog::parseAlerts("alerts.xml");
	LLNotifyBox::parseNotify("notify.xml");

	// *TODO - remove this when merging into release
	// DON'T Clean up the feature manager lookup table - settings are needed
	// for setting the graphics level.
	//gFeatureManagerp->cleanupFeatureTables();

	// Show watch cursor
	gViewerWindow->setCursor(UI_CURSOR_WAIT);

	// Finish view initialization
	gViewerWindow->initBase();

	// show viewer window
	gViewerWindow->mWindow->show();
	
	return true;
}

void LLAppViewer::closeDebug()
{
	std::string debug_filename = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"debug_info.log");
	llinfos << "Opening debug file " << debug_filename << llendl;
	std::ofstream out_file(debug_filename.c_str());
	LLSDSerialize::toPrettyXML(gDebugInfo, out_file);
	out_file.close();
}

void LLAppViewer::cleanupSavedSettings()
{
	gSavedSettings.setBOOL("MouseSun", FALSE);

	gSavedSettings.setBOOL("FlyBtnState", FALSE);

	gSavedSettings.setBOOL("FirstPersonBtnState", FALSE);
	gSavedSettings.setBOOL("ThirdPersonBtnState", TRUE);
	gSavedSettings.setBOOL("BuildBtnState", FALSE);

	gSavedSettings.setBOOL("UseEnergy", TRUE);				// force toggle to turn off, since sends message to simulator

	gSavedSettings.setBOOL("DebugWindowProc", gDebugWindowProc);
		
	gSavedSettings.setBOOL("AllowIdleAFK", gAllowIdleAFK);
	gSavedSettings.setBOOL("ShowObjectUpdates", gShowObjectUpdates);
	
	if (!gNoRender)
	{
		if (gDebugView)
		{
			gSavedSettings.setBOOL("ShowDebugConsole", gDebugView->mDebugConsolep->getVisible());
			gSavedSettings.setBOOL("ShowDebugStats", gDebugView->mFloaterStatsp->getVisible());
		}
	}

	// save window position if not fullscreen
	// as we don't track it in callbacks
	BOOL fullscreen = gViewerWindow->mWindow->getFullscreen();
	BOOL maximized = gViewerWindow->mWindow->getMaximized();
	if (!fullscreen && !maximized)
	{
		LLCoordScreen window_pos;

		if (gViewerWindow->mWindow->getPosition(&window_pos))
		{
			gSavedSettings.setS32("WindowX", window_pos.mX);
			gSavedSettings.setS32("WindowY", window_pos.mY);
		}
	}

	gSavedSettings.setF32("MapScale", gMapScale );
	gSavedSettings.setF32("MiniMapScale", gMiniMapScale );
	gSavedSettings.setBOOL("AsyncKeyboard", gHandleKeysAsync);
	gSavedSettings.setBOOL("ShowHoverTips", LLHoverView::sShowHoverTips);

	// Some things are cached in LLAgent.
	if (gAgent.mInitialized)
	{
		gSavedSettings.setF32("RenderFarClip", gAgent.mDrawDistance);
	}

	// *REMOVE: This is now done via LLAppViewer::setCrashBehavior()
	// Left vestigially in case I borked it.
	// gCrashSettings.setS32(CRASH_BEHAVIOR_SETTING, gCrashBehavior);
}

void LLAppViewer::removeCacheFiles(const char* file_mask)
{
	char mask[LL_MAX_PATH];		/* Flawfinder: ignore */
	snprintf(mask, LL_MAX_PATH, "%s%s", gDirUtilp->getDirDelimiter().c_str(), file_mask);		/* Flawfinder: ignore */
	gDirUtilp->deleteFilesInDir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "").c_str(), mask);
}

void LLAppViewer::writeSystemInfo()
{
	gDebugInfo["SLLog"] = LLError::logFileName();

	gDebugInfo["ClientInfo"]["Name"] = gChannelName;
	gDebugInfo["ClientInfo"]["MajorVersion"] = LL_VERSION_MAJOR;
	gDebugInfo["ClientInfo"]["MinorVersion"] = LL_VERSION_MINOR;
	gDebugInfo["ClientInfo"]["PatchVersion"] = LL_VERSION_PATCH;
	gDebugInfo["ClientInfo"]["BuildVersion"] = LL_VERSION_BUILD;

	gDebugInfo["CPUInfo"]["CPUFamily"] = gSysCPU.getFamily();
	gDebugInfo["CPUInfo"]["CPUMhz"] = gSysCPU.getMhz();
	gDebugInfo["CPUInfo"]["CPUAltivec"] = gSysCPU.hasAltivec();
	gDebugInfo["CPUInfo"]["CPUSSE"] = gSysCPU.hasSSE();
	gDebugInfo["CPUInfo"]["CPUSSE2"] = gSysCPU.hasSSE2();
	
	gDebugInfo["RAMInfo"] = llformat("%u", gSysMemory.getPhysicalMemoryKB());
	gDebugInfo["OSInfo"] = getOSInfo().getOSStringSimple();

	// Dump some debugging info
	llinfos << gSecondLife
			<< " version " << LL_VERSION_MAJOR << "." << LL_VERSION_MINOR << "." << LL_VERSION_PATCH
			<< llendl;

	// Dump the local time and time zone
	time_t now;
	time(&now);
	char tbuffer[256];		/* Flawfinder: ignore */
	strftime(tbuffer, 256, "%Y-%m-%dT%H:%M:%S %Z", localtime(&now));
	llinfos << "Local time: " << tbuffer << llendl;

	// query some system information
	llinfos << "CPU info:\n" << gSysCPU << llendl;
	llinfos << "Memory info:\n" << gSysMemory << llendl;
	llinfos << "OS: " << getOSInfo().getOSStringSimple() << llendl;
	llinfos << "OS info: " << getOSInfo() << llendl;
}

void LLAppViewer::handleViewerCrash()
{
	LLAppViewer* pApp = LLAppViewer::instance();
	if (pApp->beingDebugged())
	{
		// This will drop us into the debugger.
		abort();
	}

	// Returns whether a dialog was shown.
	// Only do the logic in here once
	if (pApp->mReportedCrash)
	{
		return;
	}
	pApp->mReportedCrash = TRUE;

	gDebugInfo["SettingsFilename"] = gSettingsFileName;
	gDebugInfo["CAFilename"] = gDirUtilp->getCAFile();
	gDebugInfo["ViewerExePath"] = gDirUtilp->getExecutablePathAndName().c_str();
	gDebugInfo["CurrentPath"] = gDirUtilp->getCurPath().c_str();
	gDebugInfo["CurrentSimHost"] = gAgent.getRegionHost().getHostName();

	//Write out the crash status file
	//Use marker file style setup, as that's the simplest, especially since
	//we're already in a crash situation	
	if (gDirUtilp)
	{
		LLString crash_file_name;
		if(gLLErrorActivated) crash_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,LLERROR_MARKER_FILE_NAME);
		else crash_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,ERROR_MARKER_FILE_NAME);
		llinfos << "Creating crash marker file " << crash_file_name << llendl;
		apr_file_t* crash_file =  ll_apr_file_open(crash_file_name, LL_APR_W);
		if (crash_file)
		{
			llinfos << "Created crash marker file " << crash_file_name << llendl;
		}
		else
		{
			llwarns << "Cannot create error marker file " << crash_file_name << llendl;
		}
		apr_file_close(crash_file);
	}
	
	if (gMessageSystem && gDirUtilp)
	{
		std::string filename;
		filename = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "stats.log");
		llofstream file(filename.c_str(), llofstream::binary);
		if(file.good())
		{
			gMessageSystem->summarizeLogs(file);
			file.close();
		}
	}

	if (gMessageSystem)
	{
		gMessageSystem->getCircuitInfo(gDebugInfo["CircuitInfo"]);
		gMessageSystem->stopLogging();
	}
	if (gWorldp)
	{
		gWorldp->getInfo(gDebugInfo);
	}

	// Close the debug file
	pApp->closeDebug();
	LLError::logToFile("");

	// Remove the marker file, since otherwise we'll spawn a process that'll keep it locked
	pApp->removeMarkerFile();
	
	// Call to pure virtual, handled by platform specifc llappviewer instance.
	pApp->handleCrashReporting(); 

	return;
}

void LLAppViewer::setCrashBehavior(S32 cb) 
{ 
	mCrashBehavior = cb; 
	gCrashSettings.setS32(CRASH_BEHAVIOR_SETTING, mCrashBehavior);
} 

bool LLAppViewer::anotherInstanceRunning()
{
	// We create a marker file when the program starts and remove the file when it finishes.
	// If the file is currently locked, that means another process is already running.

	std::string marker_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, MARKER_FILE_NAME);
	llinfos << "Checking marker file for lock..." << llendl;

	//Freeze case checks
	apr_file_t* fMarker = ll_apr_file_open(marker_file, LL_APR_RB);		
	if (fMarker != NULL)
	{
		// File exists, try opening with write permissions
		apr_file_close(fMarker);
		fMarker = ll_apr_file_open(marker_file, LL_APR_WB);
		if (fMarker == NULL)
		{
			// Another instance is running. Skip the rest of these operations.
			llinfos << "Marker file is locked." << llendl;
			return TRUE;
		}
		if (apr_file_lock(fMarker, APR_FLOCK_NONBLOCK | APR_FLOCK_EXCLUSIVE) != APR_SUCCESS) //flock(fileno(fMarker), LOCK_EX | LOCK_NB) == -1)
		{
			apr_file_close(fMarker);
			llinfos << "Marker file is locked." << llendl;
			return TRUE;
		}
		// No other instances; we'll lock this file now & delete on quit.
		apr_file_close(fMarker);
	}
	llinfos << "Marker file isn't locked." << llendl;
	return FALSE;
}

void LLAppViewer::initMarkerFile()
{

	//First, check for the existence of other files.
	//There are marker files for two different types of crashes
	
	mMarkerFileName = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,MARKER_FILE_NAME);
	llinfos << "Checking marker file for lock..." << llendl;

	//We've got 4 things to test for here
	// - Other Process Running (SecondLife.exec_marker present, locked)
	// - Freeze (SecondLife.exec_marker present, not locked)
	// - LLError Crash (SecondLife.llerror_marker present)
	// - Other Crash (SecondLife.error_marker present)
	// These checks should also remove these files for the last 2 cases if they currently exist

	//LLError/Error checks. Only one of these should ever happen at a time.
	LLString llerror_marker_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, LLERROR_MARKER_FILE_NAME);
	LLString error_marker_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, ERROR_MARKER_FILE_NAME);
	apr_file_t* fMarker = ll_apr_file_open(llerror_marker_file, LL_APR_RB);
	if(fMarker != NULL)
	{
		apr_file_close(fMarker);
		llinfos << "Last exec LLError crashed, setting LastExecEvent to " << LAST_EXEC_LLERROR_CRASH << llendl;
		gLastExecEvent = LAST_EXEC_LLERROR_CRASH;
	}

	fMarker = ll_apr_file_open(error_marker_file, LL_APR_RB);
	if(fMarker != NULL)
	{
		apr_file_close(fMarker);
		llinfos << "Last exec crashed, setting LastExecEvent to " << LAST_EXEC_OTHER_CRASH << llendl;
		gLastExecEvent = LAST_EXEC_OTHER_CRASH;
	}

	ll_apr_file_remove(llerror_marker_file);
	ll_apr_file_remove(error_marker_file);
	
	//Freeze case checks
	if(anotherInstanceRunning()) return;
	fMarker = ll_apr_file_open(mMarkerFileName, LL_APR_RB);		
	if (fMarker != NULL)
	{
		apr_file_close(fMarker);
		gLastExecEvent = LAST_EXEC_FROZE;
		llinfos << "Exec marker found: program froze on previous execution" << llendl;
	}

	// Create the marker file for this execution & lock it
	mMarkerFile =  ll_apr_file_open(mMarkerFileName, LL_APR_W);
	if (mMarkerFile)
	{
		llinfos << "Marker file created." << llendl;
	}
	else
	{
		llinfos << "Failed to create marker file." << llendl;
	}
	if (apr_file_lock(mMarkerFile, APR_FLOCK_NONBLOCK | APR_FLOCK_EXCLUSIVE) != APR_SUCCESS) 
	{
		apr_file_close(mMarkerFile);
		llinfos << "Marker file cannot be locked." << llendl;
		return;
	}

	llinfos << "Marker file locked." << llendl;
	llinfos << "Exiting initMarkerFile()." << llendl;
}

void LLAppViewer::removeMarkerFile()
{
	llinfos << "removeMarkerFile()" << llendl;
	if (mMarkerFile != NULL)
	{
		ll_apr_file_remove( mMarkerFileName );
		mMarkerFile = NULL;
	}
}

void LLAppViewer::forceQuit()
{ 
	LLApp::setQuitting(); 
}

void LLAppViewer::requestQuit()
{
	llinfos << "requestQuit" << llendl;

	LLViewerRegion* region = gAgent.getRegion();
	
	if( (LLStartUp::getStartupState() < STATE_STARTED) || !region )
	{
		// Quit immediately
		forceQuit();
		return;
	}

	if (gHUDManager)
	{
		LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral*)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINT, TRUE);
		effectp->setPositionGlobal(gAgent.getPositionGlobal());
		effectp->setColor(LLColor4U(gAgent.getEffectColor()));
		gHUDManager->sendEffects();
	}

	// Attempt to close all floaters that might be
	// editing things.
	if (gFloaterView)
	{
		// application is quitting
		gFloaterView->closeAllChildren(true);
	}

	send_stats();

	gLogoutTimer.reset();
	mQuitRequested = true;
}

static void finish_quit(S32 option, void *userdata)
{
	if (option == 0)
	{
		LLAppViewer::instance()->requestQuit();
	}
}

void LLAppViewer::userQuit()
{
	gViewerWindow->alertXml("ConfirmQuit", finish_quit, NULL);
}

static void finish_early_exit(S32 option, void* userdata)
{
	LLAppViewer::instance()->forceQuit();
}

void LLAppViewer::earlyExit(const LLString& msg)
{
   	llwarns << "app_early_exit: " << msg << llendl;
	gDoDisconnect = TRUE;
// 	LLStringBase<char>::format_map_t args;
// 	args["[MESSAGE]"] = mesg;
// 	gViewerWindow->alertXml("AppEarlyExit", args, finish_early_exit);
	LLAlertDialog::showCritical(msg, finish_early_exit, NULL);
}

void LLAppViewer::forceExit(S32 arg)
{
    removeMarkerFile();
    
    // *FIX:Mani - This kind of exit hardly seems appropriate.
    exit(arg);
}

void LLAppViewer::abortQuit()
{
    llinfos << "abortQuit()" << llendl;
	mQuitRequested = false;
}

bool LLAppViewer::initCache()
{
	mPurgeCache = false;
	// Purge cache if user requested it
	if (gSavedSettings.getBOOL("PurgeCacheOnStartup") ||
		gSavedSettings.getBOOL("PurgeCacheOnNextStartup"))
	{
		gSavedSettings.setBOOL("PurgeCacheOnNextStartup", false);
		mPurgeCache = true;
	}
	// Purge cache if it belongs to an old version
	else
	{
		static const S32 cache_version = 5;
		if (gSavedSettings.getS32("LocalCacheVersion") != cache_version)
		{
			mPurgeCache = true;
			gSavedSettings.setS32("LocalCacheVersion", cache_version);
		}
	}
	
	// Setup and verify the cache location
	LLString cache_location = gSavedSettings.getString("CacheLocation");
	LLString new_cache_location = gSavedSettings.getString("NewCacheLocation");
	if (new_cache_location != cache_location)
	{
		gDirUtilp->setCacheDir(gSavedSettings.getString("CacheLocation"));
		purgeCache(); // purge old cache
		gSavedSettings.setString("CacheLocation", new_cache_location);
	}
	
	if (!gDirUtilp->setCacheDir(gSavedSettings.getString("CacheLocation")))
	{
		llwarns << "Unable to set cache location" << llendl;
		gSavedSettings.setString("CacheLocation", "");
	}
	
	if (mPurgeCache)
	{
		LLSplashScreen::update("Clearing cache...");
		purgeCache();
	}

	LLSplashScreen::update("Initializing Texture Cache...");
	
	// Init the texture cache
	// Allocate 80% of the cache size for textures
	BOOL read_only = mSecondInstance ? true : false;
	const S32 MB = 1024*1024;
	S64 cache_size = (S64)(gSavedSettings.getU32("CacheSize")) * MB;
	const S64 MAX_CACHE_SIZE = 1024*MB;
	cache_size = llmin(cache_size, MAX_CACHE_SIZE);
	S64 texture_cache_size = ((cache_size * 8)/10);
	S64 extra = LLAppViewer::getTextureCache()->initCache(LL_PATH_CACHE, texture_cache_size, read_only);
	texture_cache_size -= extra;

	LLSplashScreen::update("Initializing VFS...");
	
	// Init the VFS
	S64 vfs_size = cache_size - texture_cache_size;
	const S64 MAX_VFS_SIZE = 1024 * MB; // 1 GB
	vfs_size = llmin(vfs_size, MAX_VFS_SIZE);
	vfs_size = (vfs_size / MB) * MB; // make sure it is MB aligned
	U32 vfs_size_u32 = (U32)vfs_size;
	U32 old_vfs_size = gSavedSettings.getU32("VFSOldSize") * MB;
	bool resize_vfs = (vfs_size_u32 != old_vfs_size);
	if (resize_vfs)
	{
		gSavedSettings.setU32("VFSOldSize", vfs_size_u32/MB);
	}
	llinfos << "VFS CACHE SIZE: " << vfs_size/(1024*1024) << " MB" << llendl;
	
	// This has to happen BEFORE starting the vfs
	//time_t	ltime;
	srand(time(NULL));		// Flawfinder: ignore
	U32 old_salt = gSavedSettings.getU32("VFSSalt");
	U32 new_salt;
	char old_vfs_data_file[LL_MAX_PATH];		// Flawfinder: ignore
	char old_vfs_index_file[LL_MAX_PATH];	// Flawfinder: ignore		
	char new_vfs_data_file[LL_MAX_PATH];		// Flawfinder: ignore
	char new_vfs_index_file[LL_MAX_PATH];	// Flawfinder: ignore
	char static_vfs_index_file[LL_MAX_PATH];	// Flawfinder: ignore
	char static_vfs_data_file[LL_MAX_PATH];	// Flawfinder: ignore

	if (gMultipleViewersOK)
	{
		// don't mess with renaming the VFS in this case
		new_salt = old_salt;
	}
	else
	{
		do
		{
			new_salt = rand();
		} while( new_salt == old_salt );
	}

	snprintf(old_vfs_data_file,  LL_MAX_PATH, "%s%u",		// Flawfinder: ignore
		gDirUtilp->getExpandedFilename(LL_PATH_CACHE,VFS_DATA_FILE_BASE).c_str(),
		old_salt);

	// make sure this file exists
	llstat s;
	S32 stat_result = LLFile::stat(old_vfs_data_file, &s);
	if (stat_result)
	{
		// doesn't exist, look for a data file
		std::string mask;
		mask = gDirUtilp->getDirDelimiter();
		mask += VFS_DATA_FILE_BASE;
		mask += "*";

		std::string dir;
		dir = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"");

		std::string found_file;
		if (gDirUtilp->getNextFileInDir(dir, mask, found_file, false))
		{
			snprintf(old_vfs_data_file, LL_MAX_PATH, "%s%s%s", dir.c_str(), gDirUtilp->getDirDelimiter().c_str(), found_file.c_str());		// Flawfinder: ignore

			S32 start_pos;
			S32 length = strlen(found_file.c_str());		/* Flawfinder: ignore*/
			for (start_pos = length - 1; start_pos >= 0; start_pos--)
			{
				if (found_file[start_pos] == '.')
				{
					start_pos++;
					break;
				}
			}
			if (start_pos > 0)
			{
				sscanf(found_file.c_str() + start_pos, "%d", &old_salt);
			}
			llinfos << "Default vfs data file not present, found " << old_vfs_data_file << llendl;
			llinfos << "Old salt: " << old_salt << llendl;
		}
	}

	snprintf(old_vfs_index_file, LL_MAX_PATH, "%s%u",		// Flawfinder: ignore
			gDirUtilp->getExpandedFilename(LL_PATH_CACHE,VFS_INDEX_FILE_BASE).c_str(),
			old_salt);

	stat_result = LLFile::stat(old_vfs_index_file, &s);
	if (stat_result)
	{
		// We've got a bad/missing index file, nukem!
		llwarns << "Bad or missing vfx index file " << old_vfs_index_file << llendl;
		llwarns << "Removing old vfs data file " << old_vfs_data_file << llendl;
		LLFile::remove(old_vfs_data_file);
		LLFile::remove(old_vfs_index_file);
		
		// Just in case, nuke any other old cache files in the directory.
		std::string dir;
		dir = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"");

		std::string mask;
		mask = gDirUtilp->getDirDelimiter();
		mask += VFS_DATA_FILE_BASE;
		mask += "*";

		gDirUtilp->deleteFilesInDir(dir, mask);

		mask = gDirUtilp->getDirDelimiter();
		mask += VFS_INDEX_FILE_BASE;
		mask += "*";

		gDirUtilp->deleteFilesInDir(dir, mask);
	}

	snprintf(new_vfs_data_file, LL_MAX_PATH, "%s%u",		// Flawfinder: ignore
		gDirUtilp->getExpandedFilename(LL_PATH_CACHE,VFS_DATA_FILE_BASE).c_str(),
		new_salt);

	snprintf(new_vfs_index_file, LL_MAX_PATH, "%s%u", gDirUtilp->getExpandedFilename(LL_PATH_CACHE, VFS_INDEX_FILE_BASE).c_str(),		// Flawfinder: ignore
		new_salt);


	strncpy(static_vfs_data_file, gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"static_data.db2").c_str(), LL_MAX_PATH -1);		// Flawfinder: ignore
	static_vfs_data_file[LL_MAX_PATH -1] = '\0';
	strncpy(static_vfs_index_file, gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"static_index.db2").c_str(), LL_MAX_PATH -1);		// Flawfinder: ignore
	static_vfs_index_file[LL_MAX_PATH -1] = '\0';

	if (resize_vfs)
	{
		llinfos << "Removing old vfs and re-sizing" << llendl;
		
		LLFile::remove(old_vfs_data_file);
		LLFile::remove(old_vfs_index_file);
	}
	else if (old_salt != new_salt)
	{
		// move the vfs files to a new name before opening
		llinfos << "Renaming " << old_vfs_data_file << " to " << new_vfs_data_file << llendl;
		llinfos << "Renaming " << old_vfs_index_file << " to " << new_vfs_index_file << llendl;
		LLFile::rename(old_vfs_data_file, new_vfs_data_file);
		LLFile::rename(old_vfs_index_file, new_vfs_index_file);
	}

	// Startup the VFS...
	gSavedSettings.setU32("VFSSalt", new_salt);

	// Don't remove VFS after viewer crashes.  If user has corrupt data, they can reinstall. JC
	gVFS = new LLVFS(new_vfs_index_file, new_vfs_data_file, false, vfs_size_u32, false);
	if( VFSVALID_BAD_CORRUPT == gVFS->getValidState() )
	{
		// Try again with fresh files 
		// (The constructor deletes corrupt files when it finds them.)
		llwarns << "VFS corrupt, deleted.  Making new VFS." << llendl;
		delete gVFS;
		gVFS = new LLVFS(new_vfs_index_file, new_vfs_data_file, false, vfs_size_u32, false);
	}

	gStaticVFS = new LLVFS(static_vfs_index_file, static_vfs_data_file, true, 0, false);

	BOOL success = gVFS->isValid() && gStaticVFS->isValid();
	if( !success )
	{
		return false;
	}
	else
	{
		LLVFile::initClass();
		return true;
	}
}

void LLAppViewer::purgeCache()
{
	llinfos << "Purging Texture Cache..." << llendl;
	LLAppViewer::getTextureCache()->purgeCache(LL_PATH_CACHE);
	llinfos << "Purging Cache..." << llendl;
	std::string mask = gDirUtilp->getDirDelimiter() + "*.*";
	gDirUtilp->deleteFilesInDir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"").c_str(),mask);
}

const LLString& LLAppViewer::getSecondLifeTitle() const
{
	return gSecondLife;
}

const LLString& LLAppViewer::getWindowTitle() const 
{
	return gWindowTitle;
}

void LLAppViewer::resetURIs() const
{
    // Clear URIs when picking a new server
	gLoginURIs.clear();
	gHelperURI.clear();
}

const std::vector<std::string>& LLAppViewer::getLoginURIs() const
{
	if (gLoginURIs.empty())
	{
		// not specified on the command line, use value from table
		gLoginURIs.push_back(gGridInfo[gGridChoice].mLoginURI);
	}
	return gLoginURIs;
}

const std::string& LLAppViewer::getHelperURI() const
{
	if (gHelperURI.empty())
	{
		// not specified on the command line, use value from table
		gHelperURI = gGridInfo[gGridChoice].mHelperURI;
	}
	return gHelperURI;
}

void LLAppViewer::addLoginURI(const std::string& uri)
{
    gLoginURIs.push_back(uri);
}

void LLAppViewer::setHelperURI(const std::string& uri)
{
    gHelperURI = uri;
}

void LLAppViewer::setLoginPage(const std::string& login_page)
{
	gLoginPage = login_page;
}

const std::string& LLAppViewer::getLoginPage()
{
	return gLoginPage;
}


// Callback from a dialog indicating user was logged out.  
void finish_disconnect(S32 option, void* userdata)
{
	if (1 == option)
	{
        LLAppViewer::instance()->forceQuit();
	}
}

// Callback from an early disconnect dialog, force an exit
void finish_forced_disconnect(S32 /* option */, void* /* userdata */)
{
	LLAppViewer::instance()->forceQuit();
}


void LLAppViewer::forceDisconnect(const LLString& mesg)
{
	if (gDoDisconnect)
    {
		// Already popped up one of these dialogs, don't
		// do this again.
		return;
    }
	
	// Translate the message if possible
	LLString big_reason = LLAgent::sTeleportErrorMessages[mesg];
	if ( big_reason.size() == 0 )
	{
		big_reason = mesg;
	}

	LLStringBase<char>::format_map_t args;
	gDoDisconnect = TRUE;

	if (LLStartUp::getStartupState() < STATE_STARTED)
	{
		// Tell users what happened
		args["[ERROR_MESSAGE]"] = big_reason;
		gViewerWindow->alertXml("ErrorMessage", args, finish_forced_disconnect);
	}
	else
	{
		args["[MESSAGE]"] = big_reason;
		gViewerWindow->alertXml("YouHaveBeenLoggedOut", args, finish_disconnect );
	}
}

void LLAppViewer::badNetworkHandler()
{
	// Dump the packet
	gMessageSystem->dumpPacketToLog();

	// Flush all of our caches on exit in the case of disconnect due to
	// invalid packets.

	mPurgeOnExit = TRUE;

#if LL_WINDOWS
	// Generates the minidump.
	LLWinDebug::handleException(NULL);
#endif
	LLAppViewer::handleViewerCrash();

	std::ostringstream message;
	message <<
		"The viewer has detected mangled network data indicative\n"
		"of a bad upstream network connection or an incomplete\n"
		"local installation of " << LLAppViewer::instance()->getSecondLifeTitle() << ". \n"
		" \n"
		"Try uninstalling and reinstalling to see if this resolves \n"
		"the issue. \n"
		" \n"
		"If the problem continues, see the Tech Support FAQ at: \n"
		"www.secondlife.com/support";
	forceDisconnect(message.str());
}

// This routine may get called more than once during the shutdown process.
// This can happen because we need to get the screenshot before the window
// is destroyed.
void LLAppViewer::saveFinalSnapshot()
{
	if (!mSavedFinalSnapshot && !gNoRender)
	{
		gSavedSettings.setVector3d("FocusPosOnLogout", gAgent.calcFocusPositionTargetGlobal());
		gSavedSettings.setVector3d("CameraPosOnLogout", gAgent.calcCameraPositionTargetGlobal());
		gViewerWindow->setCursor(UI_CURSOR_WAIT);
		gAgent.changeCameraToThirdPerson( FALSE );	// don't animate, need immediate switch
		gSavedSettings.setBOOL("ShowParcelOwners", FALSE);
		idle();

		LLString snap_filename = gDirUtilp->getLindenUserDir();
		snap_filename += gDirUtilp->getDirDelimiter();
		snap_filename += SCREEN_LAST_FILENAME;
		gViewerWindow->saveSnapshot(snap_filename, gViewerWindow->getWindowWidth(), gViewerWindow->getWindowHeight(), FALSE, TRUE);
		mSavedFinalSnapshot = TRUE;
	}
}

void LLAppViewer::loadNameCache()
{
	if (!gCacheName) return;

	std::string name_cache;
	name_cache = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "name.cache");
	llifstream cache_file(name_cache.c_str());
	if(cache_file.is_open())
	{
		if(gCacheName->importFile(cache_file)) return;
	}

	// Try to load from the legacy format. This should go away after a
	// while. Phoenix 2008-01-30
	FILE* name_cache_fp = LLFile::fopen(name_cache.c_str(), "r");		// Flawfinder: ignore
	if (name_cache_fp)
	{
		gCacheName->importFile(name_cache_fp);
		fclose(name_cache_fp);
	}
}

void LLAppViewer::saveNameCache()
{
	if (!gCacheName) return;

	std::string name_cache;
	name_cache = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "name.cache");
	llofstream cache_file(name_cache.c_str());
	if(cache_file.is_open())
	{
		gCacheName->exportFile(cache_file);
	}
}

bool LLAppViewer::isInProductionGrid()
{
    return (GRID_INFO_AGNI == gGridChoice);
}


/*!	@brief		This class is an LLFrameTimer that can be created with
				an elapsed time that starts counting up from the given value
				rather than 0.0.
				
				Otherwise it behaves the same way as LLFrameTimer.
*/
class LLFrameStatsTimer : public LLFrameTimer
{
public:
	LLFrameStatsTimer(F64 elapsed_already = 0.0)
		: LLFrameTimer()
		{
			mStartTime -= elapsed_already;
		}
};

///////////////////////////////////////////////////////
// idle()
//
// Called every time the window is not doing anything.
// Receive packets, update statistics, and schedule a redisplay.
///////////////////////////////////////////////////////
void LLAppViewer::idle()
{
	// Update frame timers
	static LLTimer idle_timer;

	LLControlBase::updateAllListeners();

	LLFrameTimer::updateFrameTime();
	LLEventTimer::updateClass();
	LLCriticalDamp::updateInterpolants();
	LLMortician::updateClass();
	F32 dt_raw = idle_timer.getElapsedTimeAndResetF32();

	// Cap out-of-control frame times
	// Too low because in menus, swapping, debugger, etc.
	// Too high because idle called with no objects in view, etc.
	const F32 MIN_FRAME_RATE = 1.f;
	const F32 MAX_FRAME_RATE = 200.f;

	F32 frame_rate_clamped = 1.f / dt_raw;
	frame_rate_clamped = llclamp(frame_rate_clamped, MIN_FRAME_RATE, MAX_FRAME_RATE);
	gFrameDTClamped = 1.f / frame_rate_clamped;

	// Global frame timer
	// Smoothly weight toward current frame
	gFPSClamped = (frame_rate_clamped + (4.f * gFPSClamped)) / 5.f;

	if (gQuitAfterSeconds > 0.f)
	{
		if (gRenderStartTime.getElapsedTimeF32() > gQuitAfterSeconds)
		{
			LLAppViewer::instance()->forceQuit();
		}
	}

	// Must wait until both have avatar object and mute list, so poll
	// here.
	request_initial_instant_messages();

	///////////////////////////////////
	//
	// Special case idle if still starting up
	//

	if (LLStartUp::getStartupState() < STATE_STARTED)
	{
		// Skip rest if idle startup returns false (essentially, no world yet)
		if (!idle_startup())
		{
			return;
		}
	}

	
    F32 yaw = 0.f;				// radians

	if (!gDisconnected)
	{
		LLFastTimer t(LLFastTimer::FTM_NETWORK);
		// Update spaceserver timeinfo
	    gWorldp->setSpaceTimeUSec(gWorldp->getSpaceTimeUSec() + (U32)(dt_raw * SEC_TO_MICROSEC));
    
    
	    //////////////////////////////////////
	    //
	    // Update simulator agent state
	    //

		if (gRotateRight)
		{
			gAgent.moveYaw(-1.f);
		}

	    // Handle automatic walking towards points
	    gAgentPilot.updateTarget();
	    gAgent.autoPilot(&yaw);
    
	    static LLFrameTimer agent_update_timer;
	    static U32 				last_control_flags;
    
	    //	When appropriate, update agent location to the simulator.
	    F32 agent_update_time = agent_update_timer.getElapsedTimeF32();
	    BOOL flags_changed = gAgent.controlFlagsDirty() || (last_control_flags != gAgent.getControlFlags());
    
	    if (flags_changed || (agent_update_time > (1.0f / (F32) AGENT_UPDATES_PER_SECOND)))
	    {
		    // Send avatar and camera info
		    last_control_flags = gAgent.getControlFlags();
		    send_agent_update(TRUE);
		    agent_update_timer.reset();
	    }
	}

	//////////////////////////////////////
	//
	// Manage statistics
	//
	//

	{
		// Initialize the viewer_stats_timer with an already elapsed time
		// of SEND_STATS_PERIOD so that the initial stats report will
		// be sent immediately.
		static LLFrameStatsTimer viewer_stats_timer(SEND_STATS_PERIOD);
		reset_statistics();

		// Update session stats every large chunk of time
		// *FIX: (???) SAMANTHA
		if (viewer_stats_timer.getElapsedTimeF32() >= SEND_STATS_PERIOD && !gDisconnected)
		{
			llinfos << "Transmitting sessions stats" << llendl;
			send_stats();
			viewer_stats_timer.reset();
		}

		// Print the object debugging stats
		static LLFrameTimer object_debug_timer;
		if (object_debug_timer.getElapsedTimeF32() > 5.f)
		{
			object_debug_timer.reset();
			if (gObjectList.mNumDeadObjectUpdates)
			{
				llinfos << "Dead object updates: " << gObjectList.mNumDeadObjectUpdates << llendl;
				gObjectList.mNumDeadObjectUpdates = 0;
			}
			if (gObjectList.mNumUnknownKills)
			{
				llinfos << "Kills on unknown objects: " << gObjectList.mNumUnknownKills << llendl;
				gObjectList.mNumUnknownKills = 0;
			}
			if (gObjectList.mNumUnknownUpdates)
			{
				llinfos << "Unknown object updates: " << gObjectList.mNumUnknownUpdates << llendl;
				gObjectList.mNumUnknownUpdates = 0;
			}
		}
		gFrameStats.addFrameData();
	}
	
	if (!gDisconnected)
	{
		LLFastTimer t(LLFastTimer::FTM_NETWORK);
	
	    ////////////////////////////////////////////////
	    //
	    // Network processing
	    //
	    // NOTE: Starting at this point, we may still have pointers to "dead" objects
	    // floating throughout the various object lists.
	    //
    
	    gFrameStats.start(LLFrameStats::IDLE_NETWORK);
		idleNetwork();
	    stop_glerror();
	        
	    gFrameStats.start(LLFrameStats::AGENT_MISC);

		// Check for away from keyboard, kick idle agents.
		idle_afk_check();

		//  Update statistics for this frame
		update_statistics(gFrameCount);
	}

	////////////////////////////////////////
	//
	// Handle the regular UI idle callbacks as well as
	// hover callbacks
	//

	{
// 		LLFastTimer t(LLFastTimer::FTM_IDLE_CB);

		// Do event notifications if necessary.  Yes, we may want to move this elsewhere.
		gEventNotifier.update();
		
		gIdleCallbacks.callFunctions();
	}
	
	if (gDisconnected)
    {
		return;
    }

	gViewerWindow->handlePerFrameHover();

	///////////////////////////////////////
	// Agent and camera movement
	//
		LLCoordGL current_mouse = gViewerWindow->getCurrentMouse();

	{
		// After agent and camera moved, figure out if we need to
		// deselect objects.
		gSelectMgr->deselectAllIfTooFar();

	}

	{
		// Handle pending gesture processing
		gGestureManager.update();

		gAgent.updateAgentPosition(gFrameDTClamped, yaw, current_mouse.mX, current_mouse.mY);
	}

	{
		LLFastTimer t(LLFastTimer::FTM_OBJECTLIST_UPDATE); // Actually "object update"
		gFrameStats.start(LLFrameStats::OBJECT_UPDATE);
		
        if (!(logoutRequestSent() && hasSavedFinalSnapshot()))
		{
			gObjectList.update(gAgent, *gWorldp);
		}
	}
	
	//////////////////////////////////////
	//
	// Deletes objects...
	// Has to be done after doing idleUpdates (which can kill objects)
	//

	{
		LLFastTimer t(LLFastTimer::FTM_CLEANUP);
		gFrameStats.start(LLFrameStats::CLEAN_DEAD);
		gObjectList.cleanDeadObjects();
		LLDrawable::cleanupDeadDrawables();
	}
	
	//
	// After this point, in theory we should never see a dead object
	// in the various object/drawable lists.
	//

	//////////////////////////////////////
	//
	// Update/send HUD effects
	//
	// At this point, HUD effects may clean up some references to
	// dead objects.
	//

	{
		gFrameStats.start(LLFrameStats::UPDATE_EFFECTS);
		gSelectMgr->updateEffects();
		gHUDManager->cleanupEffects();
		gHUDManager->sendEffects();
	}

	stop_glerror();

	////////////////////////////////////////
	//
	// Unpack layer data that we've received
	//

	{
		LLFastTimer t(LLFastTimer::FTM_NETWORK);
		gVLManager.unpackData();
	}
	
	/////////////////////////
	//
	// Update surfaces, and surface textures as well.
	//

	gWorldp->updateVisibilities();
	{
		const F32 max_region_update_time = .001f; // 1ms
		LLFastTimer t(LLFastTimer::FTM_REGION_UPDATE);
		gWorldp->updateRegions(max_region_update_time);
	}
	
	/////////////////////////
	//
	// Update weather effects
	//

	if (!gNoRender)
	{
		gWorldp->updateClouds(gFrameDTClamped);
		gSky.propagateHeavenlyBodies(gFrameDTClamped);				// moves sun, moon, and planets

		// Update wind vector 
		LLVector3 wind_position_region;
		static LLVector3 average_wind;

		LLViewerRegion *regionp;
		regionp = gWorldp->resolveRegionGlobal(wind_position_region, gAgent.getPositionGlobal());	// puts agent's local coords into wind_position	
		if (regionp)
		{
			gWindVec = regionp->mWind.getVelocity(wind_position_region);

			// Compute average wind and use to drive motion of water
			
			average_wind = regionp->mWind.getAverage();
			F32 cloud_density = regionp->mCloudLayer.getDensityRegion(wind_position_region);
			
			gSky.setCloudDensityAtAgent(cloud_density);
			gSky.setWind(average_wind);
			//LLVOWater::setWind(average_wind);
		}
		else
		{
			gWindVec.setVec(0.0f, 0.0f, 0.0f);
		}
	}
	stop_glerror();
	
	//////////////////////////////////////
	//
	// Update images, using the image stats generated during object update/culling
	//
	// Can put objects onto the retextured list.
	//
	gFrameStats.start(LLFrameStats::IMAGE_UPDATE);

	{
		LLFastTimer t(LLFastTimer::FTM_IMAGE_UPDATE);
		
		LLViewerImage::updateClass(gCamera->getVelocityStat()->getMean(),
									gCamera->getAngularVelocityStat()->getMean());

		gBumpImageList.updateImages();  // must be called before gImageList version so that it's textures are thrown out first.

		const F32 max_image_decode_time = llmin(0.005f, 0.005f*10.f*gFrameIntervalSeconds); // 50 ms/second decode time (no more than 5ms/frame)
		gImageList.updateImages(max_image_decode_time);
		stop_glerror();
	}

	//////////////////////////////////////
	//
	// Sort and cull in the new renderer are moved to pipeline.cpp
	// Here, particles are updated and drawables are moved.
	//
	
	if (!gNoRender)
	{
		LLFastTimer t(LLFastTimer::FTM_WORLD_UPDATE);
		gFrameStats.start(LLFrameStats::UPDATE_MOVE);
		gPipeline.updateMove();

		gFrameStats.start(LLFrameStats::UPDATE_PARTICLES);
		gWorldp->updateParticles();
	}
	stop_glerror();

	if (!LLViewerJoystick::sOverrideCamera)
	{
		gAgent.updateCamera();
	}
	else
	{
		LLViewerJoystick::updateCamera();
	}

	// objects and camera should be in sync, do LOD calculations now
	{
		LLFastTimer t(LLFastTimer::FTM_LOD_UPDATE);
		gObjectList.updateApparentAngles(gAgent);
	}

	{
		gFrameStats.start(LLFrameStats::AUDIO);
		LLFastTimer t(LLFastTimer::FTM_AUDIO_UPDATE);
		
		if (gAudiop)
		{
		    audio_update_volume(false);
			audio_update_listener();
			audio_update_wind(false);

			// this line actually commits the changes we've made to source positions, etc.
			const F32 max_audio_decode_time = 0.002f; // 2 ms decode time
			gAudiop->idle(max_audio_decode_time);
		}
	}
	
	// Handle shutdown process, for example, 
	// wait for floaters to close, send quit message,
	// forcibly quit if it has taken too long
	if (mQuitRequested)
	{
		idleShutdown();
	}

	stop_glerror();
}

void LLAppViewer::idleShutdown()
{
	// Wait for all modal alerts to get resolved
	if (LLModalDialog::activeCount() > 0)
	{
		return;
	}

	// close IM interface
	if(gIMMgr)
	{
		gIMMgr->disconnectAllSessions();
	}
	
	// Wait for all floaters to get resolved
	if (gFloaterView
		&& !gFloaterView->allChildrenClosed())
	{
		return;
	}

	static bool saved_snapshot = false;
	if (!saved_snapshot)
	{
		saved_snapshot = true;
		saveFinalSnapshot();
		return;
	}

	const F32 SHUTDOWN_UPLOAD_SAVE_TIME = 5.f;

	S32 pending_uploads = gAssetStorage->getNumPendingUploads();
	if (pending_uploads > 0
		&& gLogoutTimer.getElapsedTimeF32() < SHUTDOWN_UPLOAD_SAVE_TIME
		&& !logoutRequestSent())
	{
		static S32 total_uploads = 0;
		// Sometimes total upload count can change during logout.
		total_uploads = llmax(total_uploads, pending_uploads);
		gViewerWindow->setShowProgress(TRUE);
		S32 finished_uploads = total_uploads - pending_uploads;
		F32 percent = 100.f * finished_uploads / total_uploads;
		gViewerWindow->setProgressPercent(percent);
		char buffer[MAX_STRING];		// Flawfinder: ignore
		snprintf(buffer, MAX_STRING, "Saving final data...");		// Flawfinder: ignore
		gViewerWindow->setProgressString(buffer);
		return;
	}

	// All floaters are closed.  Tell server we want to quit.
	if( !logoutRequestSent() )
	{
		sendLogoutRequest();

		// Wait for a LogoutReply message
		gViewerWindow->setShowProgress(TRUE);
		gViewerWindow->setProgressPercent(100.f);
		gViewerWindow->setProgressString("Logging out...");
		return;
	}

	// Make sure that we quit if we haven't received a reply from the server.
	if( logoutRequestSent() 
		&& gLogoutTimer.getElapsedTimeF32() > gLogoutMaxTime )
	{
		forceQuit();
		return;
	}
}

void LLAppViewer::sendLogoutRequest()
{
	if(!mLogoutRequestSent)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_LogoutRequest);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gAgent.sendReliableMessage();

		gLogoutTimer.reset();
		gLogoutMaxTime = LOGOUT_REQUEST_TIME;
		mLogoutRequestSent = TRUE;
		
		gVoiceClient->leaveChannel();
	}
}

//
// Handle messages, and all message related stuff
//

#define TIME_THROTTLE_MESSAGES

#ifdef TIME_THROTTLE_MESSAGES
#define CHECK_MESSAGES_DEFAULT_MAX_TIME .020f // 50 ms = 50 fps (just for messages!)
static F32 CheckMessagesMaxTime = CHECK_MESSAGES_DEFAULT_MAX_TIME;
#endif

void LLAppViewer::idleNetwork()
{
	gObjectList.mNumNewObjects = 0;
	S32 total_decoded = 0;

	if (!gSavedSettings.getBOOL("SpeedTest"))
	{
		LLFastTimer t(LLFastTimer::FTM_IDLE_NETWORK); // decode
		
		// deal with any queued name requests and replies.
		gCacheName->processPending();

		LLTimer check_message_timer;
		//  Read all available packets from network 
		stop_glerror();
		const S64 frame_count = gFrameCount;  // U32->S64
		F32 total_time = 0.0f;
   		while (gMessageSystem->checkAllMessages(frame_count, gServicePump)) 
		{
			if (gDoDisconnect)
			{
				// We're disconnecting, don't process any more messages from the server
				// We're usually disconnecting due to either network corruption or a
				// server going down, so this is OK.
				break;
			}
			stop_glerror();

			total_decoded++;
			gPacketsIn++;

			if (total_decoded > MESSAGE_MAX_PER_FRAME)
			{
				break;
			}

#ifdef TIME_THROTTLE_MESSAGES
			// Prevent slow packets from completely destroying the frame rate.
			// This usually happens due to clumps of avatars taking huge amount
			// of network processing time (which needs to be fixed, but this is
			// a good limit anyway).
			total_time = check_message_timer.getElapsedTimeF32();
			if (total_time >= CheckMessagesMaxTime)
				break;
#endif
		}
		// Handle per-frame message system processing.
		gMessageSystem->processAcks();

#ifdef TIME_THROTTLE_MESSAGES
		if (total_time >= CheckMessagesMaxTime)
		{
			// Increase CheckMessagesMaxTime so that we will eventually catch up
			CheckMessagesMaxTime *= 1.035f; // 3.5% ~= x2 in 20 frames, ~8x in 60 frames
		}
		else
		{
			// Reset CheckMessagesMaxTime to default value
			CheckMessagesMaxTime = CHECK_MESSAGES_DEFAULT_MAX_TIME;
		}
#endif
		


		// we want to clear the control after sending out all necessary agent updates
		gAgent.resetControlFlags();
		stop_glerror();

		
		// Decode enqueued messages...
		S32 remaining_possible_decodes = MESSAGE_MAX_PER_FRAME - total_decoded;

		if( remaining_possible_decodes <= 0 )
		{
			llinfos << "Maxed out number of messages per frame at " << MESSAGE_MAX_PER_FRAME << llendl;
		}

		if (gPrintMessagesThisFrame)
		{
			llinfos << "Decoded " << total_decoded << " msgs this frame!" << llendl;
			gPrintMessagesThisFrame = FALSE;
		}
	}

	gObjectList.mNumNewObjectsStat.addValue(gObjectList.mNumNewObjects);

	// Retransmit unacknowledged packets.
	gXferManager->retransmitUnackedPackets();
	gAssetStorage->checkForTimeouts();

	gViewerThrottle.updateDynamicThrottle();
}

void LLAppViewer::disconnectViewer()
{
	if (gDisconnected)
	{
		return;
	}
	//
	// Cleanup after quitting.
	//	
	// Save snapshot for next time, if we made it through initialization

	llinfos << "Disconnecting viewer!" << llendl;

	// Dump our frame statistics
	gFrameStats.dump();

	// Remember if we were flying
	gSavedSettings.setBOOL("FlyingAtExit", gAgent.getFlying() );

	// Un-minimize all windows so they don't get saved minimized
	if (!gNoRender)
	{
		if (gFloaterView)
		{
			gFloaterView->restoreAll();
		}
	}

	if (gSelectMgr)
	{
		gSelectMgr->deselectAll();
	}

	if (!gNoRender)
	{
		// save inventory if appropriate
		gInventory.cache(gAgent.getInventoryRootID(), gAgent.getID());
		if(gInventoryLibraryRoot.notNull() && gInventoryLibraryOwner.notNull())
		{
			gInventory.cache(gInventoryLibraryRoot, gInventoryLibraryOwner);
		}
	}

	saveNameCache();

	// close inventory interface, close all windows
	LLInventoryView::cleanup();

	// Also writes cached agent settings to gSavedSettings
	gAgent.cleanup();

	gObjectList.destroy();
	delete gWorldp;
	gWorldp = NULL;

	cleanup_xfer_manager();
	gDisconnected = TRUE;
}

void LLAppViewer::forceErrorLLError()
{
   	llerrs << "This is an llerror" << llendl;
}

void LLAppViewer::forceErrorBreakpoint()
{
#ifdef LL_WINDOWS
    DebugBreak();
#endif
    return;
}

void LLAppViewer::forceErrorBadMemoryAccess()
{
    S32* crash = NULL;
    *crash = 0xDEADBEEF;
    return;
}

void LLAppViewer::forceErrorInifiniteLoop()
{
    while(true)
    {
        ;
    }
    return;
}
 
void LLAppViewer::forceErrorSoftwareException()
{
    // *FIX: Any way to insure it won't be handled?
    throw; 
}
