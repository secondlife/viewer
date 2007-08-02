/** 
 * @file llcolorswatch.h
 * @brief LLColorSwatch class definition
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLCOLORSWATCH_H
#define LL_LLCOLORSWATCH_H

#include "lluictrl.h"
#include "v4color.h"
#include "llfloater.h"
#include "llviewerimage.h"

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

	LLColorSwatchCtrl(const std::string& name, const LLRect& rect, const LLColor4& color,
		void (*on_commit_callback)(LLUICtrl* ctrl, void* userdata),
		void* callback_userdata);
	LLColorSwatchCtrl(const std::string& name, const LLRect& rect, const std::string& label, const LLColor4& color,
		void (*on_commit_callback)(LLUICtrl* ctrl, void* userdata),
		void* callback_userdata);

	~LLColorSwatchCtrl ();

	virtual void setValue(const LLSD& value);
	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_COLOR_SWATCH; }
	virtual LLString getWidgetTag() const { return LL_COLOR_SWATCH_CTRL_TAG; }
	virtual LLSD getValue() const { return mColor.getValue(); }
	const LLColor4&	get()							{ return mColor; }
	
	void			set(const LLColor4& color, BOOL update_picker = FALSE, BOOL from_event = FALSE);
	void			setOriginal(const LLColor4& color);
	void			setValid(BOOL valid);
	void			setLabel(const std::string& label);
	void			setCanApplyImmediately(BOOL apply) { mCanApplyImmediately = apply; }
	void			setOnCancelCallback(LLUICtrlCallback cb) { mOnCancelCallback = cb; }
	void			setOnSelectCallback(LLUICtrlCallback cb) { mOnSelectCallback = cb; }

	void			showPicker(BOOL take_focus);

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDoubleClick(S32 x,S32 y,MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent);
	virtual void	draw();
	virtual void	setEnabled( BOOL enabled );
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	static void		onColorChanged ( void* data, EColorPickOp pick_op = COLOR_CHANGE );

protected:
	BOOL			mValid;
	LLColor4		mColor;
	LLColor4		mBorderColor;
	LLTextBox*		mCaption;
	LLViewHandle mPickerHandle;
	LLViewBorder*	mBorder;
	BOOL			mCanApplyImmediately;
	LLUICtrlCallback mOnCancelCallback;
	LLUICtrlCallback mOnSelectCallback;

	LLPointer<LLViewerImage> mAlphaGradientImage;
};

#endif  // LL_LLBUTTON_H
