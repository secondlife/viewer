/** 
 * @file llui.h
 * @brief UI implementation
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

// All immediate-mode gl drawing should happen here.

#ifndef LL_LLUI_H
#define LL_LLUI_H

#include "llrect.h"
#include "llcontrol.h"
#include "llrect.h"
#include "llcoord.h"
#include "llhtmlhelp.h"
#include "llgl.h"
#include <stack>

class LLColor4;
class LLVector3;
class LLVector2;
class LLImageGL;
class LLUUID;
class LLWindow;
class LLView;

// UI colors
extern const LLColor4 UI_VERTEX_COLOR;
void make_ui_sound(const LLString& name);

BOOL ui_point_in_rect(S32 x, S32 y, S32 left, S32 top, S32 right, S32 bottom);
void gl_state_for_2d(S32 width, S32 height);

void gl_line_2d(S32 x1, S32 y1, S32 x2, S32 y2);
void gl_line_2d(S32 x1, S32 y1, S32 x2, S32 y2, const LLColor4 &color );
void gl_triangle_2d(S32 x1, S32 y1, S32 x2, S32 y2, S32 x3, S32 y3, const LLColor4& color, BOOL filled);
void gl_rect_2d_simple( S32 width, S32 height );

void gl_draw_x(const LLRect& rect, const LLColor4& color);

void gl_rect_2d(S32 left, S32 top, S32 right, S32 bottom, BOOL filled = TRUE );
void gl_rect_2d(S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &color, BOOL filled = TRUE );
void gl_rect_2d_offset_local( S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &color, S32 pixel_offset = 0, BOOL filled = TRUE );
void gl_rect_2d_offset_local( S32 left, S32 top, S32 right, S32 bottom, S32 pixel_offset = 0, BOOL filled = TRUE );
void gl_rect_2d(const LLRect& rect, BOOL filled = TRUE );
void gl_rect_2d(const LLRect& rect, const LLColor4& color, BOOL filled = TRUE );
void gl_rect_2d_checkerboard(const LLRect& rect);

void gl_drop_shadow(S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &start_color, S32 lines);

void gl_circle_2d(F32 x, F32 y, F32 radius, S32 steps, BOOL filled);
void gl_arc_2d(F32 center_x, F32 center_y, F32 radius, S32 steps, BOOL filled, F32 start_angle, F32 end_angle);
void gl_deep_circle( F32 radius, F32 depth );
void gl_ring( F32 radius, F32 width, const LLColor4& center_color, const LLColor4& side_color, S32 steps, BOOL render_center );
void gl_corners_2d(S32 left, S32 top, S32 right, S32 bottom, S32 length, F32 max_frac);
void gl_washer_2d(F32 outer_radius, F32 inner_radius, S32 steps, const LLColor4& inner_color, const LLColor4& outer_color);
void gl_washer_segment_2d(F32 outer_radius, F32 inner_radius, F32 start_radians, F32 end_radians, S32 steps, const LLColor4& inner_color, const LLColor4& outer_color);
void gl_washer_spokes_2d(F32 outer_radius, F32 inner_radius, S32 count, const LLColor4& inner_color, const LLColor4& outer_color);

void gl_draw_image(S32 x, S32 y, LLImageGL* image, const LLColor4& color = UI_VERTEX_COLOR);
void gl_draw_scaled_image(S32 x, S32 y, S32 width, S32 height, LLImageGL* image, const LLColor4& color = UI_VERTEX_COLOR);
void gl_draw_rotated_image(S32 x, S32 y, F32 degrees, LLImageGL* image, const LLColor4& color = UI_VERTEX_COLOR);
void gl_draw_scaled_rotated_image(S32 x, S32 y, S32 width, S32 height, F32 degrees,LLImageGL* image, const LLColor4& color = UI_VERTEX_COLOR);
void gl_draw_scaled_image_with_border(S32 x, S32 y, S32 border_width, S32 border_height, S32 width, S32 height, LLImageGL* image, const LLColor4 &color, BOOL solid_color = FALSE);
// Flip vertical, used for LLFloaterHTML
void gl_draw_scaled_image_inverted(S32 x, S32 y, S32 width, S32 height, LLImageGL* image, const LLColor4& color = UI_VERTEX_COLOR);

void gl_rect_2d_xor(S32 left, S32 top, S32 right, S32 bottom);
void gl_stippled_line_3d( const LLVector3& start, const LLVector3& end, const LLColor4& color, F32 phase = 0.f ); 

void gl_rect_2d_simple_tex( S32 width, S32 height );

// segmented rectangles

/*
   TL |______TOP_________| TR 
     /|                  |\  
   _/_|__________________|_\_
   L| |    MIDDLE        | |R
   _|_|__________________|_|_
    \ |    BOTTOM        | /  
   BL\|__________________|/ BR
      |                  |    
*/

