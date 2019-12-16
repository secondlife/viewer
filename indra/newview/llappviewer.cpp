/**
 * @file llappviewer.cpp
 * @brief The LLAppViewer class definitions
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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
#include "llagentui.h"
#include "llagentwearables.h"
#include "lldirpicker.h"
#include "llfloaterimcontainer.h"
#include "llimprocessing.h"
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
#include "llcalc.h"
#include "llconversationlog.h"
#include "lldxhardware.h"
#include "lltexturestats.h"
#include "lltrace.h"
#include "lltracethreadrecorder.h"
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
#include "llrender.h"
#include "llteleporthistory.h"
#include "lltoast.h"
#include "llsdutil_math.h"
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
#include "llvopartgroup.h"
#include "llweb.h"
#include "llfloatertexturefetchdebugger.h"
#include "llspellcheck.h"
#include "llscenemonitor.h"
#include "llavatarrenderinfoaccountant.h"
#include "lllocalbitmaps.h"

// Linden library includes
#include "llavatarnamecache.h"
#include "lldiriterator.h"
#include "llexperiencecache.h"
#include "llimagej2c.h"
#include "llmemory.h"
#include "llprimitive.h"
#include "llurlaction.h"
#include "llurlentry.h"
#include "llvfile.h"
#include "llvfsthread.h"
#include "llvolumemgr.h"
#include "llxfermanager.h"
#include "llphysicsextensions.h"

#include "llnotificationmanager.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"

#include "llleap.h"
#include "stringize.h"
#include "llcoros.h"
#include "llexception.h"
#if !LL_LINUX
#include "cef/dullahan.h"
#include "vlc/libvlc_version.h"
#endif // LL_LINUX

// Third party library includes
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/throw_exception.hpp>

#if LL_WINDOWS
#	include <share.h> // For _SH_DENYWR in processMarkerFiles
#else
#   include <sys/file.h> // For processMarkerFiles
#endif

#include "llapr.h"
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

#include "llsdutil.h"
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
#include "llfloateroutfitsnapshot.h"
#include "llfloatersnapshot.h"
#include "llsidepanelinventory.h"

// includes for idle() idleShutdown()
#include "llviewercontrol.h"
#include "lleventnotifier.h"
#include "llcallbacklist.h"
#include "lldeferredsounds.h"
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
#include "llcleanup.h"

#include "llcoproceduremanager.h"
#include "llviewereventrecorder.h"

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

#if LL_DARWIN
extern BOOL gHiDPISupport;
#endif

////////////////////////////////////////////////////////////
// All from the last globals push...

F32 gSimLastTime; // Used in LLAppViewer::init and send_stats()
F32 gSimFrames;

BOOL gShowObjectUpdates = FALSE;
BOOL gUseQuickTime = TRUE;

eLastExecEvent gLastExecEvent = LAST_EXEC_NORMAL;
S32 gLastExecDuration = -1; // (<0 indicates unknown)

#if LL_WINDOWS
#   define LL_PLATFORM_KEY "win"
#elif LL_DARWIN
#   define LL_PLATFORM_KEY "mac"
#elif LL_LINUX
#   define LL_PLATFORM_KEY "lnx"
else
#   error "Unknown Platform"
#endif
const char* gPlatform = LL_PLATFORM_KEY;

LLSD gDebugInfo;

U32	gFrameCount = 0;
U32 gForegroundFrameCount = 0; // number of frames that app window was in foreground
LLPumpIO* gServicePump = NULL;

U64MicrosecondsImplicit gFrameTime = 0;
F32SecondsImplicit gFrameTimeSeconds = 0.f;
F32SecondsImplicit gFrameIntervalSeconds = 0.f;
F32 gFPSClamped = 10.f;						// Pretend we start at target rate.
F32 gFrameDTClamped = 0.f;					// Time between adjacent checks to network for packets
U64MicrosecondsImplicit	gStartTime = 0; // gStartTime is "private", used only to calculate gFrameTimeSeconds

LLTimer gRenderStartTime;
LLFrameTimer gForegroundTime;
LLFrameTimer gLoggedInTime;
LLTimer gLogoutTimer;
static const F32 LOGOUT_REQUEST_TIME = 6.f;  // this will be cut short by the LogoutReply msg.
F32 gLogoutMaxTime = LOGOUT_REQUEST_TIME;


S32 gPendingMetricsUploads = 0;


BOOL				gDisconnected = FALSE;

// used to restore texture state after a mode switch
LLFrameTimer	gRestoreGLTimer;
BOOL			gRestoreGL = FALSE;
bool			gUseWireframe = FALSE;

//use for remember deferred mode in wireframe switch
bool			gInitialDeferredModeForWireframe = FALSE;

// VFS globals - see llappviewer.h
LLVFS* gStaticVFS = NULL;

LLMemoryInfo gSysMemory;
U64Bytes gMemoryAllocated(0); // updated in display_stats() in llviewerdisplay.cpp

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

BOOL gSimulateMemLeak = FALSE;

////////////////////////////////////////////////////////////
// Internal globals... that should be removed.
static std::string gArgs;
const int MAX_MARKER_LENGTH = 1024;
const std::string MARKER_FILE_NAME("SecondLife.exec_marker");
const std::string START_MARKER_FILE_NAME("SecondLife.start_marker");
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
	LLSINGLETON_EMPTY_CTOR(LLDeferredTaskList);
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
	// This URL shows up in a surprising number of places in various skin
	// files. We really only want to have to maintain a single copy of it.
	default_trans_args.insert("create_account_url");
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
		SoundData soundData(uuid, gAgent.getID(), 1.0f, LLAudioEngine::AUDIO_TYPE_UI);
		gAudiop->triggerSound(soundData);
	}
}

// A callback set in LLAppViewer::init()
static void deferred_ui_audio_callback(const LLUUID& uuid)
{
	if (gAudiop)
	{
		SoundData soundData(uuid, gAgent.getID(), 1.0f, LLAudioEngine::AUDIO_TYPE_UI);
		LLDeferredSounds::instance().deferSound(soundData);
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
	LLRender::sNsightDebugSupport = gSavedSettings.getBOOL("RenderNsightDebugSupport");
	LLVertexBuffer::sUseVAO = gSavedSettings.getBOOL("RenderUseVAO");
	LLImageGL::sGlobalUseAnisotropic	= gSavedSettings.getBOOL("RenderAnisotropic");
	LLImageGL::sCompressTextures		= gSavedSettings.getBOOL("RenderCompressTextures");
	LLVOVolume::sLODFactor				= llclamp(gSavedSettings.getF32("RenderVolumeLODFactor"), 0.01f, MAX_LOD_FACTOR);
	LLVOVolume::sDistanceFactor			= 1.f-LLVOVolume::sLODFactor * 0.1f;
	LLVolumeImplFlexible::sUpdateFactor = gSavedSettings.getF32("RenderFlexTimeFactor");
	LLVOTree::sTreeFactor				= gSavedSettings.getF32("RenderTreeLODFactor");
	LLVOAvatar::sLODFactor				= llclamp(gSavedSettings.getF32("RenderAvatarLODFactor"), 0.f, MAX_AVATAR_LOD_FACTOR);
	LLVOAvatar::sPhysicsLODFactor		= llclamp(gSavedSettings.getF32("RenderAvatarPhysicsLODFactor"), 0.f, MAX_AVATAR_LOD_FACTOR);
	LLVOAvatar::updateImpostorRendering(gSavedSettings.getU32("RenderAvatarMaxNonImpostors"));
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
	
#if LL_DARWIN
	gHiDPISupport = gSavedSettings.getBOOL("RenderHiDPI");
#endif
}

static void settings_modify()
{
	LLRenderTarget::sUseFBO				= gSavedSettings.getBOOL("RenderDeferred");
	LLPipeline::sRenderBump				= gSavedSettings.getBOOL("RenderObjectBump");
	LLPipeline::sRenderDeferred		= LLPipeline::sRenderBump && gSavedSettings.getBOOL("RenderDeferred");
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
		llofstream os(mFile.c_str());

		while (!LLAppViewer::instance()->isQuitting())
		{
			LLTrace::BlockTimer::writeLog(os);
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

std::string getRuntime()
{
	return llformat("%.4f", (F32)LLTimer::getElapsedSeconds().value());
}

LLAppViewer::LLAppViewer()
:	mMarkerFile(),
	mLogoutMarkerFile(),
	mReportedCrash(false),
	mNumSessions(0),
	mPurgeCache(false),
	mPurgeOnExit(false),
	mSecondInstance(false),
	mSavedFinalSnapshot(false),
	mSavePerAccountSettings(false),		// don't save settings on logout unless login succeeded.
	mQuitRequested(false),
	mLogoutRequestSent(false),
	mLastAgentControlFlags(0),
	mLastAgentForceUpdate(0),
	mMainloopTimeout(NULL),
	mAgentRegionLastAlive(false),
	mRandomizeFramerate(LLCachedControl<bool>(gSavedSettings,"Randomize Framerate", FALSE)),
	mPeriodicSlowFrame(LLCachedControl<bool>(gSavedSettings,"Periodic Slow Frame", FALSE)),
	mFastTimerLogThread(NULL),
	mSettingsLocationList(NULL),
	mIsFirstRun(false),
	mMinMicroSecPerFrame(0.f)
{
	if(NULL != sInstance)
	{
		LL_ERRS() << "Oh no! An instance of LLAppViewer already exists! LLAppViewer is sort of like a singleton." << LL_ENDL;
	}

    mDumpPath ="";

	// Need to do this initialization before we do anything else, since anything
	// that touches files should really go through the lldir API
	gDirUtilp->initAppDirs("SecondLife");
	//
	// IMPORTANT! Do NOT put anything that will write
	// into the log files during normal startup until AFTER
	// we run the "program crashed last time" error handler below.
	//
	sInstance = this;

	gLoggedInTime.stop();

	initLoggingAndGetLastDuration();

	processMarkerFiles();
	//
	// OK to write stuff to logs now, we've now crash reported if necessary
	//

	LLLoginInstance::instance().setPlatformInfo(gPlatform, LLOSInfo::instance().getOSVersionString(), LLOSInfo::instance().getOSStringSimple());

	// Under some circumstances we want to read the static_debug_info.log file
	// from the previous viewer run between this constructor call and the
	// init() call, which will overwrite the static_debug_info.log file for
	// THIS run. So setDebugFileNames() early.
#if LL_BUGSPLAT
	// MAINT-8917: don't create a dump directory just for the
	// static_debug_info.log file
	std::string logdir = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "");
#else // ! LL_BUGSPLAT
	// write Google Breakpad minidump files to a per-run dump directory to avoid multiple viewer issues.
	std::string logdir = gDirUtilp->getExpandedFilename(LL_PATH_DUMP, "");
#endif // ! LL_BUGSPLAT
	mDumpPath = logdir;
	setMiniDumpDir(logdir);
	setDebugFileNames(logdir);
}

LLAppViewer::~LLAppViewer()
{
	delete mSettingsLocationList;

	destroyMainloopTimeout();

	// If we got to this destructor somehow, the app didn't hang.
	removeMarkerFiles();
}

class LLUITranslationBridge : public LLTranslationBridge
{
public:
	virtual std::string getString(const std::string &xml_desc)
	{
		return LLTrans::getString(xml_desc);
	}
};

namespace {
// With Xcode 6, _exit() is too magical to use with boost::bind(), so provide
// this little helper function.
void fast_exit(int rc)
{
	_exit(rc);
}


}


bool LLAppViewer::init()
{
	setupErrorHandling(mSecondInstance);

	//
	// Start of the application
	//

	// initialize LLWearableType translation bridge.
	// Memory will be cleaned up in ::cleanupClass()
	LLWearableType::initParamSingleton(new LLUITranslationBridge());

	// initialize SSE options
	LLVector4a::initClass();

	//initialize particle index pool
	LLVOPartGroup::initClass();

	// set skin search path to default, will be overridden later
	// this allows simple skinned file lookups to work
	gDirUtilp->setSkinFolder("default", "en");

//	initLoggingAndGetLastDuration();

	//
	// OK to write stuff to logs now, we've now crash reported if necessary
	//
	init_default_trans_args();

	if (!initConfiguration())
		return false;

	LL_INFOS("InitInfo") << "Configuration initialized." << LL_ENDL ;

	//set the max heap size.
	initMaxHeapSize() ;
	LLCoros::instance().setStackSize(gSavedSettings.getS32("CoroutineStackSize"));


	// Although initLoggingAndGetLastDuration() is the right place to mess with
	// setFatalFunction(), we can't query gSavedSettings until after
	// initConfiguration().
	S32 rc(gSavedSettings.getS32("QAModeTermCode"));
	if (rc >= 0)
	{
		// QAModeTermCode set, terminate with that rc on LL_ERRS. Use
		// fast_exit() rather than exit() because normal cleanup depends too
		// much on successful startup!
		LLError::setFatalFunction(boost::bind(fast_exit, rc));
	}

    mAlloc.setProfilingEnabled(gSavedSettings.getBOOL("MemProfiling"));

	// Initialize the non-LLCurl libcurl library.  Should be called
	// before consumers (LLTextureFetch).
	mAppCoreHttp.init();

	LL_INFOS("InitInfo") << "LLCore::Http initialized." << LL_ENDL ;

    LLMachineID::init();

	{
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

	LLUI::initParamSingleton(settings_map,
		LLUIImageList::getInstance(),
		ui_audio_callback,
		deferred_ui_audio_callback);
	LL_INFOS("InitInfo") << "UI initialized." << LL_ENDL ;

	// NOW LLUI::getLanguage() should work. gDirUtilp must know the language
	// for this session ASAP so all the file-loading commands that follow,
	// that use findSkinnedFilenames(), will include the localized files.
	gDirUtilp->setSkinFolder(gDirUtilp->getSkinFolder(), LLUI::getLanguage());

	// Setup LLTrans after LLUI::initClass has been called.
	initStrings();

	// Setup notifications after LLUI::initClass() has been called.
	LLNotifications::instance();
	LL_INFOS("InitInfo") << "Notifications initialized." << LL_ENDL ;

    writeSystemInfo();

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	// *FIX: The following code isn't grouped into functions yet.

	//
	// Various introspection concerning the libs we're using - particularly
	// the libs involved in getting to a full login screen.
	//
	LL_INFOS("InitInfo") << "J2C Engine is: " << LLImageJ2C::getEngineInfo() << LL_ENDL;
	LL_INFOS("InitInfo") << "libcurl version is: " << LLCore::LLHttp::getCURLVersion() << LL_ENDL;

	/////////////////////////////////////////////////
	// OS-specific login dialogs
	/////////////////////////////////////////////////

	//test_cached_control();

	// track number of times that app has run
	mNumSessions = gSavedSettings.getS32("NumSessions");
	mNumSessions++;
	gSavedSettings.setS32("NumSessions", mNumSessions);

	// LLKeyboard relies on LLUI to know what some accelerator keys are called.
	LLKeyboard::setStringTranslatorFunc( LLTrans::getKeyboardString );

	// Provide the text fields with callbacks for opening Urls
	LLUrlAction::setOpenURLCallback(boost::bind(&LLWeb::loadURL, _1, LLStringUtil::null, LLStringUtil::null));
	LLUrlAction::setOpenURLInternalCallback(boost::bind(&LLWeb::loadURLInternal, _1, LLStringUtil::null, LLStringUtil::null, false));
	LLUrlAction::setOpenURLExternalCallback(boost::bind(&LLWeb::loadURLExternal, _1, true, LLStringUtil::null));
	LLUrlAction::setExecuteSLURLCallback(&LLURLDispatcher::dispatchFromTextEditor);

	// Let code in llui access the viewer help floater
	LLUI::getInstance()->mHelpImpl = LLViewerHelp::getInstance();

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
		LL_WARNS("InitInfo") << "Failed to init cache" << LL_ENDL;
		std::ostringstream msg;
		msg << LLTrans::getString("MBUnableToAccessFile");
		OSMessageBox(msg.str(),LLStringUtil::null,OSMB_OK);
		return 0;
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
		U64Bytes minRAM;
		minRAMString >> minRAM;

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
		if(gSysMemory.getPhysicalMemoryKB() < minRAM)
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

#if LL_WINDOWS
	if (gGLManager.mGLVersion < LLFeatureManager::getInstance()->getExpectedGLVersion())
	{
		std::string url;
		if (gGLManager.mIsIntel)
		{
			url = LLTrans::getString("IntelDriverPage");
		}
		else if (gGLManager.mIsNVIDIA)
		{
			url = LLTrans::getString("NvidiaDriverPage");
		}
		else if (gGLManager.mIsATI)
		{
			url = LLTrans::getString("AMDDriverPage");
		}

		if (!url.empty())
		{
			LLNotificationsUtil::add("OldGPUDriver", LLSD().with("URL", url));
		}
	}
#endif


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

	gGLActive = FALSE;

	LLProcess::Params updater;
	updater.desc = "updater process";
	// Because it's the updater, it MUST persist beyond the lifespan of the
	// viewer itself.
	updater.autokill = false;
#if LL_WINDOWS
	updater.executable = gDirUtilp->getExpandedFilename(LL_PATH_EXECUTABLE, "SLVersionChecker.exe");
#elif LL_DARWIN
	// explicitly run the system Python interpreter on SLVersionChecker.py
	updater.executable = "python";
	updater.args.add(gDirUtilp->add(gDirUtilp->getAppRODataDir(), "updater", "SLVersionChecker.py"));
#else
	updater.executable = gDirUtilp->getExpandedFilename(LL_PATH_EXECUTABLE, "SLVersionChecker");
#endif
	// add LEAP mode command-line argument to whichever of these we selected
	updater.args.add("leap");
	// UpdaterServiceSettings
	updater.args.add(stringize(gSavedSettings.getU32("UpdaterServiceSetting")));
	// channel
	updater.args.add(LLVersionInfo::getChannel());
	// testok
	updater.args.add(stringize(gSavedSettings.getBOOL("UpdaterWillingToTest")));
	// ForceAddressSize
	updater.args.add(stringize(gSavedSettings.getU32("ForceAddressSize")));

#if LL_WINDOWS && !LL_RELEASE_FOR_DOWNLOAD && !LL_SEND_CRASH_REPORTS
	// This is neither a release package, nor crash-reporting enabled test build
	// try to run version updater, but don't bother if it fails (file might be missing)
	LLLeap *leap_p = LLLeap::create(updater, false);
	if (!leap_p)
	{
		LL_WARNS("LLLeap") << "Failed to run LLLeap" << LL_ENDL;
	}
#else
 	// Run the updater. An exception from launching the updater should bother us.
	LLLeap::create(updater, true);
#endif

	// Iterate over --leap command-line options. But this is a bit tricky: if
	// there's only one, it won't be an array at all.
	LLSD LeapCommand(gSavedSettings.getLLSD("LeapCommand"));
	LL_DEBUGS("InitInfo") << "LeapCommand: " << LeapCommand << LL_ENDL;
	if (LeapCommand.isDefined() && ! LeapCommand.isArray())
	{
		// If LeapCommand is actually a scalar value, make an array of it.
		// Have to do it in two steps because LeapCommand.append(LeapCommand)
		// trashes content! :-P
		LLSD item(LeapCommand);
		LeapCommand.append(item);
	}
	BOOST_FOREACH(const std::string& leap, llsd::inArray(LeapCommand))
	{
		LL_INFOS("InitInfo") << "processing --leap \"" << leap << '"' << LL_ENDL;
		// We don't have any better description of this plugin than the
		// user-specified command line. Passing "" causes LLLeap to derive a
		// description from the command line itself.
		// Suppress LLLeap::Error exception: trust LLLeap's own logging. We
		// don't consider any one --leap command mission-critical, so if one
		// fails, log it, shrug and carry on.
		LLLeap::create("", leap, false); // exception=false
	}

	if (gSavedSettings.getBOOL("QAMode") && gSavedSettings.getS32("QAModeEventHostPort") > 0)
	{
		LL_WARNS("InitInfo") << "QAModeEventHostPort DEPRECATED: "
							 << "lleventhost no longer supported as a dynamic library"
							 << LL_ENDL;
	}

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

    /// Tell the Coprocedure manager how to discover and store the pool sizes
    // what I wanted
    LLCoprocedureManager::getInstance()->setPropertyMethods(
        boost::bind(&LLControlGroup::getU32, boost::ref(gSavedSettings), _1),
        boost::bind(&LLControlGroup::declareU32, boost::ref(gSavedSettings), _1, _2, _3, LLControlVariable::PERSIST_ALWAYS));

	// TODO: consider moving proxy initialization here or LLCopocedureManager after proxy initialization, may be implement
	// some other protection to make sure we don't use network before initializng proxy

	/*----------------------------------------------------------------------*/
	// nat 2016-06-29 moved the following here from the former mainLoop().
	mMainloopTimeout = new LLWatchdogTimeout();

	// Create IO Pump to use for HTTP Requests.
	gServicePump = new LLPumpIO(gAPRPoolp);

	// Note: this is where gLocalSpeakerMgr and gActiveSpeakerMgr used to be instantiated.

	LLVoiceChannel::initClass();
	LLVoiceClient::initParamSingleton(gServicePump);
	LLVoiceChannel::setCurrentVoiceChannelChangedCallback(boost::bind(&LLFloaterIMContainer::onCurrentChannelChanged, _1), true);

	joystick = LLViewerJoystick::getInstance();
	joystick->setNeedsReset(true);
	/*----------------------------------------------------------------------*/

	gSavedSettings.getControl("FramePerSecondLimit")->getSignal()->connect(boost::bind(&LLAppViewer::onChangeFrameLimit, this, _2));
	onChangeFrameLimit(gSavedSettings.getLLSD("FramePerSecondLimit"));

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
	F32Gigabytes max_heap_size_gb = (F32Gigabytes)gSavedSettings.getF32("MaxHeapSize") ;
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

