/** 
 * @file llviewerwindow.h
 * @brief Description of the LLViewerWindow class.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

//
// A note about X,Y coordinates:
//
// X coordinates are in pixels, from the left edge of the window client area
// Y coordinates are in pixels, from the BOTTOM edge of the window client area
//
// The Y coordinates therefore match OpenGL window coords, not Windows(tm) window coords.
// If Y is from the top, the variable will be called "y_from_top"

#ifndef LL_LLVIEWERWINDOW_H
#define LL_LLVIEWERWINDOW_H

#include "v3dmath.h"
#include "v2math.h"
#include "llwindow.h"
#include "lltimer.h"
#include "llstat.h"
#include "llalertdialog.h"

class LLView;
class LLViewerObject;
class LLUUID;
class LLProgressView;
class LLTool;
class LLVelocityBar;
class LLTextBox;
class LLImageRaw;
class LLHUDIcon;
class LLMouseHandler;

#define PICK_HALF_WIDTH 5
#define PICK_DIAMETER (2 * PICK_HALF_WIDTH + 1)

class LLPickInfo
{
public:
	LLPickInfo();
	LLPickInfo(const LLCoordGL& mouse_pos, 
		const LLRect& screen_region,
		MASK keyboard_mask, 
		BOOL pick_transparent, 
		BOOL pick_surface_info,
		void (*pick_callback)(const LLPickInfo& pick_info));
	~LLPickInfo();

	void fetchResults();
	LLPointer<LLViewerObject> getObject() const;
	LLUUID getObjectID() const { return mObjectID; }
	void drawPickBuffer() const;

	static bool isFlora(LLViewerObject* object);

	typedef enum e_pick_type
	{
		PICK_OBJECT,
		PICK_FLORA,
		PICK_LAND,
		PICK_ICON,
		PICK_PARCEL_WALL,
		PICK_INVALID
	} EPickType;

public:
	LLCoordGL		mMousePt;
	MASK			mKeyMask;
	void			(*mPickCallback)(const LLPickInfo& pick_info);

	EPickType		mPickType;
	LLCoordGL		mPickPt;
	LLVector3d		mPosGlobal;
	LLVector3		mObjectOffset;
	LLUUID			mObjectID;
	S32				mObjectFace;
	LLHUDIcon*		mHUDIcon;
	LLVector3       mIntersection;
	LLVector2		mUVCoords;
	LLVector2       mSTCoords;
	LLCoordScreen	mXYCoords;
	LLVector3		mNormal;
	LLVector3		mBinormal;
	BOOL			mPickTransparent;
	LLRect			mScreenRegion;
	void		    getSurfaceInfo();

private:
	void			updateXYCoords();

	BOOL			mWantSurfaceInfo;   // do we populate mUVCoord, mNormal, mBinormal?
	U8				mPickBuffer[PICK_DIAMETER * PICK_DIAMETER * 4];
	F32				mPickDepthBuffer[PICK_DIAMETER * PICK_DIAMETER];
	BOOL			mPickParcelWall;

};

#define MAX_IMAGE_SIZE 6144 //6 * 1024, max snapshot image size 6144 * 6144

class LLViewerWindow : public LLWindowCallbacks
{
public:
	//
	// CREATORS
	//
	LLViewerWindow(const std::string& title, const std::string& name, S32 x, S32 y, S32 width, S32 height, BOOL fullscreen, BOOL ignore_pixel_depth);
	virtual ~LLViewerWindow();

	void			shutdownViews();
	void			shutdownGL();
	
	void			initGLDefaults();
	void			initBase();
	void			adjustRectanglesForFirstUse(const LLRect& window);
	void            adjustControlRectanglesForFirstUse(const LLRect& window);
	void			initWorldUI();

	//
	// LLWindowCallback interface implementation
	//
	/*virtual*/ BOOL handleTranslatedKeyDown(KEY key,  MASK mask, BOOL repeated);
	/*virtual*/ BOOL handleTranslatedKeyUp(KEY key,  MASK mask);
	/*virtual*/ void handleScanKey(KEY key, BOOL key_down, BOOL key_up, BOOL key_level);
	/*virtual*/ BOOL handleUnicodeChar(llwchar uni_char, MASK mask);	// NOT going to handle extended 
	/*virtual*/ BOOL handleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	/*virtual*/ BOOL handleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	/*virtual*/ BOOL handleCloseRequest(LLWindow *window);
	/*virtual*/ void handleQuit(LLWindow *window);
	/*virtual*/ BOOL handleRightMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	/*virtual*/ BOOL handleRightMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	/*virtual*/ BOOL handleMiddleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	/*virtual*/ BOOL handleMiddleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	/*virtual*/ void handleMouseMove(LLWindow *window,  LLCoordGL pos, MASK mask);
	/*virtual*/ void handleMouseLeave(LLWindow *window);
	/*virtual*/ void handleResize(LLWindow *window,  S32 x,  S32 y);
	/*virtual*/ void handleFocus(LLWindow *window);
	/*virtual*/ void handleFocusLost(LLWindow *window);
	/*virtual*/ BOOL handleActivate(LLWindow *window, BOOL activated);
	/*virtual*/ BOOL handleActivateApp(LLWindow *window, BOOL activating);
	/*virtual*/ void handleMenuSelect(LLWindow *window,  S32 menu_item);
	/*virtual*/ BOOL handlePaint(LLWindow *window,  S32 x,  S32 y,  S32 width,  S32 height);
	/*virtual*/ void handleScrollWheel(LLWindow *window,  S32 clicks);
	/*virtual*/ BOOL handleDoubleClick(LLWindow *window,  LLCoordGL pos, MASK mask);
	/*virtual*/ void handleWindowBlock(LLWindow *window);
	/*virtual*/ void handleWindowUnblock(LLWindow *window);
	/*virtual*/ void handleDataCopy(LLWindow *window, S32 data_type, void *data);
	/*virtual*/ BOOL handleTimerEvent(LLWindow *window);
	/*virtual*/ BOOL handleDeviceChange(LLWindow *window);



	//
	// ACCESSORS
	//
	LLView*			getRootView()		const	{ return mRootView; }

	// Window in raw pixels as seen on screen.
	const LLRect&	getWindowRect()		const	{ return mWindowRect; };
	S32				getWindowDisplayHeight()	const;
	S32				getWindowDisplayWidth()	const;

	// Window in scaled pixels (via UI scale), use this for
	// UI elements checking size.
	const LLRect&	getVirtualWindowRect()		const	{ return mVirtualWindowRect; };
	S32				getWindowHeight()	const;
	S32				getWindowWidth()	const;

	LLWindow*		getWindow()			const	{ return mWindow; }
	void*			getPlatformWindow() const	{ return mWindow->getPlatformWindow(); }
	void*			getMediaWindow() 	const	{ return mWindow->getMediaWindow(); }
	void			focusClient()		const	{ return mWindow->focusClient(); };

	LLCoordGL		getLastMouse()		const	{ return mLastMousePoint; }
	S32				getLastMouseX()		const	{ return mLastMousePoint.mX; }
	S32				getLastMouseY()		const	{ return mLastMousePoint.mY; }
	LLCoordGL		getCurrentMouse()		const	{ return mCurrentMousePoint; }
	S32				getCurrentMouseX()		const	{ return mCurrentMousePoint.mX; }
	S32				getCurrentMouseY()		const	{ return mCurrentMousePoint.mY; }
	S32				getCurrentMouseDX()		const	{ return mCurrentMouseDelta.mX; }
	S32				getCurrentMouseDY()		const	{ return mCurrentMouseDelta.mY; }
	LLCoordGL		getCurrentMouseDelta()	const	{ return mCurrentMouseDelta; }
	LLStat *		getMouseVelocityStat()		{ return &mMouseVelocityStat; }
	BOOL			getLeftMouseDown()	const	{ return mLeftMouseDown; }
	BOOL			getRightMouseDown()	const	{ return mRightMouseDown; }

	const LLPickInfo&	getLastPick() const { return mLastPick; }
	const LLPickInfo&	getHoverPick() const { return mHoverPick; }

	void			setupViewport(S32 x_offset = 0, S32 y_offset = 0);
	void			setup3DRender();
	void			setup2DRender();

	LLVector3		mouseDirectionGlobal(const S32 x, const S32 y) const;
	LLVector3		mouseDirectionCamera(const S32 x, const S32 y) const;
	LLVector3       mousePointHUD(const S32 x, const S32 y) const;
		

	// Is window of our application frontmost?
	BOOL			getActive() const			{ return mActive; }

	void			getTargetWindow(BOOL& fullscreen, S32& width, S32& height) const;
		// The 'target' is where the user wants the window to be. It may not be
		// there yet, because we may be supressing fullscreen prior to login.

	const std::string&	getInitAlert() { return mInitAlert; }
	
	//
	// MANIPULATORS
	//
	void			saveLastMouse(const LLCoordGL &point);

	void			setCursor( ECursorType c );
	void			showCursor();
	void			hideCursor();
	void			moveCursorToCenter();								// move to center of window
													
	void			setShowProgress(const BOOL show);
	BOOL			getShowProgress() const;
	void			moveProgressViewToFront();
	void			setProgressString(const std::string& string);
	void			setProgressPercent(const F32 percent);
	void			setProgressMessage(const std::string& msg);
	void			setProgressCancelButtonVisible( BOOL b, const std::string& label = LLStringUtil::null );
	LLProgressView *getProgressView() const;

	void			updateObjectUnderCursor();

	BOOL			handlePerFrameHover();							// Once per frame, update UI based on mouse position

	BOOL			handleKey(KEY key, MASK mask);
	void			handleScrollWheel	(S32 clicks);

	// Hide normal UI when a logon fails, re-show everything when logon is attempted again
	void			setNormalControlsVisible( BOOL visible );
	void			setMenuBackgroundColor(bool god_mode = false, bool dev_grid = false);

	void			reshape(S32 width, S32 height);
	void			sendShapeToSim();

	void			draw();
	void			updateDebugText();
	void			drawDebugText();

	static void		loadUserImage(void **cb_data, const LLUUID &uuid);

	static void		movieSize(S32 new_width, S32 new_height);

	// snapshot functionality.
	// perhaps some of this should move to llfloatershapshot?  -MG
	typedef enum e_snapshot_type
	{
		SNAPSHOT_TYPE_COLOR,
		SNAPSHOT_TYPE_DEPTH,
		SNAPSHOT_TYPE_OBJECT_ID
	} ESnapshotType;
	BOOL			saveSnapshot(const std::string&  filename, S32 image_width, S32 image_height, BOOL show_ui = TRUE, BOOL do_rebuild = FALSE, ESnapshotType type = SNAPSHOT_TYPE_COLOR);
	BOOL			rawSnapshot(LLImageRaw *raw, S32 image_width, S32 image_height, BOOL keep_window_aspect = TRUE, BOOL is_texture = FALSE,
								BOOL show_ui = TRUE, BOOL do_rebuild = FALSE, ESnapshotType type = SNAPSHOT_TYPE_COLOR, S32 max_size = MAX_IMAGE_SIZE );
	BOOL			thumbnailSnapshot(LLImageRaw *raw, S32 preview_width, S32 preview_height, BOOL show_ui, BOOL do_rebuild, ESnapshotType type) ;
	BOOL			isSnapshotLocSet() const { return ! sSnapshotDir.empty(); }
	void			resetSnapshotLoc() const { sSnapshotDir.clear(); }
	BOOL		    saveImageNumbered(LLImageFormatted *image);

	// Reset the directory where snapshots are saved.
	// Client will open directory picker on next snapshot save.
	void resetSnapshotLoc();

	void			playSnapshotAnimAndSound();
	
	// draws selection boxes around selected objects, must call displayObjects first
	void			renderSelections( BOOL for_gl_pick, BOOL pick_parcel_walls, BOOL for_hud );
	void			performPick();
	void			returnEmptyPicks();


	void			pickAsync(S32 x, S32 y_from_bot, MASK mask, void (*callback)(const LLPickInfo& pick_info),
							  BOOL pick_transparent = FALSE, BOOL get_surface_info = FALSE);
	LLPickInfo		pickImmediate(S32 x, S32 y, BOOL pick_transparent);
	static void     hoverPickCallback(const LLPickInfo& pick_info);
	
	LLViewerObject* cursorIntersect(S32 mouse_x = -1, S32 mouse_y = -1, F32 depth = 512.f,
									LLViewerObject *this_object = NULL,
									S32 this_face = -1,
									S32* face_hit = NULL,
									LLVector3 *intersection = NULL,
									LLVector2 *uv = NULL,
									LLVector3 *normal = NULL,
									LLVector3 *binormal = NULL);
	
	
	// Returns a pointer to the last object hit
	//LLViewerObject	*getObject();
	//LLViewerObject  *lastNonFloraObjectHit();

	//const LLVector3d& getObjectOffset();
	//const LLVector3d& lastNonFloraObjectHitOffset();

	// mousePointOnLand() returns true if found point
	BOOL			mousePointOnLandGlobal(const S32 x, const S32 y, LLVector3d *land_pos_global);
	BOOL			mousePointOnPlaneGlobal(LLVector3d& point, const S32 x, const S32 y, const LLVector3d &plane_point, const LLVector3 &plane_normal);
	LLVector3d		clickPointInWorldGlobal(const S32 x, const S32 y_from_bot, LLViewerObject* clicked_object) const;
	BOOL			clickPointOnSurfaceGlobal(const S32 x, const S32 y, LLViewerObject *objectp, LLVector3d &point_global) const;

	// Prints window implementation details
	void			dumpState();

	// Request display setting changes	
	void			toggleFullscreen(BOOL show_progress);

	// handle shutting down GL and bringing it back up
	BOOL			checkSettings();
	void			restartDisplay(BOOL show_progress_bar);
	BOOL			changeDisplaySettings(BOOL fullscreen, LLCoordScreen size, BOOL disable_vsync, BOOL show_progress_bar);
	BOOL			getIgnoreDestroyWindow() { return mIgnoreActivate; }
	F32				getDisplayAspectRatio() const;
	const LLVector2& getDisplayScale() const { return mDisplayScale; }
	void			calcDisplayScale();

	void			drawPickBuffer() const;

	LLAlertDialog* alertXml(const std::string& xml_filename,
				  LLAlertDialog::alert_callback_t callback = NULL, void* user_data = NULL);
	LLAlertDialog* alertXml(const std::string& xml_filename, const LLStringUtil::format_map_t& args,
				  LLAlertDialog::alert_callback_t callback = NULL, void* user_data = NULL);
	LLAlertDialog* alertXmlEditText(const std::string& xml_filename, const LLStringUtil::format_map_t& args,
						  LLAlertDialog::alert_callback_t callback, void* user_data,
						  LLAlertDialog::alert_text_callback_t text_callback, void *text_data,
						  const LLStringUtil::format_map_t& edit_args = LLStringUtil::format_map_t(),
						  BOOL draw_asterixes = FALSE);

	static bool alertCallback(S32 modal);
	