typedef enum e_rounded_edge
{
	ROUNDED_RECT_LEFT	= 0x1, 
	ROUNDED_RECT_TOP	= 0x2, 
	ROUNDED_RECT_RIGHT	= 0x4, 
	ROUNDED_RECT_BOTTOM	= 0x8,
	ROUNDED_RECT_ALL	= 0xf
}ERoundedEdge;


void gl_segmented_rect_2d_tex(const S32 left, const S32 top, const S32 right, const S32 bottom, const S32 texture_width, const S32 texture_height, const S32 border_size, const U32 edges = ROUNDED_RECT_ALL);
void gl_segmented_rect_2d_fragment_tex(const S32 left, const S32 top, const S32 right, const S32 bottom, const S32 texture_width, const S32 texture_height, const S32 border_size, const F32 start_fragment, const F32 end_fragment, const U32 edges = ROUNDED_RECT_ALL);
void gl_segmented_rect_3d_tex(const LLVector2& border_scale, const LLVector3& border_width, const LLVector3& border_height, const LLVector3& width_vec, const LLVector3& height_vec, U32 edges = ROUNDED_RECT_ALL);
void gl_segmented_rect_3d_tex_top(const LLVector2& border_scale, const LLVector3& border_width, const LLVector3& border_height, const LLVector3& width_vec, const LLVector3& height_vec);

inline void gl_rect_2d( const LLRect& rect, BOOL filled )
{
	gl_rect_2d( rect.mLeft, rect.mTop, rect.mRight, rect.mBottom, filled );
}

inline void gl_rect_2d_offset_local( const LLRect& rect, S32 pixel_offset, BOOL filled)
{
	gl_rect_2d_offset_local( rect.mLeft, rect.mTop, rect.mRight, rect.mBottom, pixel_offset, filled );
}

// No longer used
// Initializes translation table
// void init_tr();

// Returns a string from the string table in the correct language
// LLString tr(const LLString& english_chars);

// Used to hide the flashing text cursor when window doesn't have focus.
extern BOOL gShowTextEditCursor;

class LLImageProviderInterface;
typedef	void (*LLUIAudioCallback)(const LLUUID& uuid);

class LLUI
{
public:
	static void initClass(LLControlGroup* config, 
						  LLControlGroup* colors, 
						  LLControlGroup* assets, 
						  LLImageProviderInterface* image_provider,
						  LLUIAudioCallback audio_callback = NULL,
						  const LLVector2 *scale_factor = NULL,
						  const LLString& language = LLString::null);
	static void cleanupClass();

	static void pushMatrix();
	static void popMatrix();
	static void loadIdentity();
	static void translate(F32 x, F32 y, F32 z = 0.0f);

	//helper functions (should probably move free standing rendering helper functions here)
	static LLString locateSkin(const LLString& filename);
	static void pushClipRect(const LLRect& rect);
	static void popClipRect();
	static void setCursorPositionScreen(S32 x, S32 y);
	static void setCursorPositionLocal(LLView* viewp, S32 x, S32 y);
	static void setScaleFactor(const LLVector2& scale_factor);
	static void setLineWidth(F32 width);
	static LLUUID findAssetUUIDByName(const LLString&	name);
	static LLVector2 getWindowSize();
	static void screenPointToGL(S32 screen_x, S32 screen_y, S32 *gl_x, S32 *gl_y);
	static void glPointToScreen(S32 gl_x, S32 gl_y, S32 *screen_x, S32 *screen_y);
	static void screenRectToGL(const LLRect& screen, LLRect *gl);
	static void glRectToScreen(const LLRect& gl, LLRect *screen);
	static void setHtmlHelp(LLHtmlHelp* html_help);

private:
	static void setScissorRegionScreen(const LLRect& rect);
	static void setScissorRegionLocal(const LLRect& rect); // works assuming LLUI::translate has been called

public:
	static LLControlGroup* sConfigGroup;
	static LLControlGroup* sColorsGroup;
	static LLControlGroup* sAssetsGroup;
	static LLImageProviderInterface* sImageProvider;
	static LLUIAudioCallback sAudioCallback;
	static LLVector2		sGLScaleFactor;
	static LLWindow*		sWindow;
	static BOOL             sShowXUINames;
	static LLHtmlHelp*		sHtmlHelp;
	static std::stack<LLRect> sClipRectStack;

};

