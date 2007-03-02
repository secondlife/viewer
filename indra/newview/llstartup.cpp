/** 
 * @file llstartup.cpp
 * @brief startup routines.
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llstartup.h"

#if LL_WINDOWS
#	include <process.h>		// _spawnl()
#else
#	include <sys/stat.h>		// mkdir()
#endif

#include "audioengine.h"

#if LL_FMOD
#include "audioengine_fmod.h"
#endif

#include "audiosettings.h"
#include "llcachename.h"
#include "llviewercontrol.h"
#include "lldir.h"
#include "lleconomy.h"
#include "llerrorcontrol.h"
#include "llfiltersd2xmlrpc.h"
#include "llfocusmgr.h"
#include "imageids.h"
#include "lllandmark.h"
#include "llloginflags.h"
#include "llmd5.h"
#include "llmemorystream.h"
#include "llregionhandle.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llsecondlifeurls.h"
#include "llstring.h"
#include "lluserrelations.h"
#include "llversion.h"
#include "llvfs.h"
#include "llwindow.h"		// for shell_open
#include "llxorcipher.h"	// saved password, MAC address
#include "message.h"
#include "v3math.h"

#include "llagent.h"
#include "llagentpilot.h"
#include "llasynchostbyname.h"
#include "llfloateravatarpicker.h"
#include "llcallbacklist.h"
#include "llcallingcard.h"
#include "llcolorscheme.h"
#include "llconsole.h"
#include "llcontainerview.h"
#include "lldebugview.h"
#include "lldrawable.h"
#include "lleventnotifier.h"
#include "llface.h"
#include "llfeaturemanager.h"
#include "llfloaterchat.h"
#include "llfloatergesture.h"
#include "llfloaterland.h"
#include "llfloatertopobjects.h"
#include "llfloaterrate.h"
#include "llfloatertos.h"
#include "llfloaterworldmap.h"
#include "llframestats.h"
#include "llframestatview.h"
#include "llgesturemgr.h"
#include "llgroupmgr.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llhttpclient.h"
#include "llimagebmp.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"
#include "llkeyboard.h"
#include "llpanellogin.h"
#include "llmutelist.h"
#include "llnotify.h"
#include "llpanelavatar.h"
#include "llpaneldirbrowser.h"
#include "llpaneldirland.h"
#include "llpanelevent.h"
#include "llpanelclassified.h"
#include "llpanelpick.h"
#include "llpanelplace.h"
#include "llpanelgrouplandmoney.h"
#include "llpanelgroupnotices.h"
#include "llpreview.h"
#include "llpreviewscript.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "llstatview.h"
#include "llsurface.h"
#include "lltoolmgr.h"
#include "llui.h"
#include "llurlwhitelist.h"
#include "lluserauth.h"
#include "llviewerassetstorage.h"
#include "llviewercamera.h"
#include "llviewerdisplay.h"
#include "llviewergesture.h"
#include "llviewerimagelist.h"
#include "llviewermenu.h"
#include "llviewermessage.h"
#include "llviewernetwork.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerthrottle.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvoclouds.h"
#include "llworld.h"
#include "llworldmap.h"
#include "llxfermanager.h"
#include "pipeline.h"
#include "viewer.h"
#include "llmediaengine.h"
#include "llfasttimerview.h"
#include "llweb.h"
#include "llfloaterhtml.h"

#if LL_LIBXUL_ENABLED
#include "llmozlib.h"
#endif // LL_LIBXUL_ENABLED

#if LL_WINDOWS
#include "llwindebug.h"
#include "lldxhardware.h"
#endif

#if LL_QUICKTIME_ENABLED
#if LL_DARWIN
#include <QuickTime/QuickTime.h>
#else
// quicktime specific includes
#include "MacTypes.h"
#include "QTML.h"
#include "Movies.h"
#include "FixMath.h"
#endif
#endif

//
// exported globals
//

// HACK: Allow server to change sun and moon IDs.
// I can't figure out how to pass the appropriate
// information into the LLVOSky constructor.  JC
LLUUID gSunTextureID = IMG_SUN;
LLUUID gMoonTextureID = IMG_MOON;
LLUUID gCloudTextureID = IMG_CLOUD_POOF;

const char* SCREEN_HOME_FILENAME = "screen_home.bmp";
const char* SCREEN_LAST_FILENAME = "screen_last.bmp";

//
// Imported globals
//
extern LLPointer<LLImageGL> gStartImageGL;
extern S32 gStartImageWidth;
extern S32 gStartImageHeight;
extern std::string gSerialNumber;

//
// local globals
//

static LLHost gAgentSimHost;
static BOOL gSkipOptionalUpdate = FALSE;

bool gQuickTimeInitialized = false;
static bool gGotUseCircuitCodeAck = false;
LLString gInitialOutfit;
LLString gInitialOutfitGender;	// "male" or "female"


//
// local function declaration
//

void login_show();
void login_callback(S32 option, void* userdata);
LLString load_password_from_disk();
void save_password_to_disk(const char* hashed_password);
BOOL is_hex_string(U8* str, S32 len);
void show_first_run_dialog();
void first_run_dialog_callback(S32 option, void* userdata);
void set_startup_status(const F32 frac, const char* string, const char* msg);
void on_userserver_name_resolved( BOOL success, const LLString& host_name, U32 ip, void* userdata );
void login_alert_status(S32 option, void* user_data);
void update_app(BOOL mandatory, const std::string& message);
void update_dialog_callback(S32 option, void *userdata);
void login_packet_failed(void**, S32 result);
void use_circuit_callback(void**, S32 result);
void register_viewer_callbacks(LLMessageSystem* msg);
void init_stat_view();
void asset_callback_nothing(LLVFS*, const LLUUID&, LLAssetType::EType, void*, S32);
void dialog_choose_gender_first_start();
void callback_choose_gender(S32 option, void* userdata);
void init_start_screen(S32 location_id);
void release_start_screen();
void process_connect_to_userserver(LLMessageSystem* msg, void**);


//
// exported functionality
//

//
// local classes
//
class LLGestureInventoryFetchObserver : public LLInventoryFetchObserver
{
public:
	LLGestureInventoryFetchObserver() {}
	virtual void done()
	{
		// we've downloaded all the items, so repaint the dialog
		LLFloaterGesture::refreshAll();

		gInventory.removeObserver(this);
		delete this;
	}
};



// Returns FALSE to skip other idle processing. Should only return
// TRUE when all initialization done.
BOOL idle_startup()
{
	LLMemType mt1(LLMemType::MTYPE_STARTUP);
	
	const F32 PRECACHING_DELAY = gSavedSettings.getF32("PrecachingDelay");
	const F32 TIMEOUT_SECONDS = 5.f;
	const S32 MAX_TIMEOUT_COUNT = 3;
	static LLTimer timeout;
	static S32 timeout_count = 0;

	static LLTimer login_time;

	// until this is encapsulated, this little hack for the
	// auth/transform loop will do.
	static F32 progress = 0.10f;

	static std::string auth_uri;
	static std::string auth_method;
	static std::string auth_desc;
	static std::string auth_message;
	static LLString firstname;
	static LLString lastname;
	static LLString password;
	static std::vector<const char*> requested_options;

	static U32 region_size = 256;
	static F32 region_scale = 1.f;
	static U64 first_sim_handle = 0;
	static LLHost first_sim;
	static std::string first_sim_seed_cap;

	static LLVector3 initial_sun_direction(1.f, 0.f, 0.f);
	static LLVector3 agent_start_position_region(10.f, 10.f, 10.f);		// default for when no space server
	static LLVector3 agent_start_look_at(1.0f, 0.f, 0.f);
	static std::string agent_start_location = "safe";

	// last location by default
	static S32  agent_location_id = START_LOCATION_ID_LAST;
	static S32  location_which = START_LOCATION_ID_LAST;

	static BOOL show_connect_box = TRUE;
	static BOOL remember_password = TRUE;

	static BOOL stipend_since_login = FALSE;

	static BOOL samename = FALSE;

	static BOOL did_precache = FALSE;
	
	BOOL do_normal_idle = FALSE;

	// HACK: These are things from the main loop that usually aren't done
	// until initialization is complete, but need to be done here for things
	// to work.
	gIdleCallbacks.callFunctions();
	gViewerWindow->handlePerFrameHover();
	LLMortician::updateClass();

	if (gNoRender)
	{
		// HACK, skip optional updates if you're running drones
		gSkipOptionalUpdate = TRUE;
	}
	else
	{
		// Update images?
		gImageList.updateImages(0.01f);
	}

	if (STATE_FIRST == gStartupState)
	{
		gViewerWindow->showCursor();
		gViewerWindow->getWindow()->setCursor(UI_CURSOR_WAIT);

		/////////////////////////////////////////////////
		//
		// Initialize stuff that doesn't need data from userserver/simulators
		//

		if (gFeatureManagerp->isSafe())
		{
			gViewerWindow->alertXml("DisplaySetToSafe");
		}
		else if (gSavedSettings.getS32("LastFeatureVersion") < gFeatureManagerp->getVersion())
		{
			if (gSavedSettings.getS32("LastFeatureVersion") != 0)
			{
				gViewerWindow->alertXml("DisplaySetToRecommended");
			}
		}
		gSavedSettings.setS32("LastFeatureVersion", gFeatureManagerp->getVersion());

		LLString xml_file = LLUI::locateSkin("xui_version.xml");
		LLXMLNodePtr root;
		bool xml_ok = false;
		if (LLXMLNode::parseFile(xml_file, root, NULL))
		{
			if( (root->hasName("xui_version") ) )
			{
				LLString value = root->getValue();
				F32 version = 0.0f;
				LLString::convertToF32(value, version);
				if (version >= 1.0f)
				{
					xml_ok = true;
				}
			}
		}
		if (!xml_ok)
		{
			// XUI:translate (maybe - very unlikely error message)
			// Note: alerts.xml may be invalid - if this gets translated it will need to be in the code
			LLString bad_xui_msg = "An error occured while updating Second Life. Please download the latest version from www.secondlife.com.";
			app_early_exit(bad_xui_msg);
		}
		//
		// Statistics stuff
		//

		// Load autopilot and stats stuff
		gAgentPilot.load(gSavedSettings.getString("StatsPilotFile").c_str());
		gFrameStats.setFilename(gSavedSettings.getString("StatsFile"));
		gFrameStats.setSummaryFilename(gSavedSettings.getString("StatsSummaryFile"));

		//gErrorStream.setTime(gSavedSettings.getBOOL("LogTimestamps"));

		// Load the throttle settings
		gViewerThrottle.load();

		//
		// Initialize messaging system
		//
		llinfos << "Initializing messaging system..." << llendl;

		std::string message_template_path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"message_template.msg");

		FILE* found_template = NULL;
		found_template = LLFile::fopen(message_template_path.c_str(), "r");		/* Flawfinder: ignore */
		if (found_template)
		{
			fclose(found_template);

			U32 port = gAgent.mViewerPort;

			if ((NET_USE_OS_ASSIGNED_PORT == port) &&   // if nothing specified on command line (-port)
			    (gSavedSettings.getBOOL("ConnectionPortEnabled")))
			  {
			    port = gSavedSettings.getU32("ConnectionPort");
			  }

			if(!start_messaging_system(
				   message_template_path,
				   port,
				   LL_VERSION_MAJOR,
				   LL_VERSION_MINOR,
				   LL_VERSION_PATCH,
				   FALSE,
				   std::string()))
			{
				std::string msg = llformat("Unable to start networking, error %d", gMessageSystem->getErrorCode());
				app_early_exit(msg);
			}
		}
		else
		{
			app_early_exit("Unable to initialize communications.");
		}

		if(gMessageSystem && gMessageSystem->isOK())
		{
			// Initialize all of the callbacks in case of bad message
			// system data
			LLMessageSystem* msg = gMessageSystem;
			msg->setExceptionFunc(MX_UNREGISTERED_MESSAGE,
								  invalid_message_callback,
								  NULL);
			msg->setExceptionFunc(MX_PACKET_TOO_SHORT,
								  invalid_message_callback,
								  NULL);
			msg->setExceptionFunc(MX_RAN_OFF_END_OF_PACKET,
								  invalid_message_callback,
								  NULL);
			msg->setExceptionFunc(MX_WROTE_PAST_BUFFER_SIZE,
								  invalid_message_callback,
								  NULL);

			if (gSavedSettings.getBOOL("LogMessages") || gLogMessages)
			{
				llinfos << "Message logging activated!" << llendl;
				msg->startLogging();
			}

			// start the xfer system. by default, choke the downloads
			// a lot...
			const S32 VIEWER_MAX_XFER = 3;
			start_xfer_manager(gVFS);
			gXferManager->setMaxIncomingXfers(VIEWER_MAX_XFER);
			F32 xfer_throttle_bps = gSavedSettings.getF32("XferThrottle");
			if (xfer_throttle_bps > 1.f)
			{
				gXferManager->setUseAckThrottling(TRUE);
				gXferManager->setAckThrottleBPS(xfer_throttle_bps);
			}
			gAssetStorage = new LLViewerAssetStorage(msg, gXferManager, gVFS, gUserServer);

			msg->mPacketRing.setDropPercentage(gPacketDropPercentage);
			if (gInBandwidth != 0.f)
			{
				llinfos << "Setting packetring incoming bandwidth to " << gInBandwidth << llendl;
				msg->mPacketRing.setUseInThrottle(TRUE);
				msg->mPacketRing.setInBandwidth(gInBandwidth);
			}
			if (gOutBandwidth != 0.f)
			{
				llinfos << "Setting packetring outgoing bandwidth to " << gOutBandwidth << llendl;
				msg->mPacketRing.setUseOutThrottle(TRUE);
				msg->mPacketRing.setOutBandwidth(gOutBandwidth);
			}
		}

		// initialize the economy
		gGlobalEconomy = new LLGlobalEconomy();

		//---------------------------------------------------------------------
		// LibXUL (Mozilla) initialization
		//---------------------------------------------------------------------
		#if LL_LIBXUL_ENABLED
		set_startup_status(0.48f, "Initializing embedded web browser...", gAgent.mMOTD.c_str());
		display_startup();
		llinfos << "Initializing embedded web browser..." << llendl;

		#if LL_DARWIN
			// For Mac OS, we store both the shared libraries and the runtime files (chrome/, plugins/, etc) in
			// Second Life.app/Contents/MacOS/.  This matches the way Firefox is distributed on the Mac.
			std::string profileBaseDir(gDirUtilp->getExecutableDir());
		#elif LL_WINDOWS
			std::string profileBaseDir( gDirUtilp->getExpandedFilename( LL_PATH_APP_SETTINGS, "" ) );
			profileBaseDir += gDirUtilp->getDirDelimiter();
			#ifdef LL_DEBUG
			profileBaseDir += "mozilla_debug";
			#else
			profileBaseDir += "mozilla";
			#endif
                #else
			std::string profileBaseDir( gDirUtilp->getExpandedFilename( LL_PATH_APP_SETTINGS, "" ) );
			profileBaseDir += gDirUtilp->getDirDelimiter();
			profileBaseDir += "mozilla";
		#endif

#if LL_LINUX
		// Yuck, Mozilla init plays with the locale - push/pop
		// the locale to protect it, as exotic/non-C locales
		// causes our code lots of general critical weirdness
		// and crashness. (SL-35450)
		char *saved_locale = setlocale(LC_ALL, NULL);
#endif // LL_LINUX
		LLMozLib::getInstance()->init( profileBaseDir, gDirUtilp->getExpandedFilename( LL_PATH_MOZILLA_PROFILE, "" ) );
#if LL_LINUX
		if (saved_locale)
			setlocale(LC_ALL, saved_locale);
