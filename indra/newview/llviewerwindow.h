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
class LLMouseHandler;
class LLProgressView;
class LLTool;
class LLVelocityBar;
class LLViewerWindow;
class LLTextBox;
class LLImageRaw;
class LLHUDIcon;

#define MAX_IMAGE_SIZE 6144 //6 * 1024, max snapshot image size 6144 * 6144

class LLViewerWindow : public LLWindowCallbacks
{
public:
	//
	// CREATORS
	//
	LLViewerWindow(char* title, char* name, S32 x, S32 y, S32 width, S32 height, BOOL fullscreen, BOOL ignore_pixel_depth);
	virtual ~LLViewerWindow();

	void			initGLDefaults();
	void			initBase();
	void			adjustRectanglesForFirstUse(const LLRect& full_window);
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
	/*virtual*/ void handleMenuSelect(LLWindow *window,  S32 menu_item);
	/*virtual*/ BOOL handlePaint(LLWindow *window,  S32 x,  S32 y,  S32 width,  S32 height);
	/*virtual*/ void handleScrollWheel(LLWindow *window,  S32 clicks);
	/*virtual*/ BOOL handleDoubleClick(LLWindow *window,  LLCoordGL pos, MASK mask);
	/*virtual*/ void handleWindowBlock(LLWindow *window);
	/*virtual*/ void handleWindowUnblock(LLWindow *window);
	/*virtual*/ void handleDataCopy(LLWindow *window, S32 data_type, void *data);


	//
	// ACCESSORS
	//
	LLView*			getRootView()		const	{ return mRootView; }
	const LLRect&	getWindowRect()		const	{ return mWindowRect; };
	const LLRect&	getVirtualWindowRect()		const	{ return mVirtualWindowRect; };
	S32				getWindowHeight()	const;
	S32				getWindowWidth()	const;
	S32				getWindowDisplayHeight()	const;
	S32				getWindowDisplayWidth()	const;
	LLWindow*		getWindow()			const	{ return mWindow; }
	void*			getPlatformWindow() const	{ return mWindow->getPlatformWindow(); }
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

	LLUICtrl*		getTopCtrl() const;
	BOOL			hasTopCtrl(LLView* view) const;

	void			setupViewport(S32 x_offset = 0, S32 y_offset = 0);
	void			setup3DRender();
	void			setup2DRender();

	BOOL			isPickPending()				{ return mPickPending; }

	LLVector3		mouseDirectionGlobal(const S32 x, const S32 y) const;
	LLVector3		mouseDirectionCamera(const S32 x, const S32 y) const;

	// Is window of our application frontmost?
	BOOL			getActive() const			{ return mActive; }

	void			getTargetWindow(BOOL& fullscreen, S32& width, S32& height) const;
		// The 'target' is where the user wants the window to be. It may not be
		// there yet, because we may be supressing fullscreen prior to login.

	const LLString&	getInitAlert() { return mInitAlert; }
	
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
	void			setProgressString(const LLString& string);
	void			setProgressPercent(const F32 percent);
	void			setProgressMessage(const LLString& msg);
	void			setProgressCancelButtonVisible( BOOL b, const LLString&  label );
	LLProgressView *getProgressView() const;

	void			updateObjectUnderCursor();

	BOOL			handlePerFrameHover();							// Once per frame, update UI based on mouse position

	BOOL			handleKey(KEY key, MASK mask);
	void			handleScrollWheel	(S32 clicks);

	// Hide normal UI when a logon fails, re-show everything when logon is attempted again
	void			setNormalControlsVisible( BOOL visible );
    void            setMenuBackgroundColor(bool god_mode = false, bool dev_grid = false);

	// Handle the application becoming active (frontmost) or inactive
	//BOOL			handleActivate(BOOL activate);

	void			setKeyboardFocus(LLUICtrl* new_focus);		// new_focus = NULL to release the focus.
	LLUICtrl*		getKeyboardFocus();	
	BOOL			hasKeyboardFocus( const LLUICtrl* possible_focus ) const;
	BOOL			childHasKeyboardFocus( const LLView* parent ) const;
	
	void			setMouseCapture(LLMouseHandler* new_captor);	// new_captor = NULL to release the mouse.
	LLMouseHandler*	getMouseCaptor() const;

	void			setTopCtrl(LLUICtrl* new_top); // set new_top = NULL to release top_view.

	void			reshape(S32 width, S32 height);
	void			sendShapeToSim();

	void			draw();
//	void			drawSelectedObjects();
	void			updateDebugText();
	void			drawDebugText();

	static void		loadUserImage(void **cb_data, const LLUUID &uuid);

	static void		movieSize(S32 new_width, S32 new_height);

	typedef enum e_snapshot_type
	{
		SNAPSHOT_TYPE_COLOR,
		SNAPSHOT_TYPE_DEPTH,
		SNAPSHOT_TYPE_OBJECT_ID
	} ESnapshotType;

	BOOL			saveSnapshot(const LLString&  filename, S32 image_width, S32 image_height, BOOL show_ui = TRUE, BOOL do_rebuild = FALSE, ESnapshotType type = SNAPSHOT_TYPE_COLOR);
	BOOL			rawSnapshot(LLImageRaw *raw, S32 image_width, S32 image_height, BOOL keep_window_aspect = TRUE, BOOL is_texture = FALSE,
								BOOL show_ui = TRUE, BOOL do_rebuild = FALSE, ESnapshotType type = SNAPSHOT_TYPE_COLOR, S32 max_size = MAX_IMAGE_SIZE );
	BOOL		    saveImageNumbered(LLImageRaw *raw, const LLString& extension = LLString());

