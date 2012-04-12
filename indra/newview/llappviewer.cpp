/** 
 * @file llappviewer.cpp
 * @brief The LLAppViewer class definitions
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

#include "llviewerprecompiledheaders.h"

#include "llappviewer.h"

// Viewer includes
#include "llversioninfo.h"
#include "llversionviewer.h"
#include "llfeaturemanager.h"
#include "lluictrlfactory.h"
#include "lltexteditor.h"
#include "llerrorcontrol.h"
#include "lleventtimer.h"
#include "llviewertexturelist.h"
#include "llgroupmgr.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llagentlanguage.h"
#include "llagentwearables.h"
#include "llwindow.h"
#include "llviewerstats.h"
#include "llviewerstatsrecorder.h"
#include "llmarketplacefunctions.h"
#include "llmarketplacenotifications.h"
#include "llmd5.h"
#include "llmeshrepository.h"
#include "llpumpio.h"
#include "llmimetypes.h"
#include "llslurl.h"
#include "llstartup.h"
#include "llfocusmgr.h"
#include "llviewerjoystick.h"
#include "llallocator.h"
#include "llares.h" 
#include "llcurl.h"
#include "llcalc.h"
#include "lltexturestats.h"
#include "lltexturestats.h"
#include "llviewerwindow.h"
#include "llviewerdisplay.h"
#include "llviewermedia.h"
#include "llviewerparcelmedia.h"
#include "llviewermediafocus.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llworldmap.h"
#include "llmutelist.h"
#include "llviewerhelp.h"
#include "lluicolortable.h"
#include "llurldispatcher.h"
#include "llurlhistory.h"
//#include "llfirstuse.h"
#include "llrender.h"
#include "llteleporthistory.h"
#include "lltoast.h"
#include "lllocationhistory.h"
#include "llfasttimerview.h"
#include "llvector4a.h"
#include "llviewermenufile.h"
#include "llvoicechannel.h"
#include "llvoavatarself.h"
#include "llurlmatch.h"
#include "lltextutil.h"
#include "lllogininstance.h"
#include "llprogressview.h"
#include "llvocache.h"
#include "llweb.h"
#include "llsecondlifeurls.h"
#include "llupdaterservice.h"
#include "llcallfloater.h"

// Linden library includes
#include "llavatarnamecache.h"
#include "lldiriterator.h"
#include "llimagej2c.h"
#include "llmemory.h"
#include "llprimitive.h"
#include "llurlaction.h"
#include "llurlentry.h"
#include "llvfile.h"
#include "llvfsthread.h"
#include "llvolumemgr.h"
#include "llxfermanager.h"

#include "llnotificationmanager.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"

// Third party library includes
#include <boost/bind.hpp>
#include <boost/foreach.hpp>



#if LL_WINDOWS
#	include <share.h> // For _SH_DENYWR in initMarkerFile
#else
#   include <sys/file.h> // For initMarkerFile support
#endif

#include "llapr.h"
#include "apr_dso.h"
#include <boost/lexical_cast.hpp>

#include "llviewerkeyboard.h"
#include "lllfsthread.h"
#include "llworkerthread.h"
#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "llimageworker.h"
#include "llevents.h"

// The files below handle dependencies from cleanup.
#include "llkeyframemotion.h"
#include "llworldmap.h"
#include "llhudmanager.h"
#include "lltoolmgr.h"
#include "llassetstorage.h"
#include "llpolymesh.h"
#include "llproxy.h"
#include "llaudioengine.h"
#include "llstreamingaudio.h"
#include "llviewermenu.h"
#include "llselectmgr.h"
#include "lltrans.h"
#include "lltransutil.h"
#include "lltracker.h"
#include "llviewerparcelmgr.h"
#include "llworldmapview.h"
#include "llpostprocess.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"

#include "lldebugview.h"
#include "llconsole.h"
#include "llcontainerview.h"
#include "lltooltip.h"

#include "llsdserialize.h"

#include "llworld.h"
#include "llhudeffecttrail.h"
#include "llvectorperfoptions.h"
#include "llslurl.h"
#include "llwatchdog.h"

// Included so that constants/settings might be initialized
// in save_settings_to_globals()
#include "llbutton.h"
#include "llstatusbar.h"
#include "llsurface.h"
#include "llvosky.h"
#include "llvotree.h"
#include "llvoavatar.h"
#include "llfolderview.h"
#include "llagentpilot.h"
#include "llvovolume.h"
#include "llflexibleobject.h" 
#include "llvosurfacepatch.h"
#include "llviewerfloaterreg.h"
#include "llcommandlineparser.h"
#include "llfloatermemleak.h"
#include "llfloaterreg.h"
#include "llfloatersnapshot.h"
#include "llfloaterinventory.h"

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
#include "llparcel.h"
#include "llavatariconctrl.h"
#include "llgroupiconctrl.h"
#include "llviewerassetstats.h"

// Include for security api initialization
#include "llsecapi.h"
#include "llmachineid.h"
#include "llmainlooprepeater.h"

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
// define a self-registering event API object
#include "llappviewerlistener.h"

#if (LL_LINUX || LL_SOLARIS) && LL_GTK
#include "glib.h"
#endif // (LL_LINUX || LL_SOLARIS) && LL_GTK

#if LL_MSVC
// disable boost::lexical_cast warning
#pragma warning (disable:4702)
#endif

static LLAppViewerListener sAppViewerListener(LLAppViewer::instance);

////// Windows-specific includes to the bottom - nasty defines in these pollute the preprocessor
//
//----------------------------------------------------------------------------
// viewer.cpp - these are only used in viewer, should be easily moved.

#if LL_DARWIN
extern void init_apple_menu(const char* product);
#endif // LL_DARWIN

extern BOOL gRandomizeFramerate;
extern BOOL gPeriodicSlowFrame;
extern BOOL gDebugGL;

////////////////////////////////////////////////////////////
// All from the last globals push...

F32 gSimLastTime; // Used in LLAppViewer::init and send_stats()
F32 gSimFrames;

BOOL gShowObjectUpdates = FALSE;
BOOL gUseQuickTime = TRUE;

eLastExecEvent gLastExecEvent = LAST_EXEC_NORMAL;

LLSD gDebugInfo;

U32	gFrameCount = 0;
U32 gForegroundFrameCount = 0; // number of frames that app window was in foreground
LLPumpIO* gServicePump = NULL;

U64 gFrameTime = 0;
F32 gFrameTimeSeconds = 0.f;
F32 gFrameIntervalSeconds = 0.f;
F32 gFPSClamped = 10.f;						// Pretend we start at target rate.
F32 gFrameDTClamped = 0.f;					// Time between adjacent checks to network for packets
U64	gStartTime = 0; // gStartTime is "private", used only to calculate gFrameTimeSeconds
U32 gFrameStalls = 0;
const F64 FRAME_STALL_THRESHOLD = 1.0;

LLTimer gRenderStartTime;
LLFrameTimer gForegroundTime;
LLFrameTimer gLoggedInTime;
LLTimer gLogoutTimer;
static const F32 LOGOUT_REQUEST_TIME = 6.f;  // this will be cut short by the LogoutReply msg.
F32 gLogoutMaxTime = LOGOUT_REQUEST_TIME;

BOOL				gDisconnected = FALSE;

// used to restore texture state after a mode switch
LLFrameTimer	gRestoreGLTimer;
BOOL			gRestoreGL = FALSE;
BOOL				gUseWireframe = FALSE;

// VFS globals - see llappviewer.h
LLVFS* gStaticVFS = NULL;

LLMemoryInfo gSysMemory;
U64 gMemoryAllocated = 0; // updated in display_stats() in llviewerdisplay.cpp

std::string gLastVersionChannel;

LLVector3			gWindVec(3.0, 3.0, 0.0);
LLVector3			gRelativeWindVec(0.0, 0.0, 0.0);

U32		gPacketsIn = 0;

BOOL				gPrintMessagesThisFrame = FALSE;

BOOL gRandomizeFramerate = FALSE;
BOOL gPeriodicSlowFrame = FALSE;

BOOL gCrashOnStartup = FALSE;
BOOL gLLErrorActivated = FALSE;
BOOL gLogoutInProgress = FALSE;

////////////////////////////////////////////////////////////
// Internal globals... that should be removed.
static std::string gArgs;

const std::string MARKER_FILE_NAME("SecondLife.exec_marker");
const std::string ERROR_MARKER_FILE_NAME("SecondLife.error_marker");
const std::string LLERROR_MARKER_FILE_NAME("SecondLife.llerror_marker");
const std::string LOGOUT_MARKER_FILE_NAME("SecondLife.logout_marker");
static BOOL gDoDisconnect = FALSE;
static std::string gLaunchFileOnQuit;

// Used on Win32 for other apps to identify our window (eg, win_setup)
const char* const VIEWER_WINDOW_CLASSNAME = "Second Life";

//-- LLDeferredTaskList ------------------------------------------------------

/**
 * A list of deferred tasks.
 *
 * We sometimes need to defer execution of some code until the viewer gets idle,
 * e.g. removing an inventory item from within notifyObservers() may not work out.
 *
 * Tasks added to this list will be executed in the next LLAppViewer::idle() iteration.
 * All tasks are executed only once.
 */
class LLDeferredTaskList: public LLSingleton<LLDeferredTaskList>
{
	LOG_CLASS(LLDeferredTaskList);

	friend class LLAppViewer;
	typedef boost::signals2::signal<void()> signal_t;

	void addTask(const signal_t::slot_type& cb)
	{
		mSignal.connect(cb);
	}

	void run()
	{
		if (!mSignal.empty())
		{
			mSignal();
			mSignal.disconnect_all_slots();
		}
	}

	signal_t mSignal;
};

//----------------------------------------------------------------------------

// List of entries from strings.xml to always replace
static std::set<std::string> default_trans_args;
void init_default_trans_args()
{
	default_trans_args.insert("SECOND_LIFE"); // World
	default_trans_args.insert("APP_NAME");
	default_trans_args.insert("CAPITALIZED_APP_NAME");
	default_trans_args.insert("SECOND_LIFE_GRID");
	default_trans_args.insert("SUPPORT_SITE");
}

//----------------------------------------------------------------------------
// File scope definitons
const char *VFS_DATA_FILE_BASE = "data.db2.x.";
const char *VFS_INDEX_FILE_BASE = "index.db2.x.";


struct SettingsFile : public LLInitParam::Block<SettingsFile>
{
	Mandatory<std::string>	name;
	Optional<std::string>	file_name;
	Optional<bool>			required,
							persistent;
	Optional<std::string>	file_name_setting;

	SettingsFile()
	:	name("name"),
		file_name("file_name"),
		required("required", false),
		persistent("persistent", true),
		file_name_setting("file_name_setting")
	{}
};

struct SettingsGroup : public LLInitParam::Block<SettingsGroup>
{
	Mandatory<std::string>	name;
	Mandatory<S32>			path_index;
	Multiple<SettingsFile>	files;

	SettingsGroup()
	:	name("name"),
		path_index("path_index"),
		files("file")
	{}
};

struct SettingsFiles : public LLInitParam::Block<SettingsFiles>
{
	Multiple<SettingsGroup>	groups;

	SettingsFiles()
	: groups("group")
	{}
};

static std::string gWindowTitle;

LLAppViewer::LLUpdaterInfo *LLAppViewer::sUpdaterInfo = NULL ;

//----------------------------------------------------------------------------
// Metrics logging control constants
//----------------------------------------------------------------------------
static const F32 METRICS_INTERVAL_DEFAULT = 600.0;
static const F32 METRICS_INTERVAL_QA = 30.0;
static F32 app_metrics_interval = METRICS_INTERVAL_DEFAULT;
static bool app_metrics_qa_mode = false;

void idle_afk_check()
{
	// check idle timers
	F32 current_idle = gAwayTriggerTimer.getElapsedTimeF32();
	F32 afk_timeout  = gSavedSettings.getS32("AFKTimeout");
	if (afk_timeout && (current_idle > afk_timeout) && ! gAgent.getAFK())
	{
		LL_INFOS("IdleAway") << "Idle more than " << afk_timeout << " seconds: automatically changing to Away status" << LL_ENDL;
		gAgent.setAFK();
	}
}

// A callback set in LLAppViewer::init()
static void ui_audio_callback(const LLUUID& uuid)
{
	if (gAudiop)
	{
		gAudiop->triggerSound(uuid, gAgent.getID(), 1.0f, LLAudioEngine::AUDIO_TYPE_UI);
	}
}

bool	create_text_segment_icon_from_url_match(LLUrlMatch* match,LLTextBase* base)
{
	if(!match || !base || base->getPlainText())
		return false;

	LLUUID match_id = match->getID();

	LLIconCtrl* icon;

	if(gAgent.isInGroup(match_id, TRUE))
	{
		LLGroupIconCtrl::Params icon_params;
		icon_params.group_id = match_id;
		icon_params.rect = LLRect(0, 16, 16, 0);
		icon_params.visible = true;
		icon = LLUICtrlFactory::instance().create<LLGroupIconCtrl>(icon_params);
	}
	else
	{
		LLAvatarIconCtrl::Params icon_params;
		icon_params.avatar_id = match_id;
		icon_params.rect = LLRect(0, 16, 16, 0);
		icon_params.visible = true;
		icon = LLUICtrlFactory::instance().create<LLAvatarIconCtrl>(icon_params);
	}

	LLInlineViewSegment::Params params;
	params.force_newline = false;
	params.view = icon;
	params.left_pad = 4;
	params.right_pad = 4;
	params.top_pad = -2;
	params.bottom_pad = 2;

	base->appendWidget(params," ",false);
	
	return true;
}

void request_initial_instant_messages()
{
	static BOOL requested = FALSE;
	if (!requested
		&& gMessageSystem
		&& LLMuteList::getInstance()->isLoaded()
		&& isAgentAvatarValid())
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
static void settings_to_globals()
{
	LLBUTTON_H_PAD		= gSavedSettings.getS32("ButtonHPad");
	BTN_HEIGHT_SMALL	= gSavedSettings.getS32("ButtonHeightSmall");
	BTN_HEIGHT			= gSavedSettings.getS32("ButtonHeight");

	MENU_BAR_HEIGHT		= gSavedSettings.getS32("MenuBarHeight");
	MENU_BAR_WIDTH		= gSavedSettings.getS32("MenuBarWidth");

	LLSurface::setTextureSize(gSavedSettings.getU32("RegionTextureSize"));
	
	LLRender::sGLCoreProfile = gSavedSettings.getBOOL("RenderGLCoreProfile");

	LLImageGL::sGlobalUseAnisotropic	= gSavedSettings.getBOOL("RenderAnisotropic");
	LLVOVolume::sLODFactor				= gSavedSettings.getF32("RenderVolumeLODFactor");
	LLVOVolume::sDistanceFactor			= 1.f-LLVOVolume::sLODFactor * 0.1f;
	LLVolumeImplFlexible::sUpdateFactor = gSavedSettings.getF32("RenderFlexTimeFactor");
	LLVOTree::sTreeFactor				= gSavedSettings.getF32("RenderTreeLODFactor");
	LLVOAvatar::sLODFactor				= gSavedSettings.getF32("RenderAvatarLODFactor");
	LLVOAvatar::sPhysicsLODFactor		= gSavedSettings.getF32("RenderAvatarPhysicsLODFactor");
	LLVOAvatar::sMaxVisible				= (U32)gSavedSettings.getS32("RenderAvatarMaxVisible");
	LLVOAvatar::sVisibleInFirstPerson	= gSavedSettings.getBOOL("FirstPersonAvatarVisible");
	// clamp auto-open time to some minimum usable value
	LLFolderView::sAutoOpenTime			= llmax(0.25f, gSavedSettings.getF32("FolderAutoOpenDelay"));
	LLSelectMgr::sRectSelectInclusive	= gSavedSettings.getBOOL("RectangleSelectInclusive");
	LLSelectMgr::sRenderHiddenSelections = gSavedSettings.getBOOL("RenderHiddenSelections");
	LLSelectMgr::sRenderLightRadius = gSavedSettings.getBOOL("RenderLightRadius");

	gAgentPilot.setNumRuns(gSavedSettings.getS32("StatsNumRuns"));
	gAgentPilot.setQuitAfterRuns(gSavedSettings.getBOOL("StatsQuitAfterRuns"));
	gAgent.setHideGroupTitle(gSavedSettings.getBOOL("RenderHideGroupTitle"));

	gDebugWindowProc = gSavedSettings.getBOOL("DebugWindowProc");
	gShowObjectUpdates = gSavedSettings.getBOOL("ShowObjectUpdates");
	LLWorldMapView::sMapScale = gSavedSettings.getF32("MapScale");
}

static void settings_modify()
{
	LLRenderTarget::sUseFBO				= gSavedSettings.getBOOL("RenderDeferred");
	LLPipeline::sRenderDeferred			= gSavedSettings.getBOOL("RenderDeferred");
	LLVOAvatar::sUseImpostors			= gSavedSettings.getBOOL("RenderUseImpostors");
	LLVOSurfacePatch::sLODFactor		= gSavedSettings.getF32("RenderTerrainLODFactor");
	LLVOSurfacePatch::sLODFactor *= LLVOSurfacePatch::sLODFactor; //square lod factor to get exponential range of [1,4]
	gDebugGL = gSavedSettings.getBOOL("RenderDebugGL") || gDebugSession;
	gDebugPipeline = gSavedSettings.getBOOL("RenderDebugPipeline");
}

class LLFastTimerLogThread : public LLThread
{
public:
	std::string mFile;

	LLFastTimerLogThread(std::string& test_name) : LLThread("fast timer log")
 	{
		std::string file_name = test_name + std::string(".slp");
		mFile = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, file_name);
	}

	void run()
	{
		std::ofstream os(mFile.c_str());
		
		while (!LLAppViewer::instance()->isQuitting())
		{
			LLFastTimer::writeLog(os);
			os.flush();
			ms_sleep(32);
		}

		os.close();
	}
};

//virtual
bool LLAppViewer::initSLURLHandler()
{
	// does nothing unless subclassed
	return false;
}

//virtual
bool LLAppViewer::sendURLToOtherInstance(const std::string& url)
{
	// does nothing unless subclassed
	return false;
}

//----------------------------------------------------------------------------
// LLAppViewer definition

// Static members.
// The single viewer app.
LLAppViewer* LLAppViewer::sInstance = NULL;
LLTextureCache* LLAppViewer::sTextureCache = NULL; 
LLImageDecodeThread* LLAppViewer::sImageDecodeThread = NULL; 
LLTextureFetch* LLAppViewer::sTextureFetch = NULL; 

LLAppViewer::LLAppViewer() : 
	mMarkerFile(),
	mLogoutMarkerFile(NULL),
	mReportedCrash(false),
	mNumSessions(0),
	mPurgeCache(false),
	mPurgeOnExit(false),
	mSecondInstance(false),
	mSavedFinalSnapshot(false),
	mForceGraphicsDetail(false),
	mQuitRequested(false),
	mLogoutRequestSent(false),
	mYieldTime(-1),
	mMainloopTimeout(NULL),
	mAgentRegionLastAlive(false),
	mRandomizeFramerate(LLCachedControl<bool>(gSavedSettings,"Randomize Framerate", FALSE)),
	mPeriodicSlowFrame(LLCachedControl<bool>(gSavedSettings,"Periodic Slow Frame", FALSE)),
	mFastTimerLogThread(NULL),
	mUpdater(new LLUpdaterService()),
	mSettingsLocationList(NULL)
{
	if(NULL != sInstance)
	{
		llerrs << "Oh no! An instance of LLAppViewer already exists! LLAppViewer is sort of like a singleton." << llendl;
	}

	setupErrorHandling();
	sInstance = this;
	gLoggedInTime.stop();
	
	LLLoginInstance::instance().setUpdaterService(mUpdater.get());
}

LLAppViewer::~LLAppViewer()
{
	delete mSettingsLocationList;

	LLLoginInstance::instance().setUpdaterService(0);
	
	destroyMainloopTimeout();

	// If we got to this destructor somehow, the app didn't hang.
	removeMarkerFile();
}

