/** 
 * @file llbutton.h
 * @brief Header for buttons
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

#ifndef LL_LLBUTTON_H
#define LL_LLBUTTON_H

#include "lluuid.h"
#include "llbadgeowner.h"
#include "llcontrol.h"
#include "llflashtimer.h"
#include "lluictrl.h"
#include "v4color.h"
#include "llframetimer.h"
#include "llfontgl.h"
#include "lluiimage.h"
#include "lluistring.h"

//
// Constants
//

// PLEASE please use these "constants" when building your own buttons.
// They are loaded from settings.xml at run time.
extern S32	LLBUTTON_H_PAD;
extern S32	BTN_HEIGHT_SMALL;
extern S32	BTN_HEIGHT;

//
// Helpful functions
//
S32 round_up(S32 grid, S32 value);


class LLUICtrlFactory;

//
// Classes
//

class LLButton
: public LLUICtrl, public LLBadgeOwner
, public ll::ui::SearchableControl
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		// text label
		Optional<std::string>	label_selected;
		Optional<bool>			label_shadow;
		Optional<bool>			auto_resize;
		Optional<bool>			use_ellipses;
		Optional<bool>			use_font_color;

		// images
		Optional<LLUIImage*>	image_unselected,
								image_selected,
								image_hover_selected,
								image_hover_unselected,
								image_disabled_selected,
								image_disabled,
								image_flash,
								image_pressed,
								image_pressed_selected,
								image_overlay;

		Optional<std::string>	image_overlay_alignment;
		
		// colors
		Optional<LLUIColor>		label_color,
								label_color_selected,
								label_color_disabled,
								label_color_disabled_selected,
								image_color,
								image_color_disabled,
								image_overlay_color,
								image_overlay_selected_color,
								image_overlay_disabled_color,
								flash_color;

		// layout
		Optional<S32>			pad_right;
		Optional<S32>			pad_left;
		Optional<S32>			pad_bottom; // under text label
		
		//image overlay paddings
		Optional<S32>			image_top_pad;
		Optional<S32>			image_bottom_pad;

		/**
		 * Space between image_overlay and label
		 */
		Optional<S32>			imgoverlay_label_space;

		// callbacks
		Optional<CommitCallbackParam>	click_callback, // alias -> commit_callback
										mouse_down_callback,
										mouse_up_callback,
										mouse_held_callback;
		
		// misc
		Optional<bool>			is_toggle,
								scale_image,
								commit_on_return,
								commit_on_capture_lost,
								display_pressed_state;
		
		Optional<F32>				hover_glow_amount;
		Optional<TimeIntervalParam>	held_down_delay;

		Optional<bool>				use_draw_context_alpha;
		
		Optional<LLBadge::Params>	badge;

		Optional<bool>				handle_right_mouse;

		Optional<bool>				button_flash_enable;
		Optional<S32>				button_flash_count;
		Optional<F32>				button_flash_rate;

		Params();
	};
	
protected:
	friend class LLUICtrlFactory;
	LLButton(const Params&);