static LLTrace::BlockTimerStatHandle FTM_MESSAGES("System Messages");
static LLTrace::BlockTimerStatHandle FTM_SLEEP("Sleep");
static LLTrace::BlockTimerStatHandle FTM_YIELD("Yield");

static LLTrace::BlockTimerStatHandle FTM_TEXTURE_CACHE("Texture Cache");
static LLTrace::BlockTimerStatHandle FTM_DECODE("Image Decode");
static LLTrace::BlockTimerStatHandle FTM_VFS("VFS Thread");
static LLTrace::BlockTimerStatHandle FTM_LFS("LFS Thread");
static LLTrace::BlockTimerStatHandle FTM_PAUSE_THREADS("Pause Threads");
static LLTrace::BlockTimerStatHandle FTM_IDLE("Idle");
static LLTrace::BlockTimerStatHandle FTM_PUMP("Pump");
static LLTrace::BlockTimerStatHandle FTM_PUMP_SERVICE("Service");
static LLTrace::BlockTimerStatHandle FTM_SERVICE_CALLBACK("Callback");
static LLTrace::BlockTimerStatHandle FTM_AGENT_AUTOPILOT("Autopilot");
static LLTrace::BlockTimerStatHandle FTM_AGENT_UPDATE("Update");

// externally visible timers
LLTrace::BlockTimerStatHandle FTM_FRAME("Frame");

bool LLAppViewer::frame()
{
	bool ret = false;

	if (gSimulateMemLeak)
	{
		try
		{
			ret = doFrame();
		}
		catch (const LLContinueError&)
		{
			LOG_UNHANDLED_EXCEPTION("");
		}
		catch (std::bad_alloc)
		{
			LLMemory::logMemoryInfo(TRUE);
			LLFloaterMemLeak* mem_leak_instance = LLFloaterReg::findTypedInstance<LLFloaterMemLeak>("mem_leaking");
			if (mem_leak_instance)
			{
				mem_leak_instance->stop();
			}
			LL_WARNS() << "Bad memory allocation in LLAppViewer::frame()!" << LL_ENDL;
		}
	}
	else
	{ 
		try
		{
			ret = doFrame();
		}
		catch (const LLContinueError&)
		{
			LOG_UNHANDLED_EXCEPTION("");
		}
	}

	return ret;
}

bool LLAppViewer::doFrame()
{
	LLEventPump& mainloop(LLEventPumps::instance().obtain("mainloop"));
	LLSD newFrame;

	LL_RECORD_BLOCK_TIME(FTM_FRAME);
	LLTrace::BlockTimer::processTimes();
	LLTrace::get_frame_recording().nextPeriod();
	LLTrace::BlockTimer::logStats();

	LLTrace::get_thread_recorder()->pullFromChildren();

	//clear call stack records
	LL_CLEAR_CALLSTACKS();

	//check memory availability information
	checkMemory() ;

	{
		pingMainloopTimeout("Main:MiscNativeWindowEvents");

		if (gViewerWindow)
		{
			LL_RECORD_BLOCK_TIME(FTM_MESSAGES);
			gViewerWindow->getWindow()->processMiscNativeEvents();
		}

		pingMainloopTimeout("Main:GatherInput");

		if (gViewerWindow)
		{
			LL_RECORD_BLOCK_TIME(FTM_MESSAGES);
			if (!restoreErrorTrap())
			{
				LL_WARNS() << " Someone took over my signal/exception handler (post messagehandling)!" << LL_ENDL;
			}

			gViewerWindow->getWindow()->gatherInput();
		}

		//memory leaking simulation
		if (gSimulateMemLeak)
		{
			LLFloaterMemLeak* mem_leak_instance =
				LLFloaterReg::findTypedInstance<LLFloaterMemLeak>("mem_leaking");
			if (mem_leak_instance)
			{
				mem_leak_instance->idle();
			}
		}

		// canonical per-frame event
		mainloop.post(newFrame);

		if (!LLApp::isExiting())
		{
			pingMainloopTimeout("Main:JoystickKeyboard");

			// Scan keyboard for movement keys.  Command keys and typing
			// are handled by windows callbacks.  Don't do this until we're
			// done initializing.  JC
			if (gViewerWindow
				&& (gHeadlessClient || gViewerWindow->getWindow()->getVisible())
				&& gViewerWindow->getActive()
				&& !gViewerWindow->getWindow()->getMinimized()
				&& LLStartUp::getStartupState() == STATE_STARTED
				&& (gHeadlessClient || !gViewerWindow->getShowProgress())
				&& !gFocusMgr.focusLocked())
			{
				joystick->scanJoystick();
				gKeyboard->scanKeyboard();
			}

			// Update state based on messages, user input, object idle.
			{
				pauseMainloopTimeout(); // *TODO: Remove. Messages shouldn't be stalling for 20+ seconds!

				LL_RECORD_BLOCK_TIME(FTM_IDLE);
				idle();

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
			if (!LLApp::isExiting() && !gHeadlessClient && gViewerWindow)
			{
				pingMainloopTimeout("Main:Display");
				gGLActive = TRUE;

				static U64 last_call = 0;
				if (!gTeleportDisplay)
				{
					// Frame/draw throttling
					U64 elapsed_time = LLTimer::getTotalTime() - last_call;
					if (elapsed_time < mMinMicroSecPerFrame)
					{
						LL_RECORD_BLOCK_TIME(FTM_SLEEP);
						// llclamp for when time function gets funky
						U64 sleep_time = llclamp(mMinMicroSecPerFrame - elapsed_time, (U64)1, (U64)1e6);
						micro_sleep(sleep_time, 0);
					}
				}
				last_call = LLTimer::getTotalTime();

				display();

				pingMainloopTimeout("Main:Snapshot");
				LLFloaterSnapshot::update(); // take snapshots
					LLFloaterOutfitSnapshot::update();
				gGLActive = FALSE;
			}
		}

		pingMainloopTimeout("Main:Sleep");

		pauseMainloopTimeout();

		// Sleep and run background threads
		{
			LL_RECORD_BLOCK_TIME(FTM_SLEEP);

			// yield some time to the os based on command line option
			static LLCachedControl<S32> yield_time(gSavedSettings, "YieldTime", -1);
			if(yield_time >= 0)
			{
				LL_RECORD_BLOCK_TIME(FTM_YIELD);
				ms_sleep(yield_time);
			}

			// yield cooperatively when not running as foreground window
			if (   (gViewerWindow && !gViewerWindow->getWindow()->getVisible())
					|| !gFocusMgr.getAppHasFocus())
			{
				// Sleep if we're not rendering, or the window is minimized.
				static LLCachedControl<S32> s_bacground_yeild_time(gSavedSettings, "BackgroundYieldTime", 40);
				S32 milliseconds_to_sleep = llclamp((S32)s_bacground_yeild_time, 0, 1000);
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
				LL_INFOS() << "Periodic slow frame - sleeping 500 ms" << LL_ENDL;
				ms_sleep(500);
			}

			S32 total_work_pending = 0;
			S32 total_io_pending = 0;
			{
				S32 work_pending = 0;
				S32 io_pending = 0;
				F32 max_time = llmin(gFrameIntervalSeconds.value() *10.f, 1.f);

				work_pending += updateTextureThreads(max_time);

				{
					LL_RECORD_BLOCK_TIME(FTM_VFS);
 					io_pending += LLVFSThread::updateClass(1);
				}
				{
					LL_RECORD_BLOCK_TIME(FTM_LFS);
 					io_pending += LLLFSThread::updateClass(1);
				}

				if (io_pending > 1000)
				{
					ms_sleep(llmin(io_pending/100,100)); // give the vfs some time to catch up
				}

				total_work_pending += work_pending ;
				total_io_pending += io_pending ;

			}
			gMeshRepo.update() ;

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

			//texture fetching debugger
			if(LLTextureFetchDebugger::isEnabled())
			{
				LLFloaterTextureFetchDebugger* tex_fetch_debugger_instance =
					LLFloaterReg::findTypedInstance<LLFloaterTextureFetchDebugger>("tex_fetch_debugger");
				if(tex_fetch_debugger_instance)
				{
					tex_fetch_debugger_instance->idle() ;
				}
			}

			resumeMainloopTimeout();

			pingMainloopTimeout("Main:End");
		}
	}

	if (LLApp::isExiting())
	{
		// Save snapshot for next time, if we made it through initialization
		if (STATE_STARTED == LLStartUp::getStartupState())
		{
			saveFinalSnapshot();
		}

		if (LLVoiceClient::instanceExists())
		{
			LLVoiceClient::getInstance()->terminate();
		}

		delete gServicePump;

		destroyMainloopTimeout();

		LL_INFOS() << "Exiting main_loop" << LL_ENDL;
	}

	return ! LLApp::isRunning();
}

S32 LLAppViewer::updateTextureThreads(F32 max_time)
{
	S32 work_pending = 0;
	{
		LL_RECORD_BLOCK_TIME(FTM_TEXTURE_CACHE);
 		work_pending += LLAppViewer::getTextureCache()->update(max_time); // unpauses the texture cache thread
	}
	{
		LL_RECORD_BLOCK_TIME(FTM_DECODE);
	 	work_pending += LLAppViewer::getImageDecodeThread()->update(max_time); // unpauses the image thread
	}
	{
		LL_RECORD_BLOCK_TIME(FTM_DECODE);
	 	work_pending += LLAppViewer::getTextureFetch()->update(max_time); // unpauses the texture fetch thread
	}
	return work_pending;
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
		LL_INFOS() << "Waiting for pending IO to finish: " << pending << LL_ENDL;
		ms_sleep(100);
	}
}