#endif // LL_LINUX

		std::ostringstream codec;
		codec << "[Second Life " << LL_VERSION_MAJOR << "." << LL_VERSION_MINOR << "." << LL_VERSION_PATCH << "." << LL_VERSION_BUILD << "]";
		LLMozLib::getInstance()->setBrowserAgentId( codec.str() );
		#endif

		//-------------------------------------------------
		// Init audio, which may be needed for prefs dialog
		// or audio cues in connection UI.
		//-------------------------------------------------

		if (gUseAudio)
		{
#if LL_FMOD
			gAudiop = (LLAudioEngine *) new LLAudioEngine_FMOD();
#else
			gAudiop = NULL;
#endif

			if (gAudiop)
			{
#if LL_WINDOWS
				// FMOD on Windows needs the window handle to stop playing audio
				// when window is minimized. JC
				void* window_handle = (HWND)gViewerWindow->getPlatformWindow();
#else
				void* window_handle = NULL;
#endif
				BOOL init = gAudiop->init(kAUDIO_NUM_SOURCES, window_handle);
				if(!init)
				{
					llwarns << "Unable to initialize audio engine" << llendl;
				}
				gAudiop->setMuted(TRUE);
			}
		}

		if (LLTimer::knownBadTimer())
		{
			llwarns << "Unreliable timers detected (may be bad PCI chipset)!!" << llendl;
		}

		// Get ready to show the login dialog
		if (!gConnectToSomething)
		{
			// Don't use a session token, and generate a random user id
			gAgentID.generate();
			gAgentSessionID = LLUUID::null;

			gStartupState = STATE_WORLD_INIT;
			return do_normal_idle;
		}
		else if (!gRunLocal)
		{
			//
			// Log on to userserver
			//
			if( !gCmdLineFirstName.empty() 
				&& !gCmdLineLastName.empty() 
				&& !gCmdLinePassword.empty())
			{
				firstname = gCmdLineFirstName;
				lastname = gCmdLineLastName;

				LLMD5 pass((unsigned char*)gCmdLinePassword.c_str());
				char md5pass[33];		/* Flawfinder: ignore */
				pass.hex_digest(md5pass);
				password = md5pass;

				remember_password = gSavedSettings.getBOOL("RememberPassword");
				show_connect_box = FALSE;
			}
			else if (gAutoLogin || gSavedSettings.getBOOL("AutoLogin"))
			{
				firstname = gSavedSettings.getString("FirstName");
				lastname = gSavedSettings.getString("LastName");
				password = load_password_from_disk();
				remember_password = TRUE;
				show_connect_box = FALSE;
			}
			else
			{
				// if not automatically logging in, display login dialog
				// until a valid userserver is selected
				firstname = gSavedSettings.getString("FirstName");
				lastname = gSavedSettings.getString("LastName");
				password = load_password_from_disk();
				remember_password = gSavedSettings.getBOOL("RememberPassword");
				show_connect_box = TRUE;
			}

			// Go to the next startup state
			gStartupState++;
			return do_normal_idle;
		}
		else
		{
			gStartupState++;
			return do_normal_idle;
		}
	}

	if (STATE_LOGIN_SHOW == gStartupState)
	{
		gViewerWindow->getWindow()->setCursor(UI_CURSOR_ARROW);
		// Push our window frontmost
		gViewerWindow->getWindow()->show();

		timeout_count = 0;

		if (gConnectToSomething && !gRunLocal)
		{
			if (show_connect_box)
			{
				if (gNoRender)
				{
					llerrs << "Need to autologin or use command line with norender!" << llendl;
				}
				// Make sure the process dialog doesn't hide things
				gViewerWindow->setShowProgress(FALSE);

				// Show the login dialog
				login_show();

				// connect dialog is already shown, so fill in the names
				LLPanelLogin::setFields( firstname, lastname, password, remember_password );
				LLPanelLogin::giveFocus();

				gSavedSettings.setBOOL("FirstRunThisInstall", FALSE);

				gStartupState++;
			}
			else
			{
				// skip directly to message template verification
				gStartupState = STATE_LOGIN_CLEANUP;
			}
		}
		else
		{
			gMessageSystem->setCircuitProtection(FALSE);
			gStartupState = STATE_LOGIN_CLEANUP;
		}

		timeout.reset();
		return do_normal_idle;
	}

	if (STATE_LOGIN_WAIT == gStartupState)
	{
		// Don't do anything.  Wait for the login view to call the login_callback,
		// which will push us to the next state.

		// Sleep so we don't spin the CPU
		ms_sleep(1);
		return do_normal_idle;
	}

	if (STATE_LOGIN_CLEANUP == gStartupState)
	{
		if (show_connect_box)
		{
			// Load all the name information out of the login view
			LLPanelLogin::getFields(firstname, lastname, password, remember_password);

			// HACK: Try to make not jump on login
			gKeyboard->resetKeys();
		}

		if (!firstname.empty() && !lastname.empty())
		{
			gSavedSettings.setString("FirstName", firstname);
			gSavedSettings.setString("LastName", lastname);

			llinfos << "Attempting login as: " << firstname << " " << lastname << llendl;
			write_debug("Attempting login as: ");
			write_debug(firstname);
			write_debug(" ");
			write_debug(lastname);
			write_debug("\n");
		}

		// create necessary directories
		// *FIX: these mkdir's should error check
		gDirUtilp->setLindenUserDir(firstname.c_str(), lastname.c_str());


		LLFile::mkdir(gDirUtilp->getLindenUserDir().c_str());

		// the mute list is loaded in the llmutelist class.

		gSavedSettings.loadFromFile(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT,"overrides.xml"));

		// handle the per account settings setup
		gPerAccountSettingsFileName = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, DEFAULT_SETTINGS_FILE);

		// per account settings.  Set defaults here if not found.  If we get a bunch of these, eventually move to a function.
		gSavedPerAccountSettings.loadFromFile(gPerAccountSettingsFileName);

		// Need to set the LastLogoff time here if we don't have one.  LastLogoff is used for "Recent Items" calculation
		// and startup time is close enough if we don't have a real value.
		if (gSavedPerAccountSettings.getU32("LastLogoff") == 0)
		{
			gSavedPerAccountSettings.setU32("LastLogoff", time_corrected());
		}

		//Default the path if one isn't set.
		if (gSavedPerAccountSettings.getString("InstantMessageLogPath").empty())
		{
			gDirUtilp->setChatLogsDir(gDirUtilp->getOSUserAppDir());
			gSavedPerAccountSettings.setString("InstantMessageLogPath",gDirUtilp->getChatLogsDir());
		}
		else
		{
			gDirUtilp->setChatLogsDir(gSavedPerAccountSettings.getString("InstantMessageLogPath"));		
		}
		
		gDirUtilp->setPerAccountChatLogsDir(firstname.c_str(), lastname.c_str());

		LLFile::mkdir(gDirUtilp->getChatLogsDir().c_str());
		LLFile::mkdir(gDirUtilp->getPerAccountChatLogsDir().c_str());

		
#if LL_WINDOWS
		if (gSavedSettings.getBOOL("UseDebugLogin") && show_connect_box)
#else
		if (show_connect_box)