bool LLAppViewer::init()
{	
	//
	// Start of the application
	//
	// IMPORTANT! Do NOT put anything that will write
	// into the log files during normal startup until AFTER
	// we run the "program crashed last time" error handler below.
	//
	LLFastTimer::reset();

	// initialize SSE options
	LLVector4a::initClass();

	// Need to do this initialization before we do anything else, since anything
	// that touches files should really go through the lldir API
	gDirUtilp->initAppDirs("SecondLife");
	// set skin search path to default, will be overridden later
	// this allows simple skinned file lookups to work
	gDirUtilp->setSkinFolder("default");

	initLogging();
	
	//
	// OK to write stuff to logs now, we've now crash reported if necessary
	//
	
	init_default_trans_args();
	
	if (!initConfiguration())
		return false;

	LL_INFOS("InitInfo") << "Configuration initialized." << LL_ENDL ;

	//set the max heap size.
	initMaxHeapSize() ;

	LLPrivateMemoryPoolManager::initClass((BOOL)gSavedSettings.getBOOL("MemoryPrivatePoolEnabled"), (U32)gSavedSettings.getU32("MemoryPrivatePoolSize")) ;

	// write Google Breakpad minidump files to our log directory
	std::string logdir = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "");
	logdir += gDirUtilp->getDirDelimiter();
	setMiniDumpDir(logdir);

	// Although initLogging() is the right place to mess with
	// setFatalFunction(), we can't query gSavedSettings until after
	// initConfiguration().
	S32 rc(gSavedSettings.getS32("QAModeTermCode"));
	if (rc >= 0)
	{
		// QAModeTermCode set, terminate with that rc on LL_ERRS. Use _exit()
		// rather than exit() because normal cleanup depends too much on
		// successful startup!
		LLError::setFatalFunction(boost::bind(_exit, rc));
	}

    mAlloc.setProfilingEnabled(gSavedSettings.getBOOL("MemProfiling"));

#if LL_RECORD_VIEWER_STATS
	LLViewerStatsRecorder::initClass();
#endif

    // *NOTE:Mani - LLCurl::initClass is not thread safe. 
    // Called before threads are created.
    LLCurl::initClass(gSavedSettings.getF32("CurlRequestTimeOut"), 
						gSavedSettings.getS32("CurlMaximumNumberOfHandles"), 
						gSavedSettings.getBOOL("CurlUseMultipleThreads"));
	LL_INFOS("InitInfo") << "LLCurl initialized." << LL_ENDL ;

    LLMachineID::init();
	
	{
		// Viewer metrics initialization
		//static LLCachedControl<bool> metrics_submode(gSavedSettings,
		//											 "QAModeMetrics",
		//											 false,
		//											 "Enables QA features (logging, faster cycling) for metrics collector");

		if (gSavedSettings.getBOOL("QAModeMetrics"))
		{
			app_metrics_qa_mode = true;
			app_metrics_interval = METRICS_INTERVAL_QA;
		}
		LLViewerAssetStatsFF::init();
	}

	initThreads();
	LL_INFOS("InitInfo") << "Threads initialized." << LL_ENDL ;

	// Initialize settings early so that the defaults for ignorable dialogs are
	// picked up and then correctly re-saved after launching the updater (STORM-1268).
	LLUI::settings_map_t settings_map;
	settings_map["config"] = &gSavedSettings;
	settings_map["ignores"] = &gWarningSettings;
	settings_map["floater"] = &gSavedSettings; // *TODO: New settings file
	settings_map["account"] = &gSavedPerAccountSettings;

	LLUI::initClass(settings_map,
		LLUIImageList::getInstance(),
		ui_audio_callback,
		&LLUI::sGLScaleFactor);
	LL_INFOS("InitInfo") << "UI initialized." << LL_ENDL ;

	// Setup paths and LLTrans after LLUI::initClass has been called.
	LLUI::setupPaths();
	LLTransUtil::parseStrings("strings.xml", default_trans_args);
	LLTransUtil::parseLanguageStrings("language_settings.xml");

	// Setup notifications after LLUI::setupPaths() has been called.
	LLNotifications::instance();
	LL_INFOS("InitInfo") << "Notifications initialized." << LL_ENDL ;

    writeSystemInfo();

	// Initialize updater service (now that we have an io pump)
	initUpdater();
	if(isQuitting())
	{
		// Early out here because updater set the quitting flag.
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	// *FIX: The following code isn't grouped into functions yet.

	// Statistics / debug timer initialization
	init_statistics();
	
	//
	// Various introspection concerning the libs we're using - particularly
	// the libs involved in getting to a full login screen.
	//
	LL_INFOS("InitInfo") << "J2C Engine is: " << LLImageJ2C::getEngineInfo() << LL_ENDL;
	LL_INFOS("InitInfo") << "libcurl version is: " << LLCurl::getVersionString() << LL_ENDL;

	/////////////////////////////////////////////////
	// OS-specific login dialogs
	/////////////////////////////////////////////////

	//test_cached_control();

	// track number of times that app has run
	mNumSessions = gSavedSettings.getS32("NumSessions");
	mNumSessions++;
	gSavedSettings.setS32("NumSessions", mNumSessions);

	if (gSavedSettings.getBOOL("VerboseLogs"))
	{
		LLError::setPrintLocation(true);
	}

	// LLKeyboard relies on LLUI to know what some accelerator keys are called.
	LLKeyboard::setStringTranslatorFunc( LLTrans::getKeyboardString );

	LLWeb::initClass();			  // do this after LLUI
	
	// Provide the text fields with callbacks for opening Urls
	LLUrlAction::setOpenURLCallback(boost::bind(&LLWeb::loadURL, _1, LLStringUtil::null, LLStringUtil::null));
	LLUrlAction::setOpenURLInternalCallback(boost::bind(&LLWeb::loadURLInternal, _1, LLStringUtil::null, LLStringUtil::null));
	LLUrlAction::setOpenURLExternalCallback(boost::bind(&LLWeb::loadURLExternal, _1, true, LLStringUtil::null));
	LLUrlAction::setExecuteSLURLCallback(&LLURLDispatcher::dispatchFromTextEditor);

	// Let code in llui access the viewer help floater
	LLUI::sHelpImpl = LLViewerHelp::getInstance();

	LL_INFOS("InitInfo") << "UI initialization is done." << LL_ENDL ;

	// Load translations for tooltips
	LLFloater::initClass();

	/////////////////////////////////////////////////
	
	LLToolMgr::getInstance(); // Initialize tool manager if not already instantiated
	
	LLViewerFloaterReg::registerFloaters();
	
	/////////////////////////////////////////////////
	//
	// Load settings files
	//
	//
	LLGroupMgr::parseRoleActions("role_actions.xml");

	LLAgent::parseTeleportMessages("teleport_strings.xml");

	// load MIME type -> media impl mappings
	std::string mime_types_name;
#if LL_DARWIN
	mime_types_name = "mime_types_mac.xml";
#elif LL_LINUX
	mime_types_name = "mime_types_linux.xml";
#else
	mime_types_name = "mime_types.xml";
#endif
	LLMIMETypes::parseMIMETypes( mime_types_name ); 

	// Copy settings to globals. *TODO: Remove or move to appropriage class initializers
	settings_to_globals();
	// Setup settings listeners
	settings_setup_listeners();
	// Modify settings based on system configuration and compile options
	settings_modify();

	// Find partition serial number (Windows) or hardware serial (Mac)
	mSerialNumber = generateSerialNumber();

	// do any necessary set-up for accepting incoming SLURLs from apps
	initSLURLHandler();

	if(false == initHardwareTest())
	{
		// Early out from user choice.
		return false;
	}
	LL_INFOS("InitInfo") << "Hardware test initialization done." << LL_ENDL ;

	// Prepare for out-of-memory situations, during which we will crash on
	// purpose and save a dump.
#if LL_WINDOWS && LL_RELEASE_FOR_DOWNLOAD && LL_USE_SMARTHEAP
	MemSetErrorHandler(first_mem_error_handler);
#endif // LL_WINDOWS && LL_RELEASE_FOR_DOWNLOAD && LL_USE_SMARTHEAP

	// *Note: this is where gViewerStats used to be created.

	//
	// Initialize the VFS, and gracefully handle initialization errors
	//

	if (!initCache())
	{
		std::ostringstream msg;
		msg << LLTrans::getString("MBUnableToAccessFile");
		OSMessageBox(msg.str(),LLStringUtil::null,OSMB_OK);
		return 1;
	}
	LL_INFOS("InitInfo") << "Cache initialization is done." << LL_ENDL ;

	// Initialize the repeater service.
	LLMainLoopRepeater::instance().start();

	//
	// Initialize the window
	//
	gGLActive = TRUE;
	initWindow();
	LL_INFOS("InitInfo") << "Window is initialized." << LL_ENDL ;

	// initWindow also initializes the Feature List, so now we can initialize this global.
	LLCubeMap::sUseCubeMaps = LLFeatureManager::getInstance()->isFeatureAvailable("RenderCubeMap");

	// call all self-registered classes
	LLInitClassList::instance().fireCallbacks();

	LLFolderViewItem::initClass(); // SJB: Needs to happen after initWindow(), not sure why but related to fonts
		
	gGLManager.getGLInfo(gDebugInfo);
	gGLManager.printGLInfoString();

	// Load Default bindings
	std::string key_bindings_file = gDirUtilp->findFile("keys.xml",
														gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, ""),
														gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, ""));


	if (!gViewerKeyboard.loadBindingsXML(key_bindings_file))
	{
		std::string key_bindings_file = gDirUtilp->findFile("keys.ini",
															gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, ""),
															gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, ""));
		if (!gViewerKeyboard.loadBindings(key_bindings_file))
		{
			LL_ERRS("InitInfo") << "Unable to open keys.ini" << LL_ENDL;
		}
	}

	// If we don't have the right GL requirements, exit.
	if (!gGLManager.mHasRequirements)
	{	
		// can't use an alert here since we're exiting and
		// all hell breaks lose.
		OSMessageBox(
			LLNotifications::instance().getGlobalString("UnsupportedGLRequirements"),
			LLStringUtil::null,
			OSMB_OK);
		return 0;
	}

	// Without SSE2 support we will crash almost immediately, warn here.
	if (!gSysCPU.hasSSE2())
	{	
		// can't use an alert here since we're exiting and
		// all hell breaks lose.
		OSMessageBox(
			LLNotifications::instance().getGlobalString("UnsupportedCPUSSE2"),
			LLStringUtil::null,
			OSMB_OK);
		return 0;
	}

	// alert the user if they are using unsupported hardware
	if(!gSavedSettings.getBOOL("AlertedUnsupportedHardware"))
	{
		bool unsupported = false;
		LLSD args;
		std::string minSpecs;
		
		// get cpu data from xml
		std::stringstream minCPUString(LLNotifications::instance().getGlobalString("UnsupportedCPUAmount"));
		S32 minCPU = 0;
		minCPUString >> minCPU;

		// get RAM data from XML
		std::stringstream minRAMString(LLNotifications::instance().getGlobalString("UnsupportedRAMAmount"));
		U64 minRAM = 0;
		minRAMString >> minRAM;
		minRAM = minRAM * 1024 * 1024;

		if(!LLFeatureManager::getInstance()->isGPUSupported() && LLFeatureManager::getInstance()->getGPUClass() != GPU_CLASS_UNKNOWN)
		{
			minSpecs += LLNotifications::instance().getGlobalString("UnsupportedGPU");
			minSpecs += "\n";
			unsupported = true;
		}
		if(gSysCPU.getMHz() < minCPU)
		{
			minSpecs += LLNotifications::instance().getGlobalString("UnsupportedCPU");
			minSpecs += "\n";
			unsupported = true;
		}
		if(gSysMemory.getPhysicalMemoryClamped() < minRAM)
		{
			minSpecs += LLNotifications::instance().getGlobalString("UnsupportedRAM");
			minSpecs += "\n";
			unsupported = true;
		}

		if (LLFeatureManager::getInstance()->getGPUClass() == GPU_CLASS_UNKNOWN)
		{
			LLNotificationsUtil::add("UnknownGPU");
		} 
			
		if(unsupported)
		{
			if(!gSavedSettings.controlExists("WarnUnsupportedHardware") 
				|| gSavedSettings.getBOOL("WarnUnsupportedHardware"))
			{
				args["MINSPECS"] = minSpecs;
				LLNotificationsUtil::add("UnsupportedHardware", args );
			}

		}
	}


	// save the graphics card
	gDebugInfo["GraphicsCard"] = LLFeatureManager::getInstance()->getGPUString();

	// Save the current version to the prefs file
	gSavedSettings.setString("LastRunVersion",
							 LLVersionInfo::getChannelAndVersion());

	gSimLastTime = gRenderStartTime.getElapsedTimeF32();
	gSimFrames = (F32)gFrameCount;

	LLViewerJoystick::getInstance()->init(false);

	try {
		initializeSecHandler();
	}
	catch (LLProtectedDataException ex)
	{
	  LLNotificationsUtil::add("CorruptedProtectedDataStore");
	}
	LLHTTPClient::setCertVerifyCallback(secapiSSLCertVerifyCallback);


	gGLActive = FALSE;
	if (gSavedSettings.getBOOL("QAMode") && gSavedSettings.getS32("QAModeEventHostPort") > 0)
	{
		loadEventHostModule(gSavedSettings.getS32("QAModeEventHostPort"));
	}
	
	LLViewerMedia::initClass();
	LL_INFOS("InitInfo") << "Viewer media initialized." << LL_ENDL ;

	LLTextUtil::TextHelpers::iconCallbackCreationFunction = create_text_segment_icon_from_url_match;

	//EXT-7013 - On windows for some locale (Japanese) standard 
	//datetime formatting functions didn't support some parameters such as "weekday".
	//Names for days and months localized in xml are also useful for Polish locale(STORM-107).
	std::string language = gSavedSettings.getString("Language");
	if(language == "ja" || language == "pl")
	{
		LLStringOps::setupWeekDaysNames(LLTrans::getString("dateTimeWeekdaysNames"));
		LLStringOps::setupWeekDaysShortNames(LLTrans::getString("dateTimeWeekdaysShortNames"));
		LLStringOps::setupMonthNames(LLTrans::getString("dateTimeMonthNames"));
		LLStringOps::setupMonthShortNames(LLTrans::getString("dateTimeMonthShortNames"));
		LLStringOps::setupDayFormat(LLTrans::getString("dateTimeDayFormat"));

		LLStringOps::sAM = LLTrans::getString("dateTimeAM");
		LLStringOps::sPM = LLTrans::getString("dateTimePM");
	}

	LLAgentLanguage::init();

	return true;
}

void LLAppViewer::initMaxHeapSize()
{
	//set the max heap size.
	//here is some info regarding to the max heap size:
	//------------------------------------------------------------------------------------------
	// OS       | setting | SL address bits | max manageable memory space | max heap size
	// Win 32   | default | 32-bit          | 2GB                         | < 1.7GB
	// Win 32   | /3G     | 32-bit          | 3GB                         | < 1.7GB or 2.7GB
	//Linux 32  | default | 32-bit          | 3GB                         | < 2.7GB
	//Linux 32  |HUGEMEM  | 32-bit          | 4GB                         | < 3.7GB
	//64-bit OS |default  | 32-bit          | 4GB                         | < 3.7GB
	//64-bit OS |default  | 64-bit          | N/A (> 4GB)                 | N/A (> 4GB)
	//------------------------------------------------------------------------------------------
	//currently SL is built under 32-bit setting, we set its max heap size no more than 1.6 GB.

	//F32 max_heap_size_gb = llmin(1.6f, (F32)gSavedSettings.getF32("MaxHeapSize")) ;
	F32 max_heap_size_gb = gSavedSettings.getF32("MaxHeapSize") ;
	BOOL enable_mem_failure_prevention = (BOOL)gSavedSettings.getBOOL("MemoryFailurePreventionEnabled") ;

	LLMemory::initMaxHeapSizeGB(max_heap_size_gb, enable_mem_failure_prevention) ;
}

void LLAppViewer::checkMemory()
{
	const static F32 MEMORY_CHECK_INTERVAL = 1.0f ; //second
	//const static F32 MAX_QUIT_WAIT_TIME = 30.0f ; //seconds
	//static F32 force_quit_timer = MAX_QUIT_WAIT_TIME + MEMORY_CHECK_INTERVAL ;

	if(!gGLManager.mDebugGPU)
	{
		return ;
	}

	if(MEMORY_CHECK_INTERVAL > mMemCheckTimer.getElapsedTimeF32())
	{
		return ;
	}
	mMemCheckTimer.reset() ;

		//update the availability of memory
		LLMemory::updateMemoryInfo() ;

	bool is_low = LLMemory::isMemoryPoolLow() ;

	LLPipeline::throttleNewMemoryAllocation(is_low) ;		
	
	if(is_low)
	{
		LLMemory::logMemoryInfo() ;
	}
}

static LLFastTimer::DeclareTimer FTM_MESSAGES("System Messages");
static LLFastTimer::DeclareTimer FTM_SLEEP("Sleep");
static LLFastTimer::DeclareTimer FTM_TEXTURE_CACHE("Texture Cache");
static LLFastTimer::DeclareTimer FTM_DECODE("Image Decode");
static LLFastTimer::DeclareTimer FTM_VFS("VFS Thread");
static LLFastTimer::DeclareTimer FTM_LFS("LFS Thread");
static LLFastTimer::DeclareTimer FTM_PAUSE_THREADS("Pause Threads");
static LLFastTimer::DeclareTimer FTM_IDLE("Idle");
static LLFastTimer::DeclareTimer FTM_PUMP("Pump");
static LLFastTimer::DeclareTimer FTM_PUMP_ARES("Ares");
static LLFastTimer::DeclareTimer FTM_PUMP_SERVICE("Service");
static LLFastTimer::DeclareTimer FTM_SERVICE_CALLBACK("Callback");
static LLFastTimer::DeclareTimer FTM_AGENT_AUTOPILOT("Autopilot");
static LLFastTimer::DeclareTimer FTM_AGENT_UPDATE("Update");

