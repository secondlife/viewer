/** 
 * @file llappviewer.h
 * @brief The LLAppViewer class declaration
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

#ifndef LL_LLAPPVIEWER_H
#define LL_LLAPPVIEWER_H

class LLTextureCache;
class LLWorkerThread;
class LLTextureFetch;

class LLCommandLineParser;

class LLAppViewer : public LLApp
{
public:
	LLAppViewer();
	virtual ~LLAppViewer();

    /**
     * @brief Access to the LLAppViewer singleton.
     * 
     * The LLAppViewer singleton is created in main()/WinMain().
     * So don't use it in pre-entry (static initialization) code.
     */
    static LLAppViewer* instance() {return sInstance; } 

	//
	// Main application logic
	//
	virtual bool init();			// Override to do application initialization
	virtual bool cleanup();			// Override to do application cleanup
	virtual bool mainLoop(); // Override for the application main loop.  Needs to at least gracefully notice the QUITTING state and exit.

	// Application control
	void forceQuit(); // Puts the viewer into 'shutting down without error' mode.
	void requestQuit(); // Request a quit. A kinder, gentler quit.
	void userQuit(); // The users asks to quit. Confirm, then requestQuit()
    void earlyExit(const LLString& msg); // Display an error dialog and forcibly quit.
    void forceExit(S32 arg); // exit() immediately (after some cleanup).
    void abortQuit();  // Called to abort a quit request.

    bool quitRequested() { return mQuitRequested; }
    bool logoutRequestSent() { return mLogoutRequestSent; }

	void closeDebug();

	const LLOSInfo& getOSInfo() const { return mSysOSInfo; }

	// Report true if under the control of a debugger. A null-op default.
	virtual bool beingDebugged() { return false; } 

	S32 getCrashBehavior() const { return mCrashBehavior; } 
	void setCrashBehavior(S32 cb);
	virtual void handleCrashReporting() = 0; // What to do with crash report?
	static void handleViewerCrash(); // Hey! The viewer crashed. Do this.

	// Thread accessors
	static LLTextureCache* getTextureCache() { return sTextureCache; }
	static LLWorkerThread* getImageDecodeThread() { return sImageDecodeThread; }
	static LLTextureFetch* getTextureFetch() { return sTextureFetch; }

	const std::string& getSerialNumber() { return mSerialNumber; }
	
	bool getPurgeCache() const { return mPurgeCache; }
	
	const LLString& getSecondLifeTitle() const; // The Second Life title.
	const LLString& getWindowTitle() const; // The window display name.

    // Helpers for URIs
    void addLoginURI(const std::string& uri);
    void setHelperURI(const std::string& uri);
    const std::vector<std::string>& getLoginURIs() const;
    const std::string& getHelperURI() const;
    void resetURIs() const;

    void forceDisconnect(const LLString& msg); // Force disconnection, with a message to the user.
    void badNetworkHandler(); // Cause a crash state due to bad network packet.

	bool hasSavedFinalSnapshot() { return mSavedFinalSnapshot; }
	void saveFinalSnapshot(); 

    void loadNameCache();
    void saveNameCache();

    bool isInProductionGrid();

	void removeMarkerFile();
	
    // LLAppViewer testing helpers.
    // *NOTE: These will potentially crash the viewer. Only for debugging.
    virtual void forceErrorLLError();
    virtual void forceErrorBreakpoint();
    virtual void forceErrorBadMemoryAccess();
    virtual void forceErrorInifiniteLoop();
    virtual void forceErrorSoftwareException();

	void loadSettingsFromDirectory(ELLPath path_index);
protected:
	virtual bool initWindow(); // Initialize the viewer's window.
	virtual bool initLogging(); // Initialize log files, logging system, return false on failure.
	virtual void initConsole() {}; // Initialize OS level debugging console.
	virtual bool initHardwareTest() { return true; } // A false result indicates the app should quit.

    virtual bool initParseCommandLine(LLCommandLineParser& clp) 
        { return true; } // Allow platforms to specify the command line args.
	
	virtual std::string generateSerialNumber() = 0; // Platforms specific classes generate this.