#endif
		{
			LLString server_label;
			S32 domain_name_index;
			LLPanelLogin::getServer( server_label, domain_name_index );
			gUserServerChoice = (EUserServerDomain) domain_name_index;
			gSavedSettings.setS32("ServerChoice", gUserServerChoice);
			if (gUserServerChoice == USERSERVER_OTHER)
			{
				gUserServer.setHostByName( server_label.c_str() );
				snprintf(gUserServerName, MAX_STRING, "%s", server_label.c_str());		/* Flawfinder: ignore */
			}
		}

		if (show_connect_box)
		{
			LLString location;
			LLPanelLogin::getLocation( location );
			LLURLSimString::setString( location );
			LLPanelLogin::close();
		}

		//For HTML parsing in text boxes.
		LLTextEditor::setLinkColor( gSavedSettings.getColor4("HTMLLinkColor") );
		LLTextEditor::setURLCallbacks ( &LLWeb::loadURL, &process_secondlife_url );

		//-------------------------------------------------
		// Handle startup progress screen
		//-------------------------------------------------

		// on startup the user can request to go to their home,
		// their last location, or some URL "-url //sim/x/y[/z]"
		// All accounts have both a home and a last location, and we don't support
		// more locations than that.  Choose the appropriate one.  JC
		if (LLURLSimString::parse())
		{
			// a startup URL was specified
			agent_location_id = START_LOCATION_ID_TELEHUB;

			// doesn't really matter what location_which is, since
			// agent_start_look_at will be overwritten when the
			// UserLoginLocationReply arrives
			location_which = START_LOCATION_ID_LAST;
		}
		else if (gSavedSettings.getBOOL("LoginLastLocation"))
		{
			agent_location_id = START_LOCATION_ID_LAST;	// last location
			location_which = START_LOCATION_ID_LAST;
		}
		else
		{
			agent_location_id = START_LOCATION_ID_HOME;	// home
			location_which = START_LOCATION_ID_HOME;
		}

		gViewerWindow->getWindow()->setCursor(UI_CURSOR_WAIT);

		if (!gNoRender)
		{
			init_start_screen(agent_location_id);
		}

		// Display the startup progress bar.
		gViewerWindow->setShowProgress(TRUE);
		gViewerWindow->setProgressCancelButtonVisible(TRUE, "Quit");

		// Poke the VFS, which could potentially block for a while if
		// Windows XP is acting up
		set_startup_status(0.05f, "Verifying cache files (can take 60-90 seconds)...", NULL);
		display_startup();

		gVFS->pokeFiles();

		// color init must be after saved settings loaded
		init_colors();

		// Request userserver domain name
		set_startup_status(0.05f, "Finding Server Domain Name...", NULL);

		// We're prematurely switching out of this state because the
		// userserver name resolver can potentiallly occur before reaching the end of the
		// switch statement.  Also, if it's done at the bottom, sometimes we will
		// skip the userserver resolved step (in the local cases) - djs 09/24/03
		gStartupState++;
		timeout.reset();

		switch( gUserServerChoice )
		{
		case USERSERVER_AGNI:
			gInProductionGrid = TRUE;
		case USERSERVER_DMZ:
		case USERSERVER_ADITI:
		case USERSERVER_SIVA:
		case USERSERVER_SHAKTI:
		case USERSERVER_DURGA:
		case USERSERVER_SOMA:
		case USERSERVER_GANGA:
		case USERSERVER_UMA:
		{
				const char* host_name = gUserServerDomainName[gUserServerChoice].mName;
				snprintf(gUserServerName, MAX_STRING, "%s", host_name);		/* Flawfinder: ignore */
				llinfos << "Resolving " <<
					gUserServerDomainName[gUserServerChoice].mLabel <<
					" userserver domain name " << host_name << llendl;

				BOOL requested_domain_name = gAsyncHostByName.startRequest( host_name, on_userserver_name_resolved, NULL );
				if( !requested_domain_name )
				//BOOL resolved_domain_name = gUserServer.setHostByName( host_name );
				//if( !resolved_domain_name )
				{
					llwarns << "setHostByName failed" << llendl;
					
					LLStringBase<char>::format_map_t args;
					args["[HOST_NAME]"] = host_name;

					gViewerWindow->alertXml("UnableToConnect", args, login_alert_done );
					gStartupState = STATE_LOGIN_SHOW;
					return FALSE;
				}
				break;
			}

		case USERSERVER_LOCAL:
			llinfos << "Using local userserver" << llendl;
			gUserServer.setAddress( LOOPBACK_ADDRESS_STRING );
			gStartupState = STATE_USERSERVER_RESOLVED;
			break;

		case USERSERVER_OTHER:
			llinfos << "Userserver set explicitly" << llendl;
			gStartupState = STATE_USERSERVER_RESOLVED;
			break;

		case USERSERVER_NONE:
		default:
			llerrs << "No userserver IP address specified" << llendl;
			break;
		}
		return do_normal_idle;
	}

	if (STATE_RESOLVING_USERSERVER == gStartupState)
	{
		// Don't do anything.  Wait for LL_WM_HOST_RESOLVED which is handled by LLAsyncHostByName,
		// which calls on_userserver_name_resolved, which will push us to the next state.
		if (timeout.getElapsedTimeF32() > TIMEOUT_SECONDS*3.f)
		{
			// Cancel the pending asynchostbyname request

			gViewerWindow->alertXml("CanNotFindServer",
				login_alert_status, NULL);

			// Back up to login screen
			gStartupState = STATE_LOGIN_SHOW;
			gViewerStats->incStat(LLViewerStats::ST_LOGIN_TIMEOUT_COUNT);
		}
		ms_sleep(1);
		return do_normal_idle;
	}

	if (STATE_USERSERVER_RESOLVED == gStartupState)
	{
		if (!gUserServer.isOk())
		{
			LLStringBase<char>::format_map_t args;
			args["[IP_ADDRESS]"] = u32_to_ip_string( gUserServer.getAddress() );

			gViewerWindow->alertXml("PleaseSelectServer", args, login_alert_done );

			gStartupState = STATE_LOGIN_SHOW;
			return FALSE;
		}

		write_debug("Userserver: ");
		char tmp_str[256];		/* Flawfinder: ignore */
		gUserServer.getIPString(tmp_str, 256);
		write_debug(tmp_str);
		write_debug("\n");

		gStartupState++;
	}

	if (STATE_MESSAGE_TEMPLATE_SEND == gStartupState)
	{
		set_startup_status(0.10f, "Verifying protocol version...", NULL);

		LLHost mt_host;
		if (!gRunLocal)
		{
			// open up user server circuit (trusted)
			gMessageSystem->enableCircuit(gUserServer, TRUE);
			mt_host = gUserServer;
		}
		else
		{
			mt_host = gAgentSimHost;
		}

		llinfos << "Verifying message template..." << llendl;

		// register with the message system so it knows we're
		// expecting this message
		LLMessageSystem* msg = gMessageSystem;
		msg->setHandlerFuncFast(_PREHASH_TemplateChecksumReply, null_message_callback, NULL);
		msg->newMessageFast(_PREHASH_SecuredTemplateChecksumRequest);
		msg->nextBlockFast(_PREHASH_TokenBlock);
		lldebugs << "random token: " << gTemplateToken << llendl;
		msg->addUUIDFast(_PREHASH_Token, gTemplateToken);
		msg->sendReliable(mt_host);

		timeout.reset();
		gStartupState++;
		return do_normal_idle;
	}

	if (STATE_MESSAGE_TEMPLATE_WAIT == gStartupState)
	{
		U32 remote_template_checksum = 0;

		U8 major_version = 0;
		U8 minor_version = 0;
		U8 patch_version = 0;
		U8 server_version = 0;
		U32 flags = 0x0;

		LLMessageSystem* msg = gMessageSystem;
		while (msg->checkMessages(gFrameCount))
		{
			if (msg->isMessageFast(_PREHASH_TemplateChecksumReply))
			{
				LLUUID token;
				msg->getUUID("TokenBlock", "Token", token);
				if(token != gTemplateToken)
				{
					llwarns << "Incorrect token in template checksum reply: "
							<< token << llendl;
					return do_normal_idle;
				}
				msg->getU32("DataBlock", "Checksum", remote_template_checksum);
				msg->getU8 ("DataBlock", "MajorVersion", major_version);
				msg->getU8 ("DataBlock", "MinorVersion", minor_version);
				msg->getU8 ("DataBlock", "PatchVersion", patch_version);
				msg->getU8 ("DataBlock", "ServerVersion", server_version);
				msg->getU32("DataBlock", "Flags", flags);

				BOOL update_available = FALSE;
				BOOL mandatory = FALSE;

				if (remote_template_checksum != msg->mMessageFileChecksum)
				{
					llinfos << "Message template out of sync" << llendl;
					// Mandatory update -- message template checksum doesn't match
					update_available = TRUE;
					mandatory = TRUE;
				}

				BOOL quit = FALSE;
				if (update_available)
				{
					if (show_connect_box)
					{
						update_app(mandatory, "");
						gStartupState = STATE_UPDATE_CHECK;
						return FALSE;
					}
					else
					{
						quit = TRUE;
					}
				}
				// Bail out and clean up circuit
				if (quit)
				{
					msg->newMessageFast(_PREHASH_CloseCircuit);
					msg->sendMessage( msg->getSender() );
					app_force_quit(NULL);
					return FALSE;
				}

				// If we get here, we've got a compatible message template

				if (!mandatory)
				{
					llinfos << "Message template is current!" << llendl;
				}
				gStartupState = STATE_LOGIN_AUTH_INIT;
				timeout.reset();
				// unregister with the message system so it knows we're no longer expecting this message
				msg->setHandlerFuncFast(_PREHASH_TemplateChecksumReply, NULL,	NULL);

				msg->newMessageFast(_PREHASH_CloseCircuit);
				msg->sendMessage(gUserServer);
				msg->disableCircuit(gUserServer);
				if (gRunLocal)
				{
					msg->enableCircuit(gAgentSimHost, TRUE);

					// Don't use a session token, and generate a random user id
					gAgentID.generate();
					gAgentSessionID = LLUUID::null;

					// Skip userserver queries.
					gStartupState = STATE_WORLD_INIT;
				}
			}
		}
		gMessageSystem->processAcks();

		if (timeout.getElapsedTimeF32() > TIMEOUT_SECONDS)
		{
			if (timeout_count > MAX_TIMEOUT_COUNT)
			{
				gViewerWindow->alertXml("SystemMayBeDown",
					login_alert_status,
					NULL);

				// Back up to login screen
				gStartupState = STATE_LOGIN_SHOW;
				gViewerStats->incStat(LLViewerStats::ST_LOGIN_TIMEOUT_COUNT);
			}
			else
			{
				llinfos << "Resending on timeout" << llendl;
				gStartupState--;
				timeout_count++;
			}
		}

		return do_normal_idle;
	}

	if (STATE_UPDATE_CHECK == gStartupState)
	{
		return do_normal_idle;
	}

	if(STATE_LOGIN_AUTH_INIT == gStartupState)
	{
//#define LL_MINIMIAL_REQUESTED_OPTIONS
		lldebugs << "STATE_LOGIN_AUTH_INIT" << llendl;
		if (!gUserAuthp)
		{
			gUserAuthp = new LLUserAuth();
		}
		requested_options.clear();
		requested_options.push_back("inventory-root");
		requested_options.push_back("inventory-skeleton");
		//requested_options.push_back("inventory-meat");
		//requested_options.push_back("inventory-skel-targets");
#if (!defined LL_MINIMIAL_REQUESTED_OPTIONS)
		if(gRequestInventoryLibrary)
		{
			requested_options.push_back("inventory-lib-root");
			requested_options.push_back("inventory-lib-owner");
			requested_options.push_back("inventory-skel-lib");
		//	requested_options.push_back("inventory-meat-lib");
		}

		requested_options.push_back("initial-outfit");
		requested_options.push_back("gestures");
		requested_options.push_back("event_categories");
		requested_options.push_back("event_notifications");
		requested_options.push_back("classified_categories");
		//requested_options.push_back("inventory-targets");
		requested_options.push_back("buddy-list");
		requested_options.push_back("ui-config");
#endif
		requested_options.push_back("login-flags");
		requested_options.push_back("global-textures");
		if(gGodConnect)
		{
			gSavedSettings.setBOOL("UseDebugMenus", TRUE);
			requested_options.push_back("god-connect");
		}
		auth_uri = getLoginURI();
		auth_method = "login_to_simulator";
		auth_desc = "Logging in.  ";
		auth_desc += gSecondLife;
		auth_desc += " may appear frozen.  Please wait.";
		++gStartupState;
	}

	if (STATE_LOGIN_AUTHENTICATE == gStartupState)
	{
		lldebugs << "STATE_LOGIN_AUTHENTICATE" << llendl;
		set_startup_status(progress, auth_desc.c_str(), auth_message.c_str());
		progress += 0.02f;
		display_startup();
		
		std::stringstream start;
		if (LLURLSimString::parse())
		{
			// a startup URL was specified
			std::stringstream unescaped_start;
			unescaped_start << "uri:" 
				<< LLURLSimString::sInstance.mSimName << "&" 
				<< LLURLSimString::sInstance.mX << "&" 
				<< LLURLSimString::sInstance.mY << "&" 
				<< LLURLSimString::sInstance.mZ;
			start << xml_escape_string(unescaped_start.str().c_str());
		}
		else if (gSavedSettings.getBOOL("LoginLastLocation"))
		{
			start << "last";
		}
		else
		{
			start << "home";
		}

		char hashed_mac_string[MD5HEX_STR_SIZE];		/* Flawfinder: ignore */
		LLMD5 hashed_mac;
		hashed_mac.update( gMACAddress, MAC_ADDRESS_BYTES );
		hashed_mac.finalize();
		hashed_mac.hex_digest(hashed_mac_string);
		
		gUserAuthp->authenticate(
			auth_uri.c_str(),
			auth_method.c_str(),
			firstname.c_str(),
			lastname.c_str(),
			password.c_str(),
			start.str().c_str(),
			gSkipOptionalUpdate,
			gAcceptTOS,
			gAcceptCriticalMessage,
			gViewerDigest,
			gLastExecFroze,
			requested_options,
			hashed_mac_string,
			gSerialNumber);
		// reset globals
		gAcceptTOS = FALSE;
		gAcceptCriticalMessage = FALSE;
		++gStartupState;
		return do_normal_idle;
	}

	if(STATE_LOGIN_NO_DATA_YET == gStartupState)
	{
		//lldebugs << "STATE_LOGIN_NO_DATA_YET" << llendl;
		if (!gUserAuthp)
		{
			llerrs << "No userauth in STATE_LOGIN_NO_DATA_YET!" << llendl;
		}
		// Process messages to keep from dropping circuit.
		LLMessageSystem* msg = gMessageSystem;
		while (msg->checkMessages(gFrameCount))
		{
		}
		msg->processAcks();
		LLUserAuth::UserAuthcode error = gUserAuthp->authResponse();
		if(LLUserAuth::E_NO_RESPONSE_YET == error)
		{
			//llinfos << "waiting..." << llendl;
			return do_normal_idle;
		}
		++gStartupState;
		progress += 0.01f;
		set_startup_status(progress, auth_desc.c_str(), auth_message.c_str());
		return do_normal_idle;
	}

	if(STATE_LOGIN_DOWNLOADING == gStartupState)
	{
		lldebugs << "STATE_LOGIN_DOWNLOADING" << llendl;
		if (!gUserAuthp)
		{
			llerrs << "No userauth in STATE_LOGIN_DOWNLOADING!" << llendl;
		}
		// Process messages to keep from dropping circuit.
		LLMessageSystem* msg = gMessageSystem;
		while (msg->checkMessages(gFrameCount))
		{
		}
		msg->processAcks();
		LLUserAuth::UserAuthcode error = gUserAuthp->authResponse();
		if(LLUserAuth::E_DOWNLOADING == error)
		{
			//llinfos << "downloading..." << llendl;
			return do_normal_idle;
		}
		++gStartupState;
		progress += 0.01f;
		set_startup_status(progress, "Processing Response...", auth_message.c_str());
		return do_normal_idle;
	}

	if(STATE_LOGIN_PROCESS_RESPONSE == gStartupState)
	{
		lldebugs << "STATE_LOGIN_PROCESS_RESPONSE" << llendl;
		std::ostringstream emsg;
		BOOL quit = FALSE;
		const char* login_response = NULL;
		const char* reason_response = NULL;
		const char* message_response = NULL;
		BOOL successful_login = FALSE;
		LLUserAuth::UserAuthcode error = gUserAuthp->authResponse();
		// reset globals
		gAcceptTOS = FALSE;
		gAcceptCriticalMessage = FALSE;
		switch(error)
		{
		case LLUserAuth::E_OK:
			login_response = gUserAuthp->getResponse("login");
			if(login_response && (0 == strcmp(login_response, "true")))
			{
				// Yay, login!
				successful_login = TRUE;
			}
			else if(login_response && (0 == strcmp(login_response, "indeterminate")))
			{
				llinfos << "Indeterminate login..." << llendl;
				auth_uri = gUserAuthp->getResponse("next_url");
				auth_method = gUserAuthp->getResponse("next_method");
				auth_message = gUserAuthp->getResponse("message");
				if(auth_method.substr(0, 5) == "login")
				{
					auth_desc.assign("Authenticating...");
				}
				else
				{
					auth_desc.assign("Performing account maintenance...");
				}
				// ignoring the duration & options array for now.
				// Go back to authenticate.
				gStartupState = STATE_LOGIN_AUTHENTICATE;
				return do_normal_idle;
			}
			else
			{
				emsg << "Login failed.\n";
				reason_response = gUserAuthp->getResponse("reason");
				message_response = gUserAuthp->getResponse("message");

				if (gHideLinks && reason_response && (0 == strcmp(reason_response, "disabled")))
				{
					emsg << gDisabledMessage;
				}
				else if (message_response)
				{
					emsg << message_response;
				}

				if(reason_response && (0 == strcmp(reason_response, "tos")))
				{
					if (show_connect_box)
					{
						llinfos << "Need tos agreement" << llendl;
						gStartupState = STATE_UPDATE_CHECK;
						LLFloaterTOS* tos_dialog = LLFloaterTOS::show(LLFloaterTOS::TOS_TOS,
																	message_response);
						tos_dialog->startModal();
						// LLFloaterTOS deletes itself.
						return FALSE;
					}
					else
					{
						quit = TRUE;
					}
				}
				if(reason_response && (0 == strcmp(reason_response, "critical")))
				{
					if (show_connect_box)
					{
						llinfos << "Need critical message" << llendl;
						gStartupState = STATE_UPDATE_CHECK;
						LLFloaterTOS* tos_dialog = LLFloaterTOS::show(LLFloaterTOS::TOS_CRITICAL_MESSAGE,
																	message_response);
						tos_dialog->startModal();
						// LLFloaterTOS deletes itself.
						return FALSE;
					}
					else
					{
						quit = TRUE;
					}
				}
				if(reason_response && (0 == strcmp(reason_response, "key")))
				{
					// Couldn't login because user/password is wrong
					// Clear the password
					password = "";
				}
				if(reason_response && (0 == strcmp(reason_response, "update")))
				{
					auth_message = gUserAuthp->getResponse("message");
					if (show_connect_box)
					{
						update_app(TRUE, auth_message);
						gStartupState = STATE_UPDATE_CHECK;
						return FALSE;
					}
					else
					{
						quit = TRUE;
					}
				}
				if(reason_response && (0 == strcmp(reason_response, "optional")))
				{
					llinfos << "Login got optional update" << llendl;
					auth_message = gUserAuthp->getResponse("message");
					if (show_connect_box)
					{
						update_app(FALSE, auth_message);
						gStartupState = STATE_UPDATE_CHECK;
						gSkipOptionalUpdate = TRUE;
						return FALSE;
					}
				}
			}
			break;
		case LLUserAuth::E_COULDNT_RESOLVE_HOST:
		case LLUserAuth::E_SSL_PEER_CERTIFICATE:
		case LLUserAuth::E_UNHANDLED_ERROR:
		default:
			emsg << "Unable to connect to " << gSecondLife << ".\n";
			emsg << gUserAuthp->errorMessage();
			break;
		case LLUserAuth::E_SSL_CACERT:
		case LLUserAuth::E_SSL_CONNECT_ERROR:
			emsg << "Unable to establish a secure connection to the login server.\n";
			emsg << gUserAuthp->errorMessage();
			break;
		}

		// Version update and we're not showing the dialog
		if(quit)
		{
			delete gUserAuthp;
			gUserAuthp = NULL;
			app_force_quit(NULL);
			return FALSE;
		}

		if(successful_login)
		{
		    if (!gUserAuthp)
		    {
			    llerrs << "No userauth on successful login!" << llendl;
		    }

			// unpack login data needed by the application
			const char* text;
			text = gUserAuthp->getResponse("agent_id");
			if(text) gAgentID.set(text);
			write_debug("AgentID: ");
			write_debug(text);
			write_debug("\n");
			
			text = gUserAuthp->getResponse("session_id");
			if(text) gAgentSessionID.set(text);
			write_debug("SessionID: ");
			write_debug(text);
			write_debug("\n");
			
			text = gUserAuthp->getResponse("secure_session_id");
			if(text) gAgent.mSecureSessionID.set(text);

			text = gUserAuthp->getResponse("first_name");
			if(text) 
			{
				// Remove quotes from string.  Login.cgi sends these to force
				// names that look like numbers into strings.
				firstname.assign(text);
				LLString::replaceChar(firstname, '"', ' ');
				LLString::trim(firstname);
			}
			text = gUserAuthp->getResponse("last_name");
			if(text) lastname.assign(text);
			gSavedSettings.setString("FirstName", firstname);
			gSavedSettings.setString("LastName", lastname);
			if (remember_password)
			{
				save_password_to_disk(password.c_str());
			}
			else
			{
				save_password_to_disk(NULL);
			}
			gSavedSettings.setBOOL("RememberPassword", remember_password);
			gSavedSettings.setBOOL("LoginLastLocation", gSavedSettings.getBOOL("LoginLastLocation"));
			gSavedSettings.setBOOL("LoggedIn", TRUE);

			text = gUserAuthp->getResponse("agent_access");
			if(text && (text[0] == 'M'))
			{
				gAgent.mAccess = SIM_ACCESS_MATURE;
			}
			else
			{
				gAgent.mAccess = SIM_ACCESS_PG;
			}

			text = gUserAuthp->getResponse("start_location");
			if(text) agent_start_location.assign(text);
			text = gUserAuthp->getResponse("circuit_code");
			if(text)
			{
				gMessageSystem->mOurCircuitCode = strtoul(text, NULL, 10);
			}
			const char* sim_ip_str = gUserAuthp->getResponse("sim_ip");
			const char* sim_port_str = gUserAuthp->getResponse("sim_port");
			if(sim_ip_str && sim_port_str)
			{
				U32 sim_port = strtoul(sim_port_str, NULL, 10);
				first_sim.set(sim_ip_str, sim_port);
				if (first_sim.isOk())
				{
					gMessageSystem->enableCircuit(first_sim, TRUE);
				}
			}
			const char* region_x_str = gUserAuthp->getResponse("region_x");
			const char* region_y_str = gUserAuthp->getResponse("region_y");
			if(region_x_str && region_y_str)
			{
				U32 region_x = strtoul(region_x_str, NULL, 10);
				U32 region_y = strtoul(region_y_str, NULL, 10);
				first_sim_handle = to_region_handle(region_x, region_y);
			}
			
			const char* look_at_str = gUserAuthp->getResponse("look_at");
			if (look_at_str)
			{
				LLMemoryStream mstr((U8*)look_at_str, strlen(look_at_str));		/* Flawfinder: ignore */
				LLSD sd = LLSDNotationParser::parse(mstr);
				agent_start_look_at = ll_vector3_from_sd(sd);
			}

			text = gUserAuthp->getResponse("seed_capability");
			if (text) first_sim_seed_cap = text;
						
			text = gUserAuthp->getResponse("seconds_since_epoch");
			if(text)
			{
				U32 server_utc_time = strtoul(text, NULL, 10);
				if(server_utc_time)
				{
					time_t now = time(NULL);
					gUTCOffset = (server_utc_time - now);
				}
			}

			const char* home_location = gUserAuthp->getResponse("home");
			if(home_location)
			{
				LLMemoryStream mstr((U8*)home_location, strlen(home_location));		/* Flawfinder: ignore */
				LLSD sd = LLSDNotationParser::parse(mstr);
				S32 region_x = sd["region_handle"][0].asInteger();
				S32 region_y = sd["region_handle"][1].asInteger();
				U64 region_handle = to_region_handle(region_x, region_y);
				LLVector3 position = ll_vector3_from_sd(sd["position"]);
				gAgent.setHomePosRegion(region_handle, position);
			}

			gAgent.mMOTD.assign(gUserAuthp->getResponse("message"));
			LLUserAuth::options_t options;
			if(gUserAuthp->getOptions("inventory-root", options))
			{
				LLUserAuth::response_t::iterator it;
				it = options[0].find("folder_id");
				if(it != options[0].end())
				{
					gAgent.mInventoryRootID.set((*it).second.c_str());
					//gInventory.mock(gAgent.getInventoryRootID());
				}
			}

			options.clear();
			if(gUserAuthp->getOptions("login-flags", options))
			{
				LLUserAuth::response_t::iterator it;
				LLUserAuth::response_t::iterator no_flag = options[0].end();
				it = options[0].find("ever_logged_in");
				if(it != no_flag)
				{
					if((*it).second == "N") gAgent.setFirstLogin(TRUE);
					else gAgent.setFirstLogin(FALSE);
				}
				it = options[0].find("stipend_since_login");
				if(it != no_flag)
				{
					if((*it).second == "Y") stipend_since_login = TRUE;
				}
				it = options[0].find("gendered");
				if(it != no_flag)
				{
					if((*it).second == "Y") gAgent.setGenderChosen(TRUE);
				}
				it = options[0].find("daylight_savings");
				if(it != no_flag)
				{
					if((*it).second == "Y")  gPacificDaylightTime = TRUE;
					else gPacificDaylightTime = FALSE;
				}
			}
			options.clear();
			if (gUserAuthp->getOptions("initial-outfit", options)
				&& !options.empty())
			{
				LLUserAuth::response_t::iterator it;
				LLUserAuth::response_t::iterator it_end = options[0].end();
				it = options[0].find("folder_name");
				if(it != it_end)
				{
					gInitialOutfit = (*it).second;
				}
				it = options[0].find("gender");
				if (it != it_end)
				{
					gInitialOutfitGender = (*it).second;
				}
			}

			options.clear();
			if(gUserAuthp->getOptions("global-textures", options))
			{
				// Extract sun and moon texture IDs.  These are used
				// in the LLVOSky constructor, but I can't figure out
				// how to pass them in.  JC
				LLUserAuth::response_t::iterator it;
				LLUserAuth::response_t::iterator no_texture = options[0].end();
				it = options[0].find("sun_texture_id");
				if(it != no_texture)
				{
					gSunTextureID.set((*it).second.c_str());
				}
				it = options[0].find("moon_texture_id");
				if(it != no_texture)
				{
					gMoonTextureID.set((*it).second.c_str());
				}
				it = options[0].find("cloud_texture_id");
				if(it != no_texture)
				{
					gCloudTextureID.set((*it).second.c_str());
				}
			}


			// JC: gesture loading done below, when we have an asset system
			// in place.  Don't delete/clear user_credentials until then.

			if(gAgentID.notNull()
			   && gAgentSessionID.notNull()
			   && gMessageSystem->mOurCircuitCode
			   && first_sim.isOk()
			   && gAgent.mInventoryRootID.notNull())
			{
				++gStartupState;
			}
			else
			{
				if (gNoRender)
				{
					llinfos << "Bad login - missing return values" << llendl;
					llinfos << emsg << llendl;
					exit(0);
				}
				// Bounce back to the login screen.
				LLStringBase<char>::format_map_t args;
				args["[ERROR_MESSAGE]"] = emsg.str();
				gViewerWindow->alertXml("ErrorMessage", args, login_alert_done);
				gStartupState = STATE_LOGIN_SHOW;
			}
		}
		else
		{
			if (gNoRender)
			{
				llinfos << "Failed to login!" << llendl;
				llinfos << emsg << llendl;
				exit(0);
			}
			// Bounce back to the login screen.
			LLStringBase<char>::format_map_t args;
			args["[ERROR_MESSAGE]"] = emsg.str();
			gViewerWindow->alertXml("ErrorMessage", args, login_alert_done);
			gStartupState = STATE_LOGIN_SHOW;
		}
		return do_normal_idle;
	}

	//---------------------------------------------------------------------
	// World Init
	//---------------------------------------------------------------------
	if (STATE_WORLD_INIT == gStartupState)
	{
		set_startup_status(0.40f, "Initializing World...", gAgent.mMOTD.c_str());
		display_startup();
		// We should have an agent id by this point.
		llassert(!(gAgentID == LLUUID::null));

		// Finish agent initialization.  (Requires gSavedSettings, builds camera)
		gAgent.init();

		// Since we connected, save off the settings so the user doesn't have to
		// type the name/password again if we crash.
		gSavedSettings.saveToFile(gSettingsFileName, TRUE);

		// Create selection manager
		// Must be done before menus created, because many enabled callbacks
		// require its existance.
		gSelectMgr = new LLSelectMgr();
		gParcelMgr = new LLViewerParcelMgr();
		gHUDManager = new LLHUDManager();
		gMuteListp = new LLMuteList();

		//
		// Initialize classes w/graphics stuff.
		//
		LLSurface::initClasses();

		LLFace::initClass();

		LLDrawable::initClass();

		// RN: don't initialize VO classes in drone mode, they are too closely tied to rendering
		LLViewerObject::initVOClasses();

		display_startup();

		// World initialization must be done after above window init
		gWorldp = new LLWorld(region_size, region_scale);

		// User might have overridden far clip
		gWorldp->setLandFarClip( gAgent.mDrawDistance );

		if (!gRunLocal)
		{
			// Before we create the first region, we need to set the agent's mOriginGlobal
			// This is necessary because creating objects before this is set will result in a
			// bad mPositionAgent cache.

			gAgent.initOriginGlobal(from_region_handle(first_sim_handle));

			gWorldp->addRegion(first_sim_handle, first_sim);

			LLViewerRegion *regionp = gWorldp->getRegionFromHandle(first_sim_handle);
			llinfos << "Adding initial simulator " << regionp->getOriginGlobal() << llendl;
			
			regionp->setSeedCapability(first_sim_seed_cap);
			
			// Set agent's initial region to be the one we just created.
			gAgent.setRegion(regionp);

			// Set agent's initial position, which will be read by LLVOAvatar when the avatar
			// object is created.  I think this must be done after setting the region.  JC
			gAgent.setPositionAgent(agent_start_position_region);
		}
		else
		{
			// With one simulator, assume region is at 0,0, hence has regionHandle 0
			// VEFFECT: Login
			gWorldp->addRegion(0, gAgentSimHost);
			gAgent.setRegion(gWorldp->getRegionFromHandle(0));
		}

		display_startup();

		// Initialize UI
		if (!gNoRender)
		{
			// Initialize all our tools.  Must be done after saved settings loaded.
			gToolMgr = new LLToolMgr();
			gToolMgr->initTools();
			// Quickly get something onscreen to look at.
			gViewerWindow->initWorldUI();

			// Move the progress view in front of the UI
			gViewerWindow->moveProgressViewToFront();

			LLError::logToFixedBuffer(gDebugView->mDebugConsolep);
			// set initial visibility of debug console
			gDebugView->mDebugConsolep->setVisible(gSavedSettings.getBOOL("ShowDebugConsole"));
			gDebugView->mStatViewp->setVisible(gSavedSettings.getBOOL("ShowDebugStats"));
		}

		//
		// Set message handlers
		//
		llinfos << "Initializing communications..." << llendl;

		// register callbacks for messages. . . do this after initial handshake to make sure that we don't catch any unwanted
		register_viewer_callbacks(gMessageSystem);

		// Debugging info parameters
		gMessageSystem->setMaxMessageTime( 0.5f );			// Spam if decoding all msgs takes more than 500 ms

		#ifndef	LL_RELEASE_FOR_DOWNLOAD
			gMessageSystem->setTimeDecodes( TRUE );				// Time the decode of each msg
			gMessageSystem->setTimeDecodesSpamThreshold( 0.05f );  // Spam if a single msg takes over 50ms to decode
		#endif

		gXferManager->registerCallbacks(gMessageSystem);

		gCacheName = new LLCacheName(gMessageSystem);
		gCacheName->addObserver(callback_cache_name);

		// Load stored cache if possible
		load_name_cache();

		// Data storage for map of world.
		gWorldMap = new LLWorldMap();

		// register null callbacks for audio until the audio system is initialized
		gMessageSystem->setHandlerFuncFast(_PREHASH_SoundTrigger, null_message_callback, NULL);
		gMessageSystem->setHandlerFuncFast(_PREHASH_AttachedSound, null_message_callback, NULL);

		//reset statistics
		gViewerStats->resetStats();

		if (!gNoRender)
		{
			//
			// Set up all of our statistics UI stuff.
			//
			init_stat_view();
		}

		display_startup();
		//
		// Set up region and surface defaults
		//


		// Sets up the parameters for the first simulator

		llinfos << "Initializing camera..." << llendl;
		gFrameTime    = totalTime();
		F32 last_time = gFrameTimeSeconds;
		gFrameTimeSeconds = (S64)(gFrameTime - gStartTime)/SEC_TO_MICROSEC;

		gFrameIntervalSeconds = gFrameTimeSeconds - last_time;
		if (gFrameIntervalSeconds < 0.f)
		{
			gFrameIntervalSeconds = 0.f;
		}

		// Make sure agent knows correct aspect ratio
		gCamera->setViewHeightInPixels(gViewerWindow->getWindowDisplayHeight());
		if (gViewerWindow->mWindow->getFullscreen())
		{
			gCamera->setAspect(gViewerWindow->getDisplayAspectRatio());
		}
		else
		{
			gCamera->setAspect( (F32) gViewerWindow->getWindowWidth() / (F32) gViewerWindow->getWindowHeight());
		}

		// Move agent to starting location. The position handed to us by
		// the space server is in global coordinates, but the agent frame
		// is in region local coordinates. Therefore, we need to adjust
		// the coordinates handed to us to fit in the local region.

		gAgent.setPositionAgent(agent_start_position_region);
		gAgent.resetAxes(agent_start_look_at);
		gAgent.stopCameraAnimation();
		gAgent.resetCamera();

		// Initialize global class data needed for surfaces (i.e. textures)
		if (!gNoRender)
		{
			llinfos << "Initializing sky..." << llendl;
			// Initialize all of the viewer object classes for the first time (doing things like texture fetches.
			gSky.init(initial_sun_direction);
		}

		set_startup_status(0.45f, "Decoding UI images...", gAgent.mMOTD.c_str());
		display_startup();
		llinfos << "Decoding images..." << llendl;
		// For all images pre-loaded into viewer cache, decode them.
		// Need to do this AFTER we init the sky
		gImageList.decodeAllImages();
		gStartupState++;

		// JC - Do this as late as possible to increase likelihood Purify
		// will run.
		if (!gRunLocal)
		{
			LLMessageSystem* msg = gMessageSystem;
			if (!msg->mOurCircuitCode)
			{
				llwarns << "Attempting to connect to simulator with a zero circuit code!" << llendl;
			}
			msg->enableCircuit(first_sim, TRUE);
			// now, use the circuit info to tell simulator about us!
			llinfos << "viewer: UserLoginLocationReply() Enabling " << first_sim << " with code " << msg->mOurCircuitCode << llendl;
			msg->newMessageFast(_PREHASH_UseCircuitCode);
			msg->nextBlockFast(_PREHASH_CircuitCode);
			msg->addU32Fast(_PREHASH_Code, msg->mOurCircuitCode);
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->addUUIDFast(_PREHASH_ID, gAgent.getID());
			msg->sendReliable(
				first_sim,
				MAX_TIMEOUT_COUNT,
				FALSE,
				TIMEOUT_SECONDS,
				use_circuit_callback,
				NULL);
		}

		timeout.reset();
		if (!gConnectToSomething)
		{
			gStartupState = STATE_MISC;
		}

		return do_normal_idle;
	}

	//---------------------------------------------------------------------
	// LLMediaEngine Init
	//---------------------------------------------------------------------
	if (STATE_QUICKTIME_INIT == gStartupState)
	{
		if (gViewerWindow)
		{
			if (gSavedSettings.getBOOL("MuteAudio"))
			{
				LLMediaEngine::updateClass( 0.0f );
			}
			else
			{
				LLMediaEngine::updateClass( gSavedSettings.getF32( "MediaAudioVolume" ) );
			}
		}

		#if LL_QUICKTIME_ENABLED	// windows only right now but will be ported to mac 
		if (!gQuickTimeInitialized)
		{
			// initialize quicktime libraries (fails gracefully if quicktime not installed ($QUICKTIME)
			llinfos << "Initializing QuickTime...." << llendl;
			set_startup_status(0.47f, "Initializing QuickTime...", gAgent.mMOTD.c_str());
			display_startup();
			#if LL_WINDOWS
				// Only necessary/available on Windows.
				if ( InitializeQTML ( 0L ) != noErr )
				{
					// quicktime init failed - turn off media engine support
					LLMediaEngine::getInstance ()->setAvailable ( FALSE );
					llinfos << "...not found - unable to initialize." << llendl;
					set_startup_status(0.47f, "QuickTime not found - unable to initialize.", gAgent.mMOTD.c_str());
				}
				else
				{
					llinfos << ".. initialized successfully." << llendl;
					set_startup_status(0.47f, "QuickTime initialized successfully.", gAgent.mMOTD.c_str());
				};
			#endif
			EnterMovies ();
			gQuickTimeInitialized = true;
		}
		#endif

		gStartupState++;
		return do_normal_idle;
	}

	//---------------------------------------------------------------------
	// Agent Send
	//---------------------------------------------------------------------
	if(STATE_WORLD_WAIT == gStartupState)
	{
		//llinfos << "Waiting for simulator ack...." << llendl;
		set_startup_status(0.49f, "Waiting for region handshake...", gAgent.mMOTD.c_str());
		if(gGotUseCircuitCodeAck)
		{
			++gStartupState;
		}
		LLMessageSystem* msg = gMessageSystem;
		while (msg->checkMessages(gFrameCount))
		{
		}
		msg->processAcks();
		return do_normal_idle;
	}

	//---------------------------------------------------------------------
	// Agent Send
	//---------------------------------------------------------------------
	if (STATE_AGENT_SEND == gStartupState)
	{
		llinfos << "Connecting to region..." << llendl;
		set_startup_status(0.50f, "Connecting to region...", gAgent.mMOTD.c_str());
		// register with the message system so it knows we're
		// expecting this message
		LLMessageSystem* msg = gMessageSystem;
		msg->setHandlerFuncFast(
			_PREHASH_AgentMovementComplete,
			process_agent_movement_complete,
			NULL);
		LLViewerRegion* regionp = gAgent.getRegion();
		if(!gRunLocal && regionp)
		{
			send_complete_agent_movement(regionp->getHost());
			gAssetStorage->setUpstream(regionp->getHost());
			gCacheName->setUpstream(regionp->getHost());
			msg->newMessageFast(_PREHASH_EconomyDataRequest);
			gAgent.sendReliableMessage();
		}
		else
		{
			gStartupState++;
		}

		// Create login effect
		// But not on first login, because you can't see your avatar then
		if (!gAgent.isFirstLogin())
		{
			LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINT, TRUE);
			effectp->setPositionGlobal(gAgent.getPositionGlobal());
			effectp->setColor(LLColor4U(gAgent.getEffectColor()));
			gHUDManager->sendEffects();
		}

		gStartupState++;

		timeout.reset();
		return do_normal_idle;
	}

	//---------------------------------------------------------------------
	// Agent Wait
	//---------------------------------------------------------------------
	if (STATE_AGENT_WAIT == gStartupState)
	{
		LLMessageSystem* msg = gMessageSystem;
		while (msg->checkMessages(gFrameCount))
		{
			if (msg->isMessageFast(_PREHASH_AgentMovementComplete))
			{
				gStartupState++;
				// Sometimes we have more than one message in the
				// queue. break out of this loop and continue
				// processing. If we don't, then this could skip one
				// or more login steps.
				break;
			}
			else
			{
				//llinfos << "Awaiting AvatarInitComplete, got "
				//<< msg->getMessageName() << llendl;
			}
		}
		msg->processAcks();
		return do_normal_idle;
	}

	//---------------------------------------------------------------------
	// Inventory Send
	//---------------------------------------------------------------------
	if (STATE_INVENTORY_SEND == gStartupState)
	{
		if (!gUserAuthp)
		{
			llerrs << "No userauth in STATE_INVENTORY_SEND!" << llendl;
		}

		if (gConnectToSomething && !gRunLocal)
		{
			// unpack thin inventory
			LLUserAuth::options_t options;
			options.clear();
			//bool dump_buffer = false;
			
			if(gUserAuthp->getOptions("inventory-lib-root", options)
			   && !options.empty())
			{
				// should only be one
				LLUserAuth::response_t::iterator it;
				it = options[0].find("folder_id");
				if(it != options[0].end())
				{
					gInventoryLibraryRoot.set((*it).second.c_str());
				}
			}
 			options.clear();
			if(gUserAuthp->getOptions("inventory-lib-owner", options)
			   && !options.empty())
			{
				// should only be one
				LLUserAuth::response_t::iterator it;
				it = options[0].find("agent_id");
				if(it != options[0].end())
				{
					gInventoryLibraryOwner.set((*it).second.c_str());
				}
			}
 			options.clear();
 			if(gUserAuthp->getOptions("inventory-skel-lib", options)
			   && gInventoryLibraryOwner.notNull())
 			{
 				if(!gInventory.loadSkeleton(options, gInventoryLibraryOwner))
 				{
 					llwarns << "Problem loading inventory-skel-lib" << llendl;
 				}
 			}
 			options.clear();
 			if(gUserAuthp->getOptions("inventory-skeleton", options))
 			{
 				if(!gInventory.loadSkeleton(options, gAgent.getID()))
 				{
 					llwarns << "Problem loading inventory-skel-targets"
							<< llendl;
 				}
 			}

			options.clear();
 			if(gUserAuthp->getOptions("buddy-list", options))
 			{
				LLUserAuth::options_t::iterator it = options.begin();
				LLUserAuth::options_t::iterator end = options.end();
				LLAvatarTracker::buddy_map_t list;
				LLUUID agent_id;
				S32 has_rights = 0, given_rights = 0;
				for (; it != end; ++it)
				{
					LLUserAuth::response_t::const_iterator option_it;
					option_it = (*it).find("buddy_id");
					if(option_it != (*it).end())
					{
						agent_id.set((*option_it).second.c_str());
					}
					option_it = (*it).find("buddy_rights_has");
					if(option_it != (*it).end())
					{
						has_rights = atoi((*option_it).second.c_str());
					}
					option_it = (*it).find("buddy_rights_given");
					if(option_it != (*it).end())
					{
						given_rights = atoi((*option_it).second.c_str());
					}
					list[agent_id] = new LLRelationship(given_rights, has_rights, false);
				}
				LLAvatarTracker::instance().addBuddyList(list);
 			}

			options.clear();
 			if(gUserAuthp->getOptions("ui-config", options))
 			{
				LLUserAuth::options_t::iterator it = options.begin();
				LLUserAuth::options_t::iterator end = options.end();
				for (; it != end; ++it)
				{
					LLUserAuth::response_t::const_iterator option_it;
					option_it = (*it).find("allow_first_life");
					if(option_it != (*it).end())
					{
						if (option_it->second == "Y")
						{
							LLPanelAvatar::sAllowFirstLife = TRUE;
						}
					}
				}
 			}

			options.clear();
			if(gUserAuthp->getOptions("event_categories", options))
			{
				LLEventInfo::loadCategories(options);
			}
			if(gUserAuthp->getOptions("event_notifications", options))
			{
				gEventNotifier.load(options);
			}
			options.clear();
			if(gUserAuthp->getOptions("classified_categories", options))
			{
				LLClassifiedInfo::loadCategories(options);
			}
			gInventory.buildParentChildMap();
			gInventory.addChangedMask(LLInventoryObserver::ALL, LLUUID::null);
			gInventory.notifyObservers();

			// set up callbacks
			LLMessageSystem* msg = gMessageSystem;
			LLInventoryModel::registerCallbacks(msg);
			LLAvatarTracker::instance().registerCallbacks(msg);
			LLLandmark::registerCallbacks(msg);

			// request mute list
			gMuteListp->requestFromServer(gAgent.getID());

			// Get money and ownership credit information
			msg->newMessageFast(_PREHASH_MoneyBalanceRequest);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_MoneyData);
			msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null );
			gAgent.sendReliableMessage();

			// request all group information
			// *FIX: This will not do the right thing if the message
			// gets there before the requestuserserverconnection
			// circuit is completed.
			gAgent.sendAgentDataUpdateRequest();


			// NOTE: removed as part of user-privacy
			// enhancements. this information should be available from
			// login. 2006-10-16 Phoenix.
			// get the users that have been granted modify powers
			//msg->newMessageFast(_PREHASH_RequestGrantedProxies);
			//msg->nextBlockFast(_PREHASH_AgentData);
			//msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			//msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			//gAgent.sendReliableMessage();

			BOOL shown_at_exit = gSavedSettings.getBOOL("ShowInventory");

			// Create the inventory views
			LLInventoryView::showAgentInventory();

			// Hide the inventory if it wasn't shown at exit
			if(!shown_at_exit)
			{
				LLInventoryView::toggleVisibility(NULL);
			}
		}
		gStartupState++;
		return do_normal_idle;
	}

	//---------------------------------------------------------------------
	// Assert agent to userserver
	//---------------------------------------------------------------------
	if (STATE_CONNECT_USERSERVER == gStartupState)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->enableCircuit(gUserServer, TRUE);
		msg->newMessage("ConnectAgentToUserserver");
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->sendReliable(gUserServer);
		gStartupState++;
		return do_normal_idle;
	}

	//---------------------------------------------------------------------
	// Misc
	//---------------------------------------------------------------------
	if (STATE_MISC == gStartupState)
	{
		// Create a few objects if we don't actually have a world
		if (!gConnectToSomething)
		{
			// could add them here
		}

		// We have a region, and just did a big inventory download.
		// We can estimate the user's connection speed, and set their
		// max bandwidth accordingly.  JC
		if (gSavedSettings.getBOOL("FirstLoginThisInstall")
			&& gUserAuthp)
		{
			// This is actually a pessimistic computation, because TCP may not have enough
			// time to ramp up on the (small) default inventory file to truly measure max
			// bandwidth. JC
			F64 rate_bps = gUserAuthp->getLastTransferRateBPS();
			const F32 FAST_RATE_BPS = 600.f * 1024.f;
			const F32 FASTER_RATE_BPS = 750.f * 1024.f;
			F32 max_bandwidth = gViewerThrottle.getMaxBandwidth();
			if (rate_bps > FASTER_RATE_BPS
				&& rate_bps > max_bandwidth)
			{
				llinfos << "Fast network connection, increasing max bandwidth to " 
					<< FASTER_RATE_BPS/1024.f 
					<< " Kbps" << llendl;
				gViewerThrottle.setMaxBandwidth(FASTER_RATE_BPS / 1024.f);
			}
			else if (rate_bps > FAST_RATE_BPS
				&& rate_bps > max_bandwidth)
			{
				llinfos << "Fast network connection, increasing max bandwidth to " 
					<< FAST_RATE_BPS/1024.f 
					<< " Kbps" << llendl;
				gViewerThrottle.setMaxBandwidth(FAST_RATE_BPS / 1024.f);
			}
		}

		// We're successfully logged in.
		gSavedSettings.setBOOL("FirstLoginThisInstall", FALSE);


		// based on the comments, we've successfully logged in so we can delete the 'forced'
		// URL that the updater set in settings.ini (in a mostly paranoid fashion)
		LLString nextLoginLocation = gSavedSettings.getString( "NextLoginLocation" );
		if ( nextLoginLocation.length() )
		{
			// clear it
			gSavedSettings.setString( "NextLoginLocation", "" );

			// and make sure it's saved
			gSavedSettings.saveToFile( gSettingsFileName, TRUE );
		};

		if (!gNoRender)
		{
			// JC: Initializing audio requests many sounds for download.
			init_audio();

			// JC: Initialize "active" gestures.  This may also trigger
			// many gesture downloads, if this is the user's first
			// time on this machine or -purge has been run.
			LLUserAuth::options_t gesture_options;
			if (gUserAuthp->getOptions("gestures", gesture_options))
			{
				llinfos << "Gesture Manager loading " << gesture_options.size()
					<< llendl;
				std::vector<LLUUID> item_ids;
				LLUserAuth::options_t::iterator resp_it;
				for (resp_it = gesture_options.begin();
					 resp_it != gesture_options.end();
					 ++resp_it)
				{
					const LLUserAuth::response_t& response = *resp_it;
					LLUUID item_id;
					LLUUID asset_id;
					LLUserAuth::response_t::const_iterator option_it;

					option_it = response.find("item_id");
					if (option_it != response.end())
					{
						const std::string& uuid_string = (*option_it).second;
						item_id.set(uuid_string.c_str());
					}
					option_it = response.find("asset_id");
					if (option_it != response.end())
					{
						const std::string& uuid_string = (*option_it).second;
						asset_id.set(uuid_string.c_str());
					}

					if (item_id.notNull() && asset_id.notNull())
					{
						// Could schedule and delay these for later.
						const BOOL no_inform_server = FALSE;
						const BOOL no_deactivate_similar = FALSE;
						gGestureManager.activateGestureWithAsset(item_id, asset_id,
																 no_inform_server,
																 no_deactivate_similar);
						// We need to fetch the inventory items for these gestures
						// so we have the names to populate the UI.
						item_ids.push_back(item_id);
					}
				}

				LLGestureInventoryFetchObserver* fetch = new LLGestureInventoryFetchObserver();
				fetch->fetchItems(item_ids);
				// deletes itself when done
				gInventory.addObserver(fetch);
			}
		}
		gDisplaySwapBuffers = TRUE;

		LLMessageSystem* msg = gMessageSystem;
		msg->setHandlerFuncFast(_PREHASH_SoundTrigger,				process_sound_trigger);
		msg->setHandlerFuncFast(_PREHASH_PreloadSound,				process_preload_sound);
		msg->setHandlerFuncFast(_PREHASH_AttachedSound,				process_attached_sound);
		msg->setHandlerFuncFast(_PREHASH_AttachedSoundGainChange,	process_attached_sound_gain_change);
		//msg->setHandlerFuncFast(_PREHASH_AttachedSoundCutoffRadius,	process_attached_sound_cutoff_radius);
		msg->setHandlerFunc(
			"ConnectToUserserver",
			process_connect_to_userserver);

		llinfos << "Initialization complete" << llendl;
		gInitializationComplete = TRUE;

		gRenderStartTime.reset();

		// HACK: Inform simulator of window size.
		// Do this here so it's less likely to race with RegisterNewAgent.
		// TODO: Put this into RegisterNewAgent
		// JC - 7/20/2002
		gViewerWindow->sendShapeToSim();

		// Ignore stipend information for now.  Money history is on the web site.
		// if needed, show the money history window
		//if (stipend_since_login && !gNoRender)
		//{
		//}

		if (!gAgent.isFirstLogin())
		{
			bool url_ok = LLURLSimString::sInstance.parse();
			if (!((agent_start_location == "url" && url_ok) ||
                  (!url_ok && ((agent_start_location == "last" && gSavedSettings.getBOOL("LoginLastLocation")) ||
							   (agent_start_location == "home" && !gSavedSettings.getBOOL("LoginLastLocation"))))))
			{
				// The reason we show the alert is because we want to
				// reduce confusion for when you log in and your provided
				// location is not your expected location. So, if this is
				// your first login, then you do not have an expectation,
				// thus, do not show this alert.
				LLString::format_map_t args;
				if (url_ok)
				{
					args["[TYPE]"] = "desired";
					args["[HELP]"] = " ";
				}
				else if (gSavedSettings.getBOOL("LoginLastLocation"))
				{
					args["[TYPE]"] = "last";
					args["[HELP]"] = " \n ";
				}
				else
				{
					args["[TYPE]"] = "home";
					args["[HELP]"] = " \nYou may want to set a new home location.\n ";
				}
				gViewerWindow->alertXml("AvatarMoved", args);
			}
			else
			{
				if (samename)
				{
					// restore old camera pos
					gAgent.setFocusOnAvatar(FALSE, FALSE);
					gAgent.setCameraPosAndFocusGlobal(gSavedSettings.getVector3d("CameraPosOnLogout"), gSavedSettings.getVector3d("FocusPosOnLogout"), LLUUID::null);
					BOOL limit_hit = FALSE;
					gAgent.calcCameraPositionTargetGlobal(&limit_hit);
					if (limit_hit)
					{
						gAgent.setFocusOnAvatar(TRUE, FALSE);
					}
					gAgent.stopCameraAnimation();
				}
			}
		}

		gStartupState++;
		timeout.reset();
		return do_normal_idle;
	}

	if (STATE_PRECACHE == gStartupState)
	{
		do_normal_idle = TRUE;
		if (!did_precache)
		{
			did_precache = TRUE;
			// Don't preload map information!  The amount of data for all the
			// map items (icons for classifieds, avatar locations, etc.) is
			// huge, and not throttled.  This overflows the downstream
			// pipe during startup, when lots of information is being sent.
			// The problem manifests itself as invisible avatars on login. JC
			//gWorldMap->setCurrentLayer(0); // pre-load layer 0 of the world map

			gImageList.doPreloadImages(); // pre-load some images from static VFS
		}
		
		F32 timeout_frac = timeout.getElapsedTimeF32()/PRECACHING_DELAY;
		// wait precache-delay and for agent's avatar or a lot longer.
		if(((timeout_frac > 1.f) && gAgent.getAvatarObject())
		   || (timeout_frac > 3.f))
		{
			gStartupState++;
		}
		else
		{
			set_startup_status(0.50f + 0.50f * timeout_frac, "Precaching...",
							   gAgent.mMOTD.c_str());
		}

		return do_normal_idle;
	}

	if (STATE_WEARABLES_WAIT == gStartupState)
	{
		do_normal_idle = TRUE;

		static LLFrameTimer wearables_timer;

		const F32 wearables_time = wearables_timer.getElapsedTimeF32();
		const F32 MAX_WEARABLES_TIME = 10.f;

		if(gAgent.getWearablesLoaded() || !gAgent.isGenderChosen())
		{
			gStartupState++;
		}
		else if (wearables_time > MAX_WEARABLES_TIME)
		{
			gViewerWindow->alertXml("ClothingLoading");
			gViewerStats->incStat(LLViewerStats::ST_WEARABLES_TOO_LONG);
			gStartupState++;
		}
		else
		{
			set_startup_status(0.f + 0.25f * wearables_time / MAX_WEARABLES_TIME,
							 "Downloading clothing...",
							 gAgent.mMOTD.c_str());
		}
		return do_normal_idle;
	}

	if (STATE_CLEANUP == gStartupState)
	{
		set_startup_status(1.0, "", NULL);

		do_normal_idle = TRUE;

		// Let the map know about the inventory.
		if(gFloaterWorldMap)
		{
			gFloaterWorldMap->observeInventory(&gInventory);
			gFloaterWorldMap->observeFriends();
		}

		gViewerWindow->showCursor();
		gViewerWindow->getWindow()->resetBusyCount();
		gViewerWindow->getWindow()->setCursor(UI_CURSOR_ARROW);
		//llinfos << "Done releasing bitmap" << llendl;
		gViewerWindow->setShowProgress(FALSE);
		gViewerWindow->setProgressCancelButtonVisible(FALSE, "");

		// We're not away from keyboard, even though login might have taken
		// a while. JC
		gAgent.clearAFK();

		// Have the agent start watching the friends list so we can update proxies
		gAgent.observeFriends();
		if (gSavedSettings.getBOOL("LoginAsGod"))
		{
			gAgent.requestEnterGodMode();
		}
		
		// On first start, ask user for gender
		dialog_choose_gender_first_start();

		// Start automatic replay if the flag is set.
		if (gSavedSettings.getBOOL("StatsAutoRun"))
		{
			LLUUID id;
			llinfos << "Starting automatic playback" << llendl;
			gAgentPilot.startPlayback();
		}

		// ok, if we've gotten this far and have a startup URL
		if (LLURLSimString::sInstance.parse())
		{
			// kick off request for landmark to startup URL
			if(gFloaterWorldMap)
			{
				LLVector3 pos = gAgent.getPositionAgent();
				if( LLURLSimString::sInstance.mSimName != gAgent.getRegion()->getName()
					|| LLURLSimString::sInstance.mX != llfloor(pos[VX])
					|| LLURLSimString::sInstance.mY != llfloor(pos[VY]) )
				{
					LLFloaterWorldMap::show(NULL, TRUE);
					gFloaterWorldMap->trackURL(LLURLSimString::sInstance.mSimName,
											   LLURLSimString::sInstance.mX,
											   LLURLSimString::sInstance.mY,
											   LLURLSimString::sInstance.mZ);
				}
			}
		}

		// Clean up the userauth stuff.
		if (gUserAuthp)
		{
			delete gUserAuthp;
			gUserAuthp = NULL;
		}

		gStartupState++;
		//RN: unmute audio now that we are entering world
		//JC: But only if the user wants audio working.
		if (gAudiop)
		{
			BOOL mute = gSavedSettings.getBOOL("MuteAudio");
			gAudiop->setMuted(mute);
		}

		// reset keyboard focus to sane state of pointing at world
		gFocusMgr.setKeyboardFocus(NULL, NULL);

#if 0 // sjb: enable for auto-enabling timer display 
		gDebugView->mFastTimerView->setVisible(TRUE);
#endif
		return do_normal_idle;
	}

	llwarns << "Reached end of idle_startup for state " << gStartupState << llendl;
	return do_normal_idle;
}

