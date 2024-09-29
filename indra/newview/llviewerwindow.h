/**
 * @file llviewerwindow.h
 * @brief Description of the LLViewerWindow class.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "llcursortypes.h"
#include "llwindowcallbacks.h"
#include "lltimer.h"
#include "llmousehandler.h"
#include "llnotifications.h"
#include "llhandle.h"
#include "llinitparam.h"
#include "lltrace.h"
#include "llsnapshotmodel.h"

#include <boost/function.hpp>
#include <boost/signals2.hpp>

class LLView;
class LLViewerObject;
class LLUUID;
class LLProgressView;
class LLTool;
class LLVelocityBar;
class LLPanel;
class LLImageRaw;
class LLImageFormatted;
class LLHUDIcon;
class LLWindow;
class LLRootView;
class LLWindowListener;
class LLViewerWindowListener;
class LLVOPartGroup;
class LLPopupView;
class LLCubeMap;
class LLCubeMapArray;

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
        bool pick_transparent,
        bool pick_rigged,
        bool pick_particle,
        bool pick_reflection_probe,
        bool pick_surface_info,
        bool pick_unselectable,
        void (*pick_callback)(const LLPickInfo& pick_info));

    void fetchResults();
    LLPointer<LLViewerObject> getObject() const;
    LLUUID getObjectID() const { return mObjectID; }
    bool isValid() const { return mPickType != PICK_INVALID; }

    static bool isFlora(LLViewerObject* object);

public:
    LLCoordGL       mMousePt;
    MASK            mKeyMask;
    void            (*mPickCallback)(const LLPickInfo& pick_info);

    EPickType       mPickType;
    LLCoordGL       mPickPt;
    LLVector3d      mPosGlobal;
    LLVector3       mObjectOffset;
    LLUUID          mObjectID;
    LLUUID          mParticleOwnerID;
    LLUUID          mParticleSourceID;
    S32             mObjectFace;
    S32             mGLTFNodeIndex = -1;
    S32             mGLTFPrimitiveIndex = -1;
    LLHUDIcon*      mHUDIcon;
    LLVector3       mIntersection;
    LLVector2       mUVCoords;
    LLVector2       mSTCoords;
    LLCoordScreen   mXYCoords;
    LLVector3       mNormal;
    LLVector4       mTangent;
    LLVector3       mBinormal;
    bool            mPickTransparent;
    bool            mPickRigged;
    bool            mPickParticle;
    bool            mPickUnselectable;
    bool            mPickReflectionProbe = false;
    void            getSurfaceInfo();

private:
    void            updateXYCoords();

    bool            mWantSurfaceInfo;   // do we populate mUVCoord, mNormal, mBinormal?

};

static const U32 MAX_SNAPSHOT_IMAGE_SIZE = 7680; // max snapshot image size 7680 * 7680 UHDTV2

class LLViewerWindow : public LLWindowCallbacks
{
public:
    //
    // CREATORS
    //
    struct Params : public LLInitParam::Block<Params>
    {
        Mandatory<std::string>      title,
                                    name;
        Mandatory<S32>              x,
                                    y,
                                    width,
                                    height,
                                    min_width,
                                    min_height;
        Optional<bool>              fullscreen,
                                    ignore_pixel_depth,
                                    first_run;

        Params();
    };

    LLViewerWindow(const Params& p);
    virtual ~LLViewerWindow();

    void            shutdownViews();
    void            shutdownGL();

    void            initGLDefaults();
    void            initBase();
    void            adjustRectanglesForFirstUse(const LLRect& window);
    void            adjustControlRectanglesForFirstUse(const LLRect& window);
    void            initWorldUI();
    void            setUIVisibility(bool);
    bool            getUIVisibility();
    void            handlePieMenu(S32 x, S32 y, MASK mask);

    void            reshapeStatusBarContainer();
    void            resetStatusBarContainer(); // undo changes done by resetStatusBarContainer on initWorldUI()

    bool handleAnyMouseClick(LLWindow *window, LLCoordGL pos, MASK mask, EMouseClickType clicktype, bool down, bool &is_toolmgr_action);

    //
    // LLWindowCallback interface implementation
    //
    /*virtual*/ bool handleTranslatedKeyDown(KEY key,  MASK mask, bool repeated);
    /*virtual*/ bool handleTranslatedKeyUp(KEY key,  MASK mask);
    /*virtual*/ void handleScanKey(KEY key, bool key_down, bool key_up, bool key_level);
    /*virtual*/ bool handleUnicodeChar(llwchar uni_char, MASK mask);    // NOT going to handle extended
    /*virtual*/ bool handleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
    /*virtual*/ bool handleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
    /*virtual*/ bool handleCloseRequest(LLWindow *window);
    /*virtual*/ void handleQuit(LLWindow *window);
    /*virtual*/ bool handleRightMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
    /*virtual*/ bool handleRightMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
    /*virtual*/ bool handleMiddleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
    /*virtual*/ bool handleMiddleMouseUp(LLWindow *window, LLCoordGL pos, MASK mask);
    /*virtual*/ bool handleOtherMouseDown(LLWindow *window, LLCoordGL pos, MASK mask, S32 button);
    /*virtual*/ bool handleOtherMouseUp(LLWindow *window, LLCoordGL pos, MASK mask, S32 button);
    bool handleOtherMouse(LLWindow *window, LLCoordGL pos, MASK mask, S32 button, bool down);
    /*virtual*/ LLWindowCallbacks::DragNDropResult handleDragNDrop(LLWindow *window, LLCoordGL pos, MASK mask, LLWindowCallbacks::DragNDropAction action, std::string data);
                void handleMouseMove(LLWindow *window,  LLCoordGL pos, MASK mask);
                void handleMouseDragged(LLWindow *window,  LLCoordGL pos, MASK mask);
    /*virtual*/ void handleMouseLeave(LLWindow *window);
    /*virtual*/ void handleResize(LLWindow *window,  S32 x,  S32 y);
    /*virtual*/ void handleFocus(LLWindow *window);
    /*virtual*/ void handleFocusLost(LLWindow *window);
    /*virtual*/ bool handleActivate(LLWindow *window, bool activated);
    /*virtual*/ bool handleActivateApp(LLWindow *window, bool activating);
    /*virtual*/ void handleMenuSelect(LLWindow *window,  S32 menu_item);
    /*virtual*/ bool handlePaint(LLWindow *window,  S32 x,  S32 y,  S32 width,  S32 height);
    /*virtual*/ void handleScrollWheel(LLWindow *window,  S32 clicks);
    /*virtual*/ void handleScrollHWheel(LLWindow *window,  S32 clicks);
    /*virtual*/ bool handleDoubleClick(LLWindow *window,  LLCoordGL pos, MASK mask);
    /*virtual*/ void handleWindowBlock(LLWindow *window);
    /*virtual*/ void handleWindowUnblock(LLWindow *window);
    /*virtual*/ void handleDataCopy(LLWindow *window, S32 data_type, void *data);
    /*virtual*/ bool handleTimerEvent(LLWindow *window);
    /*virtual*/ bool handleDeviceChange(LLWindow *window);
    /*virtual*/ bool handleDPIChanged(LLWindow *window, F32 ui_scale_factor, S32 window_width, S32 window_height);
    /*virtual*/ bool handleWindowDidChangeScreen(LLWindow *window);

    /*virtual*/ void handlePingWatchdog(LLWindow *window, const char * msg);
    /*virtual*/ void handlePauseWatchdog(LLWindow *window);
    /*virtual*/ void handleResumeWatchdog(LLWindow *window);
    /*virtual*/ std::string translateString(const char* tag);
    /*virtual*/ std::string translateString(const char* tag,
                    const std::map<std::string, std::string>& args);

    // signal on update of WorldView rect
    typedef boost::function<void (LLRect old_world_rect, LLRect new_world_rect)> world_rect_callback_t;
    typedef boost::signals2::signal<void (LLRect old_world_rect, LLRect new_world_rect)> world_rect_signal_t;
    world_rect_signal_t mOnWorldViewRectUpdated;
    boost::signals2::connection setOnWorldViewRectUpdated(world_rect_callback_t cb) { return mOnWorldViewRectUpdated.connect(cb); }

    //
    // ACCESSORS
    //
    LLRootView*         getRootView()       const;

    // 3D world area in scaled pixels (via UI scale), use for most UI computations
    LLRect          getWorldViewRectScaled() const;
    S32             getWorldViewHeightScaled() const;
    S32             getWorldViewWidthScaled() const;

    // 3D world area, in raw unscaled pixels
    LLRect          getWorldViewRectRaw() const     { return mWorldViewRectRaw; }
    S32             getWorldViewHeightRaw() const;
    S32             getWorldViewWidthRaw() const;

    // Window in scaled pixels (via UI scale), use for most UI computations
    LLRect          getWindowRectScaled() const     { return mWindowRectScaled; }
    S32             getWindowHeightScaled() const;
    S32             getWindowWidthScaled() const;

    // Window in raw pixels as seen on screen.
    LLRect          getWindowRectRaw() const        { return mWindowRectRaw; }
    S32             getWindowHeightRaw() const;
    S32             getWindowWidthRaw() const;

    LLWindow*       getWindow()         const   { return mWindow; }
    void*           getPlatformWindow() const;
    void*           getMediaWindow()    const;
    void            focusClient()       const;

    LLCoordGL       getLastMouse()      const   { return mLastMousePoint; }
    S32             getLastMouseX()     const   { return mLastMousePoint.mX; }
    S32             getLastMouseY()     const   { return mLastMousePoint.mY; }
    LLCoordGL       getCurrentMouse()       const   { return mCurrentMousePoint; }
    S32             getCurrentMouseX()      const   { return mCurrentMousePoint.mX; }
    S32             getCurrentMouseY()      const   { return mCurrentMousePoint.mY; }
    S32             getCurrentMouseDX()     const   { return mCurrentMouseDelta.mX; }
    S32             getCurrentMouseDY()     const   { return mCurrentMouseDelta.mY; }
    LLCoordGL       getCurrentMouseDelta()  const   { return mCurrentMouseDelta; }
    static LLTrace::SampleStatHandle<>* getMouseVelocityStat()      { return &sMouseVelocityStat; }
    bool            getLeftMouseDown()  const   { return mLeftMouseDown; }
    bool            getMiddleMouseDown()    const   { return mMiddleMouseDown; }
    bool            getRightMouseDown() const   { return mRightMouseDown; }

    const LLPickInfo&   getLastPick() const { return mLastPick; }

    void            setup2DViewport(S32 x_offset = 0, S32 y_offset = 0);
    void            setup3DViewport(S32 x_offset = 0, S32 y_offset = 0);
    void            setup3DRender();
    void            setup2DRender();

    LLVector3       mouseDirectionGlobal(const S32 x, const S32 y) const;
    LLVector3       mouseDirectionCamera(const S32 x, const S32 y) const;
    LLVector3       mousePointHUD(const S32 x, const S32 y) const;


    // Is window of our application frontmost?
    bool            getActive() const           { return mActive; }

    const std::string&  getInitAlert() { return mInitAlert; }

    //
    // MANIPULATORS
    //
    void            saveLastMouse(const LLCoordGL &point);

    void            setCursor( ECursorType c );
    void            showCursor();
    void            hideCursor();
    bool            getCursorHidden() { return mCursorHidden; }
    void            moveCursorToCenter();                               // move to center of window

    void            initTextures(S32 location_id);
    void            setShowProgress(const bool show);
    bool            getShowProgress() const;
    void            setProgressString(const std::string& string);
    void            setProgressPercent(const F32 percent);
    void            setProgressMessage(const std::string& msg);
    void            setProgressCancelButtonVisible( bool b, const std::string& label = LLStringUtil::null );
    LLProgressView *getProgressView() const;
    void            revealIntroPanel();
    void            setStartupComplete();

    void            updateObjectUnderCursor();

    void            updateUI();     // Once per frame, update UI based on mouse position, calls following update* functions
    void                updateLayout();
    void                updateMouseDelta();
    void                updateKeyboardFocus();

    void            updateWorldViewRect(bool use_full_window=false);
    LLView*         getToolBarHolder() { return mToolBarHolder.get(); }
    LLView*         getHintHolder() { return mHintHolder.get(); }
    LLView*         getLoginPanelHolder() { return mLoginPanelHolder.get(); }
    bool            handleKey(KEY key, MASK mask);
    bool            handleKeyUp(KEY key, MASK mask);
    void            handleScrollWheel   (S32 clicks);
    void            handleScrollHWheel  (S32 clicks);

    // add and remove views from "popup" layer
    void            addPopup(LLView* popup);
    void            removePopup(LLView* popup);
    void            clearPopups();

    // Hide normal UI when a logon fails, re-show everything when logon is attempted again
    void            setNormalControlsVisible( bool visible );
    void            setMenuBackgroundColor(bool god_mode = false, bool dev_grid = false);

    void            reshape(S32 width, S32 height);
    void            sendShapeToSim();

    void            draw();
    void            updateDebugText();
    void            drawDebugText();

    static void     loadUserImage(void **cb_data, const LLUUID &uuid);

    static void     movieSize(S32 new_width, S32 new_height);

    // snapshot functionality.
    // perhaps some of this should move to llfloatershapshot?  -MG

    bool            saveSnapshot(const std::string&  filename, S32 image_width, S32 image_height, bool show_ui = true, bool show_hud = true, bool do_rebuild = false, LLSnapshotModel::ESnapshotLayerType type = LLSnapshotModel::SNAPSHOT_TYPE_COLOR, LLSnapshotModel::ESnapshotFormat format = LLSnapshotModel::SNAPSHOT_FORMAT_BMP);
    bool            rawSnapshot(LLImageRaw *raw, S32 image_width, S32 image_height, bool keep_window_aspect = true, bool is_texture = false,
        bool show_ui = true, bool show_hud = true, bool do_rebuild = false, bool no_post = false, LLSnapshotModel::ESnapshotLayerType type = LLSnapshotModel::SNAPSHOT_TYPE_COLOR, S32 max_size = MAX_SNAPSHOT_IMAGE_SIZE);

    bool            simpleSnapshot(LLImageRaw *raw, S32 image_width, S32 image_height, const int num_render_passes);



    // take a cubemap snapshot
    // origin - vantage point to take the snapshot from
    // cubearray - cubemap array for storing the results
    // index - cube index in the array to use (cube index, not face-layer)
    // face - which cube face to update
    // near_clip - near clip setting to use
    bool cubeSnapshot(const LLVector3 &origin, LLCubeMapArray *cubearray, S32 index, S32 face, F32 near_clip, bool render_avatars,
                      bool customCullingPlane = false, LLPlane cullingPlane = LLPlane(LLVector3(0, 0, 0), LLVector3(0, 0, 1)));


    // special implementation of simpleSnapshot for reflection maps
    bool            reflectionSnapshot(LLImageRaw* raw, S32 image_width, S32 image_height, const int num_render_passes);

    bool            thumbnailSnapshot(LLImageRaw *raw, S32 preview_width, S32 preview_height, bool show_ui, bool show_hud, bool do_rebuild, bool no_post, LLSnapshotModel::ESnapshotLayerType type);
    bool            isSnapshotLocSet() const;
    void            resetSnapshotLoc() const;

    typedef boost::signals2::signal<void(void)> snapshot_saved_signal_t;

    void            saveImageNumbered(LLImageFormatted *image, bool force_picker, const snapshot_saved_signal_t::slot_type& success_cb, const snapshot_saved_signal_t::slot_type& failure_cb);
    void            onDirectorySelected(const std::vector<std::string>& filenames, LLImageFormatted *image, const snapshot_saved_signal_t::slot_type& success_cb, const snapshot_saved_signal_t::slot_type& failure_cb);
    void            saveImageLocal(LLImageFormatted *image, const snapshot_saved_signal_t::slot_type& success_cb, const snapshot_saved_signal_t::slot_type& failure_cb);
    void            onSelectionFailure(const snapshot_saved_signal_t::slot_type& failure_cb);

    // Reset the directory where snapshots are saved.
    // Client will open directory picker on next snapshot save.
    void resetSnapshotLoc();

    void            playSnapshotAnimAndSound();

    // draws selection boxes around selected objects, must call displayObjects first
    void            renderSelections( bool for_gl_pick, bool pick_parcel_walls, bool for_hud );
    void            performPick();
    void            returnEmptyPicks();

    void            pickAsync(  S32 x,
                                S32 y_from_bot,
                                MASK mask,
                                void (*callback)(const LLPickInfo& pick_info),
                                bool pick_transparent = false,
                                bool pick_rigged = false,
                                bool pick_unselectable = false,
                                bool pick_reflection_probes = false);
    LLPickInfo      pickImmediate(S32 x, S32 y, bool pick_transparent, bool pick_rigged = false, bool pick_particle = false, bool pick_unselectable = true, bool pick_reflection_probe = false);
    LLHUDIcon* cursorIntersectIcon(S32 mouse_x, S32 mouse_y, F32 depth,
                                           LLVector4a* intersection);

    LLViewerObject* cursorIntersect(S32 mouse_x = -1, S32 mouse_y = -1, F32 depth = 512.f,
                                    LLViewerObject *this_object = NULL,
                                    S32 this_face = -1,
                                    bool pick_transparent = false,
                                    bool pick_rigged = false,
                                    bool pick_unselectable = true,
                                    bool pick_reflection_probe = true,
                                    S32* face_hit = NULL,
                                    S32* gltf_node_hit = nullptr,
                                    S32* gltf_primitive_hit = nullptr,
                                    LLVector4a *intersection = NULL,
                                    LLVector2 *uv = NULL,
                                    LLVector4a *normal = NULL,
                                    LLVector4a *tangent = NULL,
                                    LLVector4a* start = NULL,
                                    LLVector4a* end = NULL);


    // Returns a pointer to the last object hit
    //LLViewerObject    *getObject();
    //LLViewerObject  *lastNonFloraObjectHit();

    //const LLVector3d& getObjectOffset();
    //const LLVector3d& lastNonFloraObjectHitOffset();

    // mousePointOnLand() returns true if found point
    bool            mousePointOnLandGlobal(const S32 x, const S32 y, LLVector3d *land_pos_global, bool ignore_distance = false);
    bool            mousePointOnPlaneGlobal(LLVector3d& point, const S32 x, const S32 y, const LLVector3d &plane_point, const LLVector3 &plane_normal);
    LLVector3d      clickPointInWorldGlobal(const S32 x, const S32 y_from_bot, LLViewerObject* clicked_object) const;
    bool            clickPointOnSurfaceGlobal(const S32 x, const S32 y, LLViewerObject *objectp, LLVector3d &point_global) const;

    // Prints window implementation details
    void            dumpState();

    // handle shutting down GL and bringing it back up
    void            requestResolutionUpdate();
    void            checkSettings();

    F32             getWorldViewAspectRatio() const;
    const LLVector2& getDisplayScale() const { return mDisplayScale; }
    void            calcDisplayScale();
    static LLRect   calcScaledRect(const LLRect & rect, const LLVector2& display_scale);

    static std::string getLastSnapshotDir();

    LLView* getFloaterSnapRegion() { return mFloaterSnapRegion; }
    LLPanel* getChicletContainer() { return mChicletContainer; }