bool LLAppViewer::mainLoop()
{
	LLMemType mt1(LLMemType::MTYPE_MAIN);
	mMainloopTimeout = new LLWatchdogTimeout();
	
	//-------------------------------------------
	// Run main loop until time to quit
	//-------------------------------------------

	// Create IO Pump to use for HTTP Requests.
	gServicePump = new LLPumpIO(gAPRPoolp);
	LLHTTPClient::setPump(*gServicePump);
	LLCurl::setCAFile(gDirUtilp->getCAFile());

	// Note: this is where gLocalSpeakerMgr and gActiveSpeakerMgr used to be instantiated.

	LLVoiceChannel::initClass();
	LLVoiceClient::getInstance()->init(gServicePump);
	LLVoiceChannel::setCurrentVoiceChannelChangedCallback(boost::bind(&LLCallFloater::sOnCurrentChannelChanged, _1), true);
	LLTimer frameTimer,idleTimer;
	LLTimer debugTime;
	LLViewerJoystick* joystick(LLViewerJoystick::getInstance());
	joystick->setNeedsReset(true);

    LLEventPump& mainloop(LLEventPumps::instance().obtain("mainloop"));
    // As we do not (yet) send data on the mainloop LLEventPump that varies
    // with each frame, no need to instantiate a new LLSD event object each
    // time. Obviously, if that changes, just instantiate the LLSD at the
    // point of posting.
    LLSD newFrame;

	//LLPrivateMemoryPoolTester::getInstance()->run(false) ;
	//LLPrivateMemoryPoolTester::getInstance()->run(true) ;
	//LLPrivateMemoryPoolTester::destroy() ;

	// Handle messages
	while (!LLApp::isExiting())
	{
		LLFastTimer::nextFrame(); // Should be outside of any timer instances

		//clear call stack records
		llclearcallstacks;

		//check memory availability information
		checkMemory() ;
		
		try
		{
			pingMainloopTimeout("Main:MiscNativeWindowEvents");

			if (gViewerWindow)
			{
				LLFastTimer t2(FTM_MESSAGES);
				gViewerWindow->getWindow()->processMiscNativeEvents();
			}
		
			pingMainloopTimeout("Main:GatherInput");
			
			if (gViewerWindow)
			{
				LLFastTimer t2(FTM_MESSAGES);
				if (!restoreErrorTrap())
				{
					llwarns << " Someone took over my signal/exception handler (post messagehandling)!" << llendl;
				}

				gViewerWindow->getWindow()->gatherInput();
			}

#if 1 && !LL_RELEASE_FOR_DOWNLOAD
			// once per second debug info
			if (debugTime.getElapsedTimeF32() > 1.f)
			{
				debugTime.reset();
			}
			
#endif
			//memory leaking simulation
			LLFloaterMemLeak* mem_leak_instance =
				LLFloaterReg::findTypedInstance<LLFloaterMemLeak>("mem_leaking");
			if(mem_leak_instance)
			{
				mem_leak_instance->idle() ;				
			}			

            // canonical per-frame event
            mainloop.post(newFrame);

			if (!LLApp::isExiting())
			{
				pingMainloopTimeout("Main:JoystickKeyboard");
				
				// Scan keyboard for movement keys.  Command keys and typing
				// are handled by windows callbacks.  Don't do this until we're
				// done initializing.  JC
				if ((gHeadlessClient || gViewerWindow->getWindow()->getVisible())
					&& gViewerWindow->getActive()
					&& !gViewerWindow->getWindow()->getMinimized()
					&& LLStartUp::getStartupState() == STATE_STARTED
					&& (gHeadlessClient || !gViewerWindow->getShowProgress())
					&& !gFocusMgr.focusLocked())
				{
					LLMemType mjk(LLMemType::MTYPE_JOY_KEY);
					joystick->scanJoystick();
					gKeyboard->scanKeyboard();
				}

				// Update state based on messages, user input, object idle.
				{
					pauseMainloopTimeout(); // *TODO: Remove. Messages shouldn't be stalling for 20+ seconds!
					
					LLFastTimer t3(FTM_IDLE);
					idle();

					if (gAres != NULL && gAres->isInitialized())
					{
						LLMemType mt_ip(LLMemType::MTYPE_IDLE_PUMP);
						pingMainloopTimeout("Main:ServicePump");				
						LLFastTimer t4(FTM_PUMP);
						{
							LLFastTimer t(FTM_PUMP_ARES);
							gAres->process();
						}
						{
							LLFastTimer t(FTM_PUMP_SERVICE);
							// this pump is necessary to make the login screen show up
							gServicePump->pump();

							{
								LLFastTimer t(FTM_SERVICE_CALLBACK);
								gServicePump->callback();
							}
						}
					}
					
					resumeMainloopTimeout();
				}
 
				if (gDoDisconnect && (LLStartUp::getStartupState() == STATE_STARTED))
				{
					pauseMainloopTimeout();
					saveFinalSnapshot();
					disconnectViewer();
					resumeMainloopTimeout();
				}

				// Render scene.
				// *TODO: Should we run display() even during gHeadlessClient?  DK 2011-02-18
				if (!LLApp::isExiting() && !gHeadlessClient)
				{
					pingMainloopTimeout("Main:Display");
					gGLActive = TRUE;
					display();
					pingMainloopTimeout("Main:Snapshot");
					LLFloaterSnapshot::update(); // take snapshots
					gGLActive = FALSE;
				}

			}

			pingMainloopTimeout("Main:Sleep");
			
			pauseMainloopTimeout();

			// Sleep and run background threads
			{
				LLMemType mt_sleep(LLMemType::MTYPE_SLEEP);
				LLFastTimer t2(FTM_SLEEP);
				
				// yield some time to the os based on command line option
				if(mYieldTime >= 0)
				{
					ms_sleep(mYieldTime);
				}

				// yield cooperatively when not running as foreground window
				if (   (gViewerWindow && !gViewerWindow->getWindow()->getVisible())
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
				
				if (mRandomizeFramerate)
				{
					ms_sleep(rand() % 200);
				}

				if (mPeriodicSlowFrame
					&& (gFrameCount % 10 == 0))
				{
					llinfos << "Periodic slow frame - sleeping 500 ms" << llendl;
					ms_sleep(500);
				}

				static const F64 FRAME_SLOW_THRESHOLD = 0.5; //2 frames per seconds				
				const F64 max_idle_time = llmin(.005*10.0*gFrameTimeSeconds, 0.005); // 5 ms a second
				idleTimer.reset();
				bool is_slow = (frameTimer.getElapsedTimeF64() > FRAME_SLOW_THRESHOLD) ;
				S32 total_work_pending = 0;
				S32 total_io_pending = 0;	
				while(!is_slow)//do not unpause threads if the frame rates are very low.
				{
					S32 work_pending = 0;
					S32 io_pending = 0;
					F32 max_time = llmin(gFrameIntervalSeconds*10.f, 1.f);

					{
						LLFastTimer ftm(FTM_TEXTURE_CACHE);
 						work_pending += LLAppViewer::getTextureCache()->update(max_time); // unpauses the texture cache thread
					}
					{
						LLFastTimer ftm(FTM_DECODE);
	 					work_pending += LLAppViewer::getImageDecodeThread()->update(max_time); // unpauses the image thread
					}
					{
						LLFastTimer ftm(FTM_DECODE);
	 					work_pending += LLAppViewer::getTextureFetch()->update(max_time); // unpauses the texture fetch thread
					}

					{
						LLFastTimer ftm(FTM_VFS);
	 					io_pending += LLVFSThread::updateClass(1);
					}
					{
						LLFastTimer ftm(FTM_LFS);
	 					io_pending += LLLFSThread::updateClass(1);
					}

					if (io_pending > 1000)
					{
						ms_sleep(llmin(io_pending/100,100)); // give the vfs some time to catch up
					}

					total_work_pending += work_pending ;
					total_io_pending += io_pending ;
					
					if (!work_pending || idleTimer.getElapsedTimeF64() >= max_idle_time)
					{
						break;
					}
				}
				gMeshRepo.update() ;
				
				if(!LLCurl::getCurlThread()->update(1))
				{
					LLCurl::getCurlThread()->pause() ; //nothing in the curl thread.
				}

				if(!total_work_pending) //pause texture fetching threads if nothing to process.
				{
					LLAppViewer::getTextureCache()->pause();
					LLAppViewer::getImageDecodeThread()->pause();
					LLAppViewer::getTextureFetch()->pause(); 
				}
				if(!total_io_pending) //pause file threads if nothing to process.
				{
					LLVFSThread::sLocal->pause(); 
					LLLFSThread::sLocal->pause(); 
				}									

				if ((LLStartUp::getStartupState() >= STATE_CLEANUP) &&
					(frameTimer.getElapsedTimeF64() > FRAME_STALL_THRESHOLD))
				{
					gFrameStalls++;
				}
				frameTimer.reset();

				resumeMainloopTimeout();
	
				pingMainloopTimeout("Main:End");
			}	
		}
		catch(std::bad_alloc)
		{			
			LLMemory::logMemoryInfo(TRUE) ;

			//stop memory leaking simulation
			LLFloaterMemLeak* mem_leak_instance =
				LLFloaterReg::findTypedInstance<LLFloaterMemLeak>("mem_leaking");
			if(mem_leak_instance)
			{
				mem_leak_instance->stop() ;				
				llwarns << "Bad memory allocation in LLAppViewer::mainLoop()!" << llendl ;
			}
			else
			{
				//output possible call stacks to log file.
				LLError::LLCallStacks::print() ;

				llerrs << "Bad memory allocation in LLAppViewer::mainLoop()!" << llendl ;
			}
		}
	}

	// Save snapshot for next time, if we made it through initialization
	if (STATE_STARTED == LLStartUp::getStartupState())
	{
		try
		{
			saveFinalSnapshot();
		}
		catch(std::bad_alloc)
		{
			llwarns << "Bad memory allocation when saveFinalSnapshot() is called!" << llendl ;

			//stop memory leaking simulation
			LLFloaterMemLeak* mem_leak_instance =
				LLFloaterReg::findTypedInstance<LLFloaterMemLeak>("mem_leaking");
			if(mem_leak_instance)
			{
				mem_leak_instance->stop() ;				
			}	
		}
	}
	
	delete gServicePump;

	destroyMainloopTimeout();

	llinfos << "Exiting main_loop" << llendflush;

	return true;
}

void LLAppViewer::flushVFSIO()
{
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
}

bool LLAppViewer::cleanup()
{
	//ditch LLVOAvatarSelf instance
	gAgentAvatarp = NULL;

	// workaround for DEV-35406 crash on shutdown
	LLEventPumps::instance().reset();

	if (LLFastTimerView::sAnalyzePerformance)
	{
		llinfos << "Analyzing performance" << llendl;
		std::string baseline_name = LLFastTimer::sLogName + "_baseline.slp";
		std::string current_name  = LLFastTimer::sLogName + ".slp"; 
		std::string report_name   = LLFastTimer::sLogName + "_report.csv";

		LLFastTimerView::doAnalysis(
			gDirUtilp->getExpandedFilename(LL_PATH_LOGS, baseline_name),
			gDirUtilp->getExpandedFilename(LL_PATH_LOGS, current_name),
			gDirUtilp->getExpandedFilename(LL_PATH_LOGS, report_name));		
	}
	LLMetricPerformanceTesterBasic::cleanClass();

	// remove any old breakpad minidump files from the log directory
	if (! isError())
	{
		std::string logdir = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "");
		logdir += gDirUtilp->getDirDelimiter();
		gDirUtilp->deleteFilesInDir(logdir, "*-*-*-*-*.dmp");
	}

	// *TODO - generalize this and move DSO wrangling to a helper class -brad
	std::set<struct apr_dso_handle_t *>::const_iterator i;
	for(i = mPlugins.begin(); i != mPlugins.end(); ++i)
	{
		int (*ll_plugin_stop_func)(void) = NULL;
		apr_status_t rv = apr_dso_sym((apr_dso_handle_sym_t*)&ll_plugin_stop_func, *i, "ll_plugin_stop");
		ll_plugin_stop_func();

		rv = apr_dso_unload(*i);
	}
	mPlugins.clear();

	//flag all elements as needing to be destroyed immediately
	// to ensure shutdown order
	LLMortician::setZealous(TRUE);

	LLVoiceClient::getInstance()->terminate();
	
	disconnectViewer();

	llinfos << "Viewer disconnected" << llendflush;

	display_cleanup(); 

	release_start_screen(); // just in case

	LLError::logToFixedBuffer(NULL);

	llinfos << "Cleaning Up" << llendflush;

	// shut down mesh streamer
	gMeshRepo.shutdown();

	// Must clean up texture references before viewer window is destroyed.
	if(LLHUDManager::instanceExists())
	{
		LLHUDManager::getInstance()->updateEffects();
		LLHUDObject::updateAll();
		LLHUDManager::getInstance()->cleanupEffects();
		LLHUDObject::cleanupHUDObjects();
		llinfos << "HUD Objects cleaned up" << llendflush;
	}

	LLKeyframeDataCache::clear();
	
 	// End TransferManager before deleting systems it depends on (Audio, VFS, AssetStorage)
#if 0 // this seems to get us stuck in an infinite loop...
	gTransferManager.cleanup();
#endif
	
	// Note: this is where gWorldMap used to be deleted.

	// Note: this is where gHUDManager used to be deleted.
	if(LLHUDManager::instanceExists())
	{
		LLHUDManager::getInstance()->shutdownClass();
	}

	delete gAssetStorage;
	gAssetStorage = NULL;

	LLPolyMesh::freeAllMeshes();

	LLStartUp::cleanupNameCache();

	// Note: this is where gLocalSpeakerMgr and gActiveSpeakerMgr used to be deleted.

	LLWorldMap::getInstance()->reset(); // release any images

	LLCalc::cleanUp();

	llinfos << "Global stuff deleted" << llendflush;

	if (gAudiop)
	{
		// shut down the streaming audio sub-subsystem first, in case it relies on not outliving the general audio subsystem.

		LLStreamingAudioInterface *sai = gAudiop->getStreamingAudioImpl();
		delete sai;
		gAudiop->setStreamingAudioImpl(NULL);

		// shut down the audio subsystem

		bool want_longname = false;
		if (gAudiop->getDriverName(want_longname) == "FMOD")
		{
			// This hack exists because fmod likes to occasionally
			// crash or hang forever when shutting down, for no
			// apparent reason.
			llwarns << "Hack, skipping FMOD audio engine cleanup" << llendflush;
		}
		else
		{
			gAudiop->shutdown();
		}

		delete gAudiop;
		gAudiop = NULL;
	}

	// Note: this is where LLFeatureManager::getInstance()-> used to be deleted.

	// Patch up settings for next time
	// Must do this before we delete the viewer window,
	// such that we can suck rectangle information out of
	// it.
	cleanupSavedSettings();
	llinfos << "Settings patched up" << llendflush;

	// delete some of the files left around in the cache.
	removeCacheFiles("*.wav");
	removeCacheFiles("*.tmp");
	removeCacheFiles("*.lso");
	removeCacheFiles("*.out");
	removeCacheFiles("*.dsf");
	removeCacheFiles("*.bodypart");
	removeCacheFiles("*.clothing");

	llinfos << "Cache files removed" << llendflush;

	// Wait for any pending VFS IO
	flushVFSIO();
	llinfos << "Shutting down Views" << llendflush;

	// Destroy the UI
	if( gViewerWindow)
		gViewerWindow->shutdownViews();

	llinfos << "Cleaning up Inventory" << llendflush;
	
	// Cleanup Inventory after the UI since it will delete any remaining observers
	// (Deleted observers should have already removed themselves)
	gInventory.cleanupInventory();

	llinfos << "Cleaning up Selections" << llendflush;
	
	// Clean up selection managers after UI is destroyed, as UI may be observing them.
	// Clean up before GL is shut down because we might be holding on to objects with texture references
	LLSelectMgr::cleanupGlobals();
	
	llinfos << "Shutting down OpenGL" << llendflush;

	// Shut down OpenGL
	if( gViewerWindow)
	{
		gViewerWindow->shutdownGL();
	
		// Destroy window, and make sure we're not fullscreen
		// This may generate window reshape and activation events.
		// Therefore must do this before destroying the message system.
		delete gViewerWindow;
		gViewerWindow = NULL;
		llinfos << "ViewerWindow deleted" << llendflush;
	}

	llinfos << "Cleaning up Keyboard & Joystick" << llendflush;
	
	// viewer UI relies on keyboard so keep it aound until viewer UI isa gone
	delete gKeyboard;
	gKeyboard = NULL;

	// Turn off Space Navigator and similar devices
	LLViewerJoystick::getInstance()->terminate();
	
	llinfos << "Cleaning up Objects" << llendflush;
	
	LLViewerObject::cleanupVOClasses();
	
	LLPostProcess::cleanupClass();

	LLTracker::cleanupInstance();
	
	// *FIX: This is handled in LLAppViewerWin32::cleanup().
	// I'm keeping the comment to remember its order in cleanup,
	// in case of unforseen dependency.
	//#if LL_WINDOWS
	//	gDXHardware.cleanup();
	//#endif // LL_WINDOWS

	LLVolumeMgr* volume_manager = LLPrimitive::getVolumeManager();
	if (!volume_manager->cleanup())
	{
		llwarns << "Remaining references in the volume manager!" << llendflush;
	}
	LLPrimitive::cleanupVolumeManager();

	llinfos << "Additional Cleanup..." << llendflush;	
	
	LLViewerParcelMgr::cleanupGlobals();

	// *Note: this is where gViewerStats used to be deleted.

 	//end_messaging_system();

	LLFollowCamMgr::cleanupClass();
	//LLVolumeMgr::cleanupClass();
	LLPrimitive::cleanupVolumeManager();
	LLWorldMapView::cleanupClass();
	LLFolderViewItem::cleanupClass();
	LLUI::cleanupClass();
	
	//
	// Shut down the VFS's AFTER the decode manager cleans up (since it cleans up vfiles).
	// Also after viewerwindow is deleted, since it may have image pointers (which have vfiles)
	// Also after shutting down the messaging system since it has VFS dependencies

	//
	llinfos << "Cleaning up VFS" << llendflush;
	LLVFile::cleanupClass();

	llinfos << "Saving Data" << llendflush;
	
	// Store the time of our current logoff
	gSavedPerAccountSettings.setU32("LastLogoff", time_corrected());

	// Must do this after all panels have been deleted because panels that have persistent rects
	// save their rects on delete.
	gSavedSettings.saveToFile(gSavedSettings.getString("ClientSettingsFile"), TRUE);
	
	LLUIColorTable::instance().saveUserSettings();

	// PerAccountSettingsFile should be empty if no user has been logged on.
	// *FIX:Mani This should get really saved in a "logoff" mode. 
	if (gSavedSettings.getString("PerAccountSettingsFile").empty())
	{
		llinfos << "Not saving per-account settings; don't know the account name yet." << llendl;
	}
	else
	{
		gSavedPerAccountSettings.saveToFile(gSavedSettings.getString("PerAccountSettingsFile"), TRUE);
		llinfos << "Saved settings" << llendflush;
	}

	std::string warnings_settings_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, getSettingsFilename("Default", "Warnings"));
	gWarningSettings.saveToFile(warnings_settings_filename, TRUE);

	// Save URL history file
	LLURLHistory::saveFile("url_history.xml");

	// save mute list. gMuteList used to also be deleted here too.
	LLMuteList::getInstance()->cache(gAgent.getID());

	if (mPurgeOnExit)
	{
		llinfos << "Purging all cache files on exit" << llendflush;
		std::string mask = gDirUtilp->getDirDelimiter() + "*.*";
		gDirUtilp->deleteFilesInDir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE,""),mask);
	}

	removeMarkerFile(); // Any crashes from here on we'll just have to ignore
	
	writeDebugInfo();

	LLLocationHistory::getInstance()->save();

	LLAvatarIconIDCache::getInstance()->save();
	
	LLViewerMedia::saveCookieFile();

	// Stop the plugin read thread if it's running.
	LLPluginProcessParent::setUseReadThread(false);

	llinfos << "Shutting down Threads" << llendflush;

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
		pending += LLCurl::getCurlThread()->update(1) ;
		F64 idle_time = idleTimer.getElapsedTimeF64();
		if(!pending)
		{
			break ; //done
		}
		else if(idle_time >= max_idle_time)
		{
			llwarns << "Quitting with pending background tasks." << llendl;
			break;
		}
	}
	LLCurl::getCurlThread()->pause() ;

	// Delete workers first
	// shotdown all worker threads before deleting them in case of co-dependencies
	sTextureFetch->shutdown();
	sTextureCache->shutdown();	
	sImageDecodeThread->shutdown();
	
	sTextureFetch->shutDownTextureCacheThread() ;
	sTextureFetch->shutDownImageDecodeThread() ;

	LLFilePickerThread::cleanupClass();

	delete sTextureCache;
    sTextureCache = NULL;
	delete sTextureFetch;
    sTextureFetch = NULL;
	delete sImageDecodeThread;
    sImageDecodeThread = NULL;
	delete mFastTimerLogThread;
	mFastTimerLogThread = NULL;
	
	if (LLFastTimerView::sAnalyzePerformance)
	{
		llinfos << "Analyzing performance" << llendl;
		
		std::string baseline_name = LLFastTimer::sLogName + "_baseline.slp";
		std::string current_name  = LLFastTimer::sLogName + ".slp"; 
		std::string report_name   = LLFastTimer::sLogName + "_report.csv";

		LLFastTimerView::doAnalysis(
			gDirUtilp->getExpandedFilename(LL_PATH_LOGS, baseline_name),
			gDirUtilp->getExpandedFilename(LL_PATH_LOGS, current_name),
			gDirUtilp->getExpandedFilename(LL_PATH_LOGS, report_name));
	}	

	LLMetricPerformanceTesterBasic::cleanClass() ;

#if LL_RECORD_VIEWER_STATS
	LLViewerStatsRecorder::cleanupClass();
#endif

	llinfos << "Cleaning up Media and Textures" << llendflush;

	//Note:
	//LLViewerMedia::cleanupClass() has to be put before gTextureList.shutdown()
	//because some new image might be generated during cleaning up media. --bao
	LLViewerMedia::cleanupClass();
	LLViewerParcelMedia::cleanupClass();
	gTextureList.shutdown(); // shutdown again in case a callback added something
	LLUIImageList::getInstance()->cleanUp();
	
	// This should eventually be done in LLAppViewer
	LLImage::cleanupClass();
	LLVFSThread::cleanupClass();
	LLLFSThread::cleanupClass();

#ifndef LL_RELEASE_FOR_DOWNLOAD
	llinfos << "Auditing VFS" << llendl;
	if(gVFS)
	{
		gVFS->audit();
	}
#endif

	llinfos << "Misc Cleanup" << llendflush;
	
	// For safety, the LLVFS has to be deleted *after* LLVFSThread. This should be cleaned up.
	// (LLVFS doesn't know about LLVFSThread so can't kill pending requests) -Steve
	delete gStaticVFS;
	gStaticVFS = NULL;
	delete gVFS;
	gVFS = NULL;
	
	gSavedSettings.cleanup();
	LLUIColorTable::instance().clear();

	LLWatchdog::getInstance()->cleanup();

	LLViewerAssetStatsFF::cleanup();
	
	llinfos << "Shutting down message system" << llendflush;
	end_messaging_system();

	// *NOTE:Mani - The following call is not thread safe. 
	LLCurl::cleanupClass();

	// If we're exiting to launch an URL, do that here so the screen
	// is at the right resolution before we launch IE.
	if (!gLaunchFileOnQuit.empty())
	{
		llinfos << "Launch file on quit." << llendflush;
#if LL_WINDOWS
		// Indicate an application is starting.
		SetCursor(LoadCursor(NULL, IDC_WAIT));
#endif

		// HACK: Attempt to wait until the screen res. switch is complete.
		ms_sleep(1000);

		LLWeb::loadURLExternal( gLaunchFileOnQuit, false );
		llinfos << "File launched." << llendflush;
	}
	llinfos << "Cleaning up LLProxy." << llendl;
	LLProxy::cleanupClass();

	LLMainLoopRepeater::instance().stop();

	//release all private memory pools.
	LLPrivateMemoryPoolManager::destroyClass() ;

	ll_close_fail_log();

	MEM_TRACK_RELEASE

    llinfos << "Goodbye!" << llendflush;

	// return 0;
	return true;
}