//
// local function definition
//

void unsupported_graphics_callback(S32 option, void* userdata)
{
	if (0 == option)
	{
		llinfos << "User cancelled after driver check" << llendl;
		std::string help_path;
		help_path = gDirUtilp->getExpandedFilename(LL_PATH_HELP,
			"unsupported_card.html");
		app_force_quit( help_path.c_str() );
	}

	LLPanelLogin::giveFocus();
}

void check_driver_callback(S32 option, void* userdata)
{
	if (0 == option)
	{
		llinfos << "User cancelled after driver check" << llendl;
		std::string help_path;
		help_path = gDirUtilp->getExpandedFilename(LL_PATH_HELP,
			"graphics_driver_update.html");
		app_force_quit( help_path.c_str() );
	}

	LLPanelLogin::giveFocus();
}

void login_show()
{
	LLPanelLogin::show(	gViewerWindow->getVirtualWindowRect(),
						gSavedSettings.getBOOL("UseDebugLogin"),
						login_callback, NULL );

	// Make sure all the UI textures are present and decoded.
	gImageList.decodeAllImages();

	if( USERSERVER_OTHER == gUserServerChoice )
	{
		LLPanelLogin::addServer( gUserServerName, USERSERVER_OTHER );
	}
	else
	{
		LLPanelLogin::addServer( gUserServerDomainName[gUserServerChoice].mLabel, gUserServerChoice );
	}

	// Arg!  We hate loops!
	LLPanelLogin::addServer( gUserServerDomainName[USERSERVER_DMZ].mLabel,	USERSERVER_DMZ );
	LLPanelLogin::addServer( gUserServerDomainName[USERSERVER_LOCAL].mLabel,	USERSERVER_LOCAL );
	LLPanelLogin::addServer( gUserServerDomainName[USERSERVER_AGNI].mLabel,	USERSERVER_AGNI );
	LLPanelLogin::addServer( gUserServerDomainName[USERSERVER_ADITI].mLabel,	USERSERVER_ADITI );
	LLPanelLogin::addServer( gUserServerDomainName[USERSERVER_SIVA].mLabel,	USERSERVER_SIVA );
	LLPanelLogin::addServer( gUserServerDomainName[USERSERVER_DURGA].mLabel,	USERSERVER_DURGA );
	LLPanelLogin::addServer( gUserServerDomainName[USERSERVER_SHAKTI].mLabel,	USERSERVER_SHAKTI );
	LLPanelLogin::addServer( gUserServerDomainName[USERSERVER_GANGA].mLabel,	USERSERVER_GANGA );
	LLPanelLogin::addServer( gUserServerDomainName[USERSERVER_UMA].mLabel,	USERSERVER_UMA );
	LLPanelLogin::addServer( gUserServerDomainName[USERSERVER_SOMA].mLabel,	USERSERVER_SOMA );
}

