/** 
 * @file llui.h
 * @brief GL function declarations and other general static UI services.
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

// All immediate-mode gl drawing should happen here.

#ifndef LL_LLUI_H
#define LL_LLUI_H

#include "llpointer.h"		// LLPointer<>
#include "llrect.h"
#include "llcontrol.h"
#include "llcoord.h"
#include "llglslshader.h"
#include "llinitparam.h"
#include "llregistry.h"
#include "lluicolor.h"
#include "lluicolortable.h"
#include <boost/signals2.hpp>
#include "lllazyvalue.h"
#include "llframetimer.h"
#include <limits>

// LLUIFactory
#include "llsd.h"

// for initparam specialization
#include "llfontgl.h"


class LLColor4; 
class LLVector3;
class LLVector2;
class LLUIImage;
class LLUUID;
class LLWindow;
class LLView;
class LLHelp;

// UI colors
extern const LLColor4 UI_VERTEX_COLOR;
void make_ui_sound(const char* name);

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
void gl_rect_2d_checkerboard(const LLRect& rect, GLfloat alpha = 1.0f);

void gl_drop_shadow(S32 left, S32 top, S32 right, S32 bottom, const LLColor4 &start_color, S32 lines);

void gl_circle_2d(F32 x, F32 y, F32 radius, S32 steps, BOOL filled);
void gl_arc_2d(F32 center_x, F32 center_y, F32 radius, S32 steps, BOOL filled, F32 start_angle, F32 end_angle);
void gl_deep_circle( F32 radius, F32 depth );
void gl_ring( F32 radius, F32 width, const LLColor4& center_color, const LLColor4& side_color, S32 steps, BOOL render_center );
void gl_corners_2d(S32 left, S32 top, S32 right, S32 bottom, S32 length, F32 max_frac);
void gl_washer_2d(F32 outer_radius, F32 inner_radius, S32 steps, const LLColor4& inner_color, const LLColor4& outer_color);
void gl_washer_segment_2d(F32 outer_radius, F32 inner_radius, F32 start_radians, F32 end_radians, S32 steps, const LLColor4& inner_color, const LLColor4& outer_color);

void gl_draw_image(S32 x, S32 y, LLTexture* image, const LLColor4& color = UI_VERTEX_COLOR, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f));
void gl_draw_scaled_image(S32 x, S32 y, S32 width, S32 height, LLTexture* image, const LLColor4& color = UI_VERTEX_COLOR, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f));
void gl_draw_rotated_image(S32 x, S32 y, F32 degrees, LLTexture* image, const LLColor4& color = UI_VERTEX_COLOR, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f));
void gl_draw_scaled_rotated_image(S32 x, S32 y, S32 width, S32 height, F32 degrees,LLTexture* image, const LLColor4& color = UI_VERTEX_COLOR, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f));
void gl_draw_scaled_image_with_border(S32 x, S32 y, S32 border_width, S32 border_height, S32 width, S32 height, LLTexture* image, const LLColor4 &color, BOOL solid_color = FALSE, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f));
void gl_draw_scaled_image_with_border(S32 x, S32 y, S32 width, S32 height, LLTexture* image, const LLColor4 &color, BOOL solid_color = FALSE, const LLRectf& uv_rect = LLRectf(0.f, 1.f, 1.f, 0.f), const LLRectf& scale_rect = LLRectf(0.f, 1.f, 1.f, 0.f));

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

class LLImageProviderInterface;

typedef	void (*LLUIAudioCallback)(const LLUUID& uuid);

class LLUI
{
	LOG_CLASS(LLUI);
public:
	//
	// Classes
	//

	struct RangeS32 
	{
		struct Params : public LLInitParam::Block<Params>
		{
			Optional<S32>	minimum,
							maximum;

			Params()
			:	minimum("min", 0),
				maximum("max", S32_MAX)
			{}
		};

		// correct for inverted params
		RangeS32(const Params& p = Params())
		:	mMin(p.minimum),
			mMax(p.maximum)
		{
			sanitizeRange();
		}

		RangeS32(S32 minimum, S32 maximum)
		:	mMin(minimum),
			mMax(maximum)
		{
			sanitizeRange();
		}

		S32 clamp(S32 input)
		{
			if (input < mMin) return mMin;
			if (input > mMax) return mMax;
			return input;
		}

		void setRange(S32 minimum, S32 maximum)
		{
			mMin = minimum;
			mMax = maximum;
			sanitizeRange();
		}

		S32 getMin() { return mMin; }
		S32 getMax() { return mMax; }

		bool operator==(const RangeS32& other) const
		{
			return mMin == other.mMin 
				&& mMax == other.mMax;
		}
	private:
		void sanitizeRange()
		{
			if (mMin > mMax)
			{
				llwarns << "Bad interval range (" << mMin << ", " << mMax << ")" << llendl;
				// since max is usually the most dangerous one to ignore (buffer overflow, etc), prefer it
				// in the case of a malformed range
				mMin = mMax;
			}
		}


		S32	mMin,
			mMax;
	};

	struct ClampedS32 : public RangeS32
	{
		struct Params : public LLInitParam::Block<Params, RangeS32::Params>
		{
			Mandatory<S32> value;

			Params()
			:	value("", 0)
			{
				addSynonym(value, "value");
			}
		};

		ClampedS32(const Params& p)
		:	RangeS32(p)
		{}

		ClampedS32(const RangeS32& range)
		:	RangeS32(range)
		{
			// set value here, after range has been sanitized
			mValue = clamp(0);
		}

		ClampedS32(S32 value, const RangeS32& range = RangeS32())
		:	RangeS32(range)
		{
			mValue = clamp(value);
		}

		S32 get()
		{
			return mValue;
		}

		void set(S32 value)
		{
			mValue = clamp(value);
		}


	private:
		S32 mValue;
	};

	//
	// Methods
	//
	typedef std::map<std::string, LLControlGroup*> settings_map_t;
	typedef boost::function<void(LLView*)> add_popup_t;
	typedef boost::function<void(LLView*)> remove_popup_t;
	typedef boost::function<void(void)> clear_popups_t;

	static void initClass(const settings_map_t& settings,
						  LLImageProviderInterface* image_provider,
						  LLUIAudioCallback audio_callback = NULL,
						  const LLVector2 *scale_factor = NULL,
						  const std::string& language = LLStringUtil::null);
	static void cleanupClass();
	static void setPopupFuncs(const add_popup_t& add_popup, const remove_popup_t&, const clear_popups_t& );

	static void pushMatrix();
	static void popMatrix();
	static void loadIdentity();
	static void translate(F32 x, F32 y, F32 z = 0.0f);

	static LLRect	sDirtyRect;
	static BOOL		sDirty;
	static void		dirtyRect(LLRect rect);

	// Return the ISO639 language name ("en", "ko", etc.) for the viewer UI.
	// http://www.loc.gov/standards/iso639-2/php/code_list.php
	static std::string getLanguage();

	//helper functions (should probably move free standing rendering helper functions here)
	static LLView* getRootView() { return sRootView; }
	static void setRootView(LLView* view) { sRootView = view; }
	/**
	 * Walk the LLView tree to resolve a path
	 * Paths can be discovered using Develop > XUI > Show XUI Paths
	 *
	 * A leading "/" indicates the root of the tree is the starting
	 * position of the search, (otherwise the context node is used)
	 *
	 * Adjacent "//" mean that the next level of the search is done
	 * recursively ("descendant" rather than "child").
	 *
	 * Return values: If no match is found, NULL is returned,
	 * otherwise the matching LLView* is returned.
	 *
	 * Examples:
	 *
	 * "/" -> return the root view
	 * "/foo" -> find "foo" as a direct child of the root
	 * "foo" -> find "foo" as a direct child of the context node
	 * "//foo" -> find the first "foo" child anywhere in the tree
	 * "/foo/bar" -> find "foo" as direct child of the root, and
	 *      "bar" as a direct child of "foo"
	 * "//foo//bar/baz" -> find the first "foo" anywhere in the
	 *      tree, the first "bar" anywhere under it, and "baz"
	 *      as a direct child of that
	 */
	static const LLView* resolvePath(const LLView* context, const std::string& path);
	static LLView* resolvePath(LLView* context, const std::string& path);
	static std::string locateSkin(const std::string& filename);
	static void setMousePositionScreen(S32 x, S32 y);
	static void getMousePositionScreen(S32 *x, S32 *y);
	static void setMousePositionLocal(const LLView* viewp, S32 x, S32 y);
	static void getMousePositionLocal(const LLView* viewp, S32 *x, S32 *y);
	static void setScaleFactor(const LLVector2& scale_factor);
	static void setLineWidth(F32 width);
	static LLPointer<LLUIImage> getUIImageByID(const LLUUID& image_id, S32 priority = 0);
	static LLPointer<LLUIImage> getUIImage(const std::string& name, S32 priority = 0);
	static LLVector2 getWindowSize();
	static void screenPointToGL(S32 screen_x, S32 screen_y, S32 *gl_x, S32 *gl_y);
	static void glPointToScreen(S32 gl_x, S32 gl_y, S32 *screen_x, S32 *screen_y);
	static void screenRectToGL(const LLRect& screen, LLRect *gl);
	static void glRectToScreen(const LLRect& gl, LLRect *screen);
	// Returns the control group containing the control name, or the default group
	static LLControlGroup& getControlControlGroup (const std::string& controlname);
	static F32 getMouseIdleTime() { return sMouseIdleTimer.getElapsedTimeF32(); }
	static void resetMouseIdleTimer() { sMouseIdleTimer.reset(); }
	static LLWindow* getWindow() { return sWindow; }

	static void addPopup(LLView*);
	static void removePopup(LLView*);
	static void clearPopups();

	static void reportBadKeystroke();

	// Ensures view does not overlap mouse cursor, but is inside
	// the view's parent rectangle.  Used for tooltips, inspectors.
	// Optionally override the view's default X/Y, which are relative to the
	// view's parent.
	static void positionViewNearMouse(LLView* view,	S32 spawn_x = S32_MAX, S32 spawn_y = S32_MAX);

	//
	// Data
	//
	static settings_map_t sSettingGroups;
	static LLUIAudioCallback sAudioCallback;
	static LLVector2		sGLScaleFactor;
	static LLWindow*		sWindow;
	static LLView*			sRootView;
	static LLHelp*			sHelpImpl;
