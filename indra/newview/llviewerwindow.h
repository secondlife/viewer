/** 
 * @file llviewerwindow.h
 * @brief Description of the LLViewerWindow class.
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
#include "llwindowcallbacks.h"
#include "lltimer.h"
#include "llstat.h"
#include "llmousehandler.h"
#include "llcursortypes.h"
#include "llhandle.h"
#include "llimage.h"

#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include <boost/scoped_ptr.hpp>


class LLView;
class LLViewerObject;
class LLUUID;
class LLProgressView;
class LLTool;
class LLVelocityBar;
class LLPanel;
class LLImageRaw;
class LLHUDIcon;
class LLWindow;
class LLRootView;
class LLViewerWindowListener;
class LLPopupView;

#define PICK_HALF_WIDTH 5
#define PICK_DIAMETER (2 * PICK_HALF_WIDTH + 1)

class LLPickInfo
{
public:
	typedef enum
	{
		PICK_OBJECT,
		PICK_FLORA,
		PICK_LAND,
		PICK_ICON,
		PICK_PARCEL_WALL,
		PICK_INVALID
	} EPickType;

public:
	LLPickInfo();
	LLPickInfo(const LLCoordGL& mouse_pos, 
		MASK keyboard_mask, 
		BOOL pick_transparent, 
		BOOL pick_surface_info,
		void (*pick_callback)(const LLPickInfo& pick_info));

	void fetchResults();
	LLPointer<LLViewerObject> getObject() const;
	LLUUID getObjectID() const { return mObjectID; }
	bool isValid() const { return mPickType != PICK_INVALID; }

	static bool isFlora(LLViewerObject* object);

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
	void		    getSurfaceInfo();

private:
	void			updateXYCoords();

	BOOL			mWantSurfaceInfo;   // do we populate mUVCoord, mNormal, mBinormal?

};

static const U32 MAX_SNAPSHOT_IMAGE_SIZE = 6 * 1024; // max snapshot image size 6144 * 6144

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

	BOOL handleAnyMouseClick(LLWindow *window,  LLCoordGL pos, MASK mask, LLMouseHandler::EClickType clicktype, BOOL down);

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
	/*virtual*/ LLWindowCallbacks::DragNDropResult handleDragNDrop(LLWindow *window, LLCoordGL pos, MASK mask, LLWindowCallbacks::DragNDropAction action, std::string data);
				void handleMouseMove(LLWindow *window,  LLCoordGL pos, MASK mask);
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

	/*virtual*/ void handlePingWatchdog(LLWindow *window, const char * msg);
	/*virtual*/ void handlePauseWatchdog(LLWindow *window);
	/*virtual*/ void handleResumeWatchdog(LLWindow *window);
	/*virtual*/ std::string translateString(const char* tag);
	/*virtual*/ std::string translateString(const char* tag,
					const std::map<std::string, std::string>& args);
	
	// signal on bottom tray width changed
	typedef boost::function<void (void)> bottom_tray_callback_t;
	typedef boost::signals2::signal<void (void)> bottom_tray_signal_t;
	bottom_tray_signal_t mOnBottomTrayWidthChanged;
	boost::signals2::connection setOnBottomTrayWidthChanged(bottom_tray_callback_t cb) { return mOnBottomTrayWidthChanged.connect(cb); }
	// signal on update of WorldView rect
	typedef boost::function<void (LLRect old_world_rect, LLRect new_world_rect)> world_rect_callback_t;
	typedef boost::signals2::signal<void (LLRect old_world_rect, LLRect new_world_rect)> world_rect_signal_t;
	world_rect_signal_t mOnWorldViewRectUpdated;
	boost::signals2::connection setOnWorldViewRectUpdated(world_rect_callback_t cb) { return mOnWorldViewRectUpdated.connect(cb); }

	//
	// ACCESSORS
	//
	LLRootView*			getRootView()		const;

	// 3D world area in scaled pixels (via UI scale), use for most UI computations
	LLRect			getWorldViewRectScaled() const;
	S32				getWorldViewHeightScaled() const;
	S32				getWorldViewWidthScaled() const;

	// 3D world area, in raw unscaled pixels
	LLRect			getWorldViewRectRaw() const		{ return mWorldViewRectRaw; }
	S32 			getWorldViewHeightRaw() const;
	S32 			getWorldViewWidthRaw() const;

	// Window in scaled pixels (via UI scale), use for most UI computations
	LLRect			getWindowRectScaled() const		{ return mWindowRectScaled; }
	S32				getWindowHeightScaled() const;
	S32				getWindowWidthScaled() const;

	// Window in raw pixels as seen on screen.
	LLRect			getWindowRectRaw() const		{ return mWindowRectRaw; }
	S32				getWindowHeightRaw() const;
	S32				getWindowWidthRaw() const;

	LLWindow*		getWindow()			const	{ return mWindow; }
	void*			getPlatformWindow() const;
	void*			getMediaWindow() 	const;
	void			focusClient()		const;

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
	BOOL			getMiddleMouseDown()	const	{ return mMiddleMouseDown; }
	BOOL			getRightMouseDown()	const	{ return mRightMouseDown; }

	const LLPickInfo&	getLastPick() const { return mLastPick; }

	void			setup2DViewport(S32 x_offset = 0, S32 y_offset = 0);
	void			setup3DViewport(S32 x_offset = 0, S32 y_offset = 0);
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
	BOOL            getCursorHidden() { return mCursorHidden; }
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

	void			updateUI();		// Once per frame, update UI based on mouse position, calls following update* functions
	void				updateLayout();						
	void				updateMouseDelta();		
	void				updateKeyboardFocus();		

	void			updateWorldViewRect(bool use_full_window=false);
	LLView*			getNonSideTrayView() { return mNonSideTrayView.get(); }
	LLView*			getFloaterViewHolder() { return mFloaterViewHolder.get(); }
	BOOL			handleKey(KEY key, MASK mask);
	void			handleScrollWheel	(S32 clicks);

	// add and remove views from "popup" layer
	void			addPopup(LLView* popup);
	void			removePopup(LLView* popup);
	void			clearPopups();

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
	typedef enum
	{
		SNAPSHOT_TYPE_COLOR,
		SNAPSHOT_TYPE_DEPTH,
		SNAPSHOT_TYPE_OBJECT_ID
	} ESnapshotType;
	BOOL			saveSnapshot(const std::string&  filename, S32 image_width, S32 image_height, BOOL show_ui = TRUE, BOOL do_rebuild = FALSE, ESnapshotType type = SNAPSHOT_TYPE_COLOR);
	BOOL			rawSnapshot(LLImageRaw *raw, S32 image_width, S32 image_height, BOOL keep_window_aspect = TRUE, BOOL is_texture = FALSE,
								BOOL show_ui = TRUE, BOOL do_rebuild = FALSE, ESnapshotType type = SNAPSHOT_TYPE_COLOR, S32 max_size = MAX_SNAPSHOT_IMAGE_SIZE );
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

	void			pickAsync(S32 x, S32 y_from_bot, MASK mask, void (*callback)(const LLPickInfo& pick_info), BOOL pick_transparent = FALSE);
	LLPickInfo		pickImmediate(S32 x, S32 y, BOOL pick_transparent);
	LLHUDIcon* cursorIntersectIcon(S32 mouse_x, S32 mouse_y, F32 depth,
										   LLVector3* intersection);

	LLViewerObject* cursorIntersect(S32 mouse_x = -1, S32 mouse_y = -1, F32 depth = 512.f,
									LLViewerObject *this_object = NULL,
									S32 this_face = -1,
									BOOL pick_transparent = FALSE,
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
	void			requestResolutionUpdate(bool fullscreen_checked);
	void			requestResolutionUpdate(); // doesn't affect fullscreen
	BOOL			checkSettings();
	void			restartDisplay(BOOL show_progress_bar);
	BOOL			changeDisplaySettings(BOOL fullscreen, LLCoordScreen size, BOOL disable_vsync, BOOL show_progress_bar);
	BOOL			getIgnoreDestroyWindow() { return mIgnoreActivate; }
	F32				getDisplayAspectRatio() const;
	F32				getWorldViewAspectRatio() const;
	const LLVector2& getDisplayScale() const { return mDisplayScale; }
	void			calcDisplayScale();
	static LLRect 	calcScaledRect(const LLRect & rect, const LLVector2& display_scale);

private:
	bool                    shouldShowToolTipFor(LLMouseHandler *mh);
	static bool onAlert(const LLSD& notify);

	void			switchToolByMask(MASK mask);
	void			destroyWindow();
	void			drawMouselookInstructions();
	void			stopGL(BOOL save_state = TRUE);
	void			restoreGL(const std::string& progress_message = LLStringUtil::null);
	void			initFonts(F32 zoom_factor = 1.f);
	void			schedulePick(LLPickInfo& pick_info);
	S32				getChatConsoleBottomPad(); // Vertical padding for child console rect, varied by bottom clutter
	LLRect			getChatConsoleRect(); // Get optimal cosole rect.

public:
	LLWindow*		mWindow;						// graphical window object

protected:
	BOOL			mActive;
	BOOL			mWantFullscreen;
	BOOL			mShowFullscreenProgress;

	LLRect			mWindowRectRaw;				// whole window, including UI
	LLRect			mWindowRectScaled;			// whole window, scaled by UI size
	LLRect			mWorldViewRectRaw;			// area of screen for 3D world
	LLRect			mWorldViewRectScaled;		// area of screen for 3D world scaled by UI size
	LLRootView*		mRootView;					// a view of size mWindowRectRaw, containing all child views
	LLVector2		mDisplayScale;

	LLCoordGL		mCurrentMousePoint;			// last mouse position in GL coords
	LLCoordGL		mLastMousePoint;		// Mouse point at last frame.
	LLCoordGL		mCurrentMouseDelta;		//amount mouse moved this frame
	LLStat			mMouseVelocityStat;
	BOOL			mLeftMouseDown;
	BOOL			mMiddleMouseDown;
	BOOL			mRightMouseDown;

	LLProgressView	*mProgressView;

	LLFrameTimer	mToolTipFadeTimer;
	LLPanel*		mToolTip;
	std::string		mLastToolTipMessage;
	LLRect			mToolTipStickyRect;			// Once a tool tip is shown, it will stay visible until the mouse leaves this rect.

	BOOL			mMouseInWindow;				// True if the mouse is over our window or if we have captured the mouse.
	BOOL			mFocusCycleMode;
	typedef std::set<LLHandle<LLView> > view_handle_set_t;
	view_handle_set_t mMouseHoverViews;

	// Variables used for tool override switching based on modifier keys.  JC
	MASK			mLastMask;			// used to detect changes in modifier mask
	LLTool*			mToolStored;		// the tool we're overriding
	BOOL			mHideCursorPermanent;	// true during drags, mouselook
	BOOL            mCursorHidden;
	LLPickInfo		mLastPick;
	std::vector<LLPickInfo> mPicks;
	LLRect			mPickScreenRegion; // area of frame buffer for rendering pick frames (generally follows mouse to avoid going offscreen)
	LLTimer         mPickTimer;        // timer for scheduling n picks per second

	std::string		mOverlayTitle;		// Used for special titles such as "Second Life - Special E3 2003 Beta"

	BOOL			mIgnoreActivate;

	std::string		mInitAlert;			// Window / GL initialization requires an alert

	LLHandle<LLView> mWorldViewPlaceholder;	// widget that spans the portion of screen dedicated to rendering the 3d world
	LLHandle<LLView> mNonSideTrayView;		// parent of world view + bottom bar, etc...everything but the side tray
	LLHandle<LLView> mFloaterViewHolder;	// container for floater_view
	LLPopupView*	mPopupView;			// container for transient popups
	
	class LLDebugText* mDebugText; // Internal class for debug text
	
	bool			mResDirty;
	bool			mStatesDirty;
	bool			mIsFullscreenChecked; // Did the user check the fullscreen checkbox in the display settings
	U32			mCurrResolutionIndex;

    boost::scoped_ptr<LLViewerWindowListener> mViewerWindowListener;

protected:
	static std::string sSnapshotBaseName;
	static std::string sSnapshotDir;

	static std::string sMovieBaseName;
	
private:
	// Object temporarily hovered over while dragging
	LLPointer<LLViewerObject>	mDragHoveredObject;
};

void toggle_flying(void*);
void toggle_first_person();
void toggle_build(void*);
void reset_viewer_state_on_sim(void);
void update_saved_window_size(const std::string& control,S32 delta_width, S32 delta_height);

//
// Globals
//

extern LLViewerWindow*	gViewerWindow;

extern LLFrameTimer		gAwayTimer;				// tracks time before setting the avatar away state to true
extern LLFrameTimer		gAwayTriggerTimer;		// how long the avatar has been away

extern LLViewerObject*  gDebugRaycastObject;
extern LLVector3        gDebugRaycastIntersection;
extern LLVector2        gDebugRaycastTexCoord;
extern LLVector3        gDebugRaycastNormal;
extern LLVector3        gDebugRaycastBinormal;
extern S32				gDebugRaycastFaceHit;

extern S32 CHAT_BAR_HEIGHT; 

extern BOOL			gDisplayCameraPos;
extern BOOL			gDisplayWindInfo;
extern BOOL			gDisplayFOV;
extern BOOL			gDisplayBadge;

#endif