// Callback for when login screen is closed.  Option 0 = connect, option 1 = quit.
void login_callback(S32 option, void *userdata)
{
	const S32 CONNECT_OPTION = 0;
	const S32 QUIT_OPTION = 1;

	if (CONNECT_OPTION == option)
	{
		gStartupState++;
		return;
	}
	else if (QUIT_OPTION == option)
	{
		// Make sure we don't save the password if the user is trying to clear it.
		LLString first, last, password;
		BOOL remember = TRUE;
		LLPanelLogin::getFields(first, last, password, remember);
		if (!remember)
		{
			// turn off the setting and write out to disk
			gSavedSettings.setBOOL("RememberPassword", FALSE);
			gSavedSettings.saveToFile(gSettingsFileName, TRUE);

			// stomp the saved password on disk
			save_password_to_disk(NULL);
		}

		LLPanelLogin::close();

		// Next iteration through main loop should shut down the app cleanly.
		gQuit = TRUE;

		return;
	}
	else
	{
		llwarns << "Unknown login button clicked" << llendl;
	}
}

LLString load_password_from_disk()
{
	LLString hashed_password("");

	// Look for legacy "marker" password from settings.ini
	hashed_password = gSavedSettings.getString("Marker");
	if (!hashed_password.empty())
	{
		// Stomp the Marker entry.
		gSavedSettings.setString("Marker", "");

		// Return that password.
		return hashed_password;
	}

	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,
													   "password.dat");
	FILE* fp = LLFile::fopen(filepath.c_str(), "rb");		/* Flawfinder: ignore */
	if (!fp)
	{
		return hashed_password;
	}

	// UUID is 16 bytes, written into ASCII is 32 characters
	// without trailing \0
	const S32 HASHED_LENGTH = 32;
	U8 buffer[HASHED_LENGTH+1];

	if (1 != fread(buffer, HASHED_LENGTH, 1, fp))
	{
		return hashed_password;
	}

	fclose(fp);

	// Decipher with MAC address
	LLXORCipher cipher(gMACAddress, 6);
	cipher.decrypt(buffer, HASHED_LENGTH);

	buffer[HASHED_LENGTH] = '\0';

	// Check to see if the mac address generated a bad hashed
	// password. It should be a hex-string or else the mac adress has
	// changed. This is a security feature to make sure that if you
	// get someone's password.dat file, you cannot hack their account.
	if(is_hex_string(buffer, HASHED_LENGTH))
	{
		hashed_password.assign((char*)buffer);
	}

	return hashed_password;
}