private:
    bool                    shouldShowToolTipFor(LLMouseHandler *mh);

    void            switchToolByMask(MASK mask);
    void            destroyWindow();
    void            drawMouselookInstructions();
    void            stopGL();
    void            restoreGL(const std::string& progress_message = LLStringUtil::null);
    void            initFonts(F32 zoom_factor = 1.f);
    void            schedulePick(LLPickInfo& pick_info);
    S32             getChatConsoleBottomPad(); // Vertical padding for child console rect, varied by bottom clutter
    LLRect          getChatConsoleRect(); // Get optimal cosole rect.

private:
    LLWindow*       mWindow;                        // graphical window object
    bool            mActive;
    bool            mUIVisible;

    LLNotificationChannelPtr mSystemChannel;
    LLNotificationChannelPtr mCommunicationChannel;
    LLNotificationChannelPtr mAlertsChannel;
    LLNotificationChannelPtr mModalAlertsChannel;

    LLRect          mWindowRectRaw;             // whole window, including UI
    LLRect          mWindowRectScaled;          // whole window, scaled by UI size
    LLRect          mWorldViewRectRaw;          // area of screen for 3D world
    LLRect          mWorldViewRectScaled;       // area of screen for 3D world scaled by UI size
    LLRootView*     mRootView;                  // a view of size mWindowRectRaw, containing all child views
    LLView*         mFloaterSnapRegion = nullptr;
    LLView*         mNavBarContainer = nullptr;
    LLPanel*        mStatusBarContainer = nullptr;
    LLPanel*        mChicletContainer = nullptr;
    LLPanel*        mTopInfoContainer = nullptr;
    LLVector2       mDisplayScale;

    LLCoordGL       mCurrentMousePoint;         // last mouse position in GL coords
    LLCoordGL       mLastMousePoint;        // Mouse point at last frame.
    LLCoordGL       mCurrentMouseDelta;     //amount mouse moved this frame
    bool            mLeftMouseDown;
    bool            mMiddleMouseDown;
    bool            mRightMouseDown;

    LLProgressView  *mProgressView;

    LLFrameTimer    mToolTipFadeTimer;
    LLPanel*        mToolTip;
    std::string     mLastToolTipMessage;
    LLRect          mToolTipStickyRect;         // Once a tool tip is shown, it will stay visible until the mouse leaves this rect.

    bool            mMouseInWindow;             // True if the mouse is over our window or if we have captured the mouse.
    bool            mFocusCycleMode;
    bool            mAllowMouseDragging;
    LLFrameTimer    mMouseDownTimer;
    typedef std::set<LLHandle<LLView> > view_handle_set_t;
    view_handle_set_t mMouseHoverViews;

    // Variables used for tool override switching based on modifier keys.  JC
    MASK            mLastMask;          // used to detect changes in modifier mask
    LLTool*         mToolStored;        // the tool we're overriding
    bool            mHideCursorPermanent;   // true during drags, mouselook
    bool            mCursorHidden;
    LLPickInfo      mLastPick;
    std::vector<LLPickInfo> mPicks;
    LLRect          mPickScreenRegion; // area of frame buffer for rendering pick frames (generally follows mouse to avoid going offscreen)
    LLTimer         mPickTimer;        // timer for scheduling n picks per second

    std::string     mOverlayTitle;      // Used for special titles such as "Second Life - Special E3 2003 Beta"

    std::string     mInitAlert;         // Window / GL initialization requires an alert

    LLHandle<LLView> mWorldViewPlaceholder; // widget that spans the portion of screen dedicated to rendering the 3d world
    LLHandle<LLView> mToolBarHolder;        // container for toolbars
    LLHandle<LLView> mHintHolder;           // container for hints
    LLHandle<LLView> mLoginPanelHolder;     // container for login panel
    LLPopupView*    mPopupView;         // container for transient popups

    class LLDebugText* mDebugText; // Internal class for debug text

    bool            mResDirty;
    bool            mStatesDirty;

    std::unique_ptr<LLWindowListener> mWindowListener;
    std::unique_ptr<LLViewerWindowListener> mViewerWindowListener;

    // Object temporarily hovered over while dragging
    LLPointer<LLViewerObject>   mDragHoveredObject;

    static LLTrace::SampleStatHandle<>  sMouseVelocityStat;
};

//
// Globals
//

extern LLViewerWindow*  gViewerWindow;

extern LLFrameTimer     gAwayTimer;             // tracks time before setting the avatar away state to true
extern LLFrameTimer     gAwayTriggerTimer;      // how long the avatar has been away

extern LLViewerObject*  gDebugRaycastObject;
extern LLVector4a       gDebugRaycastIntersection;
extern LLVOPartGroup*   gDebugRaycastParticle;
extern LLVector4a       gDebugRaycastParticleIntersection;
extern LLVector2        gDebugRaycastTexCoord;
extern LLVector4a       gDebugRaycastNormal;
extern LLVector4a       gDebugRaycastTangent;
extern S32              gDebugRaycastFaceHit;
extern LLVector4a       gDebugRaycastStart;
extern LLVector4a       gDebugRaycastEnd;

extern bool         gDisplayCameraPos;
extern bool         gDisplayWindInfo;
extern bool         gDisplayFOV;
extern bool         gDisplayBadge;

#endif