bool LLAppViewer::cleanup()
{
	//ditch LLVOAvatarSelf instance
	gAgentAvatarp = NULL;

    LLNotifications::instance().clear();

	// workaround for DEV-35406 crash on shutdown
	LLEventPumps::instance().reset();

	//dump scene loading monitor results
	if (LLSceneMonitor::instanceExists())
	{
		LLSceneMonitor::instance().dumpToFile(gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "scene_monitor_results.csv"));
	}

	// There used to be an 'if (LLFastTimerView::sAnalyzePerformance)' block
	// here, completely redundant with the one that occurs later in this same
	// function. Presumably the duplication was due to an automated merge gone
	// bad. Not knowing which instance to prefer, we chose to retain the later
	// one because it happens just after mFastTimerLogThread is deleted. This
	// comment is in case we guessed wrong, so we can move it here instead.

	// remove any old breakpad minidump files from the log directory
	if (! isError())
	{
		std::string logdir = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "");
		gDirUtilp->deleteFilesInDir(logdir, "*-*-*-*-*.dmp");
	}

	{
		// Kill off LLLeap objects. We can find them all because LLLeap is derived
		// from LLInstanceTracker. But collect instances first: LLInstanceTracker
		// specifically forbids adding/deleting instances while iterating.
		std::vector<LLLeap*> leaps;
		leaps.reserve(LLLeap::instanceCount());
		for (LLLeap::instance_iter li(LLLeap::beginInstances()), lend(LLLeap::endInstances());
			 li != lend; ++li)
		{
			leaps.push_back(&*li);
		}
		// Okay, now trash them all. We don't have to NULL or erase the entry
		// in 'leaps' because the whole vector is going away momentarily.
		BOOST_FOREACH(LLLeap* leap, leaps)
		{
			delete leap;
		}
	} // destroy 'leaps'

	//flag all elements as needing to be destroyed immediately
	// to ensure shutdown order
	LLMortician::setZealous(TRUE);

    // Give any remaining SLPlugin instances a chance to exit cleanly.
    LLPluginProcessParent::shutdown();

	disconnectViewer();

	LL_INFOS() << "Viewer disconnected" << LL_ENDL;

	display_cleanup();

	release_start_screen(); // just in case

	LLError::logToFixedBuffer(NULL); // stop the fixed buffer recorder

	LL_INFOS() << "Cleaning Up" << LL_ENDL;

	// shut down mesh streamer
	gMeshRepo.shutdown();

	// shut down Havok
	LLPhysicsExtensions::quitSystem();

	// Must clean up texture references before viewer window is destroyed.
	if(LLHUDManager::instanceExists())
	{
		LLHUDManager::getInstance()->updateEffects();
		LLHUDObject::updateAll();
		LLHUDManager::getInstance()->cleanupEffects();
		LLHUDObject::cleanupHUDObjects();
		LL_INFOS() << "HUD Objects cleaned up" << LL_ENDL;
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

	if (LLWorldMap::instanceExists())
	{
		LLWorldMap::getInstance()->reset(); // release any images
	}

	LLCalc::cleanUp();

	LL_INFOS() << "Global stuff deleted" << LL_ENDL;

	if (gAudiop)
	{
        // be sure to stop the internet stream cleanly BEFORE destroying the interface to stop it.
        gAudiop->stopInternetStream();
        // shut down the streaming audio sub-subsystem first, in case it relies on not outliving the general audio subsystem.
        LLStreamingAudioInterface *sai = gAudiop->getStreamingAudioImpl();
		delete sai;
		gAudiop->setStreamingAudioImpl(NULL);

        // shut down the audio subsystem
        gAudiop->shutdown();

		delete gAudiop;
		gAudiop = NULL;
	}

	// Note: this is where LLFeatureManager::getInstance()-> used to be deleted.

	// Patch up settings for next time
	// Must do this before we delete the viewer window,
	// such that we can suck rectangle information out of
	// it.
	cleanupSavedSettings();
	LL_INFOS() << "Settings patched up" << LL_ENDL;

	// delete some of the files left around in the cache.
	removeCacheFiles("*.wav");
	removeCacheFiles("*.tmp");
	removeCacheFiles("*.lso");
	removeCacheFiles("*.out");
	removeCacheFiles("*.dsf");
	removeCacheFiles("*.bodypart");
	removeCacheFiles("*.clothing");

	LL_INFOS() << "Cache files removed" << LL_ENDL;

	// Wait for any pending VFS IO
	flushVFSIO();
	LL_INFOS() << "Shutting down Views" << LL_ENDL;

	// Destroy the UI
	if( gViewerWindow)
		gViewerWindow->shutdownViews();

	LL_INFOS() << "Cleaning up Inventory" << LL_ENDL;

	// Cleanup Inventory after the UI since it will delete any remaining observers
	// (Deleted observers should have already removed themselves)
	gInventory.cleanupInventory();

	LLCoros::getInstance()->printActiveCoroutines();

	LL_INFOS() << "Cleaning up Selections" << LL_ENDL;

	// Clean up selection managers after UI is destroyed, as UI may be observing them.
	// Clean up before GL is shut down because we might be holding on to objects with texture references
	LLSelectMgr::cleanupGlobals();

	LL_INFOS() << "Shutting down OpenGL" << LL_ENDL;

	// Shut down OpenGL
	if( gViewerWindow)
	{
		gViewerWindow->shutdownGL();

		// Destroy window, and make sure we're not fullscreen
		// This may generate window reshape and activation events.
		// Therefore must do this before destroying the message system.
		delete gViewerWindow;
		gViewerWindow = NULL;
		LL_INFOS() << "ViewerWindow deleted" << LL_ENDL;
	}

	LL_INFOS() << "Cleaning up Keyboard & Joystick" << LL_ENDL;

	// viewer UI relies on keyboard so keep it aound until viewer UI isa gone
	delete gKeyboard;
	gKeyboard = NULL;

	// Turn off Space Navigator and similar devices
	LLViewerJoystick::getInstance()->terminate();

	LL_INFOS() << "Cleaning up Objects" << LL_ENDL;

	LLViewerObject::cleanupVOClasses();

	SUBSYSTEM_CLEANUP(LLAvatarAppearance);

	SUBSYSTEM_CLEANUP(LLAvatarAppearance);

	SUBSYSTEM_CLEANUP(LLPostProcess);

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
		LL_WARNS() << "Remaining references in the volume manager!" << LL_ENDL;
	}
	LLPrimitive::cleanupVolumeManager();

	LL_INFOS() << "Additional Cleanup..." << LL_ENDL;

	LLViewerParcelMgr::cleanupGlobals();

	// *Note: this is where gViewerStats used to be deleted.

 	//end_messaging_system();

	LLPrimitive::cleanupVolumeManager();
	SUBSYSTEM_CLEANUP(LLWorldMapView);
	SUBSYSTEM_CLEANUP(LLFolderViewItem);

	//
	// Shut down the VFS's AFTER the decode manager cleans up (since it cleans up vfiles).
	// Also after viewerwindow is deleted, since it may have image pointers (which have vfiles)
	// Also after shutting down the messaging system since it has VFS dependencies

	//
	LL_INFOS() << "Cleaning up VFS" << LL_ENDL;
	SUBSYSTEM_CLEANUP(LLVFile);

	LL_INFOS() << "Saving Data" << LL_ENDL;

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
		LL_INFOS() << "Not saving per-account settings; don't know the account name yet." << LL_ENDL;
	}
	// Only save per account settings if the previous login succeeded, otherwise
	// we might end up with a cleared out settings file in case a previous login
	// failed after loading per account settings.
	else if (!mSavePerAccountSettings)
	{
		LL_INFOS() << "Not saving per-account settings; last login was not successful." << LL_ENDL;
	}
	else
	{
		gSavedPerAccountSettings.saveToFile(gSavedSettings.getString("PerAccountSettingsFile"), TRUE);
		LL_INFOS() << "Saved settings" << LL_ENDL;
	}

	std::string warnings_settings_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, getSettingsFilename("Default", "Warnings"));
	gWarningSettings.saveToFile(warnings_settings_filename, TRUE);

	// Save URL history file
	LLURLHistory::saveFile("url_history.xml");

	// save mute list. gMuteList used to also be deleted here too.
	if (gAgent.isInitialized() && LLMuteList::instanceExists())
	{
		LLMuteList::getInstance()->cache(gAgent.getID());
	}

	//save call log list
	if (LLConversationLog::instanceExists())
	{
		LLConversationLog::instance().cache();
	}

	if (mPurgeOnExit)
	{
		LL_INFOS() << "Purging all cache files on exit" << LL_ENDL;
		gDirUtilp->deleteFilesInDir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE,""), "*.*");
	}

	writeDebugInfo();

	LLLocationHistory::getInstance()->save();

	LLAvatarIconIDCache::getInstance()->save();

	// Stop the plugin read thread if it's running.
	LLPluginProcessParent::setUseReadThread(false);

	LL_INFOS() << "Shutting down Threads" << LL_ENDL;

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
		if(!pending)
		{
			break ; //done
		}
		else if(idle_time >= max_idle_time)
		{
			LL_WARNS() << "Quitting with pending background tasks." << LL_ENDL;
			break;
		}
	}

	// Delete workers first
	// shotdown all worker threads before deleting them in case of co-dependencies
	mAppCoreHttp.requestStop();
	sTextureFetch->shutdown();
	sTextureCache->shutdown();
	sImageDecodeThread->shutdown();

	sTextureFetch->shutDownTextureCacheThread() ;
	sTextureFetch->shutDownImageDecodeThread() ;

	LL_INFOS() << "Shutting down message system" << LL_ENDL;
	end_messaging_system();

	// Non-LLCurl libcurl library
	mAppCoreHttp.cleanup();

	SUBSYSTEM_CLEANUP(LLFilePickerThread);
	SUBSYSTEM_CLEANUP(LLDirPickerThread);

	//MUST happen AFTER SUBSYSTEM_CLEANUP(LLCurl)
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
		LL_INFOS() << "Analyzing performance" << LL_ENDL;

		std::string baseline_name = LLTrace::BlockTimer::sLogName + "_baseline.slp";
		std::string current_name  = LLTrace::BlockTimer::sLogName + ".slp";
		std::string report_name   = LLTrace::BlockTimer::sLogName + "_report.csv";

		LLFastTimerView::doAnalysis(
			gDirUtilp->getExpandedFilename(LL_PATH_LOGS, baseline_name),
			gDirUtilp->getExpandedFilename(LL_PATH_LOGS, current_name),
			gDirUtilp->getExpandedFilename(LL_PATH_LOGS, report_name));
	}

	SUBSYSTEM_CLEANUP(LLMetricPerformanceTesterBasic) ;

	LL_INFOS() << "Cleaning up Media and Textures" << LL_ENDL;

	//Note:
	//SUBSYSTEM_CLEANUP(LLViewerMedia) has to be put before gTextureList.shutdown()
	//because some new image might be generated during cleaning up media. --bao
	gTextureList.shutdown(); // shutdown again in case a callback added something
	LLUIImageList::getInstance()->cleanUp();

	// This should eventually be done in LLAppViewer
	SUBSYSTEM_CLEANUP(LLVFSThread);
	SUBSYSTEM_CLEANUP(LLLFSThread);

#ifndef LL_RELEASE_FOR_DOWNLOAD
	LL_INFOS() << "Auditing VFS" << LL_ENDL;
	if(gVFS)
	{
		gVFS->audit();
	}
#endif

	LL_INFOS() << "Misc Cleanup" << LL_ENDL;

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

	// If we're exiting to launch an URL, do that here so the screen
	// is at the right resolution before we launch IE.
	if (!gLaunchFileOnQuit.empty())
	{
		LL_INFOS() << "Launch file on quit." << LL_ENDL;
#if LL_WINDOWS
		// Indicate an application is starting.
		SetCursor(LoadCursor(NULL, IDC_WAIT));
#endif

		// HACK: Attempt to wait until the screen res. switch is complete.
		ms_sleep(1000);

		LLWeb::loadURLExternal( gLaunchFileOnQuit, false );
		LL_INFOS() << "File launched." << LL_ENDL;
	}
	LL_INFOS() << "Cleaning up LLProxy." << LL_ENDL;
	SUBSYSTEM_CLEANUP(LLProxy);
    LLCore::LLHttp::cleanup();

	LLMainLoopRepeater::instance().stop();

	ll_close_fail_log();

	LLError::LLCallStacks::cleanup();

	removeMarkerFiles();

	// It's not at first obvious where, in this long sequence, generic cleanup
	// calls OUGHT to go. So let's say this: as we migrate cleanup from
	// explicit hand-placed calls into the generic mechanism, eventually
	// all cleanup will get subsumed into the generic calls. So the calls you
	// still see above are calls that MUST happen before the generic cleanup
	// kicks in.

	// This calls every remaining LLSingleton's cleanupSingleton() method.
	// This method should perform any cleanup that might take significant
	// realtime, or might throw an exception.
	LLSingletonBase::cleanupAll();

	// The logging subsystem depends on an LLSingleton. Any logging after
	// LLSingletonBase::deleteAll() won't be recorded.
	LL_INFOS() << "Goodbye!" << LL_ENDL;

	// This calls every remaining LLSingleton's deleteSingleton() method.
	// No class destructor should perform any cleanup that might take
	// significant realtime, or throw an exception.
	LLSingletonBase::deleteAll();

	removeDumpDir();

	// return 0;
	return true;
}

// A callback for LL_ERRS() to call during the watchdog error.
void watchdog_llerrs_callback(const std::string &error_string)
{
	gLLErrorActivated = true;

	gDebugInfo["FatalMessage"] = error_string;
	LLAppViewer::instance()->writeDebugInfo();

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
	LL_ERRS() << "Watchdog killer event" << LL_ENDL;
}