	void			playSnapshotAnimAndSound();
	
	// draws selection boxes around selected objects, must call displayObjects first
	void			renderSelections( BOOL for_gl_pick, BOOL pick_parcel_walls, BOOL for_hud );
	void			performPick();

	void			hitObjectOrLandGlobalAsync(S32 x, S32 y_from_bot, MASK mask, void (*callback)(S32 x, S32 y, MASK mask), BOOL pick_transparent = FALSE, BOOL pick_parcel_walls = FALSE);
	void			hitObjectOrLandGlobalImmediate(S32 x, S32 y, void (*callback)(S32 x, S32 y, MASK mask), BOOL pick_transparent);
	
	void			hitUIElementAsync(S32 x, S32 y_from_bot, MASK mask, void (*callback)(S32 x, S32 y, MASK mask));
	void			hitUIElementImmediate(S32 x, S32 y, void (*callback)(S32 x, S32 y, MASK mask));

	LLViewerObject*	getObjectUnderCursor(const F32 depth = 16.0f);
	
	// Returns a pointer to the last object hit
	LLViewerObject	*lastObjectHit();
	LLViewerObject  *lastNonFloraObjectHit();

	const LLVector3d& lastObjectHitOffset();
	const LLVector3d& lastNonFloraObjectHitOffset();

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
	LLAlertDialog* alertXml(const std::string& xml_filename, const LLString::format_map_t& args,
				  LLAlertDialog::alert_callback_t callback = NULL, void* user_data = NULL);
	LLAlertDialog* alertXmlEditText(const std::string& xml_filename, const LLString::format_map_t& args,
						  LLAlertDialog::alert_callback_t callback, void* user_data,
						  LLAlertDialog::alert_text_callback_t text_callback, void *text_data,
						  const LLString::format_map_t& edit_args = LLString::format_map_t(),
						  BOOL draw_asterixes = FALSE);

	static bool alertCallback(S32 modal);
	
#ifdef SABINRIG
	//Silly rig stuff
	void		printFeedback(); //RIG STUFF!
#endif //SABINRIG

private:
	void			switchToolByMask(MASK mask);
	void			destroyWindow();
	void			drawMouselookInstructions();
	void			stopGL(BOOL save_state = TRUE);
	void			restoreGL(const LLString& progress_message = LLString::null);
	void			initFonts(F32 zoom_factor = 1.f);
	
	void			analyzeHit(
						S32				x,				// input
						S32				y_from_bot,		// input
						LLViewerObject* objectp,		// input
						U32				te_offset,		// input
						LLUUID*			hit_object_id_p,// output
						S32*			hit_face_p,		// output
						LLVector3d*		hit_pos_p,		// output
						BOOL*			hit_land,		// output
						F32*			hit_u_coord,	// output
						F32*			hit_v_coord);	// output

	
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
	LLCoordGL		mPickPoint;
	LLCoordGL		mPickOffset;
	MASK			mPickMask;
	BOOL			mPickPending;
	void			(*mPickCallback)(S32 x, S32 y, MASK mask);

	LLString		mOverlayTitle;		// Used for special titles such as "Second Life - Special E3 2003 Beta"

	BOOL			mIgnoreActivate;
	U8*				mPickBuffer;

	LLString		mInitAlert;			// Window / GL initialization requires an alert
	
	class LLDebugText* mDebugText; // Internal class for debug text

protected:
	static char		sSnapshotBaseName[LL_MAX_PATH];		/* Flawfinder: ignore */
	static char		sSnapshotDir[LL_MAX_PATH];		/* Flawfinder: ignore */

	static char		sMovieBaseName[LL_MAX_PATH];		/* Flawfinder: ignore */
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
void update_saved_window_size(const LLString& control,S32 delta_width, S32 delta_height);
//
// Constants
//


//
// Globals
//

extern LLVelocityBar*	gVelocityBar;
extern LLViewerWindow*	gViewerWindow;

extern LLFrameTimer		gMouseIdleTimer;		// how long has it been since the mouse last moved?
extern LLFrameTimer		gAwayTimer;				// tracks time before setting the avatar away state to true
extern LLFrameTimer		gAwayTriggerTimer;		// how long the avatar has been away

extern LLVector3d		gLastHitPosGlobal;
extern LLVector3d		gLastHitObjectOffset;
extern LLUUID			gLastHitObjectID;
extern S32				gLastHitObjectFace;
extern BOOL				gLastHitLand;
extern F32				gLastHitUCoord;
extern F32				gLastHitVCoord;


extern LLVector3d		gLastHitNonFloraPosGlobal;
extern LLVector3d		gLastHitNonFloraObjectOffset;
extern LLUUID			gLastHitNonFloraObjectID;
extern S32				gLastHitNonFloraObjectFace;

extern S32				gLastHitUIElement;
extern LLHUDIcon*		gLastHitHUDIcon;
extern BOOL				gLastHitParcelWall;
extern BOOL				gDebugSelect;
extern BOOL				gPickFaces;
extern BOOL				gPickTransparent;

extern BOOL				gDebugFastUIRender;
extern S32 CHAT_BAR_HEIGHT; 

extern BOOL			gDisplayCameraPos;
extern BOOL			gDisplayWindInfo;
extern BOOL			gDisplayNearestWater;
extern BOOL			gDisplayFOV;

#endif