void save_password_to_disk(const char* hashed_password)
{
	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,
													   "password.dat");
	if (!hashed_password)
	{
		// No password, remove the file.
		LLFile::remove(filepath.c_str());
	}
	else
	{
		FILE* fp = LLFile::fopen(filepath.c_str(), "wb");		/* Flawfinder: ignore */
		if (!fp)
		{
			return;
		}

		// Encipher with MAC address
		const S32 HASHED_LENGTH = 32;
		U8 buffer[HASHED_LENGTH+1];

		LLString::copy((char*)buffer, hashed_password, HASHED_LENGTH+1);

		LLXORCipher cipher(gMACAddress, 6);
		cipher.encrypt(buffer, HASHED_LENGTH);

		fwrite(buffer, HASHED_LENGTH, 1, fp);

		fclose(fp);
	}
}

BOOL is_hex_string(U8* str, S32 len)
{
	BOOL rv = TRUE;
	U8* c = str;
	while(rv && len--)
	{
		switch(*c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			++c;
			break;
		default:
			rv = FALSE;
			break;
		}
	}
	return rv;
}

void show_first_run_dialog()
{
	gViewerWindow->alertXml("FirstRun", first_run_dialog_callback, NULL);
}

void first_run_dialog_callback(S32 option, void* userdata)
{
	if (0 == option)
	{
		llinfos << "First run dialog cancelling" << llendl;
		LLWeb::loadURL( CREATE_ACCOUNT_URL );
	}

	LLPanelLogin::giveFocus();
}



void set_startup_status(const F32 frac, const char *string, const char* msg)
{
	gViewerWindow->setProgressPercent(frac*100);
	gViewerWindow->setProgressString(string);

	gViewerWindow->setProgressMessage(msg);
}

void on_userserver_name_resolved( BOOL success, const LLString& host_name, U32 ip, void* userdata )
{
	if( STATE_RESOLVING_USERSERVER != gStartupState )
	{
		llwarns << "Userserver name callback returned during invalid state!" << llendl;
		return;
	}

	if( success )
	{
		gUserServer.setAddress( ip );
		llinfos << "...Userserver resolved to " << gUserServer << llendl;
		gStartupState = STATE_USERSERVER_RESOLVED;
	}
	else
	{
		llwarns << "setHostByName failed" << llendl;
		
		LLStringBase<char>::format_map_t args;
		args["[HOST_NAME]"] = host_name;
		gViewerWindow->alertXml("SetByHostFail", args, login_alert_done );
		gStartupState = STATE_LOGIN_SHOW;
	}
}

void login_alert_status(S32 option, void* user_data)
{
	if (0 == option)
	{
		// OK button
	}
	else if (1 == option)
	{
		// Help button
		std::string help_path;
		help_path = gDirUtilp->getExpandedFilename(LL_PATH_HELP, "unable_to_connect.html");
		load_url_local_file(help_path.c_str() );
	}

	LLPanelLogin::giveFocus();
}

void update_app(BOOL mandatory, const std::string& auth_msg)
{
	// store off config state, as we might quit soon
	gSavedSettings.saveToFile(gSettingsFileName, TRUE);

	std::ostringstream message;

	//XUI:translate
	std::string msg;
	if (!auth_msg.empty())
	{
		msg = "(" + auth_msg + ") \n";
	}
	LLStringBase<char>::format_map_t args;
	args["[MESSAGE]"] = msg;
	
	BOOL *mandatoryp = new BOOL(mandatory);

#if LL_WINDOWS
	if (mandatory)
	{
		gViewerWindow->alertXml("DownloadWindowsMandatory", args,
								update_dialog_callback,
								(void *)mandatoryp);
	}
	else
	{
#if LL_RELEASE_FOR_DOWNLOAD
		gViewerWindow->alertXml("DownloadWindowsReleaseForDownload", args,
								update_dialog_callback,
								(void *)mandatoryp);
#else
		gViewerWindow->alertXml("DownloadWindows", args,
								update_dialog_callback,
								(void *)mandatoryp);
#endif
	}
#else
	if (mandatory)
	{
		gViewerWindow->alertXml("DownloadMacMandatory", args,
								update_dialog_callback,
								(void *)mandatoryp);
	}
	else
	{
#if LL_RELEASE_FOR_DOWNLOAD
		gViewerWindow->alertXml("DownloadMacReleaseForDownload", args,
								update_dialog_callback,
								(void *)mandatoryp);
#else
		gViewerWindow->alertXml("DownloadMac", args,
								update_dialog_callback,
								(void *)mandatoryp);
#endif
	}
#endif

}


void update_dialog_callback(S32 option, void *userdata)
{
	std::string update_exe_path;
	BOOL mandatory = *(BOOL *)userdata;

#if !LL_RELEASE_FOR_DOWNLOAD
	if (option == 2)
	{
		gStartupState++;
		return;
	}
#endif
	
	if (option == 1)
	{
		// ...user doesn't want to do it
		if (mandatory)
		{
			app_force_quit();
			// Bump them back to the login screen.
			//gStartupState = STATE_LOGIN_SHOW;
		}
		else
		{
			gStartupState++;
		}
		return;
	}

#if LL_WINDOWS
	char ip[MAX_STRING];		/* Flawfinder: ignore */

	update_exe_path = gDirUtilp->getTempFilename();
	if (update_exe_path.empty())
	{
		// We're hosed, bail
		llwarns << "LLDir::getTempFilename() failed" << llendl;
		app_force_quit(NULL);
		return;
	}

	update_exe_path += ".exe";

	std::string updater_source = gDirUtilp->getAppRODataDir();
	updater_source += gDirUtilp->getDirDelimiter();
	updater_source += "updater.exe";

	llinfos << "Calling CopyFile source: " << updater_source.c_str()
			<< " dest: " << update_exe_path
			<< llendl;


	if (!CopyFileA(updater_source.c_str(), update_exe_path.c_str(), FALSE))
	{
		llinfos << "Unable to copy the updater!" << llendl;
		app_force_quit(NULL);
		return;
	}
	u32_to_ip_string(gUserServer.getAddress(), ip);

	std::ostringstream params;
	params << "-userserver " << gUserServerName;

	// if a sim name was passed in via command line parameter (typically through a SLURL)
	if ( LLURLSimString::sInstance.mSimString.length() )
	{
		// record the location to start at next time
		gSavedSettings.setString( "NextLoginLocation", LLURLSimString::sInstance.mSimString ); 
	};

	if (gHideLinks)
	{
		// Figure out the program name.
		const char* data_dir = gDirUtilp->getAppRODataDir().c_str();
		// Roll back from the end, stopping at the first '\'
		const char* program_name = data_dir + strlen(data_dir);		/* Flawfinder: ignore */
		while ( (data_dir != --program_name) &&
				*(program_name) != '\\');
		
		if ( *(program_name) == '\\')
		{
			// We found a '\'.
			program_name++;
		}
		else
		{
			// Oops.
			program_name = "SecondLife";
		}

		params << " -silent -name \"" << gSecondLife << "\" -program \"" << program_name << "\"";
	}

	llinfos << "Calling updater: " << update_exe_path << " " << params.str() << llendl;

 	remove_marker_file(); // In case updater fails
	
	// Use spawn() to run asynchronously
	int retval = _spawnl(_P_NOWAIT, update_exe_path.c_str(), update_exe_path.c_str(), params.str().c_str(), NULL);
	llinfos << "Spawn returned " << retval << llendl;

#elif LL_DARWIN
	// if a sim name was passed in via command line parameter (typically through a SLURL)
	if ( LLURLSimString::sInstance.mSimString.length() )
	{
		// record the location to start at next time
		gSavedSettings.setString( "NextLoginLocation", LLURLSimString::sInstance.mSimString ); 
	};

	update_exe_path = "'";
	update_exe_path += gDirUtilp->getAppRODataDir();
	update_exe_path += "/AutoUpdater.app/Contents/MacOS/AutoUpdater' -userserver ";
	update_exe_path += gUserServerName;
	update_exe_path += " -name \"";
	update_exe_path += gSecondLife;
	update_exe_path += "\" &";

	llinfos << "Calling updater: " << update_exe_path << llendl;

 	remove_marker_file(); // In case updater fails
	
	// Run the auto-updater.
	system(update_exe_path.c_str());		/* Flawfinder: ignore */
	
#elif LL_LINUX
	OSMessageBox("Automatic updating is not yet implemented for Linux.\n"
		"Please download the latest version from www.secondlife.com.",
		NULL, OSMB_OK);
	remove_marker_file();
	
#endif
	app_force_quit(NULL);
}

void use_circuit_callback(void**, S32 result)
{
	// bail if we're quitting.
	if(gQuit) return;
	static bool called = false;
	if(!called)
	{
		called = true;
		if (result)
		{
			// Make sure user knows something bad happened. JC
			llinfos << "Backing up to login screen!" << llendl;
			gViewerWindow->alertXml("LoginPacketNeverReceived",
				login_alert_status, NULL);
			gStartupState = STATE_LOGIN_SHOW;
		}
		else
		{
			gGotUseCircuitCodeAck = true;
		}
	}
}