bool LLAppViewer::initThreads()
{
	static const bool enable_threads = true;

	LLImage::initParamSingleton(gSavedSettings.getBOOL("TextureNewByteRange"),gSavedSettings.getS32("TextureReverseByteRange"));

	LLVFSThread::initClass(enable_threads && false);
	LLLFSThread::initClass(enable_threads && false);

	// Image decoding
	LLAppViewer::sImageDecodeThread = new LLImageDecodeThread(enable_threads && true);
	LLAppViewer::sTextureCache = new LLTextureCache(enable_threads && true);
	LLAppViewer::sTextureFetch = new LLTextureFetch(LLAppViewer::getTextureCache(),
													sImageDecodeThread,
													enable_threads && true,
													app_metrics_qa_mode);

	if (LLTrace::BlockTimer::sLog || LLTrace::BlockTimer::sMetricLog)
	{
		LLTrace::BlockTimer::setLogLock(new LLMutex());
		mFastTimerLogThread = new LLFastTimerLogThread(LLTrace::BlockTimer::sLogName);
		mFastTimerLogThread->start();
	}

	// Mesh streaming and caching
	gMeshRepo.init();

	LLFilePickerThread::initClass();
	LLDirPickerThread::initClass();

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

	gDebugInfo["FatalMessage"] = error_string;
	// We're not already crashing -- we simply *intend* to crash. Since we
	// haven't actually trashed anything yet, we can afford to write the whole
	// static info file.
	LLAppViewer::instance()->writeDebugInfo();

	LLError::crashAndLoop(error_string);
}

void LLAppViewer::initLoggingAndGetLastDuration()
{
	//
	// Set up logging defaults for the viewer
	//
	LLError::initForApplication( gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "")
                                ,gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "")
                                );
	LLError::setFatalFunction(errorCallback);
	//LLError::setTimeFunction(getRuntime);

	// Remove the last ".old" log file.
	std::string old_log_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,
							     "SecondLife.old");
	LLFile::remove(old_log_file);

	// Get name of the log file
	std::string log_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,
							     "SecondLife.log");
 	/*
	 * Before touching any log files, compute the duration of the last run
	 * by comparing the ctime of the previous start marker file with the ctime
	 * of the last log file.
	 */
	std::string start_marker_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, START_MARKER_FILE_NAME);
	llstat start_marker_stat;
	llstat log_file_stat;
	std::ostringstream duration_log_stream; // can't log yet, so save any message for when we can below
	int start_stat_result = LLFile::stat(start_marker_file_name, &start_marker_stat);
	int log_stat_result = LLFile::stat(log_file, &log_file_stat);
	if ( 0 == start_stat_result && 0 == log_stat_result )
	{
		int elapsed_seconds = log_file_stat.st_ctime - start_marker_stat.st_ctime;
		// only report a last run time if the last viewer was the same version
		// because this stat will be counted against this version
		if ( markerIsSameVersion(start_marker_file_name) )
		{
			gLastExecDuration = elapsed_seconds;
		}
		else
		{
			duration_log_stream << "start marker from some other version; duration is not reported";
			gLastExecDuration = -1;
		}
	}
	else
	{
		// at least one of the LLFile::stat calls failed, so we can't compute the run time
		duration_log_stream << "duration stat failure; start: "<< start_stat_result << " log: " << log_stat_result;
		gLastExecDuration = -1; // unknown
	}
	std::string duration_log_msg(duration_log_stream.str());

	// Create a new start marker file for comparison with log file time for the next run
	LLAPRFile start_marker_file ;
	start_marker_file.open(start_marker_file_name, LL_APR_WB);
	if (start_marker_file.getFileHandle())
	{
		recordMarkerVersion(start_marker_file);
		start_marker_file.close();
	}

	// Rename current log file to ".old"
	LLFile::rename(log_file, old_log_file);

	// Set the log file to SecondLife.log
	LLError::logToFile(log_file);
	if (!duration_log_msg.empty())
	{
		LL_WARNS("MarkerFile") << duration_log_msg << LL_ENDL;
	}
}

bool LLAppViewer::loadSettingsFromDirectory(const std::string& location_key,
					    bool set_defaults)
{
	if (!mSettingsLocationList)
	{
		LL_ERRS() << "Invalid settings location list" << LL_ENDL;
	}

	BOOST_FOREACH(const SettingsGroup& group, mSettingsLocationList->groups)
	{
		// skip settings groups that aren't the one we requested
		if (group.name() != location_key) continue;

		ELLPath path_index = (ELLPath)group.path_index();
		if(path_index <= LL_PATH_NONE || path_index >= LL_PATH_LAST)
		{
			LL_ERRS() << "Out of range path index in app_settings/settings_files.xml" << LL_ENDL;
			return false;
		}

		BOOST_FOREACH(const SettingsFile& file, group.files)
		{
			LL_INFOS("Settings") << "Attempting to load settings for the group " << file.name()
			    << " - from location " << location_key << LL_ENDL;

			LLControlGroup* settings_group = LLControlGroup::getInstance(file.name);
			if(!settings_group)
			{
				LL_WARNS("Settings") << "No matching settings group for name " << file.name() << LL_ENDL;
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
				LL_INFOS("Settings") << "Loaded settings file " << full_settings_path << LL_ENDL;
			}
			else
			{	// failed to load
				if(file.required)
				{
					LL_ERRS() << "Error: Cannot load required settings file from: " << full_settings_path << LL_ENDL;
					return false;
				}
				else
				{
					// only complain if we actually have a filename at this point
					if (!full_settings_path.empty())
					{
						LL_INFOS("Settings") << "Cannot load " << full_settings_path << " - No settings found." << LL_ENDL;
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

namespace
{
    void handleCommandLineError(LLControlGroupCLP& clp)
    {
		LL_WARNS() << "Error parsing command line options. Command Line options ignored."  << LL_ENDL;

		LL_INFOS() << "Command line usage:\n" << clp << LL_ENDL;

		OSMessageBox(STRINGIZE(LLTrans::getString("MBCmdLineError") << clp.getErrorMessage()),
					 LLStringUtil::null,
					 OSMB_OK);
    }
} // anonymous namespace

bool LLAppViewer::initConfiguration()
{
	//Load settings files list
	std::string settings_file_list = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "settings_files.xml");
	LLXMLNodePtr root;
	BOOL success  = LLXMLNode::parseFile(settings_file_list, root, NULL);
	if (!success)
	{
        LL_ERRS() << "Cannot load default configuration file " << settings_file_list << LL_ENDL;
	}

	mSettingsLocationList = new SettingsFiles();

	LLXUIParser parser;
	parser.readXUI(root, *mSettingsLocationList, settings_file_list);

	if (!mSettingsLocationList->validateBlock())
	{
        LL_ERRS() << "Invalid settings file list " << settings_file_list << LL_ENDL;
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
		OSMessageBox(
			"Unable to load default settings file. The installation may be corrupted.",
			LLStringUtil::null,OSMB_OK);
		return false;
	}

	initStrings(); // setup paths for LLTrans based on settings files only
	// - set procedural settings
	// Note: can't use LL_PATH_PER_SL_ACCOUNT for any of these since we haven't logged in yet
	gSavedSettings.setString("ClientSettingsFile",
        gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, getSettingsFilename("Default", "Global")));

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
		handleCommandLineError(clp);
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
		LL_INFOS("Settings")	<< "Using command line specified settings filename: "
			<< user_settings_filename << LL_ENDL;
	}

	// - load overrides from user_settings
	loadSettingsFromDirectory("User");

	if (gSavedSettings.getBOOL("FirstRunThisInstall"))
	{
		// Set firstrun flag to indicate that some further init actiona should be taken
		// like determining screen DPI value and so on
		mIsFirstRun = true;

		gSavedSettings.setBOOL("FirstRunThisInstall", FALSE);
	}

	if (clp.hasOption("sessionsettings"))
	{
		std::string session_settings_filename = clp.getOption("sessionsettings")[0];
		gSavedSettings.setString("SessionSettingsFile", session_settings_filename);
		LL_INFOS("Settings")	<< "Using session settings filename: "
			<< session_settings_filename << LL_ENDL;
	}
	loadSettingsFromDirectory("Session");

	if (clp.hasOption("usersessionsettings"))
	{
		std::string user_session_settings_filename = clp.getOption("usersessionsettings")[0];
		gSavedSettings.setString("UserSessionSettingsFile", user_session_settings_filename);
		LL_INFOS("Settings") << "Using user session settings filename: "
			<< user_session_settings_filename << LL_ENDL;

	}
	loadSettingsFromDirectory("UserSession");

	// - apply command line settings
	if (! clp.notify())
	{
		handleCommandLineError(clp);
		return false;
	}

	// Register the core crash option as soon as we can
	// if we want gdb post-mortem on cores we need to be up and running
	// ASAP or we might miss init issue etc.
	if(gSavedSettings.getBOOL("DisableCrashLogger"))
	{
		LL_WARNS() << "Crashes will be handled by system, stack trace logs and crash logger are both disabled" << LL_ENDL;
		disableCrashlogger();
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
		LL_INFOS()	<< msg.str() << LL_ENDL;

		OSMessageBox(
			msg.str(),
			LLStringUtil::null,
			OSMB_OK);

		return false;
	}

    if(clp.hasOption("set"))
    {
        const LLCommandLineParser::token_vector_t& set_values = clp.getOption("set");
        if(0x1 & set_values.size())
        {
            LL_WARNS() << "Invalid '--set' parameter count." << LL_ENDL;
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
					LL_INFOS() << "Setting " << group_part << "." << name_part << " to " << value << LL_ENDL;
					LLControlGroup* g = LLControlGroup::getInstance(group_part);
					if (g) control = g->getControl(name_part);
				}
				else
				{
					LL_INFOS() << "Setting Global." << name << " to " << value << LL_ENDL;
					control = gSavedSettings.getControl(name);
				}

                if (control)
                {
                    control->setValue(value, false);
                }
                else
                {
					LL_WARNS() << "Failed --set " << name << ": setting name unknown." << LL_ENDL;
                }
            }
        }
    }

    if  (clp.hasOption("logevents")) {
		LLViewerEventRecorder::instance().setEventLoggingOn();
    }

	std::string CmdLineChannel(gSavedSettings.getString("CmdLineChannel"));
	if(! CmdLineChannel.empty())
    {
		LLVersionInfo::resetChannel(CmdLineChannel);
	}

	// If we have specified crash on startup, set the global so we'll trigger the crash at the right time
	gCrashOnStartup = gSavedSettings.getBOOL("CrashOnStartup");

	if (gSavedSettings.getBOOL("LogPerformance"))
	{
		LLTrace::BlockTimer::sLog = true;
		LLTrace::BlockTimer::sLogName = std::string("performance");
	}

	std::string test_name(gSavedSettings.getString("LogMetrics"));
	if (! test_name.empty())
 	{
		LLTrace::BlockTimer::sMetricLog = TRUE;
		// '--logmetrics' is specified with a named test metric argument so the data gathering is done only on that test
		// In the absence of argument, every metric would be gathered (makes for a rather slow run and hard to decipher report...)
		LL_INFOS() << "'--logmetrics' argument : " << test_name << LL_ENDL;
		LLTrace::BlockTimer::sLogName = test_name;
	}

	if (clp.hasOption("graphicslevel"))
	{
		// User explicitly requested --graphicslevel on the command line. We
		// expect this switch has already set RenderQualityPerformance. Check
		// that value for validity.
		U32 graphicslevel = gSavedSettings.getU32("RenderQualityPerformance");
		if (LLFeatureManager::instance().isValidGraphicsLevel(graphicslevel))
        {
			// graphicslevel is valid: save it and engage it later. Capture
			// the requested value separately from the settings variable
			// because, if this is the first run, LLViewerWindow's constructor
			// will call LLFeatureManager::applyRecommendedSettings(), which
			// overwrites this settings variable!
			mForceGraphicsLevel = graphicslevel;
        }
	}

	LLFastTimerView::sAnalyzePerformance = gSavedSettings.getBOOL("AnalyzePerformance");
	gAgentPilot.setReplaySession(gSavedSettings.getBOOL("ReplaySession"));

	if (gSavedSettings.getBOOL("DebugSession"))
	{
		gDebugSession = TRUE;
		gDebugGL = TRUE;

		ll_init_fail_log(gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "test_failures.log"));
	}

	const LLControlVariable* skinfolder = gSavedSettings.getControl("SkinCurrent");
	if(skinfolder && LLStringUtil::null != skinfolder->getValue().asString())
	{
		// Examining "Language" may not suffice -- see LLUI::getLanguage()
		// logic. Unfortunately LLUI::getLanguage() doesn't yet do us much
		// good because we haven't yet called LLUI::initClass().
		gDirUtilp->setSkinFolder(skinfolder->getValue().asString(),
								 gSavedSettings.getString("Language"));
	}

	if (gSavedSettings.getBOOL("SpellCheck"))
	{
		std::list<std::string> dict_list;
		std::string dict_setting = gSavedSettings.getString("SpellCheckDictionary");
		boost::split(dict_list, dict_setting, boost::is_any_of(std::string(",")));
		if (!dict_list.empty())
		{
			LLSpellChecker::setUseSpellCheck(dict_list.front());
			dict_list.pop_front();
			LLSpellChecker::instance().setSecondaryDictionaries(dict_list);
		}
	}

	// Handle slurl use. NOTE: Don't let SL-55321 reappear.
	// This initial-SLURL logic, up through the call to
	// sendURLToOtherInstance(), must precede LLSplashScreen::show() --
	// because if sendURLToOtherInstance() succeeds, we take a fast exit,
	// SKIPPING the splash screen and everything else.

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

	std::string starting_location;

	std::string cmd_line_login_location(gSavedSettings.getString("CmdLineLoginLocation"));
	if(! cmd_line_login_location.empty())
	{
		starting_location = cmd_line_login_location;
	}
	else
	{
		std::string default_login_location(gSavedSettings.getString("DefaultLoginLocation"));
		if (! default_login_location.empty())
		{
			starting_location = default_login_location;
		}
	}

	LLSLURL start_slurl;
	if (! starting_location.empty())
    {
		start_slurl = starting_location;
		LLStartUp::setStartSLURL(start_slurl);
		if(start_slurl.getType() == LLSLURL::LOCATION)
		{
			LLGridManager::getInstance()->setGridChoice(start_slurl.getGrid());
		}
	}

	// NextLoginLocation is set as a side effect of LLStartUp::setStartSLURL()
	std::string nextLoginLocation = gSavedSettings.getString( "NextLoginLocation" );
	if ( !nextLoginLocation.empty() )
	{
		LL_DEBUGS("AppInit")<<"set start from NextLoginLocation: "<<nextLoginLocation<<LL_ENDL;
		LLStartUp::setStartSLURL(LLSLURL(nextLoginLocation));
	}
	else if (   (   clp.hasOption("login") || clp.hasOption("autologin"))
			 && gSavedSettings.getString("CmdLineLoginLocation").empty())
	{
		// If automatic login from command line with --login switch
		// init StartSLURL location.
		std::string start_slurl_setting = gSavedSettings.getString("LoginLocation");
		LL_DEBUGS("AppInit") << "start slurl setting '" << start_slurl_setting << "'" << LL_ENDL;
		LLStartUp::setStartSLURL(LLSLURL(start_slurl_setting));
	}
	else
	{
		// the login location will be set by the login panel (see LLPanelLogin)
	}

	//RN: if we received a URL, hand it off to the existing instance.
	// don't call anotherInstanceRunning() when doing URL handoff, as
	// it relies on checking a marker file which will not work when running
	// out of different directories

	if (start_slurl.isValid() &&
		(gSavedSettings.getBOOL("SLURLPassToOtherInstance")))
	{
		if (sendURLToOtherInstance(start_slurl.getSLURLString()))
		{
			// successfully handed off URL to existing instance, exit
			return false;
		}
    }

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
	gWindowTitle += std::string(" [DEBUG]");
#endif
	if (!gArgs.empty())
	{
	gWindowTitle += std::string(" ") + gArgs;
	}
	LLStringUtil::truncate(gWindowTitle, 255);

	//
	// Check for another instance of the app running
	// This happens AFTER LLSplashScreen::show(). That may or may not be
	// important.
	//
	if (mSecondInstance && !gSavedSettings.getBOOL("AllowMultipleViewers"))
	{
		OSMessageBox(
			LLTrans::getString("MBAlreadyRunning"),
			LLStringUtil::null,
			OSMB_OK);
		return false;
	}

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

	gLastRunVersion = gSavedSettings.getString("LastRunVersion");

	loadColorSettings();

	// Let anyone else who cares know that we've populated our settings
	// variables.
	for (LLControlGroup::key_iter ki(LLControlGroup::beginKeys()), kend(LLControlGroup::endKeys());
		 ki != kend; ++ki)
	{
		// For each named instance of LLControlGroup, send an event saying
		// we've initialized an LLControlGroup instance by that name.
		LLEventPumps::instance().obtain("LLControlGroup").post(LLSDMap("init", *ki));
	}

	return true; // Config was successful.
}