public:

	~LLButton();
	// For backward compatability only
	typedef boost::function<void(void*)> button_callback_t;

	void			addImageAttributeToXML(LLXMLNodePtr node, const std::string& imageName,
										const LLUUID&	imageID,const std::string&	xmlTagName) const;
	virtual bool	handleUnicodeCharHere(llwchar uni_char);
	virtual bool	handleKeyHere(KEY key, MASK mask);
	virtual bool	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual bool	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual bool	handleHover(S32 x, S32 y, MASK mask);
	virtual bool	handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual bool	handleRightMouseUp(S32 x, S32 y, MASK mask);
	virtual bool	handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual void	draw();
	/*virtual*/ bool postBuild();

	virtual void	onMouseLeave(S32 x, S32 y, MASK mask);
	virtual void	onMouseCaptureLost();

	virtual void	onCommit();

	void			setUnselectedLabelColor( const LLColor4& c )		{ mUnselectedLabelColor = c; }
	void			setSelectedLabelColor( const LLColor4& c )			{ mSelectedLabelColor = c; }
	void			setUseEllipses( bool use_ellipses )					{ mUseEllipses = use_ellipses; }
	void			setUseFontColor( bool use_font_color)				{ mUseFontColor = use_font_color; }


	boost::signals2::connection setClickedCallback(const CommitCallbackParam& cb);
	boost::signals2::connection setMouseDownCallback(const CommitCallbackParam& cb);
	boost::signals2::connection setMouseUpCallback(const CommitCallbackParam& cb);
	boost::signals2::connection setHeldDownCallback(const CommitCallbackParam& cb);

	boost::signals2::connection setClickedCallback( const commit_signal_t::slot_type& cb ); // mouse down and up within button
	boost::signals2::connection setMouseDownCallback( const commit_signal_t::slot_type& cb );
	boost::signals2::connection setMouseUpCallback( const commit_signal_t::slot_type& cb ); // mouse up, EVEN IF NOT IN BUTTON
	// Passes a 'count' parameter in the commit param payload, i.e. param["count"])
	boost::signals2::connection setHeldDownCallback( const commit_signal_t::slot_type& cb ); // Mouse button held down and in button

	
	// *TODO: Deprecate (for backwards compatability only)
	boost::signals2::connection setClickedCallback( button_callback_t cb, void* data );
	boost::signals2::connection setMouseDownCallback( button_callback_t cb, void* data );
	boost::signals2::connection setMouseUpCallback( button_callback_t cb, void* data );
	boost::signals2::connection setHeldDownCallback( button_callback_t cb, void* data );
		
	void			setHeldDownDelay( F32 seconds, S32 frames = 0)		{ mHeldDownDelay = seconds; mHeldDownFrameDelay = frames; }
	
	F32				getHeldDownTime() const								{ return mMouseDownTimer.getElapsedTimeF32(); }

	bool			toggleState();
	bool			getToggleState() const;
	void			setToggleState(bool b);

	void			setHighlight(bool b);
	void			setFlashing( bool b, bool force_flashing = false );
	bool			getFlashing() const		{ return mFlashing; }
    LLFlashTimer*   getFlashTimer() {return mFlashingTimer;}
	void			setFlashColor(const LLUIColor &color) { mFlashBgColor = color; };

	void			setHAlign( LLFontGL::HAlign align )		{ mHAlign = align; }
	LLFontGL::HAlign getHAlign() const						{ return mHAlign; }
	void			setLeftHPad( S32 pad )					{ mLeftHPad = pad; }
	void			setRightHPad( S32 pad )					{ mRightHPad = pad; }

	void 			setImageOverlayTopPad( S32 pad )			{ mImageOverlayTopPad = pad; }
	S32 			getImageOverlayTopPad() const				{ return mImageOverlayTopPad; }
	void 			setImageOverlayBottomPad( S32 pad )			{ mImageOverlayBottomPad = pad; }
	S32 			getImageOverlayBottomPad() const			{ return mImageOverlayBottomPad; }

	const std::string	getLabelUnselected() const { return wstring_to_utf8str(mUnselectedLabel); }
	const std::string	getLabelSelected() const { return wstring_to_utf8str(mSelectedLabel); }

	void			setImageColor(const std::string& color_control);
	void			setImageColor(const LLColor4& c);
	/*virtual*/ void	setColor(const LLColor4& c);

	void			setImages(const std::string &image_name, const std::string &selected_name);
	
	void			setDisabledImageColor(const LLColor4& c)		{ mDisabledImageColor = c; }

	void			setDisabledSelectedLabelColor( const LLColor4& c )	{ mDisabledSelectedLabelColor = c; }

	void			setImageOverlay(const std::string& image_name, LLFontGL::HAlign alignment = LLFontGL::HCENTER, const LLColor4& color = LLColor4::white);
	void 			setImageOverlay(const LLUUID& image_id, LLFontGL::HAlign alignment = LLFontGL::HCENTER, const LLColor4& color = LLColor4::white);
	LLPointer<LLUIImage> getImageOverlay() { return mImageOverlay; }
	LLFontGL::HAlign getImageOverlayHAlign() const	{ return mImageOverlayAlignment; }
	
	void            autoResize();	// resize with label of current btn state 
	void            resize(LLUIString label); // resize with label input
	void			setLabel(const std::string& label);
	void			setLabel(const LLUIString& label);
	void			setLabel( const LLStringExplicit& label);
	virtual bool	setLabelArg( const std::string& key, const LLStringExplicit& text );
	void			setLabelUnselected(const LLStringExplicit& label);
	void			setLabelSelected(const LLStringExplicit& label);
	void			setDisabledLabelColor( const LLColor4& c )		{ mDisabledLabelColor = c; }
	
	void			setFont(const LLFontGL *font)		
		{ mGLFont = ( font ? font : LLFontGL::getFontSansSerif()); }
	const LLFontGL* getFont() const { return mGLFont; }


	S32				getLastDrawCharsCount() const { return mLastDrawCharsCount; }
	bool			labelIsTruncated() const;
	const LLUIString&	getCurrentLabel() const;

	void			setScaleImage(bool scale)			{ mScaleImage = scale; }
	bool			getScaleImage() const				{ return mScaleImage; }

	void			setDropShadowedText(bool b)			{ mDropShadowedText = b; }

	void			setBorderEnabled(bool b)					{ mBorderEnabled = b; }

	void			setHoverGlowStrength(F32 strength) { mHoverGlowStrength = strength; }

	void			setImageUnselected(LLPointer<LLUIImage> image);
	void			setImageSelected(LLPointer<LLUIImage> image);
	void			setImageHoverSelected(LLPointer<LLUIImage> image);
	void			setImageHoverUnselected(LLPointer<LLUIImage> image);
	void			setImageDisabled(LLPointer<LLUIImage> image);
	void			setImageDisabledSelected(LLPointer<LLUIImage> image);
	void			setImageFlash(LLPointer<LLUIImage> image);
	void			setImagePressed(LLPointer<LLUIImage> image);
	
	void			setCommitOnReturn(bool commit) { mCommitOnReturn = commit; }
	bool			getCommitOnReturn() const { return mCommitOnReturn; }

	static void		onHeldDown(void *userdata);  // to be called by gIdleCallbacks
	static void		toggleFloaterAndSetToggleState(LLUICtrl* ctrl, const LLSD& sdname);
	static void		setFloaterToggle(LLUICtrl* ctrl, const LLSD& sdname);
	static void		setDockableFloaterToggle(LLUICtrl* ctrl, const LLSD& sdname);
	static void		showHelp(LLUICtrl* ctrl, const LLSD& sdname);

	void		setForcePressedState(bool b) { mForcePressedState = b; }
	
	void 		setAutoResize(bool auto_resize) { mAutoResize = auto_resize; }