// A callback for llerrs to call during the watchdog error.
void watchdog_llerrs_callback(const std::string &error_string)
{
	gLLErrorActivated = true;

#ifdef LL_WINDOWS
	RaiseException(0,0,0,0);
#else
	raise(SIGQUIT);
#endif
}

// A callback for the watchdog to call.
void watchdog_killer_callback()
{
	LLError::setFatalFunction(watchdog_llerrs_callback);
	llerrs << "Watchdog killer event" << llendl;
}

bool LLAppViewer::initThreads()
{
#if MEM_TRACK_MEM
	static const bool enable_threads = false;
#else
	static const bool enable_threads = true;
#endif

	LLImage::initClass(gSavedSettings.getBOOL("TextureNewByteRange"));

	LLVFSThread::initClass(enable_threads && false);
	LLLFSThread::initClass(enable_threads && false);

	// Image decoding
	LLAppViewer::sImageDecodeThread = new LLImageDecodeThread(enable_threads && true);
	LLAppViewer::sTextureCache = new LLTextureCache(enable_threads && true);
	LLAppViewer::sTextureFetch = new LLTextureFetch(LLAppViewer::getTextureCache(),
													sImageDecodeThread,
													enable_threads && true,
													app_metrics_qa_mode);	

	if (LLFastTimer::sLog || LLFastTimer::sMetricLog)
	{
		LLFastTimer::sLogLock = new LLMutex(NULL);
		mFastTimerLogThread = new LLFastTimerLogThread(LLFastTimer::sLogName);
		mFastTimerLogThread->start();
	}

	// Mesh streaming and caching
	gMeshRepo.init();

	LLFilePickerThread::initClass();

	// *FIX: no error handling here!
	return true;
}

void errorCallback(const std::string &error_string)
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	OSMessageBox(error_string, LLTrans::getString("MBFatalError"), OSMB_OK);
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
	LLFile::remove(old_log_file);

	// Rename current log file to ".old"
	std::string log_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,
							     "SecondLife.log");
	LLFile::rename(log_file, old_log_file);

	// Set the log file to SecondLife.log

	LLError::logToFile(log_file);

	// *FIX:Mani no error handling here!
	return true;
}

bool LLAppViewer::loadSettingsFromDirectory(const std::string& location_key,
					    bool set_defaults)
{	
	if (!mSettingsLocationList)
	{
		llerrs << "Invalid settings location list" << llendl;
	}

	BOOST_FOREACH(const SettingsGroup& group, mSettingsLocationList->groups)
	{
		// skip settings groups that aren't the one we requested
		if (group.name() != location_key) continue;

		ELLPath path_index = (ELLPath)group.path_index();
		if(path_index <= LL_PATH_NONE || path_index >= LL_PATH_LAST)
		{
			llerrs << "Out of range path index in app_settings/settings_files.xml" << llendl;
			return false;
		}

		BOOST_FOREACH(const SettingsFile& file, group.files)
		{
			llinfos << "Attempting to load settings for the group " << file.name()
			    << " - from location " << location_key << llendl;

			LLControlGroup* settings_group = LLControlGroup::getInstance(file.name);
			if(!settings_group)
			{
				llwarns << "No matching settings group for name " << file.name() << llendl;
				continue;
			}

			std::string full_settings_path;

			if (file.file_name_setting.isProvided() 
				&& gSavedSettings.controlExists(file.file_name_setting))
			{
				// try to find filename stored in file_name_setting control
				full_settings_path = gSavedSettings.getString(file.file_name_setting);
				if (full_settings_path.empty())
				{
					continue;
				}
				else if (!gDirUtilp->fileExists(full_settings_path))
				{
					// search in default path
					full_settings_path = gDirUtilp->getExpandedFilename((ELLPath)path_index, full_settings_path);
				}
			}
			else
			{
				// by default, use specified file name
				full_settings_path = gDirUtilp->getExpandedFilename((ELLPath)path_index, file.file_name());
			}

			if(settings_group->loadFromFile(full_settings_path, set_defaults, file.persistent))
			{	// success!
				llinfos << "Loaded settings file " << full_settings_path << llendl;
			}
			else
			{	// failed to load
				if(file.required)
				{
					llerrs << "Error: Cannot load required settings file from: " << full_settings_path << llendl;
					return false;
				}
				else
				{
					// only complain if we actually have a filename at this point
					if (!full_settings_path.empty())
					{
						llinfos << "Cannot load " << full_settings_path << " - No settings found." << llendl;
					}
				}
			}
		}
	}

	return true;
}

std::string LLAppViewer::getSettingsFilename(const std::string& location_key,
											 const std::string& file)
{
	BOOST_FOREACH(const SettingsGroup& group, mSettingsLocationList->groups)
	{
		if (group.name() == location_key)
		{
			BOOST_FOREACH(const SettingsFile& settings_file, group.files)
			{
				if (settings_file.name() == file)
				{
					return settings_file.file_name;
				}
			}
		}
	}

	return std::string();
}

void LLAppViewer::loadColorSettings()
{
	LLUIColorTable::instance().loadFromSettings();
}

bool LLAppViewer::initConfiguration()
{	
	//Load settings files list
	std::string settings_file_list = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "settings_files.xml");
	//LLControlGroup settings_control("SettingsFiles");
	//llinfos << "Loading settings file list " << settings_file_list << llendl;
	//if (0 == settings_control.loadFromFile(settings_file_list))
	//{
 //       llerrs << "Cannot load default configuration file " << settings_file_list << llendl;
	//}

	LLXMLNodePtr root;
	BOOL success  = LLXMLNode::parseFile(settings_file_list, root, NULL);
	if (!success)
	{
        llerrs << "Cannot load default configuration file " << settings_file_list << llendl;
	}

	mSettingsLocationList = new SettingsFiles();

	LLXUIParser parser;
	parser.readXUI(root, *mSettingsLocationList, settings_file_list);

	if (!mSettingsLocationList->validateBlock())
	{
        llerrs << "Invalid settings file list " << settings_file_list << llendl;
	}
		
	// The settings and command line parsing have a fragile
	// order-of-operation:
	// - load defaults from app_settings
	// - set procedural settings values
	// - read command line settings
	// - selectively apply settings needed to load user settings.
    // - load overrides from user_settings 
	// - apply command line settings (to override the overrides)
	// - load per account settings (happens in llstartup
	
	// - load defaults
	bool set_defaults = true;
	if(!loadSettingsFromDirectory("Default", set_defaults))
	{
		std::ostringstream msg;
		msg << "Unable to load default settings file. The installation may be corrupted.";
		OSMessageBox(msg.str(),LLStringUtil::null,OSMB_OK);
		return false;
	}
	
	LLUI::setupPaths(); // setup paths for LLTrans based on settings files only
	LLTransUtil::parseStrings("strings.xml", default_trans_args);
	LLTransUtil::parseLanguageStrings("language_settings.xml");
	// - set procedural settings
	// Note: can't use LL_PATH_PER_SL_ACCOUNT for any of these since we haven't logged in yet
	gSavedSettings.setString("ClientSettingsFile", 
        gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, getSettingsFilename("Default", "Global")));

	gSavedSettings.setString("VersionChannelName", LLVersionInfo::getChannel());

#ifndef	LL_RELEASE_FOR_DOWNLOAD
	// provide developer build only overrides for these control variables that are not
	// persisted to settings.xml
	LLControlVariable* c = gSavedSettings.getControl("ShowConsoleWindow");
	if (c)
	{
		c->setValue(true, false);
	}
	c = gSavedSettings.getControl("AllowMultipleViewers");
	if (c)
	{
		c->setValue(true, false);
	}
#endif

#ifndef	LL_RELEASE_FOR_DOWNLOAD
	gSavedSettings.setBOOL("QAMode", TRUE );
	gSavedSettings.setS32("WatchdogEnabled", 0);
#endif
	
	// These are warnings that appear on the first experience of that condition.
	// They are already set in the settings_default.xml file, but still need to be added to LLFirstUse
	// for disable/reset ability
//	LLFirstUse::addConfigVariable("FirstBalanceIncrease");
//	LLFirstUse::addConfigVariable("FirstBalanceDecrease");
//	LLFirstUse::addConfigVariable("FirstSit");
//	LLFirstUse::addConfigVariable("FirstMap");
//	LLFirstUse::addConfigVariable("FirstGoTo");
//	LLFirstUse::addConfigVariable("FirstBuild");
//	LLFirstUse::addConfigVariable("FirstLeftClickNoHit");
//	LLFirstUse::addConfigVariable("FirstTeleport");
//	LLFirstUse::addConfigVariable("FirstOverrideKeys");
//	LLFirstUse::addConfigVariable("FirstAttach");
//	LLFirstUse::addConfigVariable("FirstAppearance");
//	LLFirstUse::addConfigVariable("FirstInventory");
//	LLFirstUse::addConfigVariable("FirstSandbox");
//	LLFirstUse::addConfigVariable("FirstFlexible");
//	LLFirstUse::addConfigVariable("FirstDebugMenus");
//	LLFirstUse::addConfigVariable("FirstSculptedPrim");
//	LLFirstUse::addConfigVariable("FirstVoice");
//	LLFirstUse::addConfigVariable("FirstMedia");
		
	// - read command line settings.
	LLControlGroupCLP clp;
	std::string	cmd_line_config	= gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,
														  "cmd_line.xml");

	clp.configure(cmd_line_config, &gSavedSettings);

	if(!initParseCommandLine(clp))
	{
		llwarns	<< "Error parsing command line options.	Command	Line options ignored."  << llendl;
		
		llinfos	<< "Command	line usage:\n" << clp << llendl;

		std::ostringstream msg;
		msg << LLTrans::getString("MBCmdLineError") << clp.getErrorMessage();
		OSMessageBox(msg.str(),LLStringUtil::null,OSMB_OK);
		return false;
	}
	
	// - selectively apply settings 

	// If the user has specified a alternate settings file name.
	// Load	it now before loading the user_settings/settings.xml
	if(clp.hasOption("settings"))
	{
		std::string	user_settings_filename = 
			gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, 
										   clp.getOption("settings")[0]);		
		gSavedSettings.setString("ClientSettingsFile", user_settings_filename);
		llinfos	<< "Using command line specified settings filename: " 
			<< user_settings_filename << llendl;
	}

	// - load overrides from user_settings 
	loadSettingsFromDirectory("User");

	if (gSavedSettings.getBOOL("FirstRunThisInstall"))
	{
		// Note that the "FirstRunThisInstall" settings is currently unused.
		gSavedSettings.setBOOL("FirstRunThisInstall", FALSE);
	}

	if (clp.hasOption("sessionsettings"))
	{
		std::string session_settings_filename = clp.getOption("sessionsettings")[0];		
		gSavedSettings.setString("SessionSettingsFile", session_settings_filename);
		llinfos	<< "Using session settings filename: " 
			<< session_settings_filename << llendl;
	}
	loadSettingsFromDirectory("Session");

	if (clp.hasOption("usersessionsettings"))
	{
		std::string user_session_settings_filename = clp.getOption("usersessionsettings")[0];		
		gSavedSettings.setString("UserSessionSettingsFile", user_session_settings_filename);
		llinfos	<< "Using user session settings filename: " 
			<< user_session_settings_filename << llendl;

	}
	loadSettingsFromDirectory("UserSession");

	// - apply command line settings 
	clp.notify(); 

	// Register the core crash option as soon as we can
	// if we want gdb post-mortem on cores we need to be up and running
	// ASAP or we might miss init issue etc.
	if(clp.hasOption("disablecrashlogger"))
	{
		llwarns << "Crashes will be handled by system, stack trace logs and crash logger are both disabled" << llendl;
		LLAppViewer::instance()->disableCrashlogger();
	}

	// Handle initialization from settings.
	// Start up the debugging console before handling other options.
	if (gSavedSettings.getBOOL("ShowConsoleWindow"))
	{
		initConsole();
	}

	if(clp.hasOption("help"))
	{
		std::ostringstream msg;
		msg << LLTrans::getString("MBCmdLineUsg") << "\n" << clp;
		llinfos	<< msg.str() << llendl;

		OSMessageBox(
			msg.str().c_str(),
			LLStringUtil::null,
			OSMB_OK);

		return false;
	}

    if(clp.hasOption("set"))
    {
        const LLCommandLineParser::token_vector_t& set_values = clp.getOption("set");
        if(0x1 & set_values.size())
        {
            llwarns << "Invalid '--set' parameter count." << llendl;
        }
        else
        {
            LLCommandLineParser::token_vector_t::const_iterator itr = set_values.begin();
            for(; itr != set_values.end(); ++itr)
            {
                const std::string& name = *itr;
                const std::string& value = *(++itr);
                std::string name_part;
                std::string group_part;
				LLControlVariable* control = NULL;

				// Name can be further split into ControlGroup.Name, with the default control group being Global
				size_t pos = name.find('.');
				if (pos != std::string::npos)
				{
					group_part = name.substr(0, pos);
					name_part = name.substr(pos+1);
					llinfos << "Setting " << group_part << "." << name_part << " to " << value << llendl;
					LLControlGroup* g = LLControlGroup::getInstance(group_part);
					if (g) control = g->getControl(name_part);
				}
				else
				{
					llinfos << "Setting Global." << name << " to " << value << llendl;
					control = gSavedSettings.getControl(name);
				}

                if (control)
                {
                    control->setValue(value, false);
                }
                else
                {
					llwarns << "Failed --set " << name << ": setting name unknown." << llendl;
                }
            }
        }
    }

    if(clp.hasOption("channel"))
    {
		LLVersionInfo::resetChannel(clp.getOption("channel")[0]);
	}

	// If we have specified crash on startup, set the global so we'll trigger the crash at the right time
	if(clp.hasOption("crashonstartup"))
	{
		gCrashOnStartup = TRUE;
	}

	if (clp.hasOption("logperformance"))
	{
		LLFastTimer::sLog = TRUE;
		LLFastTimer::sLogName = std::string("performance");		
	}
	
	if (clp.hasOption("logmetrics"))
 	{
 		LLFastTimer::sMetricLog = TRUE ;
		// '--logmetrics' can be specified with a named test metric argument so the data gathering is done only on that test
		// In the absence of argument, every metric is gathered (makes for a rather slow run and hard to decipher report...)
		std::string test_name = clp.getOption("logmetrics")[0];
		llinfos << "'--logmetrics' argument : " << test_name << llendl;
		if (test_name == "")
		{
			llwarns << "No '--logmetrics' argument given, will output all metrics to " << DEFAULT_METRIC_NAME << llendl;
			LLFastTimer::sLogName = DEFAULT_METRIC_NAME;
		}
		else
		{
			LLFastTimer::sLogName = test_name;
		}
 	}

	if (clp.hasOption("graphicslevel"))
	{
		const LLCommandLineParser::token_vector_t& value = clp.getOption("graphicslevel");
        if(value.size() != 1)
        {
			llwarns << "Usage: -graphicslevel <0-3>" << llendl;
        }
        else
        {
			std::string detail = value.front();
			mForceGraphicsDetail = TRUE;
			
			switch (detail.c_str()[0])
			{
				case '0': 
					gSavedSettings.setU32("RenderQualityPerformance", 0);		
					break;
				case '1': 
					gSavedSettings.setU32("RenderQualityPerformance", 1);		
					break;
				case '2': 
					gSavedSettings.setU32("RenderQualityPerformance", 2);		
					break;
				case '3': 
					gSavedSettings.setU32("RenderQualityPerformance", 3);		
					break;
				default:
					mForceGraphicsDetail = FALSE;
					llwarns << "Usage: -graphicslevel <0-3>" << llendl;
					break;
			}
        }
	}

	if (clp.hasOption("analyzeperformance"))
	{
		LLFastTimerView::sAnalyzePerformance = TRUE;
	}

	if (clp.hasOption("replaysession"))
	{
		gAgentPilot.setReplaySession(TRUE);
	}

	if (clp.hasOption("nonotifications"))
	{
		gSavedSettings.getControl("IgnoreAllNotifications")->setValue(true, false);
	}
	
	if (clp.hasOption("debugsession"))
	{
		gDebugSession = TRUE;
		gDebugGL = TRUE;

		ll_init_fail_log(gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "test_failures.log"));
	}

	// Handle slurl use. NOTE: Don't let SL-55321 reappear.

    // *FIX: This init code should be made more robust to prevent 
    // the issue SL-55321 from returning. One thought is to allow 
    // only select options to be set from command line when a slurl 
    // is specified. More work on the settings system is needed to 
    // achieve this. For now...

    // *NOTE:Mani The command line parser parses tokens and is 
    // setup to bail after parsing the '--url' option or the 
    // first option specified without a '--option' flag (or
    // any other option that uses the 'last_option' setting - 
    // see LLControlGroupCLP::configure())

    // What can happen is that someone can use IE (or potentially 
    // other browsers) and do the rough equivalent of command 
    // injection and steal passwords. Phoenix. SL-55321
    if(clp.hasOption("url"))
    {
		LLStartUp::setStartSLURL(LLSLURL(clp.getOption("url")[0]));
		if(LLStartUp::getStartSLURL().getType() == LLSLURL::LOCATION) 
		{  
			LLGridManager::getInstance()->setGridChoice(LLStartUp::getStartSLURL().getGrid());
			
		}  
    }
    else if(clp.hasOption("slurl"))
    {
		LLSLURL start_slurl(clp.getOption("slurl")[0]);
		LLStartUp::setStartSLURL(start_slurl);
    }

    const LLControlVariable* skinfolder = gSavedSettings.getControl("SkinCurrent");
    if(skinfolder && LLStringUtil::null != skinfolder->getValue().asString())
    {   
		// hack to force the skin to default.
        gDirUtilp->setSkinFolder(skinfolder->getValue().asString());
		//gDirUtilp->setSkinFolder("default");
    }

    mYieldTime = gSavedSettings.getS32("YieldTime");

	// Read skin/branding settings if specified.
	//if (! gDirUtilp->getSkinDir().empty() )
	//{
	//	std::string skin_def_file = gDirUtilp->findSkinnedFilename("skin.xml");
	//	LLXmlTree skin_def_tree;

	//	if (!skin_def_tree.parseFile(skin_def_file))
	//	{
	//		llerrs << "Failed to parse skin definition." << llendl;
	//	}

	//}

#if LL_DARWIN
	// Initialize apple menubar and various callbacks
	init_apple_menu(LLTrans::getString("APP_NAME").c_str());

#if __ppc__
	// If the CPU doesn't have Altivec (i.e. it's not at least a G4), don't go any further.
	// Only test PowerPC - all Intel Macs have SSE.
	if(!gSysCPU.hasAltivec())
	{
		std::ostringstream msg;
		msg << LLTrans::getString("MBRequiresAltiVec");
		OSMessageBox(
			msg.str(),
			LLStringUtil::null,
			OSMB_OK);
		removeMarkerFile();
		return false;
	}
#endif
	
#endif // LL_DARWIN

	// Display splash screen.  Must be after above check for previous
	// crash as this dialog is always frontmost.
	std::string splash_msg;
	LLStringUtil::format_map_t args;
	args["[APP_NAME]"] = LLTrans::getString("SECOND_LIFE");
	splash_msg = LLTrans::getString("StartupLoading", args);
	LLSplashScreen::show();
	LLSplashScreen::update(splash_msg);

	//LLVolumeMgr::initClass();
	LLVolumeMgr* volume_manager = new LLVolumeMgr();
	volume_manager->useMutex();	// LLApp and LLMutex magic must be manually enabled
	LLPrimitive::setVolumeManager(volume_manager);

	// Note: this is where we used to initialize gFeatureManagerp.

	gStartTime = totalTime();

	//
	// Set the name of the window
	//
	gWindowTitle = LLTrans::getString("APP_NAME");
#if LL_DEBUG
	gWindowTitle += std::string(" [DEBUG] ") + gArgs;
#else
	gWindowTitle += std::string(" ") + gArgs;