// The following logic is replicated in initConfiguration() (to be able to get
// some initial strings before we've finished initializing enough to know the
// current language) and also in init() (to initialize for real). Somehow it
// keeps growing, necessitating a method all its own.
void LLAppViewer::initStrings()
{
	std::string strings_file = "strings.xml";
	std::string strings_path_full = gDirUtilp->findSkinnedFilenameBaseLang(LLDir::XUI, strings_file);
	if (strings_path_full.empty() || !LLFile::isfile(strings_path_full))
	{
		// initial check to make sure files are there failed
		gDirUtilp->dumpCurrentDirectories(LLError::LEVEL_WARN);
		LL_ERRS() << "Viewer failed to find localization and UI files. Please reinstall viewer from  https://secondlife.com/support/downloads/ and contact https://support.secondlife.com if issue persists after reinstall." << LL_ENDL;
	}
	LLTransUtil::parseStrings(strings_file, default_trans_args);
	LLTransUtil::parseLanguageStrings("language_settings.xml");

	// parseStrings() sets up the LLTrans substitution table. Add this one item.
	LLTrans::setDefaultArg("[sourceid]", gSavedSettings.getString("sourceid"));

	// Now that we've set "[sourceid]", have to go back through
	// default_trans_args and reinitialize all those other keys because some
	// of them, in turn, reference "[sourceid]".
	BOOST_FOREACH(std::string key, default_trans_args)
	{
		std::string brackets(key), nobrackets(key);
		// Invalid to inspect key[0] if key is empty(). But then, the entire
		// body of this loop is pointless if key is empty().
		if (key.empty())
			continue;

		if (key[0] != '[')
		{
			// key was passed without brackets. That means that 'nobrackets'
			// is correct but 'brackets' is not.
			brackets = STRINGIZE('[' << brackets << ']');
		}
		else
		{
			// key was passed with brackets. That means that 'brackets' is
			// correct but 'nobrackets' is not. Erase the left bracket.
			nobrackets.erase(0, 1);
			std::string::size_type length(nobrackets.length());
			if (length && nobrackets[length - 1] == ']')
			{
				nobrackets.erase(length - 1);
			}
		}
		// Calling LLTrans::getString() is what embeds the other default
		// translation strings into this one.
		LLTrans::setDefaultArg(brackets, LLTrans::getString(nobrackets));
	}
}

//
// This function decides whether the client machine meets the minimum requirements to
// run in a maximized window, per the consensus of davep, boa and nyx on 3/30/2011.
//
bool LLAppViewer::meetsRequirementsForMaximizedStart()
{
	bool maximizedOk = (LLFeatureManager::getInstance()->getGPUClass() >= GPU_CLASS_2);

	maximizedOk &= (gSysMemory.getPhysicalMemoryKB() >= U32Gigabytes(1));

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
		.ignore_pixel_depth(ignorePixelDepth)
		.first_run(mIsFirstRun);

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


#ifdef LL_DARWIN
	//Satisfy both MAINT-3135 (OSX 10.6 and earlier) MAINT-3288 (OSX 10.7 and later)
	LLOSInfo& os_info = LLOSInfo::instance();
	if (os_info.mMajorVer == 10 && os_info.mMinorVer < 7)
	{
		if ( os_info.mMinorVer == 6 && os_info.mBuild < 8 )
			gViewerWindow->getWindow()->setOldResize(true);
	}
#endif

	if (gSavedSettings.getBOOL("WindowMaximized"))
	{
		gViewerWindow->getWindow()->maximize();
	}

	//
	// Initialize GL stuff
	//

	if (mForceGraphicsLevel)
	{
		LLFeatureManager::getInstance()->setGraphicsLevel(*mForceGraphicsLevel, false);
		gSavedSettings.setU32("RenderQualityPerformance", *mForceGraphicsLevel);
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

	LLUI::getInstance()->mWindow = gViewerWindow->getWindow();

	// Show watch cursor
	gViewerWindow->setCursor(UI_CURSOR_WAIT);

	// Finish view initialization
	gViewerWindow->initBase();

	// show viewer window
	//gViewerWindow->getWindow()->show();

	LL_INFOS("AppInit") << "Window initialization done." << LL_ENDL;

	return true;
}

void LLAppViewer::writeDebugInfo(bool isStatic)
{
    //Try to do the minimum when writing data during a crash.
    std::string* debug_filename;
    debug_filename = ( isStatic
        ? getStaticDebugFile()
        : getDynamicDebugFile() );

    LL_INFOS() << "Writing debug file " << *debug_filename << LL_ENDL;
    llofstream out_file(debug_filename->c_str());

    isStatic ?  LLSDSerialize::toPrettyXML(gDebugInfo, out_file)
             :  LLSDSerialize::toPrettyXML(gDebugInfo["Dynamic"], out_file);
}

LLSD LLAppViewer::getViewerInfo() const
{
	// The point of having one method build an LLSD info block and the other
	// construct the user-visible About string is to ensure that the same info
	// is available to a getInfo() caller as to the user opening
	// LLFloaterAbout.
	LLSD info;
	LLSD version;
	version.append(LLVersionInfo::getMajor());
	version.append(LLVersionInfo::getMinor());
	version.append(LLVersionInfo::getPatch());
	version.append(LLVersionInfo::getBuild());
	info["VIEWER_VERSION"] = version;
	info["VIEWER_VERSION_STR"] = LLVersionInfo::getVersion();
	info["CHANNEL"] = LLVersionInfo::getChannel();
    info["ADDRESS_SIZE"] = ADDRESS_SIZE;
    std::string build_config = LLVersionInfo::getBuildConfig();
    if (build_config != "Release")
    {
        info["BUILD_CONFIG"] = build_config;
    }

	// return a URL to the release notes for this viewer, such as:
	// https://releasenotes.secondlife.com/viewer/2.1.0.123456.html
	std::string url = LLTrans::getString("RELEASE_NOTES_BASE_URL");
	if (! LLStringUtil::endsWith(url, "/"))
		url += "/";
	url += LLURI::escape(LLVersionInfo::getVersion()) + ".html";

	info["VIEWER_RELEASE_NOTES_URL"] = url;

	// Position
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		LLVector3d pos = gAgent.getPositionGlobal();
		info["POSITION"] = ll_sd_from_vector3d(pos);
		info["POSITION_LOCAL"] = ll_sd_from_vector3(gAgent.getPosAgentFromGlobal(pos));
		info["REGION"] = gAgent.getRegion()->getName();
		info["HOSTNAME"] = gAgent.getRegion()->getHost().getHostName();
		info["HOSTIP"] = gAgent.getRegion()->getHost().getString();
		info["SERVER_VERSION"] = gLastVersionChannel;
		LLSLURL slurl;
		LLAgentUI::buildSLURL(slurl);
		info["SLURL"] = slurl.getSLURLString();
	}

	// CPU
	info["CPU"] = gSysCPU.getCPUString();
	info["MEMORY_MB"] = LLSD::Integer(gSysMemory.getPhysicalMemoryKB().valueInUnits<LLUnits::Megabytes>());
	// Moved hack adjustment to Windows memory size into llsys.cpp
	info["OS_VERSION"] = LLOSInfo::instance().getOSString();
	info["GRAPHICS_CARD_VENDOR"] = (const char*)(glGetString(GL_VENDOR));
	info["GRAPHICS_CARD"] = (const char*)(glGetString(GL_RENDERER));

#if LL_WINDOWS
	std::string drvinfo = gDXHardware.getDriverVersionWMI();
	if (!drvinfo.empty())
	{
		info["GRAPHICS_DRIVER_VERSION"] = drvinfo;
	}
	else
	{
		LL_WARNS("DriverVersion")<< "Cannot get driver version from getDriverVersionWMI" << LL_ENDL;
		LLSD driver_info = gDXHardware.getDisplayInfo();
		if (driver_info.has("DriverVersion"))
		{
			info["GRAPHICS_DRIVER_VERSION"] = driver_info["DriverVersion"];
		}
	}
#endif

	info["OPENGL_VERSION"] = (const char*)(glGetString(GL_VERSION));

    // Settings

    LLRect window_rect = gViewerWindow->getWindowRectRaw();
    info["WINDOW_WIDTH"] = window_rect.getWidth();
    info["WINDOW_HEIGHT"] = window_rect.getHeight();
    info["FONT_SIZE_ADJUSTMENT"] = gSavedSettings.getF32("FontScreenDPI");
    info["UI_SCALE"] = gSavedSettings.getF32("UIScaleFactor");
    info["DRAW_DISTANCE"] = gSavedSettings.getF32("RenderFarClip");
    info["NET_BANDWITH"] = gSavedSettings.getF32("ThrottleBandwidthKBPS");
    info["LOD_FACTOR"] = gSavedSettings.getF32("RenderVolumeLODFactor");
    info["RENDER_QUALITY"] = (F32)gSavedSettings.getU32("RenderQualityPerformance");
    info["GPU_SHADERS"] = gSavedSettings.getBOOL("RenderDeferred") ? "Enabled" : "Disabled";
    info["TEXTURE_MEMORY"] = gSavedSettings.getS32("TextureMemory");

    LLSD substitution;
    substitution["datetime"] = (S32)(gVFS ? gVFS->creationTime() : 0);
    info["VFS_TIME"] = LLTrans::getString("AboutTime", substitution);

#if LL_DARWIN
    info["HIDPI"] = gHiDPISupport;
#endif

	// Libraries

	info["J2C_VERSION"] = LLImageJ2C::getEngineInfo();
	bool want_fullname = true;
	info["AUDIO_DRIVER_VERSION"] = gAudiop ? LLSD(gAudiop->getDriverName(want_fullname)) : "Undefined";
	if(LLVoiceClient::getInstance()->voiceEnabled())
	{
		LLVoiceVersionInfo version = LLVoiceClient::getInstance()->getVersion();
		std::ostringstream version_string;
		version_string << version.serverType << " " << version.serverVersion << std::endl;
		info["VOICE_VERSION"] = version_string.str();
	}
	else
	{
		info["VOICE_VERSION"] = LLTrans::getString("NotConnected");
	}

#if !LL_LINUX
	std::ostringstream cef_ver_codec;
	cef_ver_codec << "Dullahan: ";
	cef_ver_codec << DULLAHAN_VERSION_MAJOR;
	cef_ver_codec << ".";
	cef_ver_codec << DULLAHAN_VERSION_MINOR;
	cef_ver_codec << ".";
	cef_ver_codec << DULLAHAN_VERSION_BUILD;

	cef_ver_codec << " / CEF: ";
	cef_ver_codec << CEF_VERSION;

	cef_ver_codec << " / Chromium: ";
	cef_ver_codec << CHROME_VERSION_MAJOR;
	cef_ver_codec << ".";
	cef_ver_codec << CHROME_VERSION_MINOR;
	cef_ver_codec << ".";
	cef_ver_codec << CHROME_VERSION_BUILD;
	cef_ver_codec << ".";
	cef_ver_codec << CHROME_VERSION_PATCH;

	info["LIBCEF_VERSION"] = cef_ver_codec.str();
#else
	info["LIBCEF_VERSION"] = "Undefined";
#endif

#if !LL_LINUX
	std::ostringstream vlc_ver_codec;
	vlc_ver_codec << LIBVLC_VERSION_MAJOR;
	vlc_ver_codec << ".";
	vlc_ver_codec << LIBVLC_VERSION_MINOR;
	vlc_ver_codec << ".";
	vlc_ver_codec << LIBVLC_VERSION_REVISION;
	info["LIBVLC_VERSION"] = vlc_ver_codec.str();
#else
	info["LIBVLC_VERSION"] = "Undefined";
#endif

	S32 packets_in = LLViewerStats::instance().getRecording().getSum(LLStatViewer::PACKETS_IN);
	if (packets_in > 0)
	{
		info["PACKETS_LOST"] = LLViewerStats::instance().getRecording().getSum(LLStatViewer::PACKETS_LOST);
		info["PACKETS_IN"] = packets_in;
		info["PACKETS_PCT"] = 100.f*info["PACKETS_LOST"].asReal() / info["PACKETS_IN"].asReal();
	}

	if (mServerReleaseNotesURL.empty())
	{
		if (gAgent.getRegion())
		{
			info["SERVER_RELEASE_NOTES_URL"] = LLTrans::getString("RetrievingData");
		}
		else
		{
			info["SERVER_RELEASE_NOTES_URL"] = LLTrans::getString("NotConnected");
		}
	}
	else if (LLStringUtil::startsWith(mServerReleaseNotesURL, "http")) // it's an URL
	{
		info["SERVER_RELEASE_NOTES_URL"] = "[" + LLWeb::escapeURL(mServerReleaseNotesURL) + " " + LLTrans::getString("ReleaseNotes") + "]";
	}
	else
	{
		info["SERVER_RELEASE_NOTES_URL"] = mServerReleaseNotesURL;
	}

	return info;
}

std::string LLAppViewer::getViewerInfoString(bool default_string) const
{
	std::ostringstream support;

	LLSD info(getViewerInfo());

	// Render the LLSD from getInfo() as a format_map_t
	LLStringUtil::format_map_t args;

	// allow the "Release Notes" URL label to be localized
	args["ReleaseNotes"] = LLTrans::getString("ReleaseNotes", default_string);

	for (LLSD::map_const_iterator ii(info.beginMap()), iend(info.endMap());
		ii != iend; ++ii)
	{
		if (! ii->second.isArray())
		{
			// Scalar value
			if (ii->second.isUndefined())
			{
				args[ii->first] = LLTrans::getString("none_text", default_string);
			}
			else
			{
				// don't forget to render value asString()
				args[ii->first] = ii->second.asString();
			}
		}
		else
		{
			// array value: build KEY_0, KEY_1 etc. entries
			for (LLSD::Integer n(0), size(ii->second.size()); n < size; ++n)
			{
				args[STRINGIZE(ii->first << '_' << n)] = ii->second[n].asString();
			}
		}
	}

	// Now build the various pieces
	support << LLTrans::getString("AboutHeader", args, default_string);
	if (info.has("BUILD_CONFIG"))
	{
		support << "\n" << LLTrans::getString("BuildConfig", args, default_string);
	}
	if (info.has("REGION"))
	{
		support << "\n\n" << LLTrans::getString("AboutPosition", args, default_string);
	}
	support << "\n\n" << LLTrans::getString("AboutSystem", args, default_string);
	support << "\n";
	if (info.has("GRAPHICS_DRIVER_VERSION"))
	{
		support << "\n" << LLTrans::getString("AboutDriver", args, default_string);
	}
	support << "\n" << LLTrans::getString("AboutOGL", args, default_string);
	support << "\n\n" << LLTrans::getString("AboutSettings", args, default_string);
#if LL_DARWIN
	support << "\n" << LLTrans::getString("AboutOSXHiDPI", args, default_string);
#endif
	support << "\n\n" << LLTrans::getString("AboutLibs", args, default_string);
	if (info.has("COMPILER"))
	{
		support << "\n" << LLTrans::getString("AboutCompiler", args, default_string);
	}
	if (info.has("PACKETS_IN"))
	{
		support << '\n' << LLTrans::getString("AboutTraffic", args, default_string);
	}

	// SLT timestamp
	LLSD substitution;
	substitution["datetime"] = (S32)time(NULL);//(S32)time_corrected();
	support << "\n" << LLTrans::getString("AboutTime", substitution, default_string);

	return support.str();
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
	gDirUtilp->deleteFilesInDir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, ""), file_mask);
}

