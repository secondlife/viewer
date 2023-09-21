/** 
 * @file llui.h
 * @brief General static UI services.
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


#ifndef LL_LLUI_H
#define LL_LLUI_H

#include "llrect.h"
#include "llcoord.h"
#include "llcontrol.h"
#include "llcoord.h"
#include "llcontrol.h"
#include "llinitparam.h"
#include "llregistry.h"
#include "llrender2dutils.h"
#include "llpointer.h"
#include "lluicolor.h"
#include "lluicolortable.h"
#include "lluiimage.h"
#include <boost/signals2.hpp>
#include "llframetimer.h"
#include "v2math.h"
#include <limits>

// for initparam specialization
#include "llfontgl.h"

class LLUUID;
class LLWindow;
class LLView;
class LLHelp;

const S32 DRAG_N_DROP_DISTANCE_THRESHOLD = 3;
// this enum is used by the llview.h (viewer) and the llassetstorage.h (viewer and sim) 
enum EDragAndDropType
{
	DAD_NONE			= 0,
	DAD_TEXTURE			= 1,
	DAD_SOUND			= 2,
	DAD_CALLINGCARD		= 3,
	DAD_LANDMARK		= 4,
	DAD_SCRIPT			= 5,
	DAD_CLOTHING 		= 6,
	DAD_OBJECT			= 7,
	DAD_NOTECARD		= 8,
	DAD_CATEGORY		= 9,
	DAD_ROOT_CATEGORY 	= 10,
	DAD_BODYPART		= 11,
	DAD_ANIMATION		= 12,
	DAD_GESTURE			= 13,
	DAD_LINK			= 14,
	DAD_MESH            = 15,
	DAD_WIDGET          = 16,
	DAD_PERSON          = 17,
    DAD_SETTINGS        = 18,
	DAD_COUNT           = 19,   // number of types in this enum
};

// Reasons for drags to be denied.
// ordered by priority for multi-drag
enum EAcceptance
{
	ACCEPT_POSTPONED,	// we are asynchronously determining acceptance
	ACCEPT_NO,			// Uninformative, general purpose denial.
	ACCEPT_NO_CUSTOM,	// Denial with custom message.
	ACCEPT_NO_LOCKED,	// Operation would be valid, but permissions are set to disallow it.
	ACCEPT_YES_COPY_SINGLE,	// We'll take a copy of a single item
	ACCEPT_YES_SINGLE,		// Accepted. OK to drag and drop single item here.
	ACCEPT_YES_COPY_MULTI,	// We'll take a copy of multiple items
	ACCEPT_YES_MULTI		// Accepted. OK to drag and drop multiple items here.
};

enum EAddPosition
{
	ADD_TOP,
	ADD_BOTTOM,
	ADD_DEFAULT
};


void make_ui_sound(const char* name);
void make_ui_sound_deferred(const char * name);

class LLImageProviderInterface;

typedef	void (*LLUIAudioCallback)(const LLUUID& uuid);

class LLUI : public LLParamSingleton<LLUI>
{
public:
	typedef std::map<std::string, LLControlGroup*> settings_map_t;

private:
	LLSINGLETON(LLUI , const settings_map_t &settings,
						   LLImageProviderInterface* image_provider,
						   LLUIAudioCallback audio_callback,
						   LLUIAudioCallback deferred_audio_callback);
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
				LL_WARNS() << "Bad interval range (" << mMin << ", " << mMax << ")" << LL_ENDL;
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
	typedef boost::function<void(LLView*)> add_popup_t;
	typedef boost::function<void(LLView*)> remove_popup_t;
	typedef boost::function<void(void)> clear_popups_t;

	void setPopupFuncs(const add_popup_t& add_popup, const remove_popup_t&, const clear_popups_t& );

	// Return the ISO639 language name ("en", "ko", etc.) for the viewer UI.
	// http://www.loc.gov/standards/iso639-2/php/code_list.php
	std::string getUILanguage();
	static std::string getLanguage(); // static for lldateutil_test compatibility

	//helper functions (should probably move free standing rendering helper functions here)
	LLView* getRootView() { return mRootView; }
	void setRootView(LLView* view) { mRootView = view; }
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
	const LLView* resolvePath(const LLView* context, const std::string& path);
	LLView* resolvePath(LLView* context, const std::string& path);
	static std::string locateSkin(const std::string& filename);
	void setMousePositionScreen(S32 x, S32 y);
	void getMousePositionScreen(S32 *x, S32 *y);
	void setMousePositionLocal(const LLView* viewp, S32 x, S32 y);
	void getMousePositionLocal(const LLView* viewp, S32 *x, S32 *y);
	LLVector2 getWindowSize();
	void screenPointToGL(S32 screen_x, S32 screen_y, S32 *gl_x, S32 *gl_y);
	void glPointToScreen(S32 gl_x, S32 gl_y, S32 *screen_x, S32 *screen_y);
	void screenRectToGL(const LLRect& screen, LLRect *gl);
	void glRectToScreen(const LLRect& gl, LLRect *screen);
	// Returns the control group containing the control name, or the default group
	LLControlGroup& getControlControlGroup (const std::string& controlname);
	F32 getMouseIdleTime() { return mMouseIdleTimer.getElapsedTimeF32(); }
	void resetMouseIdleTimer() { mMouseIdleTimer.reset(); }
	LLWindow* getWindow() { return mWindow; }

	void addPopup(LLView*);
	void removePopup(LLView*);
	void clearPopups();

	void reportBadKeystroke();

	// Ensures view does not overlap mouse cursor, but is inside
	// the view's parent rectangle.  Used for tooltips, inspectors.
	// Optionally override the view's default X/Y, which are relative to the
	// view's parent.
	void positionViewNearMouse(LLView* view,	S32 spawn_x = S32_MAX, S32 spawn_y = S32_MAX);

	// LLRender2D wrappers
	static void pushMatrix() { LLRender2D::pushMatrix(); }
	static void popMatrix() { LLRender2D::popMatrix(); }
	static void loadIdentity() { LLRender2D::loadIdentity(); }
	static void translate(F32 x, F32 y, F32 z = 0.0f) { LLRender2D::translate(x, y, z); }

    static LLVector2& getScaleFactor();
    static void setScaleFactor(const LLVector2& scale_factor);
	static void setLineWidth(F32 width) { LLRender2D::setLineWidth(width); }
	static LLPointer<LLUIImage> getUIImageByID(const LLUUID& image_id, S32 priority = 0)
		{ return LLRender2D::getInstance()->getUIImageByID(image_id, priority); }
	static LLPointer<LLUIImage> getUIImage(const std::string& name, S32 priority = 0)
		{ return LLRender2D::getInstance()->getUIImage(name, priority); }

	//
	// Data
	//
	settings_map_t mSettingGroups;
	LLUIAudioCallback mAudioCallback;
	LLUIAudioCallback mDeferredAudioCallback;
	LLWindow*		mWindow;
	LLView*			mRootView;
	LLHelp*			mHelpImpl;
private:
	std::vector<std::string> mXUIPaths;
	LLFrameTimer		mMouseIdleTimer;
	add_popup_t		mAddPopupFunc;
	remove_popup_t	mRemovePopupFunc;
	clear_popups_t	mClearPopupsFunc;
};


// Moved LLLocalClipRect to lllocalcliprect.h

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
	:	LLCachedControl<T>(LLUI::getInstance()->getControlControlGroup(name), name, default_value, comment)
	{}
};

namespace LLInitParam
{
	template<>
	class ParamValue<LLRect> 
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
	class ParamValue<LLUIColor> 
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
	class ParamValue<const LLFontGL*> 
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
	class ParamValue<LLCoordGL>
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

#endif