private:

	bool initThreads(); // Initialize viewer threads, return false on failure.
	bool initConfiguration(); // Initialize settings from the command line/config file.

	bool initCache(); // Initialize local client cache.
	void purgeCache(); // Clear the local cache. 

	void cleanupSavedSettings(); // Sets some config data to current or default values during cleanup.
	void removeCacheFiles(const char *filemask); // Deletes cached files the match the given wildcard.

	void writeSystemInfo(); // Write system info to "debug_info.log"

	bool anotherInstanceRunning(); 
	void initMarkerFile(); 
    
    void idle(); 
    void idleShutdown();
    void idleNetwork();

    void sendLogoutRequest();
    void disconnectViewer();

	// *FIX: the app viewer class should be some sort of singleton, no?
	// Perhaps its child class is the singleton and this should be an abstract base.
	static LLAppViewer* sInstance; 

    bool mSecondInstance; // Is this a second instance of the app?

	LLString mMarkerFileName;
	apr_file_t* mMarkerFile; // A file created to indicate the app is running.

	LLOSInfo mSysOSInfo; 
	S32 mCrashBehavior;
	bool mReportedCrash;

	// Thread objects.
	static LLTextureCache* sTextureCache; 
	static LLWorkerThread* sImageDecodeThread; 
	static LLTextureFetch* sTextureFetch;

	S32 mNumSessions;

	std::string mSerialNumber;
	bool mPurgeCache;
    bool mPurgeOnExit;

	bool mSavedFinalSnapshot;

    bool mQuitRequested;				// User wants to quit, may have modified documents open.
    bool mLogoutRequestSent;			// Disconnect message sent to simulator, no longer safe to send messages to the sim.
    S32 mYieldTime;
	LLSD mSettingsFileList;
};

// consts from viewer.h
const S32 AGENT_UPDATES_PER_SECOND  = 10;

// Globals with external linkage. From viewer.h
// *NOTE:Mani - These will be removed as the Viewer App Cleanup project continues.
//
// "// llstartup" indicates that llstartup is the only client for this global.

extern BOOL gHandleKeysAsync; // gSavedSettings used by llviewerdisplay.cpp & llviewermenu.cpp
extern LLString gDisabledMessage; // llstartup
extern BOOL gHideLinks; // used by llpanellogin, lllfloaterbuycurrency, llstartup
extern LLSD gDebugInfo;

extern BOOL	gAllowIdleAFK;
extern BOOL	gShowObjectUpdates;

extern const char* DEFAULT_SETTINGS_FILE; // llstartup

extern BOOL gAcceptTOS;
extern BOOL gAcceptCriticalMessage;

extern LLUUID	gViewerDigest;  // MD5 digest of the viewer's executable file.

typedef enum 
{
	LAST_EXEC_NORMAL = 0,
	LAST_EXEC_FROZE,
	LAST_EXEC_LLERROR_CRASH,
	LAST_EXEC_OTHER_CRASH
} eLastExecEvent;

extern eLastExecEvent gLastExecEvent; // llstartup

extern U32 gFrameCount;
extern U32 gForegroundFrameCount;

extern LLPumpIO* gServicePump;

// Is the Pacific time zone (aka server time zone)
// currently in daylight savings time?
extern BOOL gPacificDaylightTime;

extern U64      gFrameTime;					// The timestamp of the most-recently-processed frame
extern F32		gFrameTimeSeconds;			// Loses msec precision after ~4.5 hours...
extern F32		gFrameIntervalSeconds;		// Elapsed time between current and previous gFrameTimeSeconds
extern F32		gFPSClamped;						// Frames per second, smoothed, weighted toward last frame
extern F32		gFrameDTClamped;
extern U64		gStartTime;

extern LLTimer gRenderStartTime;
extern LLFrameTimer gForegroundTime;

extern F32 gLogoutMaxTime;
extern LLTimer gLogoutTimer;

extern F32 gSimLastTime; 
extern F32 gSimFrames;

extern LLUUID gInventoryLibraryOwner;
extern LLUUID gInventoryLibraryRoot;

extern BOOL		gDisconnected;

// Map scale in pixels per region
extern F32 gMapScale;
extern F32 gMiniMapScale;

extern LLFrameTimer	gRestoreGLTimer;
extern BOOL			gRestoreGL;
extern BOOL		gUseWireframe;

// VFS globals - gVFS is for general use
// gStaticVFS is read-only and is shipped w/ the viewer
// it has pre-cache data like the UI .TGAs
extern LLVFS	*gStaticVFS;

extern LLMemoryInfo gSysMemory;

extern LLString gLastVersionChannel;

extern LLVector3 gWindVec;
extern LLVector3 gRelativeWindVec;
extern U32	gPacketsIn;
extern BOOL gPrintMessagesThisFrame;

extern LLUUID gSunTextureID;
extern LLUUID gMoonTextureID;

extern BOOL gRandomizeFramerate;
extern BOOL gPeriodicSlowFrame;

#endif // LL_LLAPPVIEWER_H