void LLAppViewer::writeSystemInfo()
{

    if (! gDebugInfo.has("Dynamic") )
        gDebugInfo["Dynamic"] = LLSD::emptyMap();

#if LL_WINDOWS
	gDebugInfo["SLLog"] = gDirUtilp->getExpandedFilename(LL_PATH_DUMP,"SecondLife.log");
#else
    //Not ideal but sufficient for good reporting.
    gDebugInfo["SLLog"] = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"SecondLife.old");  //LLError::logFileName();
#endif

	gDebugInfo["ClientInfo"]["Name"] = LLVersionInfo::getChannel();
	gDebugInfo["ClientInfo"]["MajorVersion"] = LLVersionInfo::getMajor();
	gDebugInfo["ClientInfo"]["MinorVersion"] = LLVersionInfo::getMinor();
	gDebugInfo["ClientInfo"]["PatchVersion"] = LLVersionInfo::getPatch();
	gDebugInfo["ClientInfo"]["BuildVersion"] = LLVersionInfo::getBuild();
	gDebugInfo["ClientInfo"]["AddressSize"] = LLVersionInfo::getAddressSize();

	gDebugInfo["CAFilename"] = gDirUtilp->getCAFile();

	gDebugInfo["CPUInfo"]["CPUString"] = gSysCPU.getCPUString();
	gDebugInfo["CPUInfo"]["CPUFamily"] = gSysCPU.getFamily();
	gDebugInfo["CPUInfo"]["CPUMhz"] = (S32)gSysCPU.getMHz();
	gDebugInfo["CPUInfo"]["CPUAltivec"] = gSysCPU.hasAltivec();
	gDebugInfo["CPUInfo"]["CPUSSE"] = gSysCPU.hasSSE();
	gDebugInfo["CPUInfo"]["CPUSSE2"] = gSysCPU.hasSSE2();

	gDebugInfo["RAMInfo"]["Physical"] = (LLSD::Integer)(gSysMemory.getPhysicalMemoryKB().value());
	gDebugInfo["RAMInfo"]["Allocated"] = (LLSD::Integer)(gMemoryAllocated.valueInUnits<LLUnits::Kilobytes>());
	gDebugInfo["OSInfo"] = LLOSInfo::instance().getOSStringSimple();

	// The user is not logged on yet, but record the current grid choice login url
	// which may have been the intended grid.
	gDebugInfo["GridName"] = LLGridManager::getInstance()->getGridId();

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
	LL_INFOS("SystemInfo") << "Application: " << LLTrans::getString("APP_NAME") << LL_ENDL;
	LL_INFOS("SystemInfo") << "Version: " << LLVersionInfo::getChannelAndVersion() << LL_ENDL;

	// Dump the local time and time zone
	time_t now;
	time(&now);
	char tbuffer[256];		/* Flawfinder: ignore */
	strftime(tbuffer, 256, "%Y-%m-%dT%H:%M:%S %Z", localtime(&now));
	LL_INFOS("SystemInfo") << "Local time: " << tbuffer << LL_ENDL;

	// query some system information
	LL_INFOS("SystemInfo") << "CPU info:\n" << gSysCPU << LL_ENDL;
	LL_INFOS("SystemInfo") << "Memory info:\n" << gSysMemory << LL_ENDL;
	LL_INFOS("SystemInfo") << "OS: " << LLOSInfo::instance().getOSStringSimple() << LL_ENDL;
	LL_INFOS("SystemInfo") << "OS info: " << LLOSInfo::instance() << LL_ENDL;

    gDebugInfo["SettingsFilename"] = gSavedSettings.getString("ClientSettingsFile");
	gDebugInfo["ViewerExePath"] = gDirUtilp->getExecutablePathAndName();
	gDebugInfo["CurrentPath"] = gDirUtilp->getCurPath();
	gDebugInfo["FirstLogin"] = (LLSD::Boolean) gAgent.isFirstLogin();
	gDebugInfo["FirstRunThisInstall"] = gSavedSettings.getBOOL("FirstRunThisInstall");
    gDebugInfo["StartupState"] = LLStartUp::getStartupStateString();

	writeDebugInfo(); // Save out debug_info.log early, in case of crash.
}

#ifdef LL_WINDOWS
//For whatever reason, in Windows when using OOP server for breakpad, the callback to get the
//name of the dump file is not getting triggered by the breakpad library.   Unfortunately they
//also didn't see fit to provide a simple query request across the pipe to get this name either.
//Since we are putting our output in a runtime generated directory and we know the header data in
//the dump format, we can however use the following hack to identify our file.
// TODO make this a member function.
void getFileList()
{
	std::stringstream filenames;

	typedef std::vector<std::string> vec;
	std::string pathname = gDirUtilp->getExpandedFilename(LL_PATH_DUMP,"");
	vec file_vec = gDirUtilp->getFilesInDir(pathname);
	for(vec::const_iterator iter=file_vec.begin(); iter!=file_vec.end(); ++iter)
	{
		filenames << *iter << " ";
		if ( ( iter->length() > 30 ) && (iter->rfind(".dmp") == (iter->length()-4) ) )
		{
			std::string fullname = pathname + *iter;
			llifstream fdat( fullname.c_str(), std::ifstream::binary);
			if (fdat)
			{
				char buf[5];
				fdat.read(buf,4);
				fdat.close();
				if (!strncmp(buf,"MDMP",4))
				{
					gDebugInfo["Dynamic"]["MinidumpPath"] = fullname;
					break;
				}
			}
		}
	}
	filenames << std::endl;
	gDebugInfo["Dynamic"]["DumpDirContents"] = filenames.str();
}
#endif

void LLAppViewer::handleViewerCrash()
{
	LL_INFOS("CRASHREPORT") << "Handle viewer crash entry." << LL_ENDL;

	LL_INFOS("CRASHREPORT") << "Last render pool type: " << LLPipeline::sCurRenderPoolType << LL_ENDL ;

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
		gDebugInfo["Dynamic"]["CrashHostUrl"] = crashHostUrl;
	}

	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if ( parcel && parcel->getMusicURL()[0])
	{
		gDebugInfo["Dynamic"]["ParcelMusicURL"] = parcel->getMusicURL();
	}
	if ( parcel && parcel->getMediaURL()[0])
	{
		gDebugInfo["Dynamic"]["ParcelMediaURL"] = parcel->getMediaURL();
	}

	gDebugInfo["Dynamic"]["SessionLength"] = F32(LLFrameTimer::getElapsedSeconds());
	gDebugInfo["Dynamic"]["RAMInfo"]["Allocated"] = LLSD::Integer(LLMemory::getCurrentRSS() / 1024);

	if(gLogoutInProgress)
	{
		gDebugInfo["Dynamic"]["LastExecEvent"] = LAST_EXEC_LOGOUT_CRASH;
	}
	else
	{
		gDebugInfo["Dynamic"]["LastExecEvent"] = gLLErrorActivated ? LAST_EXEC_LLERROR_CRASH : LAST_EXEC_OTHER_CRASH;
	}

	if(gAgent.getRegion())
	{
		gDebugInfo["Dynamic"]["CurrentSimHost"] = gAgent.getRegionHost().getHostName();
		gDebugInfo["Dynamic"]["CurrentRegion"] = gAgent.getRegion()->getName();

		const LLVector3& loc = gAgent.getPositionAgent();
		gDebugInfo["Dynamic"]["CurrentLocationX"] = loc.mV[0];
		gDebugInfo["Dynamic"]["CurrentLocationY"] = loc.mV[1];
		gDebugInfo["Dynamic"]["CurrentLocationZ"] = loc.mV[2];
	}

	if(LLAppViewer::instance()->mMainloopTimeout)
	{
		gDebugInfo["Dynamic"]["MainloopTimeoutState"] = LLAppViewer::instance()->mMainloopTimeout->getState();
	}

	// The crash is being handled here so set this value to false.
	// Otherwise the crash logger will think this crash was a freeze.
	gDebugInfo["Dynamic"]["CrashNotHandled"] = (LLSD::Boolean)false;

	//Write out the crash status file
	//Use marker file style setup, as that's the simplest, especially since
	//we're already in a crash situation
	if (gDirUtilp)
	{
		std::string crash_marker_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,
																			gLLErrorActivated
																			? LLERROR_MARKER_FILE_NAME
																			: ERROR_MARKER_FILE_NAME);
		LLAPRFile crash_marker_file ;
		crash_marker_file.open(crash_marker_file_name, LL_APR_WB);
		if (crash_marker_file.getFileHandle())
		{
			LL_INFOS("MarkerFile") << "Created crash marker file " << crash_marker_file_name << LL_ENDL;
			recordMarkerVersion(crash_marker_file);
		}
		else
		{
			LL_WARNS("MarkerFile") << "Cannot create error marker file " << crash_marker_file_name << LL_ENDL;
		}
	}
	else
	{
		LL_WARNS("MarkerFile") << "No gDirUtilp with which to create error marker file name" << LL_ENDL;
	}

#ifdef LL_WINDOWS
	Sleep(200);
#endif

	char *minidump_file = pApp->getMiniDumpFilename();
    LL_DEBUGS("CRASHREPORT") << "minidump file name " << minidump_file << LL_ENDL;
	if(minidump_file && minidump_file[0] != 0)
	{
		gDebugInfo["Dynamic"]["MinidumpPath"] = minidump_file;
	}
	else
	{
#ifdef LL_WINDOWS
		getFileList();
#else
        LL_WARNS("CRASHREPORT") << "no minidump file?" << LL_ENDL;
#endif
	}
    gDebugInfo["Dynamic"]["CrashType"]="crash";

	if (gMessageSystem && gDirUtilp)
	{
		std::string filename;
		filename = gDirUtilp->getExpandedFilename(LL_PATH_DUMP, "stats.log");
        LL_DEBUGS("CRASHREPORT") << "recording stats " << filename << LL_ENDL;
		llofstream file(filename.c_str(), std::ios_base::binary);
		if(file.good())
		{
			gMessageSystem->summarizeLogs(file);
			file.close();
		}
        else
        {
            LL_WARNS("CRASHREPORT") << "problem recording stats" << LL_ENDL;
        }
	}

	if (gMessageSystem)
	{
		gMessageSystem->getCircuitInfo(gDebugInfo["CircuitInfo"]);
		gMessageSystem->stopLogging();
	}

	if (LLWorld::instanceExists()) LLWorld::getInstance()->getInfo(gDebugInfo["Dynamic"]);

	// Close the debug file
	pApp->writeDebugInfo(false);  //false answers the isStatic question with the least overhead.
}

// static
void LLAppViewer::recordMarkerVersion(LLAPRFile& marker_file)
{
	std::string marker_version(LLVersionInfo::getChannelAndVersion());
	if ( marker_version.length() > MAX_MARKER_LENGTH )
	{
		LL_WARNS_ONCE("MarkerFile") << "Version length ("<< marker_version.length()<< ")"
									<< " greater than maximum (" << MAX_MARKER_LENGTH << ")"
									<< ": marker matching may be incorrect"
									<< LL_ENDL;
	}

	// record the viewer version in the marker file
	marker_file.write(marker_version.data(), marker_version.length());
}

bool LLAppViewer::markerIsSameVersion(const std::string& marker_name) const
{
	bool sameVersion = false;

	std::string my_version(LLVersionInfo::getChannelAndVersion());
	char marker_version[MAX_MARKER_LENGTH];
	S32  marker_version_length;

	LLAPRFile marker_file;
	marker_file.open(marker_name, LL_APR_RB);
	if (marker_file.getFileHandle())
	{
		marker_version_length = marker_file.read(marker_version, sizeof(marker_version));
		std::string marker_string(marker_version, marker_version_length);
		if ( 0 == my_version.compare( 0, my_version.length(), marker_version, 0, marker_version_length ) )
		{
			sameVersion = true;
		}
		LL_DEBUGS("MarkerFile") << "Compare markers for '" << marker_name << "': "
								<< "\n   mine '" << my_version    << "'"
								<< "\n marker '" << marker_string << "'"
								<< "\n " << ( sameVersion ? "same" : "different" ) << " version"
								<< LL_ENDL;
		marker_file.close();
	}
	return sameVersion;
}