private:
	static LLImageProviderInterface* sImageProvider;
	static std::vector<std::string> sXUIPaths;
	static LLFrameTimer		sMouseIdleTimer;
	static add_popup_t		sAddPopupFunc;
	static remove_popup_t	sRemovePopupFunc;
	static clear_popups_t	sClearPopupsFunc;
};


// Moved LLLocalClipRect to lllocalcliprect.h

//RN: maybe this needs to moved elsewhere?
class LLImageProviderInterface
{
protected:
	LLImageProviderInterface() {};
	virtual ~LLImageProviderInterface() {};
public:
	virtual LLPointer<LLUIImage> getUIImage(const std::string& name, S32 priority) = 0;
	virtual LLPointer<LLUIImage> getUIImageByID(const LLUUID& id, S32 priority) = 0;
	virtual void cleanUp() = 0;
};

class LLCallbackRegistry
{
public:
	typedef boost::signals2::signal<void()> callback_signal_t;
	
	void registerCallback(const callback_signal_t::slot_type& slot)
	{
		mCallbacks.connect(slot);
	}

	void fireCallbacks()
	{
		mCallbacks();
	}

private:
	callback_signal_t mCallbacks;
};

class LLInitClassList : 
	public LLCallbackRegistry, 
	public LLSingleton<LLInitClassList>
{
	friend class LLSingleton<LLInitClassList>;
private:
	LLInitClassList() {}
};