// UI widgets
// This MUST match UICtrlNames in lluictrlfactory.cpp
typedef enum e_widget_type
{
	WIDGET_TYPE_VIEW = 0,
	WIDGET_TYPE_ROOT_VIEW,
	WIDGET_TYPE_FLOATER_VIEW,
	WIDGET_TYPE_BUTTON,
	WIDGET_TYPE_JOYSTICK_TURN,
	WIDGET_TYPE_JOYSTICK_SLIDE,
	WIDGET_TYPE_CHECKBOX,
	WIDGET_TYPE_COLOR_SWATCH,
	WIDGET_TYPE_COMBO_BOX,
	WIDGET_TYPE_LINE_EDITOR,
	WIDGET_TYPE_SEARCH_EDITOR,
	WIDGET_TYPE_SCROLL_LIST,
	WIDGET_TYPE_NAME_LIST,
	WIDGET_TYPE_WEBBROWSER,
	WIDGET_TYPE_SLIDER,	// actually LLSliderCtrl
	WIDGET_TYPE_SLIDER_BAR, // actually LLSlider
	WIDGET_TYPE_VOLUME_SLIDER,//actually LLVolumeSliderCtrl
	WIDGET_TYPE_SPINNER,
	WIDGET_TYPE_TEXT_EDITOR,
	WIDGET_TYPE_TEXTURE_PICKER,
	WIDGET_TYPE_TEXT_BOX,
	WIDGET_TYPE_PAD,	// used in XML for positioning, not a real widget
	WIDGET_TYPE_RADIO_GROUP,
	WIDGET_TYPE_ICON,
	WIDGET_TYPE_LANDMARK_PICKER,
	WIDGET_TYPE_LOCATE,	// used in XML for positioning, not a real widget
	WIDGET_TYPE_VIEW_BORDER,	// decorative border
	WIDGET_TYPE_PANEL,
	WIDGET_TYPE_MENU,
	WIDGET_TYPE_PIE_MENU,
	WIDGET_TYPE_PIE_MENU_BRANCH,
	WIDGET_TYPE_MENU_ITEM,
	WIDGET_TYPE_MENU_ITEM_SEPARATOR,
	WIDGET_TYPE_MENU_SEPARATOR_VERTICAL,
	WIDGET_TYPE_MENU_ITEM_CALL,
	WIDGET_TYPE_MENU_ITEM_CHECK,
	WIDGET_TYPE_MENU_ITEM_BRANCH,
	WIDGET_TYPE_MENU_ITEM_BRANCH_DOWN,
	WIDGET_TYPE_MENU_ITEM_BLANK,
	WIDGET_TYPE_TEAROFF_MENU,
	WIDGET_TYPE_MENU_BAR,
	WIDGET_TYPE_TAB_CONTAINER,
	WIDGET_TYPE_SCROLL_CONTAINER, // LLScrollableContainerView
	WIDGET_TYPE_SCROLLBAR,
	WIDGET_TYPE_INVENTORY_PANEL, // LLInventoryPanel
	WIDGET_TYPE_FLOATER,
	WIDGET_TYPE_DRAG_HANDLE_TOP,
	WIDGET_TYPE_DRAG_HANDLE_LEFT,
	WIDGET_TYPE_RESIZE_HANDLE,
	WIDGET_TYPE_RESIZE_BAR,
	WIDGET_TYPE_NAME_EDITOR,
	WIDGET_TYPE_MULTI_FLOATER,
	WIDGET_TYPE_MEDIA_REMOTE,
	WIDGET_TYPE_FOLDER_VIEW,
	WIDGET_TYPE_FOLDER_ITEM,
	WIDGET_TYPE_FOLDER,
	WIDGET_TYPE_STAT_GRAPH,
	WIDGET_TYPE_STAT_VIEW,
	WIDGET_TYPE_STAT_BAR,
	WIDGET_TYPE_DROP_TARGET,
	WIDGET_TYPE_TEXTURE_BAR,
	WIDGET_TYPE_TEX_MEM_BAR,
	WIDGET_TYPE_SNAPSHOT_LIVE_PREVIEW,
	WIDGET_TYPE_STATUS_BAR,
	WIDGET_TYPE_PROGRESS_VIEW,
	WIDGET_TYPE_TALK_VIEW,
	WIDGET_TYPE_OVERLAY_BAR,
	WIDGET_TYPE_HUD_VIEW,
	WIDGET_TYPE_HOVER_VIEW,
	WIDGET_TYPE_MORPH_VIEW,
	WIDGET_TYPE_NET_MAP,
	WIDGET_TYPE_PERMISSIONS_VIEW,
	WIDGET_TYPE_MENU_HOLDER,
	WIDGET_TYPE_DEBUG_VIEW,
	WIDGET_TYPE_SCROLLING_PANEL_LIST,
	WIDGET_TYPE_AUDIO_STATUS,
	WIDGET_TYPE_CONTAINER_VIEW,
	WIDGET_TYPE_CONSOLE,
	WIDGET_TYPE_FAST_TIMER_VIEW,
	WIDGET_TYPE_VELOCITY_BAR,
	WIDGET_TYPE_TEXTURE_VIEW,
	WIDGET_TYPE_MEMORY_VIEW,
	WIDGET_TYPE_FRAME_STAT_VIEW,
	WIDGET_TYPE_LAYOUT_STACK,
	WIDGET_TYPE_DONTCARE,
	WIDGET_TYPE_COUNT
} EWidgetType;