private:
	bool                    shouldShowToolTipFor(LLMouseHandler *mh);
	void			switchToolByMask(MASK mask);
	void			destroyWindow();
	void			drawMouselookInstructions();
	void			stopGL(BOOL save_state = TRUE);
	void			restoreGL(const std::string& progress_message = LLStringUtil::null);
	void			initFonts(F32 zoom_factor = 1.f);
	void			schedulePick(LLPickInfo& pick_info);
	
public:
	LLWindow*		mWindow;						// graphical window object

protected:
	BOOL			mActive;
	BOOL			mWantFullscreen;
	BOOL			mShowFullscreenProgress;
	LLRect			mWindowRect;
	LLRect			mVirtualWindowRect;
	LLView*			mRootView;					// a view of size mWindowRect, containing all child views
	LLVector2		mDisplayScale;

	LLCoordGL		mCurrentMousePoint;			// last mouse position in GL coords
	LLCoordGL		mLastMousePoint;		// Mouse point at last frame.
	LLCoordGL		mCurrentMouseDelta;		//amount mouse moved this frame
	LLStat			mMouseVelocityStat;
	BOOL			mLeftMouseDown;
	BOOL			mRightMouseDown;

	LLProgressView	*mProgressView;

	LLTextBox*		mToolTip;
	BOOL			mToolTipBlocked;			// True after a key press or a mouse button event.  False once the mouse moves again.
	LLRect			mToolTipStickyRect;			// Once a tool tip is shown, it will stay visible until the mouse leaves this rect.

	BOOL			mMouseInWindow;				// True if the mouse is over our window or if we have captured the mouse.
	BOOL			mFocusCycleMode;

	// Variables used for tool override switching based on modifier keys.  JC
	MASK			mLastMask;			// used to detect changes in modifier mask
	LLTool*			mToolStored;		// the tool we're overriding
	BOOL			mSuppressToolbox;	// sometimes hide the toolbox, despite
										// having a camera tool selected
	BOOL			mHideCursorPermanent;	// true during drags, mouselook
	LLPickInfo		mLastPick;
	LLPickInfo		mHoverPick;
	std::vector<LLPickInfo> mPicks;
	LLRect			mPickScreenRegion; // area of frame buffer for rendering pick frames (generally follows mouse to avoid going offscreen)

	std::string		mOverlayTitle;		// Used for special titles such as "Second Life - Special E3 2003 Beta"

	BOOL			mIgnoreActivate;

	std::string		mInitAlert;			// Window / GL initialization requires an alert
	
	class LLDebugText* mDebugText; // Internal class for debug text