void register_viewer_callbacks(LLMessageSystem* msg)
{
	msg->setHandlerFuncFast(_PREHASH_LayerData,				process_layer_data );
	msg->setHandlerFuncFast(_PREHASH_ImageData,				LLViewerImage::receiveImage );
	msg->setHandlerFuncFast(_PREHASH_ImagePacket,				LLViewerImage::receiveImagePacket );
	msg->setHandlerFuncFast(_PREHASH_ObjectUpdate,				process_object_update );
	msg->setHandlerFunc("ObjectUpdateCompressed",				process_compressed_object_update );
	msg->setHandlerFunc("ObjectUpdateCached",					process_cached_object_update );
	msg->setHandlerFuncFast(_PREHASH_ImprovedTerseObjectUpdate, process_terse_object_update_improved );
	msg->setHandlerFunc("SimStats",				process_sim_stats);
	msg->setHandlerFuncFast(_PREHASH_HealthMessage,			process_health_message );
	msg->setHandlerFuncFast(_PREHASH_EconomyData,				process_economy_data);
	msg->setHandlerFunc("RegionInfo", LLViewerRegion::processRegionInfo);

	msg->setHandlerFuncFast(_PREHASH_ChatFromSimulator,		process_chat_from_simulator);
	msg->setHandlerFuncFast(_PREHASH_KillObject,				process_kill_object,	NULL);
	msg->setHandlerFuncFast(_PREHASH_SimulatorViewerTimeMessage,	process_time_synch,		NULL);
	msg->setHandlerFuncFast(_PREHASH_EnableSimulator,			process_enable_simulator);
	msg->setHandlerFuncFast(_PREHASH_DisableSimulator,			process_disable_simulator);
	msg->setHandlerFuncFast(_PREHASH_KickUser,					process_kick_user,		NULL);

	msg->setHandlerFunc("CrossedRegion", process_crossed_region);
	msg->setHandlerFuncFast(_PREHASH_TeleportFinish, process_teleport_finish);

	msg->setHandlerFuncFast(_PREHASH_AlertMessage,             process_alert_message);
	msg->setHandlerFunc("AgentAlertMessage", process_agent_alert_message);
	msg->setHandlerFuncFast(_PREHASH_MeanCollisionAlert,             process_mean_collision_alert_message,  NULL);
	msg->setHandlerFunc("ViewerFrozenMessage",             process_frozen_message);

	msg->setHandlerFuncFast(_PREHASH_RequestAvatarInfo,		process_avatar_info_request);
	msg->setHandlerFuncFast(_PREHASH_NameValuePair,			process_name_value);
	msg->setHandlerFuncFast(_PREHASH_RemoveNameValuePair,	process_remove_name_value);
	msg->setHandlerFuncFast(_PREHASH_AvatarAnimation,		process_avatar_animation);
	msg->setHandlerFuncFast(_PREHASH_AvatarAppearance,		process_avatar_appearance);
	msg->setHandlerFunc("AgentCachedTextureResponse",	LLAgent::processAgentCachedTextureResponse);
	msg->setHandlerFunc("RebakeAvatarTextures", LLVOAvatar::processRebakeAvatarTextures);
	msg->setHandlerFuncFast(_PREHASH_CameraConstraint,		process_camera_constraint);
	msg->setHandlerFuncFast(_PREHASH_AvatarSitResponse,		process_avatar_sit_response);
	msg->setHandlerFunc("SetFollowCamProperties",			process_set_follow_cam_properties);
	msg->setHandlerFunc("ClearFollowCamProperties",			process_clear_follow_cam_properties);

	msg->setHandlerFuncFast(_PREHASH_ImprovedInstantMessage,	process_improved_im);
	msg->setHandlerFuncFast(_PREHASH_ScriptQuestion,			process_script_question);
	msg->setHandlerFuncFast(_PREHASH_ObjectProperties,			LLSelectMgr::processObjectProperties, NULL);
	msg->setHandlerFuncFast(_PREHASH_ObjectPropertiesFamily,	LLSelectMgr::processObjectPropertiesFamily, NULL);
	msg->setHandlerFunc("ForceObjectSelect", LLSelectMgr::processForceObjectSelect);

	msg->setHandlerFuncFast(_PREHASH_MoneyBalanceReply,		process_money_balance_reply,	NULL);
	msg->setHandlerFuncFast(_PREHASH_CoarseLocationUpdate,		LLWorld::processCoarseUpdate, NULL);
	msg->setHandlerFuncFast(_PREHASH_ReplyTaskInventory, 		LLViewerObject::processTaskInv,	NULL);
	msg->setHandlerFuncFast(_PREHASH_DerezContainer,			process_derez_container, NULL);
	msg->setHandlerFuncFast(_PREHASH_ScriptRunningReply,
						&LLLiveLSLEditor::processScriptRunningReply);

	msg->setHandlerFuncFast(_PREHASH_DeRezAck, process_derez_ack);

	msg->setHandlerFunc("LogoutReply", process_logout_reply);

	//msg->setHandlerFuncFast(_PREHASH_AddModifyAbility,
	//					&LLAgent::processAddModifyAbility);
	//msg->setHandlerFuncFast(_PREHASH_RemoveModifyAbility,
	//					&LLAgent::processRemoveModifyAbility);
	msg->setHandlerFuncFast(_PREHASH_AgentDataUpdate,
						&LLAgent::processAgentDataUpdate);
	msg->setHandlerFuncFast(_PREHASH_AgentGroupDataUpdate,
						&LLAgent::processAgentGroupDataUpdate);
	msg->setHandlerFunc("AgentDropGroup",
						&LLAgent::processAgentDropGroup);
	// land ownership messages
	msg->setHandlerFuncFast(_PREHASH_ParcelOverlay,
						LLViewerParcelMgr::processParcelOverlay);
	msg->setHandlerFuncFast(_PREHASH_ParcelProperties,
						LLViewerParcelMgr::processParcelProperties);
	msg->setHandlerFunc("ParcelAccessListReply",
		LLViewerParcelMgr::processParcelAccessListReply);
	msg->setHandlerFunc("ParcelDwellReply",
		LLViewerParcelMgr::processParcelDwellReply);

	msg->setHandlerFunc("AvatarPropertiesReply",
						LLPanelAvatar::processAvatarPropertiesReply);
	msg->setHandlerFunc("AvatarInterestsReply",
						LLPanelAvatar::processAvatarInterestsReply);
	msg->setHandlerFunc("AvatarGroupsReply",
						LLPanelAvatar::processAvatarGroupsReply);
	msg->setHandlerFuncFast(_PREHASH_AvatarStatisticsReply,
						LLPanelAvatar::processAvatarStatisticsReply);
	msg->setHandlerFunc("AvatarNotesReply",
						LLPanelAvatar::processAvatarNotesReply);
	msg->setHandlerFunc("AvatarPicksReply",
						LLPanelAvatar::processAvatarPicksReply);
	msg->setHandlerFunc("AvatarClassifiedReply",
						LLPanelAvatar::processAvatarClassifiedReply);

	msg->setHandlerFuncFast(_PREHASH_CreateGroupReply,
						LLGroupMgr::processCreateGroupReply);
	msg->setHandlerFuncFast(_PREHASH_JoinGroupReply,
						LLGroupMgr::processJoinGroupReply);
	msg->setHandlerFuncFast(_PREHASH_EjectGroupMemberReply,
						LLGroupMgr::processEjectGroupMemberReply);
	msg->setHandlerFuncFast(_PREHASH_LeaveGroupReply,
						LLGroupMgr::processLeaveGroupReply);
	msg->setHandlerFuncFast(_PREHASH_GroupProfileReply,
						LLGroupMgr::processGroupPropertiesReply);

	msg->setHandlerFuncFast(_PREHASH_ReputationIndividualReply,
						LLFloaterRate::processReputationIndividualReply);

	msg->setHandlerFuncFast(_PREHASH_AgentWearablesUpdate,
						LLAgent::processAgentInitialWearablesUpdate );

	msg->setHandlerFunc("ScriptControlChange",
						LLAgent::processScriptControlChange );

	msg->setHandlerFuncFast(_PREHASH_GestureUpdate,
						LLViewerGestureList::processGestureUpdate);

	msg->setHandlerFuncFast(_PREHASH_ViewerEffect, LLHUDManager::processViewerEffect);

	msg->setHandlerFuncFast(_PREHASH_GrantGodlikePowers, process_grant_godlike_powers);

	msg->setHandlerFuncFast(_PREHASH_GroupAccountSummaryReply,
							LLGroupMoneyPlanningTabEventHandler::processGroupAccountSummaryReply);
	msg->setHandlerFuncFast(_PREHASH_GroupAccountDetailsReply,
							LLGroupMoneyDetailsTabEventHandler::processGroupAccountDetailsReply);
	msg->setHandlerFuncFast(_PREHASH_GroupAccountTransactionsReply,
							LLGroupMoneySalesTabEventHandler::processGroupAccountTransactionsReply);

	msg->setHandlerFuncFast(_PREHASH_UserInfoReply,
		process_user_info_reply);

	msg->setHandlerFunc("RegionHandshake", process_region_handshake, NULL);

	msg->setHandlerFunc("TeleportStart", process_teleport_start );
	msg->setHandlerFunc("TeleportProgress", process_teleport_progress);
	msg->setHandlerFunc("TeleportFailed", process_teleport_failed, NULL);
	msg->setHandlerFunc("TeleportLocal", process_teleport_local, NULL);

	msg->setHandlerFunc("ImageNotInDatabase", LLViewerImageList::processImageNotInDatabase, NULL);

	msg->setHandlerFuncFast(_PREHASH_GroupMembersReply,
						LLGroupMgr::processGroupMembersReply);
	msg->setHandlerFunc("GroupRoleDataReply",
						LLGroupMgr::processGroupRoleDataReply);
	msg->setHandlerFunc("GroupRoleMembersReply",
						LLGroupMgr::processGroupRoleMembersReply);
	msg->setHandlerFunc("GroupTitlesReply",
						LLGroupMgr::processGroupTitlesReply);
	// Special handler as this message is sometimes used for group land.
	msg->setHandlerFunc("PlacesReply", process_places_reply);
	msg->setHandlerFunc("GroupNoticesListReply", LLPanelGroupNotices::processGroupNoticesListReply);

	msg->setHandlerFunc("DirPlacesReply", LLPanelDirBrowser::processDirPlacesReply);
	msg->setHandlerFunc("DirPeopleReply", LLPanelDirBrowser::processDirPeopleReply);
	msg->setHandlerFunc("DirEventsReply", LLPanelDirBrowser::processDirEventsReply);
	msg->setHandlerFunc("DirGroupsReply", LLPanelDirBrowser::processDirGroupsReply);
	//msg->setHandlerFunc("DirPicksReply",  LLPanelDirBrowser::processDirPicksReply);
	msg->setHandlerFunc("DirClassifiedReply",  LLPanelDirBrowser::processDirClassifiedReply);
	msg->setHandlerFunc("DirLandReply",   LLPanelDirBrowser::processDirLandReply);
	msg->setHandlerFunc("DirPopularReply",LLPanelDirBrowser::processDirPopularReply);

	msg->setHandlerFunc("AvatarPickerReply", LLFloaterAvatarPicker::processAvatarPickerReply);

	msg->setHandlerFunc("MapLayerReply", LLWorldMap::processMapLayerReply);
	msg->setHandlerFunc("MapBlockReply", LLWorldMap::processMapBlockReply);
	msg->setHandlerFunc("MapItemReply", LLWorldMap::processMapItemReply);

	msg->setHandlerFunc("EventInfoReply", LLPanelEvent::processEventInfoReply);
	msg->setHandlerFunc("PickInfoReply", LLPanelPick::processPickInfoReply);
	msg->setHandlerFunc("ClassifiedInfoReply", LLPanelClassified::processClassifiedInfoReply);
	msg->setHandlerFunc("ParcelInfoReply", LLPanelPlace::processParcelInfoReply);
	msg->setHandlerFunc("ScriptDialog", process_script_dialog);
	msg->setHandlerFunc("LoadURL", process_load_url);
	msg->setHandlerFunc("ScriptTeleportRequest", process_script_teleport_request);
	msg->setHandlerFunc("EstateCovenantReply", process_covenant_reply);

	// calling cards
	msg->setHandlerFunc("OfferCallingCard", process_offer_callingcard);
	msg->setHandlerFunc("AcceptCallingCard", process_accept_callingcard);
	msg->setHandlerFunc("DeclineCallingCard", process_decline_callingcard);

	msg->setHandlerFunc("ParcelObjectOwnersReply", LLPanelLandObjects::processParcelObjectOwnersReply);

	// Reponse to the "Refresh" button on land objects floater.
	if (gSavedSettings.getBOOL("AudioStreamingVideo"))
	{
		msg->setHandlerFunc("ParcelMediaCommandMessage", LLMediaEngine::process_parcel_media);
		msg->setHandlerFunc ( "ParcelMediaUpdate", LLMediaEngine::process_parcel_media_update );
	}
	else
	{
		msg->setHandlerFunc("ParcelMediaCommandMessage", null_message_callback);
		gMessageSystem->setHandlerFunc ( "ParcelMediaUpdate", null_message_callback );
	}

	msg->setHandlerFunc("InitiateDownload", process_initiate_download);
	msg->setHandlerFunc("LandStatReply", LLFloaterTopObjects::handle_land_reply);
	msg->setHandlerFunc("GenericMessage", process_generic_message);

	msg->setHandlerFuncFast(_PREHASH_FeatureDisabled, process_feature_disabled_message);
}