// Manages generation of UI elements by LLSD, such that there is
// only one instance per uniquely identified LLSD parameter
// Class T is the instance type being managed, and INSTANCE_ADDAPTOR
// wraps an instance of the class with handlers for show/hide semantics, etc.
template <class T, class INSTANCE_ADAPTOR = T>
class LLUIInstanceMgr
{
public:
	LLUIInstanceMgr()
	{
	}

 	virtual ~LLUIInstanceMgr() 
	{ 
	}

	// default show and hide methods
	static T* showInstance(const LLSD& seed = LLSD()) 
	{ 
		T* instance = INSTANCE_ADAPTOR::getInstance(seed); 
		INSTANCE_ADAPTOR::show(instance);
		return instance;
	}

	static void hideInstance(const LLSD& seed = LLSD()) 
	{ 
		T* instance = INSTANCE_ADAPTOR::getInstance(seed); 
		INSTANCE_ADAPTOR::hide(instance);
	}

	static void toggleInstance(const LLSD& seed = LLSD())
	{
		if (INSTANCE_ADAPTOR::instanceVisible(seed))
		{
			INSTANCE_ADAPTOR::hideInstance(seed);
		}
		else
		{
			INSTANCE_ADAPTOR::showInstance(seed);
		}
	}

	static BOOL instanceVisible(const LLSD& seed = LLSD())
	{
		T* instance = INSTANCE_ADAPTOR::findInstance(seed);
		return instance != NULL && INSTANCE_ADAPTOR::visible(instance);
	}

	static T* getInstance(const LLSD& seed = LLSD()) 
	{
		T* instance = INSTANCE_ADAPTOR::findInstance(seed);
		if (instance == NULL)
		{
			instance = INSTANCE_ADAPTOR::createInstance(seed);
		}
		return instance;
	}

};

// Creates a UI singleton by ignoring the identifying parameter
// and always generating the same instance via the LLUIInstanceMgr interface.
// Note that since UI elements can be destroyed by their hierarchy, this singleton
// pattern uses a static pointer to an instance that will be re-created as needed.
template <class T, class INSTANCE_ADAPTOR = T>
class LLUISingleton: public LLUIInstanceMgr<T, INSTANCE_ADAPTOR>
{
public:
	// default constructor assumes T is derived from LLUISingleton (a true singleton)
	LLUISingleton() : LLUIInstanceMgr<T, INSTANCE_ADAPTOR>() { sInstance = (T*)this; }
	~LLUISingleton() { sInstance = NULL; }

	static T* findInstance(const LLSD& seed = LLSD())
	{
		return sInstance;
	}

	static T* createInstance(const LLSD& seed = LLSD())
	{
		if (sInstance == NULL)
		{
			sInstance = new T(seed);
		}
		return sInstance;
	}

protected:
	static T*	sInstance;
};

template <class T, class U> T* LLUISingleton<T,U>::sInstance = NULL;

class LLClipRect
{
public:
	LLClipRect(const LLRect& rect, BOOL enabled = TRUE);
	virtual ~LLClipRect();
protected:
	LLGLState		mScissorState;
	BOOL			mEnabled;
};

class LLLocalClipRect
{
public:
	LLLocalClipRect(const LLRect& rect, BOOL enabled = TRUE);
	virtual ~LLLocalClipRect();
protected:
	LLGLState		mScissorState;
	BOOL			mEnabled;
};

#endif