void LLAppViewer::processMarkerFiles()
{
	//We've got 4 things to test for here
	// - Other Process Running (SecondLife.exec_marker present, locked)
	// - Freeze (SecondLife.exec_marker present, not locked)
	// - LLError Crash (SecondLife.llerror_marker present)
	// - Other Crash (SecondLife.error_marker present)
	// These checks should also remove these files for the last 2 cases if they currently exist

	bool marker_is_same_version = true;
	// first, look for the marker created at startup and deleted on a clean exit
	mMarkerFileName = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,MARKER_FILE_NAME);
	if (LLAPRFile::isExist(mMarkerFileName, NULL, LL_APR_RB))
	{
		// File exists...
		// first, read it to see if it was created by the same version (we need this later)
		marker_is_same_version = markerIsSameVersion(mMarkerFileName);

		// now test to see if this file is locked by a running process (try to open for write)
		LL_DEBUGS("MarkerFile") << "Checking exec marker file for lock..." << LL_ENDL;
		mMarkerFile.open(mMarkerFileName, LL_APR_WB);
		apr_file_t* fMarker = mMarkerFile.getFileHandle() ;
		if (!fMarker)
		{
			LL_INFOS("MarkerFile") << "Exec marker file open failed - assume it is locked." << LL_ENDL;
			mSecondInstance = true; // lock means that instance is running.
		}
		else
		{
			// We were able to open it, now try to lock it ourselves...
			if (apr_file_lock(fMarker, APR_FLOCK_NONBLOCK | APR_FLOCK_EXCLUSIVE) != APR_SUCCESS)
			{
				LL_WARNS_ONCE("MarkerFile") << "Locking exec marker failed." << LL_ENDL;
				mSecondInstance = true; // lost a race? be conservative
			}
			else
			{
				// No other instances; we've locked this file now, so record our version; delete on quit.
				recordMarkerVersion(mMarkerFile);
				LL_DEBUGS("MarkerFile") << "Exec marker file existed but was not locked; rewritten." << LL_ENDL;
			}
		}

		if (mSecondInstance)
		{
			LL_INFOS("MarkerFile") << "Exec marker '"<< mMarkerFileName << "' owned by another instance" << LL_ENDL;
		}
		else if (marker_is_same_version)
		{
			// the file existed, is ours, and matched our version, so we can report on what it says
			LL_INFOS("MarkerFile") << "Exec marker '"<< mMarkerFileName << "' found; last exec FROZE" << LL_ENDL;
			gLastExecEvent = LAST_EXEC_FROZE;

		}
		else
		{
			LL_INFOS("MarkerFile") << "Exec marker '"<< mMarkerFileName << "' found, but versions did not match" << LL_ENDL;
		}
	}
	else // marker did not exist... last exec (if any) did not freeze
	{
		// Create the marker file for this execution & lock it; it will be deleted on a clean exit
		apr_status_t s;
		s = mMarkerFile.open(mMarkerFileName, LL_APR_WB, TRUE);

		if (s == APR_SUCCESS && mMarkerFile.getFileHandle())
		{
			LL_DEBUGS("MarkerFile") << "Exec marker file '"<< mMarkerFileName << "' created." << LL_ENDL;
			if (APR_SUCCESS == apr_file_lock(mMarkerFile.getFileHandle(), APR_FLOCK_NONBLOCK | APR_FLOCK_EXCLUSIVE))
			{
				recordMarkerVersion(mMarkerFile);
				LL_DEBUGS("MarkerFile") << "Exec marker file locked." << LL_ENDL;
			}
			else
			{
				LL_WARNS("MarkerFile") << "Exec marker file cannot be locked." << LL_ENDL;
			}
		}
		else
		{
			LL_WARNS("MarkerFile") << "Failed to create exec marker file '"<< mMarkerFileName << "'." << LL_ENDL;
		}
	}

	// now check for cases in which the exec marker may have been cleaned up by crash handlers

	// check for any last exec event report based on whether or not it happened during logout
	// (the logout marker is created when logout begins)
	std::string logout_marker_file =  gDirUtilp->getExpandedFilename(LL_PATH_LOGS, LOGOUT_MARKER_FILE_NAME);
	if(LLAPRFile::isExist(logout_marker_file, NULL, LL_APR_RB))
	{
		if (markerIsSameVersion(logout_marker_file))
		{
			gLastExecEvent = LAST_EXEC_LOGOUT_FROZE;
			LL_INFOS("MarkerFile") << "Logout crash marker '"<< logout_marker_file << "', changing LastExecEvent to LOGOUT_FROZE" << LL_ENDL;
		}
		else
		{
			LL_INFOS("MarkerFile") << "Logout crash marker '"<< logout_marker_file << "' found, but versions did not match" << LL_ENDL;
		}
		LLAPRFile::remove(logout_marker_file);
	}
	// further refine based on whether or not a marker created during an llerr crash is found
	std::string llerror_marker_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, LLERROR_MARKER_FILE_NAME);
	if(LLAPRFile::isExist(llerror_marker_file, NULL, LL_APR_RB))
	{
		if (markerIsSameVersion(llerror_marker_file))
		{
			if ( gLastExecEvent == LAST_EXEC_LOGOUT_FROZE )
			{
				gLastExecEvent = LAST_EXEC_LOGOUT_CRASH;
				LL_INFOS("MarkerFile") << "LLError marker '"<< llerror_marker_file << "' crashed, setting LastExecEvent to LOGOUT_CRASH" << LL_ENDL;
			}
			else
			{
				gLastExecEvent = LAST_EXEC_LLERROR_CRASH;
				LL_INFOS("MarkerFile") << "LLError marker '"<< llerror_marker_file << "' crashed, setting LastExecEvent to LLERROR_CRASH" << LL_ENDL;
			}
		}
		else
		{
			LL_INFOS("MarkerFile") << "LLError marker '"<< llerror_marker_file << "' found, but versions did not match" << LL_ENDL;
		}
		LLAPRFile::remove(llerror_marker_file);
	}
	// and last refine based on whether or not a marker created during a non-llerr crash is found
	std::string error_marker_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, ERROR_MARKER_FILE_NAME);
	if(LLAPRFile::isExist(error_marker_file, NULL, LL_APR_RB))
	{
		if (markerIsSameVersion(error_marker_file))
		{
			if (gLastExecEvent == LAST_EXEC_LOGOUT_FROZE)
			{
				gLastExecEvent = LAST_EXEC_LOGOUT_CRASH;
				LL_INFOS("MarkerFile") << "Error marker '"<< error_marker_file << "' crashed, setting LastExecEvent to LOGOUT_CRASH" << LL_ENDL;
			}
			else
			{
				gLastExecEvent = LAST_EXEC_OTHER_CRASH;
				LL_INFOS("MarkerFile") << "Error marker '"<< error_marker_file << "' crashed, setting LastExecEvent to " << gLastExecEvent << LL_ENDL;
			}
		}
		else
		{
			LL_INFOS("MarkerFile") << "Error marker '"<< error_marker_file << "' marker found, but versions did not match" << LL_ENDL;
		}
		LLAPRFile::remove(error_marker_file);
	}
}

void LLAppViewer::removeMarkerFiles()
{
	if (!mSecondInstance)
	{
		if (mMarkerFile.getFileHandle())
		{
			mMarkerFile.close() ;
			LLAPRFile::remove( mMarkerFileName );
			LL_DEBUGS("MarkerFile") << "removed exec marker '"<<mMarkerFileName<<"'"<< LL_ENDL;
		}
		else
		{
			LL_WARNS("MarkerFile") << "marker '"<<mMarkerFileName<<"' not open"<< LL_ENDL;
 		}

		if (mLogoutMarkerFile.getFileHandle())
		{
			mLogoutMarkerFile.close();
			LLAPRFile::remove( mLogoutMarkerFileName );
			LL_DEBUGS("MarkerFile") << "removed logout marker '"<<mLogoutMarkerFileName<<"'"<< LL_ENDL;
		}
		else
		{
			LL_WARNS("MarkerFile") << "logout marker '"<<mLogoutMarkerFileName<<"' not open"<< LL_ENDL;
		}
	}
	else
	{
		LL_WARNS("MarkerFile") << "leaving markers because this is a second instance" << LL_ENDL;
	}
}

void LLAppViewer::removeDumpDir()
{
    //Call this routine only on clean exit.  Crash reporter will clean up
    //its locking table for us.
    std::string dump_dir = gDirUtilp->getExpandedFilename(LL_PATH_DUMP, "");
    gDirUtilp->deleteDirAndContents(dump_dir);
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
	removeMarkerFiles();
	// get outta here
	_exit(final_error_code);
}

void LLAppViewer::requestQuit()
{
	LL_INFOS() << "requestQuit" << LL_ENDL;

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

	// Try to send last batch of avatar rez metrics.
	if (!gDisconnected && isAgentAvatarValid())
	{
		gAgentAvatarp->updateAvatarRezMetrics(true); // force a last packet to be sent.
	}

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
   	LL_WARNS() << "app_early_exit: " << name << LL_ENDL;
	gDoDisconnect = TRUE;
	LLNotificationsUtil::add(name, substitutions, LLSD(), finish_early_exit);
}

// case where we need the viewer to exit without any need for notifications
void LLAppViewer::earlyExitNoNotify()
{
   	LL_WARNS() << "app_early_exit with no notification: " << LL_ENDL;
	gDoDisconnect = TRUE;
	finish_early_exit( LLSD(), LLSD() );
}

void LLAppViewer::abortQuit()
{
    LL_INFOS() << "abortQuit()" << LL_ENDL;
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

		std::string old_cache_dir = gDirUtilp->add(gDirUtilp->getOSUserAppDir(), "cache");
		std::string new_cache_dir = gDirUtilp->getCacheDir(true);

		if (gDirUtilp->fileExists(old_cache_dir))
		{
			LL_INFOS() << "Migrating cache from " << old_cache_dir << " to " << new_cache_dir << LL_ENDL;

			// Migrate inventory cache to avoid pain to inventory database after mass update
			S32 file_count = 0;
			std::string file_name;
			std::string mask = "*.*";

			LLDirIterator iter(old_cache_dir, mask);
			while (iter.next(file_name))
			{
				if (file_name == "." || file_name == "..") continue;
				std::string source_path = gDirUtilp->add(old_cache_dir, file_name);
				std::string dest_path = gDirUtilp->add(new_cache_dir, file_name);
				if (!LLFile::rename(source_path, dest_path))
				{
					file_count++;
				}
			}
			LL_INFOS() << "Moved " << file_count << " files" << LL_ENDL;

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
				LL_WARNS() << "could not delete old cache directory " << old_cache_dir << LL_ENDL;
			}
		}
	}
#endif // LL_WINDOWS || LL_DARWIN
}

void dumpVFSCaches()
{
	LL_INFOS() << "======= Static VFS ========" << LL_ENDL;
	gStaticVFS->listFiles();
#if LL_WINDOWS
	LL_INFOS() << "======= Dumping static VFS to StaticVFSDump ========" << LL_ENDL;
	WCHAR w_str[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, w_str);
	S32 res = LLFile::mkdir("StaticVFSDump");
	if (res == -1)
	{
		LL_WARNS() << "Couldn't create dir StaticVFSDump" << LL_ENDL;
	}
	SetCurrentDirectory(utf8str_to_utf16str("StaticVFSDump").c_str());
	gStaticVFS->dumpFiles();
	SetCurrentDirectory(w_str);
#endif

	LL_INFOS() << "========= Dynamic VFS ====" << LL_ENDL;
	gVFS->listFiles();
#if LL_WINDOWS
	LL_INFOS() << "========= Dumping dynamic VFS to VFSDump ====" << LL_ENDL;
	res = LLFile::mkdir("VFSDump");
	if (res == -1)
	{
		LL_WARNS() << "Couldn't create dir VFSDump" << LL_ENDL;
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
	const U32 TEXTURE_CACHE_VERSION = 8;

	return TEXTURE_CACHE_VERSION ;
}

//static
U32 LLAppViewer::getObjectCacheVersion()
{
	// Viewer object cache version, change if object update
	// format changes. JC
	const U32 INDRA_OBJECT_CACHE_VERSION = 15;

	return INDRA_OBJECT_CACHE_VERSION;
}

bool LLAppViewer::initCache()
{
	mPurgeCache = false;
	BOOL read_only = mSecondInstance ? TRUE : FALSE;
	LLAppViewer::getTextureCache()->setReadOnly(read_only) ;
	LLVOCache::initParamSingleton(read_only);

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
			LL_INFOS("AppCache") << "Startup cache purge requested: " << (gSavedSettings.getBOOL("PurgeCacheOnStartup") ? "ALWAYS" : "ONCE") << LL_ENDL;
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
			LL_INFOS("AppCache") << "Cache location changed, cache needs purging" << LL_ENDL;
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
	const S64 MIN_CACHE_SIZE = 256 * MB;
	const S64 MAX_CACHE_SIZE = 9984ll * MB;
	const S64 MAX_VFS_SIZE = 1024 * MB; // 1 GB

	S64 cache_size = (S64)(gSavedSettings.getU32("CacheSize")) * MB;
	cache_size = llclamp(cache_size, MIN_CACHE_SIZE, MAX_CACHE_SIZE);

	S64 vfs_size = llmin((S64)((cache_size * 2) / 10), MAX_VFS_SIZE);
	S64 texture_cache_size = cache_size - vfs_size;

	S64 extra = LLAppViewer::getTextureCache()->initCache(LL_PATH_CACHE, texture_cache_size, texture_cache_mismatch);
	texture_cache_size -= extra;


	LLVOCache::getInstance()->initCache(LL_PATH_CACHE, gSavedSettings.getU32("CacheNumberOfRegionsForObjects"), getObjectCacheVersion());

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
			old_vfs_data_file = gDirUtilp->add(dir, found_file);

			S32 start_pos = found_file.find_last_of('.');
			if (start_pos > 0)
			{
				sscanf(found_file.substr(start_pos+1).c_str(), "%d", &old_salt);
			}
			LL_DEBUGS("AppCache") << "Default vfs data file not present, found: " << old_vfs_data_file << " Old salt: " << old_salt << LL_ENDL;
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
	std::string browser_cache = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "cef_cache");
	if (LLFile::isdir(browser_cache))
	{
		// cef does not support clear_cache and clear_cookies, so clear what we can manually.
		gDirUtilp->deleteDirAndContents(browser_cache);
	}
	gDirUtilp->deleteFilesInDir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, ""), "*");
}

//purge cache immediately, do not wait until the next login.
void LLAppViewer::purgeCacheImmediate()
{
	LL_INFOS("AppCache") << "Purging Object Cache and Texture Cache immediately..." << LL_ENDL;
	LLAppViewer::getTextureCache()->purgeCache(LL_PATH_CACHE, false);
	LLVOCache::getInstance()->removeCache(LL_PATH_CACHE, true);
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
		snap_filename += LLStartUp::getScreenLastFilename();
		// use full pixel dimensions of viewer window (not post-scale dimensions)
		gViewerWindow->saveSnapshot(snap_filename,
									gViewerWindow->getWindowWidthRaw(),
									gViewerWindow->getWindowHeightRaw(),
									FALSE,
									TRUE,
									LLSnapshotModel::SNAPSHOT_TYPE_COLOR,
									LLSnapshotModel::SNAPSHOT_FORMAT_PNG);
		mSavedFinalSnapshot = TRUE;

		if (gAgent.isInHomeRegion())
		{
			LLVector3d home;
			if (gAgent.getHomePosGlobal(&home) && dist_vec(home, gAgent.getPositionGlobal()) < 10)
			{
				// We are at home position or close to it, see if we need to create home screenshot
				// Notes:
				// 1. It might be beneficial to also replace home if file is too old
				// 2. This is far from best way/place to update screenshot since location might be not fully loaded,
				// but we don't have many options
				std::string snap_home = gDirUtilp->getLindenUserDir();
				snap_home += gDirUtilp->getDirDelimiter();
				snap_home += LLStartUp::getScreenHomeFilename();
				if (!gDirUtilp->fileExists(snap_home))
				{
					// We are at home position yet no home image exist, fix it
					LLFile::copy(snap_filename, snap_home);
				}
			}
		}
	}
}

void LLAppViewer::loadNameCache()
{
	// display names cache
	std::string filename =
		gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "avatar_name_cache.xml");
	LL_INFOS("AvNameCache") << filename << LL_ENDL;
	llifstream name_cache_stream(filename.c_str());
	if(name_cache_stream.is_open())
	{
		if ( ! LLAvatarNameCache::getInstance()->importFile(name_cache_stream))
        {
            LL_WARNS("AppInit") << "removing invalid '" << filename << "'" << LL_ENDL;
            name_cache_stream.close();
            LLFile::remove(filename);
        }
	}

	if (!gCacheName) return;

	std::string name_cache;
	name_cache = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "name.cache");
	llifstream cache_file(name_cache.c_str());
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
	llofstream name_cache_stream(filename.c_str());
	if(name_cache_stream.is_open())
	{
		LLAvatarNameCache::getInstance()->exportFile(name_cache_stream);
    }

    // real names cache
	if (gCacheName)
    {
        std::string name_cache;
        name_cache = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "name.cache");
        llofstream cache_file(name_cache.c_str());
        if(cache_file.is_open())
        {
            gCacheName->exportFile(cache_file);
        }
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