#endif
	LLStringUtil::truncate(gWindowTitle, 255);

	//RN: if we received a URL, hand it off to the existing instance.
	// don't call anotherInstanceRunning() when doing URL handoff, as
	// it relies on checking a marker file which will not work when running
	// out of different directories

	if (LLStartUp::getStartSLURL().isValid() &&
		(gSavedSettings.getBOOL("SLURLPassToOtherInstance")))
	{
		if (sendURLToOtherInstance(LLStartUp::getStartSLURL().getSLURLString()))
		{
			// successfully handed off URL to existing instance, exit
			return false;
		}
	}

	// If automatic login from command line with --login switch
	// init StartSLURL location. In interactive login, LLPanelLogin
	// will take care of it.
	if ((clp.hasOption("login") || clp.hasOption("autologin")) && !clp.hasOption("url") && !clp.hasOption("slurl"))
	{
		LLStartUp::setStartSLURL(LLSLURL(gSavedSettings.getString("LoginLocation")));
	}

	if (!gSavedSettings.getBOOL("AllowMultipleViewers"))
	{
	    //
	    // Check for another instance of the app running
	    //

		mSecondInstance = anotherInstanceRunning();
		
		if (mSecondInstance)
		{
			std::ostringstream msg;
			msg << LLTrans::getString("MBAlreadyRunning");
			OSMessageBox(
				msg.str(),
				LLStringUtil::null,
				OSMB_OK);
			return false;
		}

		initMarkerFile();
        
        checkForCrash();
    }
	else
	{
		mSecondInstance = anotherInstanceRunning();
		
		if (mSecondInstance)
		{
			// This is the second instance of SL. Turn off voice support,
			// but make sure the setting is *not* persisted.
			LLControlVariable* disable_voice = gSavedSettings.getControl("CmdLineDisableVoice");
			if(disable_voice)
			{
				const BOOL DO_NOT_PERSIST = FALSE;
				disable_voice->setValue(LLSD(TRUE), DO_NOT_PERSIST);
			}
		}

		initMarkerFile();
        
        if(!mSecondInstance)
        {
            checkForCrash();
        }
	}

   	// need to do this here - need to have initialized global settings first
	std::string nextLoginLocation = gSavedSettings.getString( "NextLoginLocation" );
	if ( !nextLoginLocation.empty() )
	{
		LLStartUp::setStartSLURL(LLSLURL(nextLoginLocation));
	};

	gLastRunVersion = gSavedSettings.getString("LastRunVersion");

	loadColorSettings();

	return true; // Config was successful.
}

namespace {
    // *TODO - decide if there's a better place for these functions.
	// do we need a file llupdaterui.cpp or something? -brad

	void apply_update_callback(LLSD const & notification, LLSD const & response)
	{
		lldebugs << "LLUpdate user response: " << response << llendl;
		if(response["OK_okcancelbuttons"].asBoolean())
		{
			llinfos << "LLUpdate restarting viewer" << llendl;
			static const bool install_if_ready = true;
			// *HACK - this lets us launch the installer immediately for now
			LLUpdaterService().startChecking(install_if_ready);
		}
	}
	
	void apply_update_ok_callback(LLSD const & notification, LLSD const & response)
	{
		llinfos << "LLUpdate restarting viewer" << llendl;
		static const bool install_if_ready = true;
		// *HACK - this lets us launch the installer immediately for now
		LLUpdaterService().startChecking(install_if_ready);
	}
	
	void on_update_downloaded(LLSD const & data)
	{
		std::string notification_name;
		void (*apply_callback)(LLSD const &, LLSD const &) = NULL;

		if(data["required"].asBoolean())
		{
			if(LLStartUp::getStartupState() <= STATE_LOGIN_WAIT)
			{
				// The user never saw the progress bar.
				apply_callback = &apply_update_ok_callback;
				notification_name = "RequiredUpdateDownloadedVerboseDialog";
			}
			else if(LLStartUp::getStartupState() < STATE_WORLD_INIT)
			{
				// The user is logging in but blocked.
				apply_callback = &apply_update_ok_callback;
				notification_name = "RequiredUpdateDownloadedDialog";
			}
			else
			{
				// The user is already logged in; treat like an optional update.
				apply_callback = &apply_update_callback;
				notification_name = "DownloadBackgroundTip";
			}
		}
		else
		{
			apply_callback = &apply_update_callback;
			if(LLStartUp::getStartupState() < STATE_STARTED)
			{
				// CHOP-262 we need to use a different notification
				// method prior to login.
				notification_name = "DownloadBackgroundDialog";
			}
			else
			{
				notification_name = "DownloadBackgroundTip";
			}
		}

		LLSD substitutions;
		substitutions["VERSION"] = data["version"];

		// truncate version at the rightmost '.' 
		std::string version_short(data["version"]);
		size_t short_length = version_short.rfind('.');
		if (short_length != std::string::npos)
		{
			version_short.resize(short_length);
		}

		LLUIString relnotes_url("[RELEASE_NOTES_BASE_URL][CHANNEL_URL]/[VERSION_SHORT]");
		relnotes_url.setArg("[VERSION_SHORT]", version_short);

		// *TODO thread the update service's response through to this point
		std::string const & channel = LLVersionInfo::getChannel();
		boost::shared_ptr<char> channel_escaped(curl_escape(channel.c_str(), channel.size()), &curl_free);

		relnotes_url.setArg("[CHANNEL_URL]", channel_escaped.get());
		relnotes_url.setArg("[RELEASE_NOTES_BASE_URL]", LLTrans::getString("RELEASE_NOTES_BASE_URL"));
		substitutions["RELEASE_NOTES_FULL_URL"] = relnotes_url.getString();

		LLNotificationsUtil::add(notification_name, substitutions, LLSD(), apply_callback);
	}

	void install_error_callback(LLSD const & notification, LLSD const & response)
	{
		LLAppViewer::instance()->forceQuit();
	}
	
	bool notify_update(LLSD const & evt)
	{
		std::string notification_name;
		switch (evt["type"].asInteger())
		{
			case LLUpdaterService::DOWNLOAD_COMPLETE:
				on_update_downloaded(evt);
				break;
			case LLUpdaterService::INSTALL_ERROR:
				if(evt["required"].asBoolean()) {
					LLNotificationsUtil::add("FailedRequiredUpdateInstall", LLSD(), LLSD(), &install_error_callback);
				} else {
					LLNotificationsUtil::add("FailedUpdateInstall");
				}
				break;
			default:
				break;
		}

		// let others also handle this event by default
		return false;
	}
	
	bool on_bandwidth_throttle(LLUpdaterService * updater, LLSD const & evt)
	{
		updater->setBandwidthLimit(evt.asInteger() * (1024/8));
		return false; // Let others receive this event.
	};
};

void LLAppViewer::initUpdater()
{
	// Initialize the updater service.
	// Generate URL to the udpater service
	// Get Channel
	// Get Version
	std::string url = gSavedSettings.getString("UpdaterServiceURL");
	std::string channel = LLVersionInfo::getChannel();
	std::string version = LLVersionInfo::getVersion();
	std::string protocol_version = gSavedSettings.getString("UpdaterServiceProtocolVersion");
	std::string service_path = gSavedSettings.getString("UpdaterServicePath");
	U32 check_period = gSavedSettings.getU32("UpdaterServiceCheckPeriod");

	mUpdater->setAppExitCallback(boost::bind(&LLAppViewer::forceQuit, this));
	mUpdater->initialize(protocol_version, 
						 url, 
						 service_path, 
						 channel, 
						 version);
 	mUpdater->setCheckPeriod(check_period);
	mUpdater->setBandwidthLimit((int)gSavedSettings.getF32("UpdaterMaximumBandwidth") * (1024/8));
	gSavedSettings.getControl("UpdaterMaximumBandwidth")->getSignal()->
		connect(boost::bind(&on_bandwidth_throttle, mUpdater.get(), _2));
	if(gSavedSettings.getU32("UpdaterServiceSetting"))
	{
		bool install_if_ready = true;
		mUpdater->startChecking(install_if_ready);
	}

    LLEventPump & updater_pump = LLEventPumps::instance().obtain(LLUpdaterService::pumpName());
    updater_pump.listen("notify_update", &notify_update);
}

void LLAppViewer::checkForCrash(void)
{
#if LL_SEND_CRASH_REPORTS
	if (gLastExecEvent == LAST_EXEC_FROZE)
    {
        llinfos << "Last execution froze, sending a crash report." << llendl;
            
		bool report_freeze = true;
		handleCrashReporting(report_freeze);
    }
#endif // LL_SEND_CRASH_REPORTS    
}

//
// This function decides whether the client machine meets the minimum requirements to
// run in a maximized window, per the consensus of davep, boa and nyx on 3/30/2011.
//
bool LLAppViewer::meetsRequirementsForMaximizedStart()
{
	bool maximizedOk = (LLFeatureManager::getInstance()->getGPUClass() >= GPU_CLASS_2);

	const U32 one_gigabyte_kb = 1024 * 1024;
	maximizedOk &= (gSysMemory.getPhysicalMemoryKB() >= one_gigabyte_kb);

	return maximizedOk;
}

bool LLAppViewer::initWindow()
{
	LL_INFOS("AppInit") << "Initializing window..." << LL_ENDL;

	// store setting in a global for easy access and modification
	gHeadlessClient = gSavedSettings.getBOOL("HeadlessClient");

	// always start windowed
	BOOL ignorePixelDepth = gSavedSettings.getBOOL("IgnorePixelDepth");

	LLViewerWindow::Params window_params;
	window_params
		.title(gWindowTitle)
		.name(VIEWER_WINDOW_CLASSNAME)
		.x(gSavedSettings.getS32("WindowX"))
		.y(gSavedSettings.getS32("WindowY"))
		.width(gSavedSettings.getU32("WindowWidth"))
		.height(gSavedSettings.getU32("WindowHeight"))
		.min_width(gSavedSettings.getU32("MinWindowWidth"))
		.min_height(gSavedSettings.getU32("MinWindowHeight"))
		.fullscreen(gSavedSettings.getBOOL("FullScreen"))
		.ignore_pixel_depth(ignorePixelDepth);

	gViewerWindow = new LLViewerWindow(window_params);

	LL_INFOS("AppInit") << "gViewerwindow created." << LL_ENDL;

	// Need to load feature table before cheking to start watchdog.
	bool use_watchdog = false;
	int watchdog_enabled_setting = gSavedSettings.getS32("WatchdogEnabled");
	if (watchdog_enabled_setting == -1)
	{
		use_watchdog = !LLFeatureManager::getInstance()->isFeatureAvailable("WatchdogDisabled");
	}
	else
	{
		// The user has explicitly set this setting; always use that value.
		use_watchdog = bool(watchdog_enabled_setting);
	}

	if (use_watchdog)
	{
		LLWatchdog::getInstance()->init(watchdog_killer_callback);
	}
	LL_INFOS("AppInit") << "watchdog setting is done." << LL_ENDL;

	LLNotificationsUI::LLNotificationManager::getInstance();
		
	if (gSavedSettings.getBOOL("WindowMaximized"))
	{
		gViewerWindow->getWindow()->maximize();
	}

	//
	// Initialize GL stuff
	//

	if (mForceGraphicsDetail)
	{
		LLFeatureManager::getInstance()->setGraphicsLevel(gSavedSettings.getU32("RenderQualityPerformance"), false);
	}
			
	// Set this flag in case we crash while initializing GL
	gSavedSettings.setBOOL("RenderInitError", TRUE);
	gSavedSettings.saveToFile( gSavedSettings.getString("ClientSettingsFile"), TRUE );

	gPipeline.init();
	LL_INFOS("AppInit") << "gPipeline Initialized" << LL_ENDL;

	stop_glerror();
	gViewerWindow->initGLDefaults();

	gSavedSettings.setBOOL("RenderInitError", FALSE);
	gSavedSettings.saveToFile( gSavedSettings.getString("ClientSettingsFile"), TRUE );

	//If we have a startup crash, it's usually near GL initialization, so simulate that.
	if(gCrashOnStartup)
	{
		LLAppViewer::instance()->forceErrorLLError();
	}

	//
	// Determine if the window should start maximized on initial run based
	// on graphics capability
	//
	if (gSavedSettings.getBOOL("FirstLoginThisInstall") && meetsRequirementsForMaximizedStart())
	{
		LL_INFOS("AppInit") << "This client met the requirements for a maximized initial screen." << LL_ENDL;
		gSavedSettings.setBOOL("WindowMaximized", TRUE);
	}

	if (gSavedSettings.getBOOL("WindowMaximized"))
	{
		gViewerWindow->getWindow()->maximize();
	}

	LLUI::sWindow = gViewerWindow->getWindow();

	// Show watch cursor
	gViewerWindow->setCursor(UI_CURSOR_WAIT);

	// Finish view initialization
	gViewerWindow->initBase();

	// show viewer window
	//gViewerWindow->getWindow()->show();

	LL_INFOS("AppInit") << "Window initialization done." << LL_ENDL;
	return true;
}

void LLAppViewer::writeDebugInfo()
{
	std::string debug_filename = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"debug_info.log");
	llinfos << "Opening debug file " << debug_filename << llendl;
	llofstream out_file(debug_filename);
	LLSDSerialize::toPrettyXML(gDebugInfo, out_file);
	out_file.close();
}

void LLAppViewer::cleanupSavedSettings()
{
	gSavedSettings.setBOOL("MouseSun", FALSE);

	gSavedSettings.setBOOL("UseEnergy", TRUE);				// force toggle to turn off, since sends message to simulator

	gSavedSettings.setBOOL("DebugWindowProc", gDebugWindowProc);
		
	gSavedSettings.setBOOL("ShowObjectUpdates", gShowObjectUpdates);
	
	if (gDebugView)
	{
		gSavedSettings.setBOOL("ShowDebugConsole", gDebugView->mDebugConsolep->getVisible());
	}

	// save window position if not maximized
	// as we don't track it in callbacks
	if(NULL != gViewerWindow)
	{
		BOOL maximized = gViewerWindow->getWindow()->getMaximized();
		if (!maximized)
		{
			LLCoordScreen window_pos;
			
			if (gViewerWindow->getWindow()->getPosition(&window_pos))
			{
				gSavedSettings.setS32("WindowX", window_pos.mX);
				gSavedSettings.setS32("WindowY", window_pos.mY);
			}
		}
	}

	gSavedSettings.setF32("MapScale", LLWorldMapView::sMapScale );

	// Some things are cached in LLAgent.
	if (gAgent.isInitialized())
	{
		gSavedSettings.setF32("RenderFarClip", gAgentCamera.mDrawDistance);
	}
}

void LLAppViewer::removeCacheFiles(const std::string& file_mask)
{
	std::string mask = gDirUtilp->getDirDelimiter() + file_mask;
	gDirUtilp->deleteFilesInDir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, ""), mask);
}

void LLAppViewer::writeSystemInfo()
{
	gDebugInfo["SLLog"] = LLError::logFileName();

	gDebugInfo["ClientInfo"]["Name"] = LLVersionInfo::getChannel();
	gDebugInfo["ClientInfo"]["MajorVersion"] = LLVersionInfo::getMajor();
	gDebugInfo["ClientInfo"]["MinorVersion"] = LLVersionInfo::getMinor();
	gDebugInfo["ClientInfo"]["PatchVersion"] = LLVersionInfo::getPatch();
	gDebugInfo["ClientInfo"]["BuildVersion"] = LLVersionInfo::getBuild();

	gDebugInfo["CAFilename"] = gDirUtilp->getCAFile();

	gDebugInfo["CPUInfo"]["CPUString"] = gSysCPU.getCPUString();
	gDebugInfo["CPUInfo"]["CPUFamily"] = gSysCPU.getFamily();
	gDebugInfo["CPUInfo"]["CPUMhz"] = (S32)gSysCPU.getMHz();
	gDebugInfo["CPUInfo"]["CPUAltivec"] = gSysCPU.hasAltivec();
	gDebugInfo["CPUInfo"]["CPUSSE"] = gSysCPU.hasSSE();
	gDebugInfo["CPUInfo"]["CPUSSE2"] = gSysCPU.hasSSE2();
	
	gDebugInfo["RAMInfo"]["Physical"] = (LLSD::Integer)(gSysMemory.getPhysicalMemoryKB());
	gDebugInfo["RAMInfo"]["Allocated"] = (LLSD::Integer)(gMemoryAllocated>>10); // MB -> KB
	gDebugInfo["OSInfo"] = getOSInfo().getOSStringSimple();

	// The user is not logged on yet, but record the current grid choice login url
	// which may have been the intended grid. This can b
	gDebugInfo["GridName"] = LLGridManager::getInstance()->getGridLabel();

	// *FIX:Mani - move this down in llappviewerwin32
#ifdef LL_WINDOWS
	DWORD thread_id = GetCurrentThreadId();
	gDebugInfo["MainloopThreadID"] = (S32)thread_id;
#endif

	// "CrashNotHandled" is set here, while things are running well,
	// in case of a freeze. If there is a freeze, the crash logger will be launched
	// and can read this value from the debug_info.log.
	// If the crash is handled by LLAppViewer::handleViewerCrash, ie not a freeze,
	// then the value of "CrashNotHandled" will be set to true.
	gDebugInfo["CrashNotHandled"] = (LLSD::Boolean)true;

	// Insert crash host url (url to post crash log to) if configured. This insures
	// that the crash report will go to the proper location in the case of a 
	// prior freeze.
	std::string crashHostUrl = gSavedSettings.get<std::string>("CrashHostUrl");
	if(crashHostUrl != "")
	{
		gDebugInfo["CrashHostUrl"] = crashHostUrl;
	}
	
	// Dump some debugging info
	LL_INFOS("SystemInfo") << LLTrans::getString("APP_NAME")
			<< " version " << LLVersionInfo::getShortVersion() << LL_ENDL;

	// Dump the local time and time zone
	time_t now;
	time(&now);
	char tbuffer[256];		/* Flawfinder: ignore */
	strftime(tbuffer, 256, "%Y-%m-%dT%H:%M:%S %Z", localtime(&now));
	LL_INFOS("SystemInfo") << "Local time: " << tbuffer << LL_ENDL;

	// query some system information
	LL_INFOS("SystemInfo") << "CPU info:\n" << gSysCPU << LL_ENDL;
	LL_INFOS("SystemInfo") << "Memory info:\n" << gSysMemory << LL_ENDL;
	LL_INFOS("SystemInfo") << "OS: " << getOSInfo().getOSStringSimple() << LL_ENDL;
	LL_INFOS("SystemInfo") << "OS info: " << getOSInfo() << LL_ENDL;

	LL_INFOS("SystemInfo") << "Timers: " << LLFastTimer::sClockType << LL_ENDL;

	writeDebugInfo(); // Save out debug_info.log early, in case of crash.
}