protected:
	LLPointer<LLUIImage> getImageUnselected() const	{ return mImageUnselected; }
	LLPointer<LLUIImage> getImageSelected() const	{ return mImageSelected; }
	void getOverlayImageSize(S32& overlay_width, S32& overlay_height);

	LLFrameTimer	mMouseDownTimer;
	bool			mNeedsHighlight;
	S32				mButtonFlashCount;
	F32				mButtonFlashRate;

	void			drawBorder(LLUIImage* imagep, const LLColor4& color, S32 size);
	void			resetMouseDownTimer();

	commit_signal_t* 			mMouseDownSignal;
	commit_signal_t* 			mMouseUpSignal;
	commit_signal_t* 			mHeldDownSignal;
	
	const LLFontGL*				mGLFont;
	
	S32							mMouseDownFrame;
	S32 						mMouseHeldDownCount; 	// Counter for parameter passed to held-down callback
	F32							mHeldDownDelay;			// seconds, after which held-down callbacks get called
	S32							mHeldDownFrameDelay;	// frames, after which held-down callbacks get called
	S32							mLastDrawCharsCount;

	LLPointer<LLUIImage>		mImageOverlay;
	LLFontGL::HAlign			mImageOverlayAlignment;
	LLUIColor					mImageOverlayColor;
	LLUIColor					mImageOverlaySelectedColor;
	LLUIColor					mImageOverlayDisabledColor;

	LLPointer<LLUIImage>		mImageUnselected;
	LLUIString					mUnselectedLabel;
	LLUIColor					mUnselectedLabelColor;

	LLPointer<LLUIImage>		mImageSelected;
	LLUIString					mSelectedLabel;
	LLUIColor					mSelectedLabelColor;

	LLPointer<LLUIImage>		mImageHoverSelected;

	LLPointer<LLUIImage>		mImageHoverUnselected;

	LLPointer<LLUIImage>		mImageDisabled;
	LLUIColor					mDisabledLabelColor;

	LLPointer<LLUIImage>		mImageDisabledSelected;
	LLUIString					mDisabledSelectedLabel;
	LLUIColor					mDisabledSelectedLabelColor;

	LLPointer<LLUIImage>		mImagePressed;
	LLPointer<LLUIImage>		mImagePressedSelected;

	/* There are two ways an image can flash- by making changes in color according to flash_color attribute
	   or by changing icon from current to the one specified in image_flash. Second way is used only if
	   flash icon name is set in attributes(by default it isn't). First way is used otherwise. */
	LLPointer<LLUIImage>		mImageFlash;

	LLUIColor					mFlashBgColor;

	LLUIColor					mImageColor;
	LLUIColor					mDisabledImageColor;

	bool						mIsToggle;
	bool						mScaleImage;

	bool						mDropShadowedText;
	bool						mAutoResize;
	bool						mUseEllipses;
	bool						mUseFontColor;
	bool						mBorderEnabled;
	bool						mFlashing;

	LLFontGL::HAlign			mHAlign;
	S32							mLeftHPad;
	S32							mRightHPad;
	S32							mBottomVPad;	// under text label

	S32							mImageOverlayTopPad;
	S32							mImageOverlayBottomPad;

	bool						mUseDrawContextAlpha;

	/*
	 * Space between image_overlay and label
	 */
	S32							mImgOverlayLabelSpace;

	F32							mHoverGlowStrength;
	F32							mCurGlowStrength;

	bool						mCommitOnReturn;
    bool						mCommitOnCaptureLost;
	bool						mFadeWhenDisabled;
	bool						mForcePressedState;
	bool						mDisplayPressedState;

	LLFrameTimer				mFrameTimer;
	LLFlashTimer *				mFlashingTimer;
	bool                        mForceFlashing; // Stick flashing color even if button is pressed
	bool						mHandleRightMouse;

protected:
	virtual std::string _getSearchText() const
	{
		return getLabelUnselected() + getToolTip();
	}
};

// Build time optimization, generate once in .cpp file
#ifndef LLBUTTON_CPP
extern template class LLButton* LLView::getChild<class LLButton>(
	const std::string& name, bool recurse) const;
#endif

#endif  // LL_LLBUTTON_H