static LLTrace::BlockTimerStatHandle FTM_AUDIO_UPDATE("Update Audio");
static LLTrace::BlockTimerStatHandle FTM_CLEANUP("Cleanup");
static LLTrace::BlockTimerStatHandle FTM_CLEANUP_DRAWABLES("Drawables");
static LLTrace::BlockTimerStatHandle FTM_CLEANUP_OBJECTS("Objects");
static LLTrace::BlockTimerStatHandle FTM_IDLE_CB("Idle Callbacks");
static LLTrace::BlockTimerStatHandle FTM_LOD_UPDATE("Update LOD");
static LLTrace::BlockTimerStatHandle FTM_OBJECTLIST_UPDATE("Update Objectlist");
static LLTrace::BlockTimerStatHandle FTM_REGION_UPDATE("Update Region");
static LLTrace::BlockTimerStatHandle FTM_WORLD_UPDATE("Update World");
static LLTrace::BlockTimerStatHandle FTM_NETWORK("Network");
static LLTrace::BlockTimerStatHandle FTM_AGENT_NETWORK("Agent Network");
static LLTrace::BlockTimerStatHandle FTM_VLMANAGER("VL Manager");
static LLTrace::BlockTimerStatHandle FTM_AGENT_POSITION("Agent Position");
static LLTrace::BlockTimerStatHandle FTM_HUD_EFFECTS("HUD Effects");

///////////////////////////////////////////////////////
// idle()
//
// Called every time the window is not doing anything.
// Receive packets, update statistics, and schedule a redisplay.
///////////////////////////////////////////////////////
void LLAppViewer::idle()
{
	pingMainloopTimeout("Main:Idle");

	// Update frame timers
	static LLTimer idle_timer;

	LLFrameTimer::updateFrameTime();
	LLFrameTimer::updateFrameCount();
	LLEventTimer::updateClass();
	LLNotificationsUI::LLToast::updateClass();
	LLSmoothInterpolation::updateInterpolants();
	LLMortician::updateClass();
	LLFilePickerThread::clearDead();  //calls LLFilePickerThread::notify()
	LLDirPickerThread::clearDead();
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
			LL_INFOS() << "Quitting after " << qas << " seconds. See setting \"QuitAfterSeconds\"." << LL_ENDL;
			LLAppViewer::instance()->forceQuit();
		}
	}

	// Must wait until both have avatar object and mute list, so poll
	// here.
	LLIMProcessing::requestOfflineMessages();

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
		LL_RECORD_BLOCK_TIME(FTM_NETWORK);
		// Update spaceserver timeinfo
	    LLWorld::getInstance()->setSpaceTimeUSec(LLWorld::getInstance()->getSpaceTimeUSec() + LLUnits::Seconds::fromValue(dt_raw));


	    //////////////////////////////////////
	    //
	    // Update simulator agent state
	    //

		if (gSavedSettings.getBOOL("RotateRight"))
		{
			gAgent.moveYaw(-1.f);
		}

		{
			LL_RECORD_BLOCK_TIME(FTM_AGENT_AUTOPILOT);
			// Handle automatic walking towards points
			gAgentPilot.updateTarget();
			gAgent.autoPilot(&yaw);
		}

		static LLFrameTimer agent_update_timer;

		// When appropriate, update agent location to the simulator.
		F32 agent_update_time = agent_update_timer.getElapsedTimeF32();
		F32 agent_force_update_time = mLastAgentForceUpdate + agent_update_time;
		BOOL force_update = gAgent.controlFlagsDirty()
							|| (mLastAgentControlFlags != gAgent.getControlFlags())
							|| (agent_force_update_time > (1.0f / (F32) AGENT_FORCE_UPDATES_PER_SECOND));
		if (force_update || (agent_update_time > (1.0f / (F32) AGENT_UPDATES_PER_SECOND)))
		{
			LL_RECORD_BLOCK_TIME(FTM_AGENT_UPDATE);
			// Send avatar and camera info
			mLastAgentControlFlags = gAgent.getControlFlags();
			mLastAgentForceUpdate = force_update ? 0 : agent_force_update_time;
			send_agent_update(force_update);
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

		// Update session stats every large chunk of time
		// *FIX: (?) SAMANTHA
		if (viewer_stats_timer.getElapsedTimeF32() >= SEND_STATS_PERIOD && !gDisconnected)
		{
			LL_INFOS() << "Transmitting sessions stats" << LL_ENDL;
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
				LL_INFOS() << "Dead object updates: " << gObjectList.mNumDeadObjectUpdates << LL_ENDL;
				gObjectList.mNumDeadObjectUpdates = 0;
			}
			if (gObjectList.mNumUnknownUpdates)
			{
				LL_INFOS() << "Unknown object updates: " << gObjectList.mNumUnknownUpdates << LL_ENDL;
				gObjectList.mNumUnknownUpdates = 0;
			}

		}
	}

	if (!gDisconnected)
	{
		LL_RECORD_BLOCK_TIME(FTM_NETWORK);

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
		update_statistics();
	}

	////////////////////////////////////////
	//
	// Handle the regular UI idle callbacks as well as
	// hover callbacks
	//

#ifdef LL_DARWIN
	if (!mQuitRequested)  //MAINT-4243
#endif
	{
// 		LL_RECORD_BLOCK_TIME(FTM_IDLE_CB);

		// Do event notifications if necessary.  Yes, we may want to move this elsewhere.
		gEventNotifier.update();

		gIdleCallbacks.callFunctions();
		gInventory.idleNotifyObservers();
		LLAvatarTracker::instance().idleNotifyObservers();
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
	if (gTeleportDisplay)
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
		LL_RECORD_BLOCK_TIME(FTM_AGENT_POSITION);
		LLGestureMgr::instance().update();

		gAgent.updateAgentPosition(gFrameDTClamped, yaw, current_mouse.mX, current_mouse.mY);
	}

	{
		LL_RECORD_BLOCK_TIME(FTM_OBJECTLIST_UPDATE);

        if (!(logoutRequestSent() && hasSavedFinalSnapshot()))
		{
			gObjectList.update(gAgent);
		}
	}

	//////////////////////////////////////
	//
	// Deletes objects...
	// Has to be done after doing idleUpdates (which can kill objects)
	//

	{
		LL_RECORD_BLOCK_TIME(FTM_CLEANUP);
		{
			LL_RECORD_BLOCK_TIME(FTM_CLEANUP_OBJECTS);
			gObjectList.cleanDeadObjects();
		}
		{
			LL_RECORD_BLOCK_TIME(FTM_CLEANUP_DRAWABLES);
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
		LL_RECORD_BLOCK_TIME(FTM_HUD_EFFECTS);
		LLSelectMgr::getInstance()->updateEffects();
		LLHUDManager::getInstance()->cleanupEffects();
		LLHUDManager::getInstance()->sendEffects();
	}

	////////////////////////////////////////
	//
	// Unpack layer data that we've received
	//

	{
		LL_RECORD_BLOCK_TIME(FTM_NETWORK);
		gVLManager.unpackData();
	}

	/////////////////////////
	//
	// Update surfaces, and surface textures as well.
	//

	LLWorld::getInstance()->updateVisibilities();
	{
		const F32 max_region_update_time = .001f; // 1ms
		LL_RECORD_BLOCK_TIME(FTM_REGION_UPDATE);
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

	LL_RECORD_BLOCK_TIME(FTM_WORLD_UPDATE);
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
		LL_RECORD_BLOCK_TIME(FTM_LOD_UPDATE);
		gObjectList.updateApparentAngles(gAgent);
	}

	// Update AV render info
	LLAvatarRenderInfoAccountant::getInstance()->idle();

	{
		LL_RECORD_BLOCK_TIME(FTM_AUDIO_UPDATE);

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

	if (gPendingMetricsUploads > 0
		&& gLogoutTimer.getElapsedTimeF32() < SHUTDOWN_UPLOAD_SAVE_TIME
		&& !logoutRequestSent())
	{
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
		//Set internal status variables and marker files before actually starting the logout process
		gLogoutInProgress = TRUE;
		if (!mSecondInstance)
		{
			mLogoutMarkerFileName = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,LOGOUT_MARKER_FILE_NAME);

			mLogoutMarkerFile.open(mLogoutMarkerFileName, LL_APR_WB);
			if (mLogoutMarkerFile.getFileHandle())
			{
				LL_INFOS("MarkerFile") << "Created logout marker file '"<< mLogoutMarkerFileName << "' " << LL_ENDL;
				recordMarkerVersion(mLogoutMarkerFile);
			}
			else
			{
				LL_WARNS("MarkerFile") << "Cannot create logout marker file " << mLogoutMarkerFileName << LL_ENDL;
			}
		}
		else
		{
			LL_INFOS("MarkerFile") << "Did not logout marker file because this is a second instance" << LL_ENDL;
		}

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
	LLAvatarNameCache *name_cache = LLAvatarNameCache::getInstance();
	bool had_capability = LLAvatarNameCache::getInstance()->hasNameLookupURL();
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
		name_cache->setNameLookupURL(name_lookup_url);
	}
	else
	{
		// Display names not available on this region
		name_cache->setNameLookupURL( std::string() );
	}

	// Error recovery - did we change state?
	if (had_capability != have_capability)
	{
		// name tags are persistant on screen, so make sure they refresh
		LLVOAvatar::invalidateNameTags();
	}

	name_cache->idle();
}

//
// Handle messages, and all message related stuff
//

#define TIME_THROTTLE_MESSAGES

#ifdef TIME_THROTTLE_MESSAGES
#define CHECK_MESSAGES_DEFAULT_MAX_TIME .020f // 50 ms = 50 fps (just for messages!)
static F32 CheckMessagesMaxTime = CHECK_MESSAGES_DEFAULT_MAX_TIME;
#endif

static LLTrace::BlockTimerStatHandle FTM_IDLE_NETWORK("Idle Network");
static LLTrace::BlockTimerStatHandle FTM_MESSAGE_ACKS("Message Acks");
static LLTrace::BlockTimerStatHandle FTM_RETRANSMIT("Retransmit");
static LLTrace::BlockTimerStatHandle FTM_TIMEOUT_CHECK("Timeout Check");
static LLTrace::BlockTimerStatHandle FTM_DYNAMIC_THROTTLE("Dynamic Throttle");
static LLTrace::BlockTimerStatHandle FTM_CHECK_REGION_CIRCUIT("Check Region Circuit");

void LLAppViewer::idleNetwork()
{
	pingMainloopTimeout("idleNetwork");

	gObjectList.mNumNewObjects = 0;
	S32 total_decoded = 0;

	if (!gSavedSettings.getBOOL("SpeedTest"))
	{
		LL_RECORD_BLOCK_TIME(FTM_IDLE_NETWORK); // decode

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
		gMessageSystem->processAcks(gSavedSettings.getF32("AckCollectTime"));

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
			LL_INFOS() << "Maxed out number of messages per frame at " << MESSAGE_MAX_PER_FRAME << LL_ENDL;
		}

		if (gPrintMessagesThisFrame)
		{
			LL_INFOS() << "Decoded " << total_decoded << " msgs this frame!" << LL_ENDL;
			gPrintMessagesThisFrame = FALSE;
		}
	}
	add(LLStatViewer::NUM_NEW_OBJECTS, gObjectList.mNumNewObjects);

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

	LL_INFOS() << "Disconnecting viewer!" << LL_ENDL;

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
	if (LLExperienceCache::instanceExists())
	{
		// TODO: LLExperienceCache::cleanup() logic should be moved to
		// cleanupSingleton().
		LLExperienceCache::instance().cleanup();
	}

	// close inventory interface, close all windows
	LLSidepanelInventory::cleanup();

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
	LLVOCache::deleteSingleton();

	// call all self-registered classes
	LLDestroyClassList::instance().fireCallbacks();

	cleanup_xfer_manager();
	gDisconnected = TRUE;

	// Pass the connection state to LLUrlEntryParcel not to attempt
	// parcel info requests while disconnected.
	LLUrlEntryParcel::setDisconnected(gDisconnected);
}

bool LLAppViewer::onChangeFrameLimit(LLSD const & evt)
{
	if (evt.asInteger() > 0)
	{
		mMinMicroSecPerFrame = 1000000 / evt.asInteger();
	}
	else
	{
		mMinMicroSecPerFrame = 0;
	}
	return false;
}

void LLAppViewer::forceErrorLLError()
{
   	LL_ERRS() << "This is a deliberate llerror" << LL_ENDL;
}

void LLAppViewer::forceErrorBreakpoint()
{
   	LL_WARNS() << "Forcing a deliberate breakpoint" << LL_ENDL;
#ifdef LL_WINDOWS
    DebugBreak();
#else
    asm ("int $3");
#endif
    return;
}

void LLAppViewer::forceErrorBadMemoryAccess()
{
   	LL_WARNS() << "Forcing a deliberate bad memory access" << LL_ENDL;
    S32* crash = NULL;
    *crash = 0xDEADBEEF;
    return;
}

void LLAppViewer::forceErrorInfiniteLoop()
{
   	LL_WARNS() << "Forcing a deliberate infinite loop" << LL_ENDL;
    while(true)
    {
        ;
    }
    return;
}

void LLAppViewer::forceErrorSoftwareException()
{
   	LL_WARNS() << "Forcing a deliberate exception" << LL_ENDL;
    LLTHROW(LLException("User selected Force Software Exception"));
}

void LLAppViewer::forceErrorDriverCrash()
{
   	LL_WARNS() << "Forcing a deliberate driver crash" << LL_ENDL;
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
			static LLCachedControl<F32> mainloop_timeout(gSavedSettings, "MainloopTimeoutDefault", 60);
			secs = mainloop_timeout;
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
//		LL_WARNS() << "!!!!!!!!!!!!! Its an error trap!!!!" << state << LL_ENDL;
//	}

	if(mMainloopTimeout)
	{
		if(secs < 0.0f)
		{
			static LLCachedControl<F32> mainloop_timeout(gSavedSettings, "MainloopTimeoutDefault", 60);
			secs = mainloop_timeout;
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

	// we logged in successfully, so save settings on logout
	LL_INFOS() << "Login successful, per account settings will be saved on log out." << LL_ENDL;
	mSavePerAccountSettings=true;
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
		LLViewerAssetStatsFF::set_region(region_handle);
	}
}

/**
 * Attempts to start a multi-threaded metrics report to be sent back to
 * the grid for consumption.
 */
void LLAppViewer::metricsSend(bool enable_reporting)
{
	if (! gViewerAssetStats)
		return;

	if (LLAppViewer::sTextureFetch)
	{
		LLViewerRegion * regionp = gAgent.getRegion();

		if (enable_reporting && regionp)
		{
			std::string	caps_url = regionp->getCapability("ViewerMetrics");

            LLSD sd = gViewerAssetStats->asLLSD(true);

			// Send a report request into 'thread1' to get the rest of the data
			// and provide some additional parameters while here.
			LLAppViewer::sTextureFetch->commandSendMetrics(caps_url,
														   gAgentSessionID,
														   gAgentID,
														   sd);
		}
		else
		{
			LLAppViewer::sTextureFetch->commandDataBreak();
		}
	}

	// Reset even if we can't report.  Rather than gather up a huge chunk of
	// data, we'll keep to our sampling interval and retain the data
	// resolution in time.
	gViewerAssetStats->restart();
}