void LLAppViewer::handleViewerCrash()
{
	llinfos << "Handle viewer crash entry." << llendl;

	llinfos << "Last render pool type: " << LLPipeline::sCurRenderPoolType << llendl ;

	LLMemory::logMemoryInfo(true) ;

	//print out recorded call stacks if there are any.
	LLError::LLCallStacks::print();

	LLAppViewer* pApp = LLAppViewer::instance();
	if (pApp->beingDebugged())
	{
		// This will drop us into the debugger.
		abort();
	}

	if (LLApp::isCrashloggerDisabled())
	{
		abort();
	}

	// Returns whether a dialog was shown.
	// Only do the logic in here once
	if (pApp->mReportedCrash)
	{
		return;
	}
	pApp->mReportedCrash = TRUE;
	
	// Insert crash host url (url to post crash log to) if configured.
	std::string crashHostUrl = gSavedSettings.get<std::string>("CrashHostUrl");
	if(crashHostUrl != "")
	{
		gDebugInfo["CrashHostUrl"] = crashHostUrl;
	}
	
	//We already do this in writeSystemInfo(), but we do it again here to make /sure/ we have a version
	//to check against no matter what
	gDebugInfo["ClientInfo"]["Name"] = LLVersionInfo::getChannel();

	gDebugInfo["ClientInfo"]["MajorVersion"] = LLVersionInfo::getMajor();
	gDebugInfo["ClientInfo"]["MinorVersion"] = LLVersionInfo::getMinor();
	gDebugInfo["ClientInfo"]["PatchVersion"] = LLVersionInfo::getPatch();
	gDebugInfo["ClientInfo"]["BuildVersion"] = LLVersionInfo::getBuild();

	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if ( parcel && parcel->getMusicURL()[0])
	{
		gDebugInfo["ParcelMusicURL"] = parcel->getMusicURL();
	}	
	if ( parcel && parcel->getMediaURL()[0])
	{
		gDebugInfo["ParcelMediaURL"] = parcel->getMediaURL();
	}
	
	
	gDebugInfo["SettingsFilename"] = gSavedSettings.getString("ClientSettingsFile");
	gDebugInfo["CAFilename"] = gDirUtilp->getCAFile();
	gDebugInfo["ViewerExePath"] = gDirUtilp->getExecutablePathAndName();
	gDebugInfo["CurrentPath"] = gDirUtilp->getCurPath();
	gDebugInfo["SessionLength"] = F32(LLFrameTimer::getElapsedSeconds());
	gDebugInfo["StartupState"] = LLStartUp::getStartupStateString();
	gDebugInfo["RAMInfo"]["Allocated"] = (LLSD::Integer) LLMemory::getCurrentRSS() >> 10;
	gDebugInfo["FirstLogin"] = (LLSD::Boolean) gAgent.isFirstLogin();
	gDebugInfo["FirstRunThisInstall"] = gSavedSettings.getBOOL("FirstRunThisInstall");

	char *minidump_file = pApp->getMiniDumpFilename();
	if(minidump_file && minidump_file[0] != 0)
	{
		gDebugInfo["MinidumpPath"] = minidump_file;
	}
	
	if(gLogoutInProgress)
	{
		gDebugInfo["LastExecEvent"] = LAST_EXEC_LOGOUT_CRASH;
	}
	else
	{
		gDebugInfo["LastExecEvent"] = gLLErrorActivated ? LAST_EXEC_LLERROR_CRASH : LAST_EXEC_OTHER_CRASH;
	}

	if(gAgent.getRegion())
	{
		gDebugInfo["CurrentSimHost"] = gAgent.getRegionHost().getHostName();
		gDebugInfo["CurrentRegion"] = gAgent.getRegion()->getName();
		
		const LLVector3& loc = gAgent.getPositionAgent();
		gDebugInfo["CurrentLocationX"] = loc.mV[0];
		gDebugInfo["CurrentLocationY"] = loc.mV[1];
		gDebugInfo["CurrentLocationZ"] = loc.mV[2];
	}

	if(LLAppViewer::instance()->mMainloopTimeout)
	{
		gDebugInfo["MainloopTimeoutState"] = LLAppViewer::instance()->mMainloopTimeout->getState();
	}
	
	// The crash is being handled here so set this value to false.
	// Otherwise the crash logger will think this crash was a freeze.
	gDebugInfo["CrashNotHandled"] = (LLSD::Boolean)false;
    
	//Write out the crash status file
	//Use marker file style setup, as that's the simplest, especially since
	//we're already in a crash situation	
	if (gDirUtilp)
	{
		std::string crash_file_name;
		if(gLLErrorActivated) crash_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,LLERROR_MARKER_FILE_NAME);
		else crash_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,ERROR_MARKER_FILE_NAME);
		llinfos << "Creating crash marker file " << crash_file_name << llendl;
		
		LLAPRFile crash_file ;
		crash_file.open(crash_file_name, LL_APR_W);
		if (crash_file.getFileHandle())
		{
			LL_INFOS("MarkerFile") << "Created crash marker file " << crash_file_name << LL_ENDL;
		}
		else
		{
			LL_WARNS("MarkerFile") << "Cannot create error marker file " << crash_file_name << LL_ENDL;
		}		
	}
	
	if (gMessageSystem && gDirUtilp)
	{
		std::string filename;
		filename = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "stats.log");
		llofstream file(filename, llofstream::binary);
		if(file.good())
		{
			llinfos << "Handle viewer crash generating stats log." << llendl;
			gMessageSystem->summarizeLogs(file);
			file.close();
		}
	}

	if (gMessageSystem)
	{
		gMessageSystem->getCircuitInfo(gDebugInfo["CircuitInfo"]);
		gMessageSystem->stopLogging();
	}

	if (LLWorld::instanceExists()) LLWorld::getInstance()->getInfo(gDebugInfo);

	// Close the debug file
	pApp->writeDebugInfo();

	LLError::logToFile("");

	// Remove the marker file, since otherwise we'll spawn a process that'll keep it locked
	if(gDebugInfo["LastExecEvent"].asInteger() == LAST_EXEC_LOGOUT_CRASH)
	{
		pApp->removeMarkerFile(true);
	}
	else
	{
		pApp->removeMarkerFile(false);
	}
	
#if LL_SEND_CRASH_REPORTS
	// Call to pure virtual, handled by platform specific llappviewer instance.
	pApp->handleCrashReporting(); 
#endif
    
	return;
}

bool LLAppViewer::anotherInstanceRunning()
{
	// We create a marker file when the program starts and remove the file when it finishes.
	// If the file is currently locked, that means another process is already running.

	std::string marker_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, MARKER_FILE_NAME);
	LL_DEBUGS("MarkerFile") << "Checking marker file for lock..." << LL_ENDL;

	//Freeze case checks
	if (LLAPRFile::isExist(marker_file, NULL, LL_APR_RB))
	{
		// File exists, try opening with write permissions
		LLAPRFile outfile ;
		outfile.open(marker_file, LL_APR_WB);
		apr_file_t* fMarker = outfile.getFileHandle() ; 
		if (!fMarker)
		{
			// Another instance is running. Skip the rest of these operations.
			LL_INFOS("MarkerFile") << "Marker file is locked." << LL_ENDL;
			return true;
		}
		if (apr_file_lock(fMarker, APR_FLOCK_NONBLOCK | APR_FLOCK_EXCLUSIVE) != APR_SUCCESS) //flock(fileno(fMarker), LOCK_EX | LOCK_NB) == -1)
		{
			LL_INFOS("MarkerFile") << "Marker file is locked." << LL_ENDL;
			return true;
		}
		// No other instances; we'll lock this file now & delete on quit.		
	}
	LL_DEBUGS("MarkerFile") << "Marker file isn't locked." << LL_ENDL;
	return false;
}

void LLAppViewer::initMarkerFile()
{
	//First, check for the existence of other files.
	//There are marker files for two different types of crashes
	
	mMarkerFileName = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,MARKER_FILE_NAME);
	LL_DEBUGS("MarkerFile") << "Checking marker file for lock..." << LL_ENDL;

	//We've got 4 things to test for here
	// - Other Process Running (SecondLife.exec_marker present, locked)
	// - Freeze (SecondLife.exec_marker present, not locked)
	// - LLError Crash (SecondLife.llerror_marker present)
	// - Other Crash (SecondLife.error_marker present)
	// These checks should also remove these files for the last 2 cases if they currently exist

	//LLError/Error checks. Only one of these should ever happen at a time.
	std::string logout_marker_file =  gDirUtilp->getExpandedFilename(LL_PATH_LOGS, LOGOUT_MARKER_FILE_NAME);
	std::string llerror_marker_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, LLERROR_MARKER_FILE_NAME);
	std::string error_marker_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, ERROR_MARKER_FILE_NAME);

	if (LLAPRFile::isExist(mMarkerFileName, NULL, LL_APR_RB) && !anotherInstanceRunning())
	{
		gLastExecEvent = LAST_EXEC_FROZE;
		LL_INFOS("MarkerFile") << "Exec marker found: program froze on previous execution" << LL_ENDL;
	}    
	if(LLAPRFile::isExist(logout_marker_file, NULL, LL_APR_RB))
	{
		gLastExecEvent = LAST_EXEC_LOGOUT_FROZE;
		LL_INFOS("MarkerFile") << "Last exec LLError crashed, setting LastExecEvent to " << gLastExecEvent << LL_ENDL;
		LLAPRFile::remove(logout_marker_file);
	}
	if(LLAPRFile::isExist(llerror_marker_file, NULL, LL_APR_RB))
	{
		if(gLastExecEvent == LAST_EXEC_LOGOUT_FROZE) gLastExecEvent = LAST_EXEC_LOGOUT_CRASH;
		else gLastExecEvent = LAST_EXEC_LLERROR_CRASH;
		LL_INFOS("MarkerFile") << "Last exec LLError crashed, setting LastExecEvent to " << gLastExecEvent << LL_ENDL;
		LLAPRFile::remove(llerror_marker_file);
	}
	if(LLAPRFile::isExist(error_marker_file, NULL, LL_APR_RB))
	{
		if(gLastExecEvent == LAST_EXEC_LOGOUT_FROZE) gLastExecEvent = LAST_EXEC_LOGOUT_CRASH;
		else gLastExecEvent = LAST_EXEC_OTHER_CRASH;
		LL_INFOS("MarkerFile") << "Last exec crashed, setting LastExecEvent to " << gLastExecEvent << LL_ENDL;
		LLAPRFile::remove(error_marker_file);
	}

	// No new markers if another instance is running.
	if(anotherInstanceRunning()) 
	{
		return;
	}
	
	// Create the marker file for this execution & lock it
	apr_status_t s;
	s = mMarkerFile.open(mMarkerFileName, LL_APR_W, TRUE);	

	if (s == APR_SUCCESS && mMarkerFile.getFileHandle())
	{
		LL_DEBUGS("MarkerFile") << "Marker file created." << LL_ENDL;
	}
	else
	{
		LL_INFOS("MarkerFile") << "Failed to create marker file." << LL_ENDL;
		return;
	}
	if (apr_file_lock(mMarkerFile.getFileHandle(), APR_FLOCK_NONBLOCK | APR_FLOCK_EXCLUSIVE) != APR_SUCCESS) 
	{
		mMarkerFile.close() ;
		LL_INFOS("MarkerFile") << "Marker file cannot be locked." << LL_ENDL;
		return;
	}

	LL_DEBUGS("MarkerFile") << "Marker file locked." << LL_ENDL;
}

void LLAppViewer::removeMarkerFile(bool leave_logout_marker)
{
	LL_DEBUGS("MarkerFile") << "removeMarkerFile()" << LL_ENDL;
	if (mMarkerFile.getFileHandle())
	{
		mMarkerFile.close() ;
		LLAPRFile::remove( mMarkerFileName );
	}
	if (mLogoutMarkerFile != NULL && !leave_logout_marker)
	{
		LLAPRFile::remove( mLogoutMarkerFileName );
		mLogoutMarkerFile = NULL;
	}
}

void LLAppViewer::forceQuit()
{ 
	LLApp::setQuitting(); 
}

//TODO: remove
void LLAppViewer::fastQuit(S32 error_code)
{
	// finish pending transfers
	flushVFSIO();
	// let sim know we're logging out
	sendLogoutRequest();
	// flush network buffers by shutting down messaging system
	end_messaging_system();
	// figure out the error code
	S32 final_error_code = error_code ? error_code : (S32)isError();
	// this isn't a crash	
	removeMarkerFile();
	// get outta here
	_exit(final_error_code);	
}

void LLAppViewer::requestQuit()
{
	llinfos << "requestQuit" << llendl;

	LLViewerRegion* region = gAgent.getRegion();
	
	if( (LLStartUp::getStartupState() < STATE_STARTED) || !region )
	{
		// If we have a region, make some attempt to send a logout request first.
		// This prevents the halfway-logged-in avatar from hanging around inworld for a couple minutes.
		if(region)
		{
			sendLogoutRequest();
		}
		
		// Quit immediately
		forceQuit();
		return;
	}

	// Try to send metrics back to the grid
	metricsSend(!gDisconnected);
	
	LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral*)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINT, TRUE);
	effectp->setPositionGlobal(gAgent.getPositionGlobal());
	effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	LLHUDManager::getInstance()->sendEffects();
	effectp->markDead() ;//remove it.

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

static bool finish_quit(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	if (option == 0)
	{
		LLAppViewer::instance()->requestQuit();
	}
	return false;
}
static LLNotificationFunctorRegistration finish_quit_reg("ConfirmQuit", finish_quit);

void LLAppViewer::userQuit()
{
	if (gDisconnected || gViewerWindow->getProgressView()->getVisible())
	{
		requestQuit();
	}
	else
	{
		LLNotificationsUtil::add("ConfirmQuit");
	}
}

static bool finish_early_exit(const LLSD& notification, const LLSD& response)
{
	LLAppViewer::instance()->forceQuit();
	return false;
}

void LLAppViewer::earlyExit(const std::string& name, const LLSD& substitutions)
{
   	llwarns << "app_early_exit: " << name << llendl;
	gDoDisconnect = TRUE;
	LLNotificationsUtil::add(name, substitutions, LLSD(), finish_early_exit);
}

// case where we need the viewer to exit without any need for notifications
void LLAppViewer::earlyExitNoNotify()
{
   	llwarns << "app_early_exit with no notification: " << llendl;
	gDoDisconnect = TRUE;
	finish_early_exit( LLSD(), LLSD() );
}

void LLAppViewer::abortQuit()
{
    llinfos << "abortQuit()" << llendl;
	mQuitRequested = false;
}

void LLAppViewer::migrateCacheDirectory()
{
#if LL_WINDOWS || LL_DARWIN
	// NOTE: (Nyx) as of 1.21, cache for mac is moving to /library/caches/SecondLife from
	// /library/application support/SecondLife/cache This should clear/delete the old dir.

	// As of 1.23 the Windows cache moved from
	//   C:\Documents and Settings\James\Application Support\SecondLife\cache
	// to
	//   C:\Documents and Settings\James\Local Settings\Application Support\SecondLife
	//
	// The Windows Vista equivalent is from
	//   C:\Users\James\AppData\Roaming\SecondLife\cache
	// to
	//   C:\Users\James\AppData\Local\SecondLife
	//
	// Note the absence of \cache on the second path.  James.

	// Only do this once per fresh install of this version.
	if (gSavedSettings.getBOOL("MigrateCacheDirectory"))
	{
		gSavedSettings.setBOOL("MigrateCacheDirectory", FALSE);

		std::string delimiter = gDirUtilp->getDirDelimiter();
		std::string old_cache_dir = gDirUtilp->getOSUserAppDir() + delimiter + "cache";
		std::string new_cache_dir = gDirUtilp->getCacheDir(true);

		if (gDirUtilp->fileExists(old_cache_dir))
		{
			llinfos << "Migrating cache from " << old_cache_dir << " to " << new_cache_dir << llendl;

			// Migrate inventory cache to avoid pain to inventory database after mass update
			S32 file_count = 0;
			std::string file_name;
			std::string mask = "*.*";

			LLDirIterator iter(old_cache_dir, mask);
			while (iter.next(file_name))
			{
				if (file_name == "." || file_name == "..") continue;
				std::string source_path = old_cache_dir + delimiter + file_name;
				std::string dest_path = new_cache_dir + delimiter + file_name;
				if (!LLFile::rename(source_path, dest_path))
				{
					file_count++;
				}
			}
			llinfos << "Moved " << file_count << " files" << llendl;

			// Nuke the old cache
			gDirUtilp->setCacheDir(old_cache_dir);
			purgeCache();
			gDirUtilp->setCacheDir(new_cache_dir);

#if LL_DARWIN
			// Clean up Mac files not deleted by removing *.*
			std::string ds_store = old_cache_dir + "/.DS_Store";
			if (gDirUtilp->fileExists(ds_store))
			{
				LLFile::remove(ds_store);
			}
#endif
			if (LLFile::rmdir(old_cache_dir) != 0)
			{
				llwarns << "could not delete old cache directory " << old_cache_dir << llendl;
			}
		}
	}
#endif // LL_WINDOWS || LL_DARWIN
}

void dumpVFSCaches()
{
	llinfos << "======= Static VFS ========" << llendl;
	gStaticVFS->listFiles();
#if LL_WINDOWS
	llinfos << "======= Dumping static VFS to StaticVFSDump ========" << llendl;
	WCHAR w_str[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, w_str);
	S32 res = LLFile::mkdir("StaticVFSDump");
	if (res == -1)
	{
		if (errno != EEXIST)
		{
			llwarns << "Couldn't create dir StaticVFSDump" << llendl;
		}
	}
	SetCurrentDirectory(utf8str_to_utf16str("StaticVFSDump").c_str());
	gStaticVFS->dumpFiles();
	SetCurrentDirectory(w_str);
#endif
						
	llinfos << "========= Dynamic VFS ====" << llendl;
	gVFS->listFiles();
#if LL_WINDOWS
	llinfos << "========= Dumping dynamic VFS to VFSDump ====" << llendl;
	res = LLFile::mkdir("VFSDump");
	if (res == -1)
	{
		if (errno != EEXIST)
		{
			llwarns << "Couldn't create dir VFSDump" << llendl;
		}
	}
	SetCurrentDirectory(utf8str_to_utf16str("VFSDump").c_str());
	gVFS->dumpFiles();
	SetCurrentDirectory(w_str);
#endif
}

//static
U32 LLAppViewer::getTextureCacheVersion() 
{
	//viewer texture cache version, change if the texture cache format changes.
	const U32 TEXTURE_CACHE_VERSION = 7;

	return TEXTURE_CACHE_VERSION ;
}

//static
U32 LLAppViewer::getObjectCacheVersion() 
{
	// Viewer object cache version, change if object update
	// format changes. JC
	const U32 INDRA_OBJECT_CACHE_VERSION = 14;

	return INDRA_OBJECT_CACHE_VERSION;
}

bool LLAppViewer::initCache()
{
	mPurgeCache = false;
	BOOL read_only = mSecondInstance ? TRUE : FALSE;
	LLAppViewer::getTextureCache()->setReadOnly(read_only) ;
	LLVOCache::getInstance()->setReadOnly(read_only);

	bool texture_cache_mismatch = false;
	if (gSavedSettings.getS32("LocalCacheVersion") != LLAppViewer::getTextureCacheVersion()) 
	{
		texture_cache_mismatch = true;
		if(!read_only) 
		{
			gSavedSettings.setS32("LocalCacheVersion", LLAppViewer::getTextureCacheVersion());
		}
	}

	if(!read_only)
	{
		// Purge cache if user requested it
		if (gSavedSettings.getBOOL("PurgeCacheOnStartup") ||
			gSavedSettings.getBOOL("PurgeCacheOnNextStartup"))
		{
			gSavedSettings.setBOOL("PurgeCacheOnNextStartup", false);
			mPurgeCache = true;
			// STORM-1141 force purgeAllTextures to get called to prevent a crash here. -brad
			texture_cache_mismatch = true;
		}
	
		// We have moved the location of the cache directory over time.
		migrateCacheDirectory();
	
		// Setup and verify the cache location
		std::string cache_location = gSavedSettings.getString("CacheLocation");
		std::string new_cache_location = gSavedSettings.getString("NewCacheLocation");
		if (new_cache_location != cache_location)
		{
			gDirUtilp->setCacheDir(gSavedSettings.getString("CacheLocation"));
			purgeCache(); // purge old cache
			gSavedSettings.setString("CacheLocation", new_cache_location);
			gSavedSettings.setString("CacheLocationTopFolder", gDirUtilp->getBaseFileName(new_cache_location));
		}
	}

	if (!gDirUtilp->setCacheDir(gSavedSettings.getString("CacheLocation")))
	{
		LL_WARNS("AppCache") << "Unable to set cache location" << LL_ENDL;
		gSavedSettings.setString("CacheLocation", "");
		gSavedSettings.setString("CacheLocationTopFolder", "");
	}
	
	if (mPurgeCache && !read_only)
	{
		LLSplashScreen::update(LLTrans::getString("StartupClearingCache"));
		purgeCache();
	}

	LLSplashScreen::update(LLTrans::getString("StartupInitializingTextureCache"));
	
	// Init the texture cache
	// Allocate 80% of the cache size for textures	
	const S32 MB = 1024 * 1024;
	const S64 MIN_CACHE_SIZE = 64 * MB;
	const S64 MAX_CACHE_SIZE = 9984ll * MB;
	const S64 MAX_VFS_SIZE = 1024 * MB; // 1 GB

	S64 cache_size = (S64)(gSavedSettings.getU32("CacheSize")) * MB;
	cache_size = llclamp(cache_size, MIN_CACHE_SIZE, MAX_CACHE_SIZE);

	S64 texture_cache_size = ((cache_size * 8) / 10);
	S64 vfs_size = cache_size - texture_cache_size;

	if (vfs_size > MAX_VFS_SIZE)
	{
		// Give the texture cache more space, since the VFS can't be bigger than 1GB.
		// This happens when the user's CacheSize setting is greater than 5GB.
		vfs_size = MAX_VFS_SIZE;
		texture_cache_size = cache_size - MAX_VFS_SIZE;
	}

	S64 extra = LLAppViewer::getTextureCache()->initCache(LL_PATH_CACHE, texture_cache_size, texture_cache_mismatch);
	texture_cache_size -= extra;

	LLVOCache::getInstance()->initCache(LL_PATH_CACHE, gSavedSettings.getU32("CacheNumberOfRegionsForObjects"), getObjectCacheVersion()) ;

	LLSplashScreen::update(LLTrans::getString("StartupInitializingVFS"));
	
	// Init the VFS
	vfs_size = llmin(vfs_size + extra, MAX_VFS_SIZE);
	vfs_size = (vfs_size / MB) * MB; // make sure it is MB aligned
	U32 vfs_size_u32 = (U32)vfs_size;
	U32 old_vfs_size = gSavedSettings.getU32("VFSOldSize") * MB;
	bool resize_vfs = (vfs_size_u32 != old_vfs_size);
	if (resize_vfs)
	{
		gSavedSettings.setU32("VFSOldSize", vfs_size_u32 / MB);
	}
	LL_INFOS("AppCache") << "VFS CACHE SIZE: " << vfs_size / (1024*1024) << " MB" << LL_ENDL;
	
	// This has to happen BEFORE starting the vfs
	// time_t	ltime;
	srand(time(NULL));		// Flawfinder: ignore
	U32 old_salt = gSavedSettings.getU32("VFSSalt");
	U32 new_salt;
	std::string old_vfs_data_file;
	std::string old_vfs_index_file;
	std::string new_vfs_data_file;
	std::string new_vfs_index_file;
	std::string static_vfs_index_file;
	std::string static_vfs_data_file;

	if (gSavedSettings.getBOOL("AllowMultipleViewers"))
	{
		// don't mess with renaming the VFS in this case
		new_salt = old_salt;
	}
	else
	{
		do
		{
			new_salt = rand();
		} while(new_salt == old_salt);
	}

	old_vfs_data_file = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, VFS_DATA_FILE_BASE) + llformat("%u", old_salt);

	// make sure this file exists
	llstat s;
	S32 stat_result = LLFile::stat(old_vfs_data_file, &s);
	if (stat_result)
	{
		// doesn't exist, look for a data file
		std::string mask;
		mask = VFS_DATA_FILE_BASE;
		mask += "*";

		std::string dir;
		dir = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "");

		std::string found_file;
		LLDirIterator iter(dir, mask);
		if (iter.next(found_file))
		{
			old_vfs_data_file = dir + gDirUtilp->getDirDelimiter() + found_file;

			S32 start_pos = found_file.find_last_of('.');
			if (start_pos > 0)
			{
				sscanf(found_file.substr(start_pos+1).c_str(), "%d", &old_salt);
			}
			LL_DEBUGS("AppCache") << "Default vfs data file not present, found: " << old_vfs_data_file << " Old salt: " << old_salt << llendl;
		}
	}

	old_vfs_index_file = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, VFS_INDEX_FILE_BASE) + llformat("%u", old_salt);

	stat_result = LLFile::stat(old_vfs_index_file, &s);
	if (stat_result)
	{
		// We've got a bad/missing index file, nukem!
		LL_WARNS("AppCache") << "Bad or missing vfx index file " << old_vfs_index_file << LL_ENDL;
		LL_WARNS("AppCache") << "Removing old vfs data file " << old_vfs_data_file << LL_ENDL;
		LLFile::remove(old_vfs_data_file);
		LLFile::remove(old_vfs_index_file);
		
		// Just in case, nuke any other old cache files in the directory.
		std::string dir;
		dir = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "");

		std::string mask;
		mask = VFS_DATA_FILE_BASE;
		mask += "*";

		gDirUtilp->deleteFilesInDir(dir, mask);

		mask = VFS_INDEX_FILE_BASE;
		mask += "*";

		gDirUtilp->deleteFilesInDir(dir, mask);
	}

	new_vfs_data_file = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, VFS_DATA_FILE_BASE) + llformat("%u", new_salt);
	new_vfs_index_file = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, VFS_INDEX_FILE_BASE) + llformat("%u", new_salt);

	static_vfs_data_file = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "static_data.db2");
	static_vfs_index_file = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "static_index.db2");

	if (resize_vfs)
	{
		LL_DEBUGS("AppCache") << "Removing old vfs and re-sizing" << LL_ENDL;
		
		LLFile::remove(old_vfs_data_file);
		LLFile::remove(old_vfs_index_file);
	}
	else if (old_salt != new_salt)
	{
		// move the vfs files to a new name before opening
		LL_DEBUGS("AppCache") << "Renaming " << old_vfs_data_file << " to " << new_vfs_data_file << LL_ENDL;
		LL_DEBUGS("AppCache") << "Renaming " << old_vfs_index_file << " to " << new_vfs_index_file << LL_ENDL;
		LLFile::rename(old_vfs_data_file, new_vfs_data_file);
		LLFile::rename(old_vfs_index_file, new_vfs_index_file);
	}

	// Startup the VFS...
	gSavedSettings.setU32("VFSSalt", new_salt);

	// Don't remove VFS after viewer crashes.  If user has corrupt data, they can reinstall. JC
	gVFS = LLVFS::createLLVFS(new_vfs_index_file, new_vfs_data_file, false, vfs_size_u32, false);
	if (!gVFS)
	{
		return false;
	}

	gStaticVFS = LLVFS::createLLVFS(static_vfs_index_file, static_vfs_data_file, true, 0, false);
	if (!gStaticVFS)
	{
		return false;
	}

	BOOL success = gVFS->isValid() && gStaticVFS->isValid();
	if (!success)
	{
		return false;
	}
	else
	{
		LLVFile::initClass();

#ifndef LL_RELEASE_FOR_DOWNLOAD
		if (gSavedSettings.getBOOL("DumpVFSCaches"))
		{
			dumpVFSCaches();
		}
#endif
		
		return true;
	}
}