protected:
	static std::string sSnapshotBaseName;
	static std::string sSnapshotDir;

	static std::string sMovieBaseName;
};	

class LLBottomPanel : public LLPanel
{
public:
	LLBottomPanel(const LLRect& rect);
	void setFocusIndicator(LLView * indicator);
	LLView * getFocusIndicator() { return mIndicator; }
	/*virtual*/ void draw();

	static void* createHUD(void* data);
	static void* createOverlayBar(void* data);
	static void* createToolBar(void* data);

protected:
	LLView * mIndicator;
};
extern LLBottomPanel * gBottomPanel;

void toggle_flying(void*);
void toggle_first_person();
void toggle_build(void*);
void reset_viewer_state_on_sim(void);
void update_saved_window_size(const std::string& control,S32 delta_width, S32 delta_height);



//
// Globals
//

extern LLVelocityBar*	gVelocityBar;
extern LLViewerWindow*	gViewerWindow;

extern LLFrameTimer		gMouseIdleTimer;		// how long has it been since the mouse last moved?
extern LLFrameTimer		gAwayTimer;				// tracks time before setting the avatar away state to true
extern LLFrameTimer		gAwayTriggerTimer;		// how long the avatar has been away

extern BOOL				gDebugSelect;

extern BOOL				gDebugFastUIRender;
extern LLViewerObject*  gDebugRaycastObject;
extern LLVector3        gDebugRaycastIntersection;
extern LLVector2        gDebugRaycastTexCoord;
extern LLVector3        gDebugRaycastNormal;
extern LLVector3        gDebugRaycastBinormal;

extern S32 CHAT_BAR_HEIGHT; 

extern BOOL			gDisplayCameraPos;
extern BOOL			gDisplayWindInfo;
extern BOOL			gDisplayNearestWater;
extern BOOL			gDisplayFOV;

#endif