class LLDestroyClassList : 
	public LLCallbackRegistry, 
	public LLSingleton<LLDestroyClassList>
{
	friend class LLSingleton<LLDestroyClassList>;
private:
	LLDestroyClassList() {}
};

template<typename T>
class LLRegisterWith
{
public:
	LLRegisterWith(boost::function<void ()> func)
	{
		T::instance().registerCallback(func);
	}

	// this avoids a MSVC bug where non-referenced static members are "optimized" away
	// even if their constructors have side effects
	void reference()
	{
		S32 dummy;
		dummy = 0;
	}
};

template<typename T>
class LLInitClass
{
public:
	LLInitClass() { sRegister.reference(); }

	static LLRegisterWith<LLInitClassList> sRegister;
private:

	static void initClass()
	{
		llerrs << "No static initClass() method defined for " << typeid(T).name() << llendl;
	}
};

template<typename T>
class LLDestroyClass
{
public:
	LLDestroyClass() { sRegister.reference(); }

	static LLRegisterWith<LLDestroyClassList> sRegister;
private:

	static void destroyClass()
	{
		llerrs << "No static destroyClass() method defined for " << typeid(T).name() << llendl;
	}
};

template <typename T> LLRegisterWith<LLInitClassList> LLInitClass<T>::sRegister(&T::initClass);
template <typename T> LLRegisterWith<LLDestroyClassList> LLDestroyClass<T>::sRegister(&T::destroyClass);