void LLAppViewer::addOnIdleCallback(const boost::function<void()>& cb)
{
	LLDeferredTaskList::instance().addTask(cb);
}

void LLAppViewer::purgeCache()
{
	LL_INFOS("AppCache") << "Purging Cache and Texture Cache..." << LL_ENDL;
	LLAppViewer::getTextureCache()->purgeCache(LL_PATH_CACHE);
	LLVOCache::getInstance()->removeCache(LL_PATH_CACHE);
	std::string mask = "*.*";
	gDirUtilp->deleteFilesInDir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, ""), mask);
}

std::string LLAppViewer::getSecondLifeTitle() const
{
	return LLTrans::getString("APP_NAME");
}

std::string LLAppViewer::getWindowTitle() const 
{
	return gWindowTitle;
}

// Callback from a dialog indicating user was logged out.  
bool finish_disconnect(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	if (1 == option)
	{
        LLAppViewer::instance()->forceQuit();
	}
	return false;
}

// Callback from an early disconnect dialog, force an exit
bool finish_forced_disconnect(const LLSD& notification, const LLSD& response)
{
	LLAppViewer::instance()->forceQuit();
	return false;
}


void LLAppViewer::forceDisconnect(const std::string& mesg)
{
	if (gDoDisconnect)
    {
		// Already popped up one of these dialogs, don't
		// do this again.
		return;
    }
	
	// *TODO: Translate the message if possible
	std::string big_reason = LLAgent::sTeleportErrorMessages[mesg];
	if ( big_reason.size() == 0 )
	{
		big_reason = mesg;
	}

	LLSD args;
	gDoDisconnect = TRUE;

	if (LLStartUp::getStartupState() < STATE_STARTED)
	{
		// Tell users what happened
		args["ERROR_MESSAGE"] = big_reason;
		LLNotificationsUtil::add("ErrorMessage", args, LLSD(), &finish_forced_disconnect);
	}
	else
	{
		args["MESSAGE"] = big_reason;
		LLNotificationsUtil::add("YouHaveBeenLoggedOut", args, LLSD(), &finish_disconnect );
	}
}

void LLAppViewer::badNetworkHandler()
{
	// Dump the packet
	gMessageSystem->dumpPacketToLog();

	// Flush all of our caches on exit in the case of disconnect due to
	// invalid packets.

	mPurgeOnExit = TRUE;

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
	
	LLApp::instance()->writeMiniDump();
}

// This routine may get called more than once during the shutdown process.
// This can happen because we need to get the screenshot before the window
// is destroyed.
void LLAppViewer::saveFinalSnapshot()
{
	if (!mSavedFinalSnapshot)
	{
		gSavedSettings.setVector3d("FocusPosOnLogout", gAgentCamera.calcFocusPositionTargetGlobal());
		gSavedSettings.setVector3d("CameraPosOnLogout", gAgentCamera.calcCameraPositionTargetGlobal());
		gViewerWindow->setCursor(UI_CURSOR_WAIT);
		gAgentCamera.changeCameraToThirdPerson( FALSE );	// don't animate, need immediate switch
		gSavedSettings.setBOOL("ShowParcelOwners", FALSE);
		idle();

		std::string snap_filename = gDirUtilp->getLindenUserDir();
		snap_filename += gDirUtilp->getDirDelimiter();
		snap_filename += SCREEN_LAST_FILENAME;
		// use full pixel dimensions of viewer window (not post-scale dimensions)
		gViewerWindow->saveSnapshot(snap_filename, gViewerWindow->getWindowWidthRaw(), gViewerWindow->getWindowHeightRaw(), FALSE, TRUE);
		mSavedFinalSnapshot = TRUE;
	}
}

void LLAppViewer::loadNameCache()
{
	// display names cache
	std::string filename =
		gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "avatar_name_cache.xml");
	LL_INFOS("AvNameCache") << filename << LL_ENDL;
	llifstream name_cache_stream(filename);
	if(name_cache_stream.is_open())
	{
		LLAvatarNameCache::importFile(name_cache_stream);
	}

	if (!gCacheName) return;

	std::string name_cache;
	name_cache = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "name.cache");
	llifstream cache_file(name_cache);
	if(cache_file.is_open())
	{
		if(gCacheName->importFile(cache_file)) return;
	}
}

void LLAppViewer::saveNameCache()
	{
	// display names cache
	std::string filename =
		gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "avatar_name_cache.xml");
	llofstream name_cache_stream(filename);
	if(name_cache_stream.is_open())
	{
		LLAvatarNameCache::exportFile(name_cache_stream);
}

	if (!gCacheName) return;

	std::string name_cache;
	name_cache = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "name.cache");
	llofstream cache_file(name_cache);
	if(cache_file.is_open())
	{
		gCacheName->exportFile(cache_file);
	}
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

static LLFastTimer::DeclareTimer FTM_AUDIO_UPDATE("Update Audio");
static LLFastTimer::DeclareTimer FTM_CLEANUP("Cleanup");
static LLFastTimer::DeclareTimer FTM_CLEANUP_DRAWABLES("Drawables");
static LLFastTimer::DeclareTimer FTM_CLEANUP_OBJECTS("Objects");
static LLFastTimer::DeclareTimer FTM_IDLE_CB("Idle Callbacks");
static LLFastTimer::DeclareTimer FTM_LOD_UPDATE("Update LOD");
static LLFastTimer::DeclareTimer FTM_OBJECTLIST_UPDATE("Update Objectlist");
static LLFastTimer::DeclareTimer FTM_REGION_UPDATE("Update Region");
static LLFastTimer::DeclareTimer FTM_WORLD_UPDATE("Update World");
static LLFastTimer::DeclareTimer FTM_NETWORK("Network");
static LLFastTimer::DeclareTimer FTM_AGENT_NETWORK("Agent Network");
static LLFastTimer::DeclareTimer FTM_VLMANAGER("VL Manager");

///////////////////////////////////////////////////////
// idle()
//
// Called every time the window is not doing anything.
// Receive packets, update statistics, and schedule a redisplay.
///////////////////////////////////////////////////////
void LLAppViewer::idle()
{
	LLMemType mt_idle(LLMemType::MTYPE_IDLE);
	pingMainloopTimeout("Main:Idle");
	
	// Update frame timers
	static LLTimer idle_timer;

	LLFrameTimer::updateFrameTime();
	LLFrameTimer::updateFrameCount();
	LLEventTimer::updateClass();
	LLNotificationsUI::LLToast::updateClass();
	LLCriticalDamp::updateInterpolants();
	LLMortician::updateClass();
	LLFilePickerThread::clearDead();  //calls LLFilePickerThread::notify()

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

	F32 qas = gSavedSettings.getF32("QuitAfterSeconds");
	if (qas > 0.f)
	{
		if (gRenderStartTime.getElapsedTimeF32() > qas)
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
		gGLActive = TRUE;
		if (!idle_startup())
		{
			gGLActive = FALSE;
			return;
		}
		gGLActive = FALSE;
	}

	
    F32 yaw = 0.f;				// radians

	if (!gDisconnected)
	{
		LLFastTimer t(FTM_NETWORK);
		// Update spaceserver timeinfo
	    LLWorld::getInstance()->setSpaceTimeUSec(LLWorld::getInstance()->getSpaceTimeUSec() + (U32)(dt_raw * SEC_TO_MICROSEC));
    
    
	    //////////////////////////////////////
	    //
	    // Update simulator agent state
	    //

		if (gSavedSettings.getBOOL("RotateRight"))
		{
			gAgent.moveYaw(-1.f);
		}

		{
			LLFastTimer t(FTM_AGENT_AUTOPILOT);
			// Handle automatic walking towards points
			gAgentPilot.updateTarget();
			gAgent.autoPilot(&yaw);
		}
    
	    static LLFrameTimer agent_update_timer;
	    static U32 				last_control_flags;
    
	    //	When appropriate, update agent location to the simulator.
	    F32 agent_update_time = agent_update_timer.getElapsedTimeF32();
	    BOOL flags_changed = gAgent.controlFlagsDirty() || (last_control_flags != gAgent.getControlFlags());
		    
	    if (flags_changed || (agent_update_time > (1.0f / (F32) AGENT_UPDATES_PER_SECOND)))
	    {
		    LLFastTimer t(FTM_AGENT_UPDATE);
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

			// ViewerMetrics FPS piggy-backing on the debug timer.
			// The 5-second interval is nice for this purpose.  If the object debug
			// bit moves or is disabled, please give this a suitable home.
			LLViewerAssetStatsFF::record_fps_main(gFPSClamped);
			LLViewerAssetStatsFF::record_avatar_stats();
		}
	}

	if (!gDisconnected)
	{
		LLFastTimer t(FTM_NETWORK);
	
	    ////////////////////////////////////////////////
	    //
	    // Network processing
	    //
	    // NOTE: Starting at this point, we may still have pointers to "dead" objects
	    // floating throughout the various object lists.
	    //
		idleNameCache();
    
		idleNetwork();
	    	        

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
// 		LLFastTimer t(FTM_IDLE_CB);

		// Do event notifications if necessary.  Yes, we may want to move this elsewhere.
		gEventNotifier.update();
		
		gIdleCallbacks.callFunctions();
		gInventory.idleNotifyObservers();
	}
	
	// Metrics logging (LLViewerAssetStats, etc.)
	{
		static LLTimer report_interval;

		// *TODO:  Add configuration controls for this
		F32 seconds = report_interval.getElapsedTimeF32();
		if (seconds >= app_metrics_interval)
		{
			metricsSend(! gDisconnected);
			report_interval.reset();
		}
	}

	if (gDisconnected)
    {
		return;
    }

	gViewerWindow->updateUI();

	///////////////////////////////////////
	// Agent and camera movement
	//
	LLCoordGL current_mouse = gViewerWindow->getCurrentMouse();

	{
		// After agent and camera moved, figure out if we need to
		// deselect objects.
		LLSelectMgr::getInstance()->deselectAllIfTooFar();

	}

	{
		// Handle pending gesture processing
		static LLFastTimer::DeclareTimer ftm("Agent Position");
		LLFastTimer t(ftm);
		LLGestureMgr::instance().update();

		gAgent.updateAgentPosition(gFrameDTClamped, yaw, current_mouse.mX, current_mouse.mY);
	}

	{
		LLFastTimer t(FTM_OBJECTLIST_UPDATE); 
		
        if (!(logoutRequestSent() && hasSavedFinalSnapshot()))
		{
			gObjectList.update(gAgent, *LLWorld::getInstance());
		}
	}
	
	//////////////////////////////////////
	//
	// Deletes objects...
	// Has to be done after doing idleUpdates (which can kill objects)
	//

	{
		LLFastTimer t(FTM_CLEANUP);
		{
			LLFastTimer t(FTM_CLEANUP_OBJECTS);
			gObjectList.cleanDeadObjects();
		}
		{
			LLFastTimer t(FTM_CLEANUP_DRAWABLES);
			LLDrawable::cleanupDeadDrawables();
		}
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
		static LLFastTimer::DeclareTimer ftm("HUD Effects");
		LLFastTimer t(ftm);
		LLSelectMgr::getInstance()->updateEffects();
		LLHUDManager::getInstance()->cleanupEffects();
		LLHUDManager::getInstance()->sendEffects();
	}

	////////////////////////////////////////
	//
	// Unpack layer data that we've received
	//

	{
		LLFastTimer t(FTM_NETWORK);
		gVLManager.unpackData();
	}
	
	/////////////////////////
	//
	// Update surfaces, and surface textures as well.
	//

	LLWorld::getInstance()->updateVisibilities();
	{
		const F32 max_region_update_time = .001f; // 1ms
		LLFastTimer t(FTM_REGION_UPDATE);
		LLWorld::getInstance()->updateRegions(max_region_update_time);
	}
	
	/////////////////////////
	//
	// Update weather effects
	//
	gSky.propagateHeavenlyBodies(gFrameDTClamped);				// moves sun, moon, and planets

	// Update wind vector 
	LLVector3 wind_position_region;
	static LLVector3 average_wind;

	LLViewerRegion *regionp;
	regionp = LLWorld::getInstance()->resolveRegionGlobal(wind_position_region, gAgent.getPositionGlobal());	// puts agent's local coords into wind_position	
	if (regionp)
	{
		gWindVec = regionp->mWind.getVelocity(wind_position_region);

		// Compute average wind and use to drive motion of water
		
		average_wind = regionp->mWind.getAverage();
		gSky.setWind(average_wind);
		//LLVOWater::setWind(average_wind);
	}
	else
	{
		gWindVec.setVec(0.0f, 0.0f, 0.0f);
	}
	
	//////////////////////////////////////
	//
	// Sort and cull in the new renderer are moved to pipeline.cpp
	// Here, particles are updated and drawables are moved.
	//
	
	LLFastTimer t(FTM_WORLD_UPDATE);
	gPipeline.updateMove();

	LLWorld::getInstance()->updateParticles();

	if (gAgentPilot.isPlaying() && gAgentPilot.getOverrideCamera())
	{
		gAgentPilot.moveCamera();
	}
	else if (LLViewerJoystick::getInstance()->getOverrideCamera())
	{ 
		LLViewerJoystick::getInstance()->moveFlycam();
	}
	else
	{
		if (LLToolMgr::getInstance()->inBuildMode())
		{
			LLViewerJoystick::getInstance()->moveObjects();
		}

		gAgentCamera.updateCamera();
	}

	// update media focus
	LLViewerMediaFocus::getInstance()->update();
	
	// Update marketplace
	LLMarketplaceInventoryImporter::update();
	LLMarketplaceInventoryNotifications::update();

	// objects and camera should be in sync, do LOD calculations now
	{
		LLFastTimer t(FTM_LOD_UPDATE);
		gObjectList.updateApparentAngles(gAgent);
	}

	{
		LLFastTimer t(FTM_AUDIO_UPDATE);
		
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

	// Execute deferred tasks.
	LLDeferredTaskList::instance().run();
	
	// Handle shutdown process, for example, 
	// wait for floaters to close, send quit message,
	// forcibly quit if it has taken too long
	if (mQuitRequested)
	{
		gGLActive = TRUE;
		idleShutdown();
	}
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



	
	// ProductEngine: Try moving this code to where we shut down sTextureCache in cleanup()
	// *TODO: ugly
	static bool saved_teleport_history = false;
	if (!saved_teleport_history)
	{
		saved_teleport_history = true;
		LLTeleportHistory::getInstance()->dump();
		LLLocationHistory::getInstance()->save(); // *TODO: find a better place for doing this
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
		gViewerWindow->setProgressString(LLTrans::getString("SavingSettings"));
		return;
	}

	// All floaters are closed.  Tell server we want to quit.
	if( !logoutRequestSent() )
	{
		sendLogoutRequest();

		// Wait for a LogoutReply message
		gViewerWindow->setShowProgress(TRUE);
		gViewerWindow->setProgressPercent(100.f);
		gViewerWindow->setProgressString(LLTrans::getString("LoggingOut"));
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
	if(!mLogoutRequestSent && gMessageSystem)
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
		
		if(LLVoiceClient::instanceExists())
		{
			LLVoiceClient::getInstance()->leaveChannel();
		}

		//Set internal status variables and marker files
		gLogoutInProgress = TRUE;
		mLogoutMarkerFileName = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,LOGOUT_MARKER_FILE_NAME);
		
		LLAPRFile outfile ;
		outfile.open(mLogoutMarkerFileName, LL_APR_W);
		mLogoutMarkerFile =  outfile.getFileHandle() ;
		if (mLogoutMarkerFile)
		{
			llinfos << "Created logout marker file " << mLogoutMarkerFileName << llendl;
    		apr_file_close(mLogoutMarkerFile);
		}
		else
		{
			llwarns << "Cannot create logout marker file " << mLogoutMarkerFileName << llendl;
		}		
	}
}

