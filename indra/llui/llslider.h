/** 
 * @file llslider.h
 * @brief A simple slider with no label.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLSLIDER_H
#define LL_LLSLIDER_H

#include "llf32uictrl.h"
#include "v4color.h"

class LLSlider : public LLF32UICtrl
{
public:
	enum ORIENTATION { HORIZONTAL, VERTICAL };

	struct Params : public LLInitParam::Block<Params, LLF32UICtrl::Params>
	{
		Optional<std::string> orientation;

		Optional<LLUIColor>	track_color,
							thumb_outline_color,
							thumb_center_color;

		Optional<LLUIImage*>	thumb_image,
								thumb_image_pressed,
								thumb_image_disabled,
								track_image_horizontal,
								track_image_vertical,
								track_highlight_horizontal_image,
								track_highlight_vertical_image;

		Optional<CommitCallbackParam>	mouse_down_callback,
										mouse_up_callback;


		Params();
	};
protected:
	LLSlider(const Params&);
	friend class LLUICtrlFactory;
public:
	void			setValue( F32 value, BOOL from_event = FALSE );
    // overrides for LLF32UICtrl methods
	virtual void	setValue(const LLSD& value )	{ setValue((F32)value.asReal(), TRUE); }
	
	virtual void 	setMinValue(const LLSD& min_value) { setMinValue((F32)min_value.asReal()); }
	virtual void 	setMaxValue(const LLSD& max_value) { setMaxValue((F32)max_value.asReal()); }
	virtual void	setMinValue(F32 min_value) { LLF32UICtrl::setMinValue(min_value); updateThumbRect(); }
	virtual void	setMaxValue(F32 max_value) { LLF32UICtrl::setMaxValue(max_value); updateThumbRect(); }

	boost::signals2::connection setMouseDownCallback( const commit_signal_t::slot_type& cb ) { return mMouseDownSignal.connect(cb); }
	boost::signals2::connection setMouseUpCallback(	const commit_signal_t::slot_type& cb )   { return mMouseUpSignal.connect(cb); }

	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleKeyHere(KEY key, MASK mask);
	virtual BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual void	draw();

private:
	void			setValueAndCommit(F32 value);
	void			updateThumbRect();

	BOOL			mVolumeSlider;
	S32				mMouseOffset;
	LLRect			mDragStartThumbRect;

	LLPointer<LLUIImage>	mThumbImage;
	LLPointer<LLUIImage>	mThumbImagePressed;
	LLPointer<LLUIImage>	mThumbImageDisabled;
	LLPointer<LLUIImage>	mTrackImageHorizontal;
	LLPointer<LLUIImage>	mTrackImageVertical;
	LLPointer<LLUIImage>	mTrackHighlightHorizontalImage;
	LLPointer<LLUIImage>	mTrackHighlightVerticalImage;

	const ORIENTATION	mOrientation;

	LLRect		mThumbRect;
	LLUIColor	mTrackColor;
	LLUIColor	mThumbOutlineColor;
	LLUIColor	mThumbCenterColor;
	
	commit_signal_t	mMouseDownSignal;
	commit_signal_t	mMouseUpSignal;
};

#endif  // LL_LLSLIDER_H