// useful parameter blocks
struct TimeIntervalParam : public LLInitParam::ChoiceBlock<TimeIntervalParam>
{
	Alternative<F32>		seconds;
	Alternative<S32>		frames;
	TimeIntervalParam()
	:	seconds("seconds"),
		frames("frames")
	{}
};

template <class T>
class LLUICachedControl : public LLCachedControl<T>
{
public:
	// This constructor will declare a control if it doesn't exist in the contol group
	LLUICachedControl(const std::string& name,
					  const T& default_value,
					  const std::string& comment = "Declared In Code")
	:	LLCachedControl<T>(LLUI::getControlControlGroup(name), name, default_value, comment)
	{}

	// This constructor will signal an error if the control doesn't exist in the control group
	LLUICachedControl(const std::string& name)
	:	LLCachedControl<T>(LLUI::getControlControlGroup(name), name)
	{}
};

namespace LLInitParam
{
	template<>
	class ParamValue<LLRect, TypeValues<LLRect> > 
	:	public CustomParamValue<LLRect>
	{
        typedef CustomParamValue<LLRect> super_t;
	public:
		Optional<S32>	left,
						top,
						right,
						bottom,
						width,
						height;

		ParamValue(const LLRect& value);

		void updateValueFromBlock();
		void updateBlockFromValue(bool make_block_authoritative);
	};

	template<>
	class ParamValue<LLUIColor, TypeValues<LLUIColor> > 
	:	public CustomParamValue<LLUIColor>
	{
        typedef CustomParamValue<LLUIColor> super_t;

	public:
		Optional<F32>			red,
								green,
								blue,
								alpha;
		Optional<std::string>	control;

		ParamValue(const LLUIColor& color);
		void updateValueFromBlock();
		void updateBlockFromValue(bool make_block_authoritative);
	};

	template<>
	class ParamValue<const LLFontGL*, TypeValues<const LLFontGL*> > 
	:	public CustomParamValue<const LLFontGL* >
	{
        typedef CustomParamValue<const LLFontGL*> super_t;
	public:
		Optional<std::string>	name,
								size,
								style;

		ParamValue(const LLFontGL* value);
		void updateValueFromBlock();
		void updateBlockFromValue(bool make_block_authoritative);
	};

	template<>
	struct TypeValues<LLFontGL::HAlign> : public TypeValuesHelper<LLFontGL::HAlign>
	{
		static void declareValues();
	};

	template<>
	struct TypeValues<LLFontGL::VAlign> : public TypeValuesHelper<LLFontGL::VAlign>
	{
		static void declareValues();
	};

	template<>
	struct TypeValues<LLFontGL::ShadowType> : public TypeValuesHelper<LLFontGL::ShadowType>
	{
		static void declareValues();
	};

	template<>
	struct ParamCompare<const LLFontGL*, false>
	{
		static bool equals(const LLFontGL* a, const LLFontGL* b);
	};


	template<>
	class ParamValue<LLCoordGL, TypeValues<LLCoordGL> >
	:	public CustomParamValue<LLCoordGL>
	{
		typedef CustomParamValue<LLCoordGL> super_t;
	public:
		Optional<S32>	x,
						y;

		ParamValue(const LLCoordGL& val);
		void updateValueFromBlock();
		void updateBlockFromValue(bool make_block_authoritative);
	};
}

extern LLGLSLShader gSolidColorProgram;
extern LLGLSLShader gUIProgram;

#endif