void LLAppViewer::idleNameCache()
{
	// Neither old nor new name cache can function before agent has a region
	LLViewerRegion* region = gAgent.getRegion();
	if (!region) return;

	// deal with any queued name requests and replies.
	gCacheName->processPending();

	// Can't run the new cache until we have the list of capabilities
	// for the agent region, and can therefore decide whether to use
	// display names or fall back to the old name system.
	if (!region->capabilitiesReceived()) return;

	// Agent may have moved to a different region, so need to update cap URL
	// for name lookups.  Can't do this in the cap grant code, as caps are
	// granted to neighbor regions before the main agent gets there.  Can't
	// do it in the move-into-region code because cap not guaranteed to be
	// granted yet, for example on teleport.
	bool had_capability = LLAvatarNameCache::hasNameLookupURL();
	std::string name_lookup_url;
	name_lookup_url.reserve(128); // avoid a memory allocation below
	name_lookup_url = region->getCapability("GetDisplayNames");
	bool have_capability = !name_lookup_url.empty();
	if (have_capability)
	{
		// we have support for display names, use it
	    U32 url_size = name_lookup_url.size();
	    // capabilities require URLs with slashes before query params:
	    // https://<host>:<port>/cap/<uuid>/?ids=<blah>
	    // but the caps are granted like:
	    // https://<host>:<port>/cap/<uuid>
	    if (url_size > 0 && name_lookup_url[url_size-1] != '/')
	    {
		    name_lookup_url += '/';
	    }
		LLAvatarNameCache::setNameLookupURL(name_lookup_url);
	}
	else
	{
		// Display names not available on this region
		LLAvatarNameCache::setNameLookupURL( std::string() );
	}

	// Error recovery - did we change state?
	if (had_capability != have_capability)
	{
		// name tags are persistant on screen, so make sure they refresh
		LLVOAvatar::invalidateNameTags();
	}

	LLAvatarNameCache::idle();
}

//
// Handle messages, and all message related stuff
//

#define TIME_THROTTLE_MESSAGES

#ifdef TIME_THROTTLE_MESSAGES
#define CHECK_MESSAGES_DEFAULT_MAX_TIME .020f // 50 ms = 50 fps (just for messages!)
static F32 CheckMessagesMaxTime = CHECK_MESSAGES_DEFAULT_MAX_TIME;
#endif

static LLFastTimer::DeclareTimer FTM_IDLE_NETWORK("Idle Network");
static LLFastTimer::DeclareTimer FTM_MESSAGE_ACKS("Message Acks");
static LLFastTimer::DeclareTimer FTM_RETRANSMIT("Retransmit");
static LLFastTimer::DeclareTimer FTM_TIMEOUT_CHECK("Timeout Check");
static LLFastTimer::DeclareTimer FTM_DYNAMIC_THROTTLE("Dynamic Throttle");
static LLFastTimer::DeclareTimer FTM_CHECK_REGION_CIRCUIT("Check Region Circuit");

void LLAppViewer::idleNetwork()
{
	LLMemType mt_in(LLMemType::MTYPE_IDLE_NETWORK);
	pingMainloopTimeout("idleNetwork");
	
	gObjectList.mNumNewObjects = 0;
	S32 total_decoded = 0;

	if (!gSavedSettings.getBOOL("SpeedTest"))
	{
		LLFastTimer t(FTM_IDLE_NETWORK); // decode
		
		LLTimer check_message_timer;
		//  Read all available packets from network 
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
	LLViewerStats::getInstance()->mNumNewObjectsStat.addValue(gObjectList.mNumNewObjects);

	// Retransmit unacknowledged packets.
	gXferManager->retransmitUnackedPackets();
	gAssetStorage->checkForTimeouts();
	gViewerThrottle.updateDynamicThrottle();

	// Check that the circuit between the viewer and the agent's current
	// region is still alive
	LLViewerRegion *agent_region = gAgent.getRegion();
	if (agent_region && (LLStartUp::getStartupState()==STATE_STARTED))
	{
		LLUUID this_region_id = agent_region->getRegionID();
		bool this_region_alive = agent_region->isAlive();
		if ((mAgentRegionLastAlive && !this_region_alive) // newly dead
		    && (mAgentRegionLastID == this_region_id)) // same region
		{
			forceDisconnect(LLTrans::getString("AgentLostConnection"));
		}
		mAgentRegionLastID = this_region_id;
		mAgentRegionLastAlive = this_region_alive;
	}
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

	// Remember if we were flying
	gSavedSettings.setBOOL("FlyingAtExit", gAgent.getFlying() );

	// Un-minimize all windows so they don't get saved minimized
	if (gFloaterView)
	{
		gFloaterView->restoreAll();
	}

	if (LLSelectMgr::getInstance())
	{
		LLSelectMgr::getInstance()->deselectAll();
	}

	// save inventory if appropriate
	gInventory.cache(gInventory.getRootFolderID(), gAgent.getID());
	if (gInventory.getLibraryRootFolderID().notNull()
		&& gInventory.getLibraryOwnerID().notNull())
	{
		gInventory.cache(
			gInventory.getLibraryRootFolderID(),
			gInventory.getLibraryOwnerID());
	}

	saveNameCache();

	// close inventory interface, close all windows
	LLFloaterInventory::cleanup();

	gAgentWearables.cleanup();
	gAgentCamera.cleanup();
	// Also writes cached agent settings to gSavedSettings
	gAgent.cleanup();

	// This is where we used to call gObjectList.destroy() and then delete gWorldp.
	// Now we just ask the LLWorld singleton to cleanly shut down.
	if(LLWorld::instanceExists())
	{
		LLWorld::getInstance()->destroyClass();
	}

	// call all self-registered classes
	LLDestroyClassList::instance().fireCallbacks();

	cleanup_xfer_manager();
	gDisconnected = TRUE;

	// Pass the connection state to LLUrlEntryParcel not to attempt
	// parcel info requests while disconnected.
	LLUrlEntryParcel::setDisconnected(gDisconnected);
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

void LLAppViewer::forceErrorInfiniteLoop()
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

void LLAppViewer::forceErrorDriverCrash()
{
	glDeleteTextures(1, NULL);
}

void LLAppViewer::initMainloopTimeout(const std::string& state, F32 secs)
{
	if(!mMainloopTimeout)
	{
		mMainloopTimeout = new LLWatchdogTimeout();
		resumeMainloopTimeout(state, secs);
	}
}

void LLAppViewer::destroyMainloopTimeout()
{
	if(mMainloopTimeout)
	{
		delete mMainloopTimeout;
		mMainloopTimeout = NULL;
	}
}

void LLAppViewer::resumeMainloopTimeout(const std::string& state, F32 secs)
{
	if(mMainloopTimeout)
	{
		if(secs < 0.0f)
		{
			secs = gSavedSettings.getF32("MainloopTimeoutDefault");
		}
		
		mMainloopTimeout->setTimeout(secs);
		mMainloopTimeout->start(state);
	}
}

void LLAppViewer::pauseMainloopTimeout()
{
	if(mMainloopTimeout)
	{
		mMainloopTimeout->stop();
	}
}

void LLAppViewer::pingMainloopTimeout(const std::string& state, F32 secs)
{
//	if(!restoreErrorTrap())
//	{
//		llwarns << "!!!!!!!!!!!!! Its an error trap!!!!" << state << llendl;
//	}
	
	if(mMainloopTimeout)
	{
		if(secs < 0.0f)
		{
			secs = gSavedSettings.getF32("MainloopTimeoutDefault");
		}

		mMainloopTimeout->setTimeout(secs);
		mMainloopTimeout->ping(state);
	}
}

void LLAppViewer::handleLoginComplete()
{
	gLoggedInTime.start();
	initMainloopTimeout("Mainloop Init");

	// Store some data to DebugInfo in case of a freeze.
	gDebugInfo["ClientInfo"]["Name"] = LLVersionInfo::getChannel();

	gDebugInfo["ClientInfo"]["MajorVersion"] = LLVersionInfo::getMajor();
	gDebugInfo["ClientInfo"]["MinorVersion"] = LLVersionInfo::getMinor();
	gDebugInfo["ClientInfo"]["PatchVersion"] = LLVersionInfo::getPatch();
	gDebugInfo["ClientInfo"]["BuildVersion"] = LLVersionInfo::getBuild();

	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if ( parcel && parcel->getMusicURL()[0])
	{
		gDebugInfo["ParcelMusicURL"] = parcel->getMusicURL();
	}	
	if ( parcel && parcel->getMediaURL()[0])
	{
		gDebugInfo["ParcelMediaURL"] = parcel->getMediaURL();
	}
	
	gDebugInfo["SettingsFilename"] = gSavedSettings.getString("ClientSettingsFile");
	gDebugInfo["CAFilename"] = gDirUtilp->getCAFile();
	gDebugInfo["ViewerExePath"] = gDirUtilp->getExecutablePathAndName();
	gDebugInfo["CurrentPath"] = gDirUtilp->getCurPath();

	if(gAgent.getRegion())
	{
		gDebugInfo["CurrentSimHost"] = gAgent.getRegionHost().getHostName();
		gDebugInfo["CurrentRegion"] = gAgent.getRegion()->getName();
	}

	if(LLAppViewer::instance()->mMainloopTimeout)
	{
		gDebugInfo["MainloopTimeoutState"] = LLAppViewer::instance()->mMainloopTimeout->getState();
	}

	mOnLoginCompleted();

	writeDebugInfo();
}

// *TODO - generalize this and move DSO wrangling to a helper class -brad
void LLAppViewer::loadEventHostModule(S32 listen_port)
{
	std::string dso_name =
#if LL_WINDOWS
	    "lleventhost.dll";
#elif LL_DARWIN
	    "liblleventhost.dylib";
#else
	    "liblleventhost.so";
#endif

	std::string dso_path = gDirUtilp->findFile(dso_name,
		gDirUtilp->getAppRODataDir(),
		gDirUtilp->getExecutableDir());

	if(dso_path == "")
	{
		llerrs << "QAModeEventHost requested but module \"" << dso_name << "\" not found!" << llendl;
		return;
	}

	LL_INFOS("eventhost") << "Found lleventhost at '" << dso_path << "'" << LL_ENDL;
#if ! defined(LL_WINDOWS)
	{
		std::string outfile("/tmp/lleventhost.file.out");
		std::string command("file '" + dso_path + "' > '" + outfile + "' 2>&1");
		int rc = system(command.c_str());
		if (rc != 0)
		{
			LL_WARNS("eventhost") << command << " ==> " << rc << ':' << LL_ENDL;
		}
		else
		{
			LL_INFOS("eventhost") << command << ':' << LL_ENDL;
		}
		{
			std::ifstream reader(outfile.c_str());
			std::string line;
			while (std::getline(reader, line))
			{
				size_t len = line.length();
				if (len && line[len-1] == '\n')
					line.erase(len-1);
				LL_INFOS("eventhost") << line << LL_ENDL;
			}
		}
		remove(outfile.c_str());
	}
#endif // LL_WINDOWS

	apr_dso_handle_t * eventhost_dso_handle = NULL;
	apr_pool_t * eventhost_dso_memory_pool = NULL;

	//attempt to load the shared library
	apr_pool_create(&eventhost_dso_memory_pool, NULL);
	apr_status_t rv = apr_dso_load(&eventhost_dso_handle,
		dso_path.c_str(),
		eventhost_dso_memory_pool);
	llassert_always(! ll_apr_warn_status(rv, eventhost_dso_handle));
	llassert_always(eventhost_dso_handle != NULL);

	int (*ll_plugin_start_func)(LLSD const &) = NULL;
	rv = apr_dso_sym((apr_dso_handle_sym_t*)&ll_plugin_start_func, eventhost_dso_handle, "ll_plugin_start");

	llassert_always(! ll_apr_warn_status(rv, eventhost_dso_handle));
	llassert_always(ll_plugin_start_func != NULL);

	LLSD args;
	args["listen_port"] = listen_port;

	int status = ll_plugin_start_func(args);

	if(status != 0)
	{
		llerrs << "problem loading eventhost plugin, status: " << status << llendl;
	}

	mPlugins.insert(eventhost_dso_handle);
}

void LLAppViewer::launchUpdater()
{
		LLSD query_map = LLSD::emptyMap();
	// *TODO place os string in a global constant
#if LL_WINDOWS  
	query_map["os"] = "win";
#elif LL_DARWIN
	query_map["os"] = "mac";
#elif LL_LINUX
	query_map["os"] = "lnx";
#elif LL_SOLARIS
	query_map["os"] = "sol";
#endif
	// *TODO change userserver to be grid on both viewer and sim, since
	// userserver no longer exists.
	query_map["userserver"] = LLGridManager::getInstance()->getGridLabel();
	query_map["channel"] = LLVersionInfo::getChannel();
	// *TODO constantize this guy
	// *NOTE: This URL is also used in win_setup/lldownloader.cpp
	LLURI update_url = LLURI::buildHTTP("secondlife.com", 80, "update.php", query_map);
	
	if(LLAppViewer::sUpdaterInfo)
	{
		delete LLAppViewer::sUpdaterInfo;
	}
	LLAppViewer::sUpdaterInfo = new LLAppViewer::LLUpdaterInfo() ;

	// if a sim name was passed in via command line parameter (typically through a SLURL)
	if ( LLStartUp::getStartSLURL().getType() == LLSLURL::LOCATION )
	{
		// record the location to start at next time
		gSavedSettings.setString( "NextLoginLocation", LLStartUp::getStartSLURL().getSLURLString()); 
	};

#if LL_WINDOWS
	LLAppViewer::sUpdaterInfo->mUpdateExePath = gDirUtilp->getTempFilename();
	if (LLAppViewer::sUpdaterInfo->mUpdateExePath.empty())
	{
		delete LLAppViewer::sUpdaterInfo ;
		LLAppViewer::sUpdaterInfo = NULL ;

		// We're hosed, bail
		LL_WARNS("AppInit") << "LLDir::getTempFilename() failed" << LL_ENDL;
		return;
	}

	LLAppViewer::sUpdaterInfo->mUpdateExePath += ".exe";

	std::string updater_source = gDirUtilp->getAppRODataDir();
	updater_source += gDirUtilp->getDirDelimiter();
	updater_source += "updater.exe";

	LL_DEBUGS("AppInit") << "Calling CopyFile source: " << updater_source
			<< " dest: " << LLAppViewer::sUpdaterInfo->mUpdateExePath
			<< LL_ENDL;


	if (!CopyFileA(updater_source.c_str(), LLAppViewer::sUpdaterInfo->mUpdateExePath.c_str(), FALSE))
	{
		delete LLAppViewer::sUpdaterInfo ;
		LLAppViewer::sUpdaterInfo = NULL ;

		LL_WARNS("AppInit") << "Unable to copy the updater!" << LL_ENDL;

		return;
	}

	LLAppViewer::sUpdaterInfo->mParams << "-url \"" << update_url.asString() << "\"";

	LL_DEBUGS("AppInit") << "Calling updater: " << LLAppViewer::sUpdaterInfo->mUpdateExePath << " " << LLAppViewer::sUpdaterInfo->mParams.str() << LL_ENDL;

	//Explicitly remove the marker file, otherwise we pass the lock onto the child process and things get weird.
	LLAppViewer::instance()->removeMarkerFile(); // In case updater fails

	// *NOTE:Mani The updater is spawned as the last thing before the WinMain exit.
	// see LLAppViewerWin32.cpp
	
#elif LL_DARWIN
	LLAppViewer::sUpdaterInfo->mUpdateExePath = "'";
	LLAppViewer::sUpdaterInfo->mUpdateExePath += gDirUtilp->getAppRODataDir();
	LLAppViewer::sUpdaterInfo->mUpdateExePath += "/mac-updater.app/Contents/MacOS/mac-updater' -url \"";
	LLAppViewer::sUpdaterInfo->mUpdateExePath += update_url.asString();
	LLAppViewer::sUpdaterInfo->mUpdateExePath += "\" -name \"";
	LLAppViewer::sUpdaterInfo->mUpdateExePath += LLAppViewer::instance()->getSecondLifeTitle();
	LLAppViewer::sUpdaterInfo->mUpdateExePath += "\" -bundleid \"";
	LLAppViewer::sUpdaterInfo->mUpdateExePath += LL_VERSION_BUNDLE_ID;
	LLAppViewer::sUpdaterInfo->mUpdateExePath += "\" &";

	LL_DEBUGS("AppInit") << "Calling updater: " << LLAppViewer::sUpdaterInfo->mUpdateExePath << LL_ENDL;

	// Run the auto-updater.
	system(LLAppViewer::sUpdaterInfo->mUpdateExePath.c_str()); /* Flawfinder: ignore */

#elif (LL_LINUX || LL_SOLARIS) && LL_GTK
	// we tell the updater where to find the xml containing string
	// translations which it can use for its own UI
	std::string xml_strings_file = "strings.xml";
	std::vector<std::string> xui_path_vec = LLUI::getXUIPaths();
	std::string xml_search_paths;
	std::vector<std::string>::const_iterator iter;
	// build comma-delimited list of xml paths to pass to updater
	for (iter = xui_path_vec.begin(); iter != xui_path_vec.end(); )
	{
		std::string this_skin_dir = gDirUtilp->getDefaultSkinDir()
			+ gDirUtilp->getDirDelimiter()
			+ (*iter);
		llinfos << "Got a XUI path: " << this_skin_dir << llendl;
		xml_search_paths.append(this_skin_dir);
		++iter;
		if (iter != xui_path_vec.end())
			xml_search_paths.append(","); // comma-delimit
	}
	// build the overall command-line to run the updater correctly
	LLAppViewer::sUpdaterInfo->mUpdateExePath = 
		gDirUtilp->getExecutableDir() + "/" + "linux-updater.bin" + 
		" --url \"" + update_url.asString() + "\"" +
		" --name \"" + LLAppViewer::instance()->getSecondLifeTitle() + "\"" +
		" --dest \"" + gDirUtilp->getAppRODataDir() + "\"" +
		" --stringsdir \"" + xml_search_paths + "\"" +
		" --stringsfile \"" + xml_strings_file + "\"";

	LL_INFOS("AppInit") << "Calling updater: " 
			    << LLAppViewer::sUpdaterInfo->mUpdateExePath << LL_ENDL;

	// *TODO: we could use the gdk equivalent to ensure the updater
	// gets started on the same screen.
	GError *error = NULL;
	if (!g_spawn_command_line_async(LLAppViewer::sUpdaterInfo->mUpdateExePath.c_str(), &error))
	{
		llerrs << "Failed to launch updater: "
		       << error->message
		       << llendl;
	}
	if (error) {
		g_error_free(error);
	}
#else
	OSMessageBox(LLTrans::getString("MBNoAutoUpdate"), LLStringUtil::null, OSMB_OK);
#endif

	// *REMOVE:Mani - Saving for reference...
	// LLAppViewer::instance()->forceQuit();
}


//virtual
void LLAppViewer::setMasterSystemAudioMute(bool mute)
{
	gSavedSettings.setBOOL("MuteAudio", mute);
}

//virtual
bool LLAppViewer::getMasterSystemAudioMute()
{
	return gSavedSettings.getBOOL("MuteAudio");
}

//----------------------------------------------------------------------------
// Metrics-related methods (static and otherwise)
//----------------------------------------------------------------------------

/**
 * LLViewerAssetStats collects data on a per-region (as defined by the agent's
 * location) so we need to tell it about region changes which become a kind of
 * hidden variable/global state in the collectors.  For collectors not running
 * on the main thread, we need to send a message to move the data over safely
 * and cheaply (amortized over a run).
 */
void LLAppViewer::metricsUpdateRegion(U64 region_handle)
{
	if (0 != region_handle)
	{
		LLViewerAssetStatsFF::set_region_main(region_handle);
		if (LLAppViewer::sTextureFetch)
		{
			// Send a region update message into 'thread1' to get the new region.
			LLAppViewer::sTextureFetch->commandSetRegion(region_handle);
		}
		else
		{
			// No 'thread1', a.k.a. TextureFetch, so update directly
			LLViewerAssetStatsFF::set_region_thread1(region_handle);
		}
	}
}


/**
 * Attempts to start a multi-threaded metrics report to be sent back to
 * the grid for consumption.
 */
void LLAppViewer::metricsSend(bool enable_reporting)
{
	if (! gViewerAssetStatsMain)
		return;

	if (LLAppViewer::sTextureFetch)
	{
		LLViewerRegion * regionp = gAgent.getRegion();

		if (enable_reporting && regionp)
		{
			std::string	caps_url = regionp->getCapability("ViewerMetrics");

			// Make a copy of the main stats to send into another thread.
			// Receiving thread takes ownership.
			LLViewerAssetStats * main_stats(new LLViewerAssetStats(*gViewerAssetStatsMain));
			
			// Send a report request into 'thread1' to get the rest of the data
			// and provide some additional parameters while here.
			LLAppViewer::sTextureFetch->commandSendMetrics(caps_url,
														   gAgentSessionID,
														   gAgentID,
														   main_stats);
			main_stats = 0;		// Ownership transferred
		}
		else
		{
			LLAppViewer::sTextureFetch->commandDataBreak();
		}
	}

	// Reset even if we can't report.  Rather than gather up a huge chunk of
	// data, we'll keep to our sampling interval and retain the data
	// resolution in time.
	gViewerAssetStatsMain->reset();
}

