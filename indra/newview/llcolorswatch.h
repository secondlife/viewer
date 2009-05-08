/** 
 * @file llcolorswatch.h
 * @brief LLColorSwatch class definition
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

#ifndef LL_LLCOLORSWATCH_H
#define LL_LLCOLORSWATCH_H

#include "lluictrl.h"
#include "v4color.h"
#include "llfloater.h"
#include "llviewerimage.h"
#include "lltextbox.h"

//
// Classes
//
class LLColor4;
class LLTextBox;
class LLFloaterColorPicker;
class LLViewerImage;

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
		Optional<LLColor4>			color;
		Optional<bool>				can_apply_immediately;
		Optional<LLUIImage*>		alpha_background_image;
		Optional<commit_callback_t> cancel_callback;
		Optional<commit_callback_t> select_callback;
		Optional<LLUIColor>			border_color;
		Optional<S32>				label_width;
		
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

	LLPointer<LLUIImage> mAlphaGradientImage;
	std::string		mFallbackImageName;
};

#endif  // LL_LLBUTTON_H
