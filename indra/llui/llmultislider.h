/** 
 * @file llmultislider.h
 * @brief A simple multislider
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#ifndef LL_MULTI_SLIDER_H
#define LL_MULTI_SLIDER_H

#include "llf32uictrl.h"
#include "v4color.h"

class LLUICtrlFactory;

class LLMultiSlider : public LLF32UICtrl
{
public:
	struct SliderParams : public LLInitParam::Block<SliderParams>
	{
		Optional<std::string>	name;
		Mandatory<F32>			value;
		SliderParams();
	};

	struct Params : public LLInitParam::Block<Params, LLF32UICtrl::Params>
	{
		Optional<S32>	max_sliders;

		Optional<bool>	allow_overlap,
						draw_track,
						use_triangle;

		Optional<LLUIColor>	track_color,
							thumb_disabled_color,
							thumb_outline_color,
							thumb_center_color,
							thumb_center_selected_color,
							triangle_color;

		Optional<CommitCallbackParam>	mouse_down_callback,
										mouse_up_callback;
		Optional<S32>		thumb_width;

		Multiple<SliderParams>	sliders;
		Params();
	};

protected:
	LLMultiSlider(const Params&);
	friend class LLUICtrlFactory;
public:
	virtual ~LLMultiSlider();
	void				setSliderValue(const std::string& name, F32 value, BOOL from_event = FALSE);
	F32					getSliderValue(const std::string& name) const;

	const std::string&	getCurSlider() const					{ return mCurSlider; }
	F32					getCurSliderValue() const				{ return getSliderValue(mCurSlider); }
	void				setCurSlider(const std::string& name);
	void				setCurSliderValue(F32 val, BOOL from_event = false) { setSliderValue(mCurSlider, val, from_event); }

	/*virtual*/ void	setValue(const LLSD& value);
	/*virtual*/ LLSD	getValue() const		{ return mValue; }

	boost::signals2::connection setMouseDownCallback( const commit_signal_t::slot_type& cb );
	boost::signals2::connection setMouseUpCallback(	const commit_signal_t::slot_type& cb );

	bool				findUnusedValue(F32& initVal);
	const std::string&	addSlider();
	const std::string&	addSlider(F32 val);
	void				addSlider(F32 val, const std::string& name);
	void				deleteSlider(const std::string& name);
	void				deleteCurSlider()			{ deleteSlider(mCurSlider); }
	void				clear();

	/*virtual*/ BOOL	handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleKeyHere(KEY key, MASK mask);
	/*virtual*/ void	draw();

protected:
	LLSD			mValue;
	std::string		mCurSlider;
	static S32		mNameCounter;

	S32				mMaxNumSliders;
	BOOL			mAllowOverlap;
	BOOL			mDrawTrack;
	BOOL			mUseTriangle;			/// hacked in toggle to use a triangle

	S32				mMouseOffset;
	LLRect			mDragStartThumbRect;
	S32				mThumbWidth;

	std::map<std::string, LLRect>	
					mThumbRects;
	LLUIColor		mTrackColor;
	LLUIColor		mThumbOutlineColor;
	LLUIColor		mThumbCenterColor;
	LLUIColor		mThumbCenterSelectedColor;
	LLUIColor		mDisabledThumbColor;
	LLUIColor		mTriangleColor;
	
	commit_signal_t*	mMouseDownSignal;
	commit_signal_t*	mMouseUpSignal;
};

#endif  // LL_MULTI_SLIDER_H