void init_stat_view()
{
	LLFrameStatView *frameviewp = gDebugView->mFrameStatView;
	frameviewp->setup(gFrameStats);
	frameviewp->mShowPercent = FALSE;

	LLRect rect;
	LLStatBar *stat_barp;
	rect = gDebugView->mStatViewp->getRect();

	//
	// Viewer advanced stats
	//
	LLStatView *stat_viewp = NULL;

	//
	// Viewer Basic
	//
	stat_viewp = new LLStatView("basic stat view", "Basic",	"OpenDebugStatBasic", rect);
	gDebugView->mStatViewp->addChildAtEnd(stat_viewp);

	stat_barp = stat_viewp->addStat("FPS", &(gViewerStats->mFPSStat));
	stat_barp->setUnitLabel(" fps");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 45.f;
	stat_barp->mTickSpacing = 7.5f;
	stat_barp->mLabelSpacing = 15.f;
	stat_barp->mPrecision = 1;
	stat_barp->mDisplayBar = TRUE;
	stat_barp->mDisplayHistory = TRUE;

	stat_barp = stat_viewp->addStat("Bandwidth", &(gViewerStats->mKBitStat));
	stat_barp->setUnitLabel(" kbps");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 900.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 300.f;
	stat_barp->mDisplayBar = TRUE;
	stat_barp->mDisplayHistory = FALSE;

	stat_barp = stat_viewp->addStat("Packet Loss", &(gViewerStats->mPacketsLostPercentStat));
	stat_barp->setUnitLabel(" %%");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 5.f;
	stat_barp->mTickSpacing = 1.f;
	stat_barp->mLabelSpacing = 1.f;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayMean = TRUE;
	stat_barp->mPrecision = 1;

	stat_barp = stat_viewp->addStat("Ping Sim", &(gViewerStats->mSimPingStat));
	stat_barp->setUnitLabel(" msec");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 1000.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 200.f;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = stat_viewp->addStat("Ping User", &(gViewerStats->mUserserverPingStat));
	stat_barp->setUnitLabel(" msec");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 1000.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 200.f;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayMean = FALSE;


	stat_viewp = new LLStatView("advanced stat view", "Advanced", "OpenDebugStatAdvanced", rect);
	gDebugView->mStatViewp->addChildAtEnd(stat_viewp);


	LLStatView *render_statviewp;
	render_statviewp = new LLStatView("render stat view", "Render", "OpenDebugStatRender", rect);
	stat_viewp->addChildAtEnd(render_statviewp);

	stat_barp = render_statviewp->addStat("KTris Drawn", &(gPipeline.mTrianglesDrawnStat));
	stat_barp->setUnitLabel("/fr");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 500.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 500.f;
	stat_barp->mPrecision = 1;
	stat_barp->mPerSec = FALSE;

	stat_barp = render_statviewp->addStat("KTris Drawn", &(gPipeline.mTrianglesDrawnStat));
	stat_barp->setUnitLabel("/sec");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 3000.f;
	stat_barp->mTickSpacing = 250.f;
	stat_barp->mLabelSpacing = 1000.f;
	stat_barp->mPrecision = 1;

	stat_barp = render_statviewp->addStat("Total Objs", &(gObjectList.mNumObjectsStat));
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 10000.f;
	stat_barp->mTickSpacing = 2500.f;
	stat_barp->mLabelSpacing = 5000.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;

	stat_barp = render_statviewp->addStat("New Objs", &(gObjectList.mNumNewObjectsStat));
	stat_barp->setLabel("New Objs");
	stat_barp->setUnitLabel("/sec");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 1000.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 500.f;
	stat_barp->mPerSec = TRUE;
	stat_barp->mDisplayBar = FALSE;


	// Pipeline statistics
	LLStatView *pipeline_statviewp;
	pipeline_statviewp = new LLStatView("pipeline stat view", "Pipeline", "", rect);
	render_statviewp->addChildAtEnd(pipeline_statviewp);

	stat_barp = pipeline_statviewp->addStat("Visible Drawables", &(gPipeline.mNumVisibleDrawablesStat));
	stat_barp->setUnitLabel("");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 10000.f;
	stat_barp->mTickSpacing = 1000.f;
	stat_barp->mLabelSpacing = 5000.f;
	stat_barp->mPerSec = FALSE;

	stat_barp = pipeline_statviewp->addStat("Visible Faces", &(gPipeline.mNumVisibleFacesStat));
	stat_barp->setUnitLabel("");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 40000.f;
	stat_barp->mTickSpacing = 5000.f;
	stat_barp->mLabelSpacing = 10000.f;
	stat_barp->mPerSec = FALSE;

	stat_barp = pipeline_statviewp->addStat("DirtyGeom", &(gPipeline.mGeometryChangesStat));
	stat_barp->setUnitLabel("/fr");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 1000.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 500.f;
	stat_barp->mPerSec = FALSE;

	stat_barp = pipeline_statviewp->addStat("DirtyLight", &(gPipeline.mLightingChangesStat));
	stat_barp->setUnitLabel("/fr");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 1000.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 500.f;
	stat_barp->mPerSec = FALSE;

	stat_barp = pipeline_statviewp->addStat("MoveList", &(gPipeline.mMoveChangesStat));
	stat_barp->setUnitLabel("dr");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 1000.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 500.f;
	stat_barp->mPerSec = FALSE;

	stat_barp = pipeline_statviewp->addStat("Compiles", &(gPipeline.mCompilesStat));
	stat_barp->setUnitLabel("/fr");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 1000.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 500.f;
	stat_barp->mPerSec = FALSE;

	stat_barp = pipeline_statviewp->addStat("Verts Relit", &(gPipeline.mVerticesRelitStat));
	stat_barp->setUnitLabel("/fr");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 40000.f;
	stat_barp->mTickSpacing = 10000.f;
	stat_barp->mLabelSpacing = 20000.f;
	stat_barp->mPerSec = FALSE;

	// Texture statistics
	LLStatView *texture_statviewp;
	texture_statviewp = new LLStatView("texture stat view", "Texture", "", rect);
	render_statviewp->addChildAtEnd(texture_statviewp);

	stat_barp = texture_statviewp->addStat("Count", &(gImageList.sNumImagesStat));
	stat_barp->setUnitLabel("");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 8000.f;
	stat_barp->mTickSpacing = 2000.f;
	stat_barp->mLabelSpacing = 4000.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;

	stat_barp = texture_statviewp->addStat("Raw Count", &(gImageList.sNumRawImagesStat));
	stat_barp->setUnitLabel("");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 8000.f;
	stat_barp->mTickSpacing = 2000.f;
	stat_barp->mLabelSpacing = 4000.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;

	stat_barp = texture_statviewp->addStat("GL Mem", &(gImageList.sGLTexMemStat));
	stat_barp->setUnitLabel("");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 400.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 200.f;
	stat_barp->mPrecision = 1;
	stat_barp->mPerSec = FALSE;

	stat_barp = texture_statviewp->addStat("Formatted Mem", &(gImageList.sFormattedMemStat));
	stat_barp->setUnitLabel("");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 400.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 200.f;
	stat_barp->mPrecision = 1;
	stat_barp->mPerSec = FALSE;

	stat_barp = texture_statviewp->addStat("Raw Mem", &(gImageList.sRawMemStat));
	stat_barp->setUnitLabel("");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 400.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 200.f;
	stat_barp->mPrecision = 1;
	stat_barp->mPerSec = FALSE;

	stat_barp = texture_statviewp->addStat("Bound Mem", &(gImageList.sGLBoundMemStat));
	stat_barp->setUnitLabel("");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 400.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 200.f;
	stat_barp->mPrecision = 1;
	stat_barp->mPerSec = FALSE;

	
	// Network statistics
	LLStatView *net_statviewp;
	net_statviewp = new LLStatView("network stat view", "Network", "OpenDebugStatNet", rect);
	stat_viewp->addChildAtEnd(net_statviewp);

	stat_barp = net_statviewp->addStat("Packets In", &(gViewerStats->mPacketsInStat));
	stat_barp->setUnitLabel("/sec");
	stat_barp->mDisplayBar = FALSE;

	stat_barp = net_statviewp->addStat("Packets Out", &(gViewerStats->mPacketsOutStat));
	stat_barp->setUnitLabel("/sec");
	stat_barp->mDisplayBar = FALSE;

	stat_barp = net_statviewp->addStat("Objects", &(gViewerStats->mObjectKBitStat));
	stat_barp->setUnitLabel(" kbps");
	stat_barp->mDisplayBar = FALSE;

	stat_barp = net_statviewp->addStat("Texture", &(gViewerStats->mTextureKBitStat));
	stat_barp->setUnitLabel(" kbps");
	stat_barp->mDisplayBar = FALSE;

	stat_barp = net_statviewp->addStat("Asset", &(gViewerStats->mAssetKBitStat));
	stat_barp->setUnitLabel(" kbps");
	stat_barp->mDisplayBar = FALSE;

	stat_barp = net_statviewp->addStat("Layers", &(gViewerStats->mLayersKBitStat));
	stat_barp->setUnitLabel(" kbps");
	stat_barp->mDisplayBar = FALSE;

	stat_barp = net_statviewp->addStat("Actual In", &(gViewerStats->mActualInKBitStat));
	stat_barp->setUnitLabel(" kbps");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 1024.f;
	stat_barp->mTickSpacing = 128.f;
	stat_barp->mLabelSpacing = 256.f;
	stat_barp->mDisplayBar = TRUE;
	stat_barp->mDisplayHistory = FALSE;

	stat_barp = net_statviewp->addStat("Actual Out", &(gViewerStats->mActualOutKBitStat));
	stat_barp->setUnitLabel(" kbps");
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 512.f;
	stat_barp->mTickSpacing = 128.f;
	stat_barp->mLabelSpacing = 256.f;
	stat_barp->mDisplayBar = TRUE;
	stat_barp->mDisplayHistory = FALSE;

	stat_barp = net_statviewp->addStat("VFS Pending Ops", &(gViewerStats->mVFSPendingOperations));
	stat_barp->setUnitLabel(" ");
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;


	// Simulator stats
	LLStatView *sim_statviewp = new LLStatView("sim stat view", "Simulator", "OpenDebugStatSim", rect);
	gDebugView->mStatViewp->addChildAtEnd(sim_statviewp);

	stat_barp = sim_statviewp->addStat("Time Dilation", &(gViewerStats->mSimTimeDilation));
	stat_barp->mPrecision = 2;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 1.f;
	stat_barp->mTickSpacing = 0.25f;
	stat_barp->mLabelSpacing = 0.5f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_statviewp->addStat("Sim FPS", &(gViewerStats->mSimFPS));
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 200.f;
	stat_barp->mTickSpacing = 20.f;
	stat_barp->mLabelSpacing = 100.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_statviewp->addStat("Physics FPS", &(gViewerStats->mSimPhysicsFPS));
	stat_barp->mPrecision = 1;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 66.f;
	stat_barp->mTickSpacing = 33.f;
	stat_barp->mLabelSpacing = 33.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_statviewp->addStat("Agent Updates/Sec", &(gViewerStats->mSimAgentUPS));
	stat_barp->mPrecision = 1;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 100.f;
	stat_barp->mTickSpacing = 25.f;
	stat_barp->mLabelSpacing = 50.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_statviewp->addStat("Main Agents", &(gViewerStats->mSimMainAgents));
	stat_barp->mPrecision = 0;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 80.f;
	stat_barp->mTickSpacing = 10.f;
	stat_barp->mLabelSpacing = 40.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_statviewp->addStat("Child Agents", &(gViewerStats->mSimChildAgents));
	stat_barp->mPrecision = 0;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 40.f;
	stat_barp->mTickSpacing = 5.f;
	stat_barp->mLabelSpacing = 10.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_statviewp->addStat("Objects", &(gViewerStats->mSimObjects));
	stat_barp->mPrecision = 0;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 30000.f;
	stat_barp->mTickSpacing = 5000.f;
	stat_barp->mLabelSpacing = 10000.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_statviewp->addStat("Active Objects", &(gViewerStats->mSimActiveObjects));
	stat_barp->mPrecision = 0;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 800.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 200.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_statviewp->addStat("Active Scripts", &(gViewerStats->mSimActiveScripts));
	stat_barp->mPrecision = 0;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 800.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 200.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_statviewp->addStat("Script Perf", &(gViewerStats->mSimLSLIPS));
	stat_barp->setUnitLabel(" ips");
	stat_barp->mPrecision = 0;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 100000.f;
	stat_barp->mTickSpacing = 25000.f;
	stat_barp->mLabelSpacing = 50000.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_statviewp->addStat("Packets In", &(gViewerStats->mSimInPPS));
	stat_barp->setUnitLabel(" pps");
	stat_barp->mPrecision = 0;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 2000.f;
	stat_barp->mTickSpacing = 250.f;
	stat_barp->mLabelSpacing = 1000.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_statviewp->addStat("Packets Out", &(gViewerStats->mSimOutPPS));
	stat_barp->setUnitLabel(" pps");
	stat_barp->mPrecision = 0;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 2000.f;
	stat_barp->mTickSpacing = 250.f;
	stat_barp->mLabelSpacing = 1000.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_statviewp->addStat("Pending Downloads", &(gViewerStats->mSimPendingDownloads));
	stat_barp->mPrecision = 0;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 800.f;
	stat_barp->mTickSpacing = 100.f;
	stat_barp->mLabelSpacing = 200.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_statviewp->addStat("Pending Uploads", &(gViewerStats->mSimPendingUploads));
	stat_barp->mPrecision = 0;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 100.f;
	stat_barp->mTickSpacing = 25.f;
	stat_barp->mLabelSpacing = 50.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_statviewp->addStat("Total Unacked Bytes", &(gViewerStats->mSimTotalUnackedBytes));
	stat_barp->setUnitLabel(" kb");
	stat_barp->mPrecision = 0;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 100000.f;
	stat_barp->mTickSpacing = 25000.f;
	stat_barp->mLabelSpacing = 50000.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	LLStatView *sim_time_viewp;
	sim_time_viewp = new LLStatView("sim perf view", "Time (ms)", "", rect);
	sim_statviewp->addChildAtEnd(sim_time_viewp);

	stat_barp = sim_time_viewp->addStat("Total Frame Time", &(gViewerStats->mSimFrameMsec));
	stat_barp->setUnitLabel("ms");
	stat_barp->mPrecision = 1;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 40.f;
	stat_barp->mTickSpacing = 10.f;
	stat_barp->mLabelSpacing = 20.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_time_viewp->addStat("Net Time", &(gViewerStats->mSimNetMsec));
	stat_barp->setUnitLabel("ms");
	stat_barp->mPrecision = 1;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 40.f;
	stat_barp->mTickSpacing = 10.f;
	stat_barp->mLabelSpacing = 20.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_time_viewp->addStat("Sim Time (Physics)", &(gViewerStats->mSimSimPhysicsMsec));
	stat_barp->setUnitLabel("ms");
	stat_barp->mPrecision = 1;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 40.f;
	stat_barp->mTickSpacing = 10.f;
	stat_barp->mLabelSpacing = 20.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_time_viewp->addStat("Sim Time (Other)", &(gViewerStats->mSimSimOtherMsec));
	stat_barp->setUnitLabel("ms");
	stat_barp->mPrecision = 1;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 40.f;
	stat_barp->mTickSpacing = 10.f;
	stat_barp->mLabelSpacing = 20.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_time_viewp->addStat("Agent Time", &(gViewerStats->mSimAgentMsec));
	stat_barp->setUnitLabel("ms");
	stat_barp->mPrecision = 1;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 40.f;
	stat_barp->mTickSpacing = 10.f;
	stat_barp->mLabelSpacing = 20.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_time_viewp->addStat("Images Time", &(gViewerStats->mSimImagesMsec));
	stat_barp->setUnitLabel("ms");
	stat_barp->mPrecision = 1;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 40.f;
	stat_barp->mTickSpacing = 10.f;
	stat_barp->mLabelSpacing = 20.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	stat_barp = sim_time_viewp->addStat("Script Time", &(gViewerStats->mSimScriptMsec));
	stat_barp->setUnitLabel("ms");
	stat_barp->mPrecision = 1;
	stat_barp->mMinBar = 0.f;
	stat_barp->mMaxBar = 40.f;
	stat_barp->mTickSpacing = 10.f;
	stat_barp->mLabelSpacing = 20.f;
	stat_barp->mPerSec = FALSE;
	stat_barp->mDisplayBar = FALSE;
	stat_barp->mDisplayMean = FALSE;

	LLRect r = gDebugView->mStatViewp->getRect();

	// Reshape based on the parameters we set.
	gDebugView->mStatViewp->reshape(r.getWidth(), r.getHeight());
}

void asset_callback_nothing(LLVFS*, const LLUUID&, LLAssetType::EType, void*, S32)
{
	// nothing
}

// *HACK: Must match name in Library or agent inventory
const char* COMMON_GESTURES_FOLDER = "Common Gestures";
const char* MALE_GESTURES_FOLDER = "Male Gestures";
const char* FEMALE_GESTURES_FOLDER = "Female Gestures";
const char* MALE_OUTFIT_FOLDER = "Male Shape & Outfit";
const char* FEMALE_OUTFIT_FOLDER = "Female Shape & Outfit";
const S32 OPT_USE_INITIAL_OUTFIT = -2;
const S32 OPT_CLOSED_WINDOW = -1;
const S32 OPT_MALE = 0;
const S32 OPT_FEMALE = 1;

void callback_choose_gender(S32 option, void* userdata)
{
	S32 gender = OPT_FEMALE;
	const char* outfit = FEMALE_OUTFIT_FOLDER;
	const char* gestures = FEMALE_GESTURES_FOLDER;
	const char* common_gestures = COMMON_GESTURES_FOLDER;
	if (!gInitialOutfit.empty())
	{
		outfit = gInitialOutfit.c_str();
		if (gInitialOutfitGender == "male")
		{
			gender = OPT_MALE;
			gestures = MALE_GESTURES_FOLDER;
		}
		else
		{
			gender = OPT_FEMALE;
			gestures = FEMALE_GESTURES_FOLDER;
		}
	}
	else
	{
		switch(option)
		{
		case OPT_MALE:
			gender = OPT_MALE;
			outfit = MALE_OUTFIT_FOLDER;
			gestures = MALE_GESTURES_FOLDER;
			break;

		case OPT_FEMALE:
		case OPT_CLOSED_WINDOW:
		default:
			gender = OPT_FEMALE;
			outfit = FEMALE_OUTFIT_FOLDER;
			gestures = FEMALE_GESTURES_FOLDER;
			break;
		}
	}

	// try to find the outfit - if not there, create some default
	// wearables.
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	LLNameCategoryCollector has_name(outfit);
	gInventory.collectDescendentsIf(LLUUID::null,
									cat_array,
									item_array,
									LLInventoryModel::EXCLUDE_TRASH,
									has_name);
	if (0 == cat_array.count())
	{
		gAgent.createStandardWearables(gender);
	}
	else
	{
		wear_outfit_by_name(outfit);
	}
	wear_outfit_by_name(gestures);
	wear_outfit_by_name(common_gestures);

	typedef std::map<LLUUID, LLMultiGesture*> item_map_t;
	item_map_t::iterator gestureIterator;

	// Must be here so they aren't invisible if they close the window.
	gAgent.setGenderChosen(TRUE);
}

// XUI:translate
void dialog_choose_gender_first_start()
{
	if (!gNoRender
		&& (!gAgent.isGenderChosen()))
	{
		if (!gInitialOutfit.empty())
		{
			gViewerWindow->alertXml("WelcomeNoClothes",
				callback_choose_gender, NULL);
		}
		else
		{	
			gViewerWindow->alertXml("WelcomeChooseSex",
				callback_choose_gender, NULL);
			
		}
	}
}

// Loads a bitmap to display during load
// location_id = 0 => last position
// location_id = 1 => home position
void init_start_screen(S32 location_id)
{
	if (gStartImageGL)
	{
		gStartImageGL = NULL;
		llinfos << "re-initializing start screen" << llendl;
	}

	llinfos << "Loading startup bitmap..." << llendl;

	LLString temp_str = gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter();

	if ((S32)START_LOCATION_ID_LAST == location_id)
	{
		temp_str += SCREEN_LAST_FILENAME;
	}
	else
	{
		temp_str += SCREEN_HOME_FILENAME;
	}

	LLPointer<LLImageBMP> start_image_bmp = new LLImageBMP;
	if( !start_image_bmp->load(temp_str) )
	{
		llinfos << "Bitmap load failed" << llendl;
		return;
	}

	gStartImageGL = new LLImageGL(FALSE);
	gStartImageWidth = start_image_bmp->getWidth();
	gStartImageHeight = start_image_bmp->getHeight();
	LLPointer<LLImageRaw> raw = new LLImageRaw;
	if (!start_image_bmp->decode(raw))
	{
		llinfos << "Bitmap decode failed" << llendl;
		gStartImageGL = NULL;
		return;
	}

	raw->expandToPowerOfTwo();
	gStartImageGL->createGLTexture(0, raw);
}


// frees the bitmap
void release_start_screen()
{
	//llinfos << "Releasing bitmap..." << llendl;
	gStartImageGL = NULL;
}

void process_connect_to_userserver(LLMessageSystem* msg, void**)
{
	// Sent unreliably since if we've become disconnected, the
	// userserver will get back to us eventually. By sending reliable,
	// we also may accidently induce two separate validations under
	// conditions where the userserver is already lagged.
	msg->newMessage("ConnectAgentToUserserver");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->sendMessage(gUserServer);
}

bool LLStartUp::canGoFullscreen()
{
	return gStartupState >= STATE_WORLD_INIT;
}


