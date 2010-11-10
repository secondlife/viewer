/** 
 * @file llcolorswatch.h
 * @brief LLColorSwatch class definition
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

#ifndef LL_LLCOLORSWATCH_H
#define LL_LLCOLORSWATCH_H

#include "lluictrl.h"
#include "v4color.h"
#include "llfloater.h"
#include "llviewertexture.h"
#include "lltextbox.h"

//
// Classes
//
class LLColor4;
class LLTextBox;
class LLFloaterColorPicker;
class LLViewerTexture;

class LLColorSwatchCtrl
: public LLUICtrl
{
public:
	typedef enum e_color_pick_op
	{
		COLOR_CHANGE,
		COLOR_SELECT,
		COLOR_CANCEL
	} EColorPickOp;

	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<LLUIColor>				color;
		Optional<bool>					can_apply_immediately;
		Optional<LLUIImage*>			alpha_background_image;
		Optional<commit_callback_t> 	cancel_callback;
		Optional<commit_callback_t> 	select_callback;
		Optional<LLUIColor>				border_color;
		Optional<S32>					label_width;
		Optional<S32>					label_height;
		
		Optional<LLTextBox::Params>		caption_text;
		Optional<LLViewBorder::Params>	border;
		Params();
	};

protected:
	LLColorSwatchCtrl(const Params& p);
	friend class LLUICtrlFactory;
public:
	~LLColorSwatchCtrl ();

	/*virtual*/ void setValue(const LLSD& value);

	/*virtual*/ LLSD getValue() const { return mColor.getValue(); }
	const LLColor4&	get()							{ return mColor; }
	
	void			set(const LLColor4& color, BOOL update_picker = FALSE, BOOL from_event = FALSE);
	void			setOriginal(const LLColor4& color);
	void			setValid(BOOL valid);
	void			setLabel(const std::string& label);
	void			setLabelWidth(S32 label_width) {mLabelWidth =label_width;}
	void			setCanApplyImmediately(BOOL apply) { mCanApplyImmediately = apply; }
	void			setOnCancelCallback(commit_callback_t cb) { mOnCancelCallback = cb; }
	void			setOnSelectCallback(commit_callback_t cb) { mOnSelectCallback = cb; }
	void			setFallbackImageName(const std::string& name) { mFallbackImageName = name; }

	void			showPicker(BOOL take_focus);

	/*virtual*/ BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleDoubleClick(S32 x,S32 y,MASK mask);
	/*virtual*/ BOOL	handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleUnicodeCharHere(llwchar uni_char);
	/*virtual*/ void	draw();
	/*virtual*/ void	setEnabled( BOOL enabled );

	static void		onColorChanged ( void* data, EColorPickOp pick_op = COLOR_CHANGE );
	void			onParentFloaterClosed();

protected:
	BOOL			mValid;
	LLColor4		mColor;
	LLUIColor		mBorderColor;
	LLTextBox*		mCaption;
	LLHandle<LLFloater> mPickerHandle;
	LLViewBorder*	mBorder;
	BOOL			mCanApplyImmediately;
	commit_callback_t mOnCancelCallback;
	commit_callback_t mOnSelectCallback;
	S32             mLabelWidth;
	S32             mLabelHeight;

	LLPointer<LLUIImage> mAlphaGradientImage;
	std::string		mFallbackImageName;
};

#endif  // LL_LLBUTTON_H
