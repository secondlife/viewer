/** 
 * @file llbutton.h
 * @brief Header for buttons
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

#ifndef LL_LLBUTTON_H
#define LL_LLBUTTON_H

#include "lluuid.h"
#include "llcontrol.h"
#include "lluictrl.h"
#include "v4color.h"
#include "llframetimer.h"
#include "llfontgl.h"
#include "llimage.h"
#include "lluistring.h"

//
// Constants
//

// PLEASE please use these "constants" when building your own buttons.
// They are loaded from settings.xml at run time.
extern S32	LLBUTTON_H_PAD;
extern S32	LLBUTTON_V_PAD;
extern S32	BTN_HEIGHT_SMALL;
extern S32	BTN_HEIGHT;

// All button widths should be rounded up to this size
extern S32	BTN_GRID;

//
// Helpful functions
//
S32 round_up(S32 grid, S32 value);


class LLUICtrlFactory;

//
// Classes
//

class LLButton
: public LLUICtrl
{
public:
	// simple button with text label
	LLButton(const std::string& name, const LLRect &rect, const std::string& control_name = std::string(), 
			 void (*on_click)(void*) = NULL, void *data = NULL);

	LLButton(const std::string& name, const LLRect& rect, 
			 const std::string &unselected_image,
			 const std::string &selected_image,
			 const std::string& control_name,	
			 void (*click_callback)(void*),
			 void *callback_data = NULL,
			 const LLFontGL* mGLFont = NULL,
			 const std::string& unselected_label = LLStringUtil::null,
			 const std::string& selected_label = LLStringUtil::null );

	virtual ~LLButton();
	void init(void (*click_callback)(void*), void *callback_data, const LLFontGL* font, const std::string& control_name);

	
	void			addImageAttributeToXML(LLXMLNodePtr node, const std::string& imageName,
										const LLUUID&	imageID,const std::string&	xmlTagName) const;
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	virtual BOOL	handleUnicodeCharHere(llwchar uni_char);
	virtual BOOL	handleKeyHere(KEY key, MASK mask);
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual void	draw();

	virtual void	onMouseCaptureLost();

	virtual void	onCommit();

	void			setUnselectedLabelColor( const LLColor4& c )		{ mUnselectedLabelColor = c; }
	void			setSelectedLabelColor( const LLColor4& c )			{ mSelectedLabelColor = c; }

	void			setClickedCallback( void (*cb)(void *data), void* data = NULL ); // mouse down and up within button
	void			setMouseDownCallback( void (*cb)(void *data) )		{ mMouseDownCallback = cb; }	// mouse down within button
	void			setMouseUpCallback( void (*cb)(void *data) )		{ mMouseUpCallback = cb; }		// mouse up, EVEN IF NOT IN BUTTON
	void			setHeldDownCallback( void (*cb)(void *data) )		{ mHeldDownCallback = cb; }		// Mouse button held down and in button
	void			setHeldDownDelay( F32 seconds, S32 frames = 0)		{ mHeldDownDelay = seconds; mHeldDownFrameDelay = frames; }

	F32				getHeldDownTime() const								{ return mMouseDownTimer.getElapsedTimeF32(); }

	BOOL			getIsToggle() const { return mIsToggle; }
	void			setIsToggle(BOOL is_toggle) { mIsToggle = is_toggle; }
	BOOL			toggleState();
	BOOL			getToggleState() const	{ return mToggleState; }
	void			setToggleState(BOOL b);

	void			setFlashing( BOOL b );
	BOOL			getFlashing() const		{ return mFlashing; }

	void			setHAlign( LLFontGL::HAlign align )		{ mHAlign = align; }
	LLFontGL::HAlign getHAlign() const						{ return mHAlign; }
	void			setLeftHPad( S32 pad )					{ mLeftHPad = pad; }
	void			setRightHPad( S32 pad )					{ mRightHPad = pad; }

	const std::string	getLabelUnselected() const { return wstring_to_utf8str(mUnselectedLabel); }
	const std::string	getLabelSelected() const { return wstring_to_utf8str(mSelectedLabel); }

	void			setImageColor(const std::string& color_control);
	void			setImageColor(const LLColor4& c);
	virtual void	setColor(const LLColor4& c);

	void			setImages(const std::string &image_name, const std::string &selected_name);
	void			setDisabledImages(const std::string &image_name, const std::string &selected_name);
	void			setDisabledImages(const std::string &image_name, const std::string &selected_name, const LLColor4& c);
	
	void			setHoverImages(const std::string &image_name, const std::string &selected_name);

	void			setDisabledImageColor(const LLColor4& c)		{ mDisabledImageColor = c; }

	void			setDisabledSelectedLabelColor( const LLColor4& c )	{ mDisabledSelectedLabelColor = c; }

	void			setImageOverlay(const std::string& image_name, LLFontGL::HAlign alignment = LLFontGL::HCENTER, const LLColor4& color = LLColor4::white);
	LLPointer<LLUIImage> getImageOverlay() { return mImageOverlay; }
	

	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const;

	void			setLabel( const LLStringExplicit& label);
	virtual BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );
	void			setLabelUnselected(const LLStringExplicit& label);
	void			setLabelSelected(const LLStringExplicit& label);
	void			setDisabledLabel(const LLStringExplicit& disabled_label);
	void			setDisabledSelectedLabel(const LLStringExplicit& disabled_label);
	void			setDisabledLabelColor( const LLColor4& c )		{ mDisabledLabelColor = c; }
	
	void			setFont(const LLFontGL *font)		
		{ mGLFont = ( font ? font : LLFontGL::sSansSerif); }
	void			setScaleImage(BOOL scale)			{ mScaleImage = scale; }
	BOOL			getScaleImage() const				{ return mScaleImage; }

	void			setDropShadowedText(BOOL b)			{ mDropShadowedText = b; }

	void			setBorderEnabled(BOOL b)					{ mBorderEnabled = b; }

	static void		onHeldDown(void *userdata);  // to be called by gIdleCallbacks

	void			setHoverGlowStrength(F32 strength) { mHoverGlowStrength = strength; }

	void			setImageUnselected(const std::string &image_name);
	const std::string& getImageUnselectedName() const { return mImageUnselectedName; }
	void			setImageSelected(const std::string &image_name);
	const std::string& getImageSelectedName() const { return mImageSelectedName; }
	void			setImageHoverSelected(const std::string &image_name);
	void			setImageHoverUnselected(const std::string &image_name);
	void			setImageDisabled(const std::string &image_name);
	void			setImageDisabledSelected(const std::string &image_name);

	void			setImageUnselected(LLPointer<LLUIImage> image);
	void			setImageSelected(LLPointer<LLUIImage> image);
	void			setImageHoverSelected(LLPointer<LLUIImage> image);
	void			setImageHoverUnselected(LLPointer<LLUIImage> image);
	void			setImageDisabled(LLPointer<LLUIImage> image);
	void			setImageDisabledSelected(LLPointer<LLUIImage> image);

	void			setCommitOnReturn(BOOL commit) { mCommitOnReturn = commit; }
	BOOL			getCommitOnReturn() const { return mCommitOnReturn; }

	void			setHelpURLCallback(const std::string &help_url);
	const std::string&	getHelpURL() const { return mHelpURL; }

protected:

	virtual void	drawBorder(const LLColor4& color, S32 size);

	void			setImageUnselectedID(const LLUUID &image_id);
	const LLUUID&	getImageUnselectedID() const { return mImageUnselectedID; }
	void			setImageSelectedID(const LLUUID &image_id);
	const LLUUID&	getImageSelectedID() const { return mImageSelectedID; }
	void			setImageHoverSelectedID(const LLUUID &image_id);
	void			setImageHoverUnselectedID(const LLUUID &image_id);
	void			setImageDisabledID(const LLUUID &image_id);
	void			setImageDisabledSelectedID(const LLUUID &image_id);
	const LLPointer<LLUIImage>&	getImageUnselected() const	{ return mImageUnselected; }
	const LLPointer<LLUIImage>& getImageSelected() const	{ return mImageSelected; }

	LLFrameTimer	mMouseDownTimer;

private:

	void			(*mClickedCallback)(void* data );
	void			(*mMouseDownCallback)(void *data);
	void			(*mMouseUpCallback)(void *data);
	void			(*mHeldDownCallback)(void *data);

	const LLFontGL	*mGLFont;
	
	S32				mMouseDownFrame;
	F32				mHeldDownDelay;		// seconds, after which held-down callbacks get called
	S32				mHeldDownFrameDelay;	// frames, after which held-down callbacks get called

	LLPointer<LLUIImage>	mImageOverlay;
	LLFontGL::HAlign		mImageOverlayAlignment;
	LLColor4				mImageOverlayColor;

	LLPointer<LLUIImage>	mImageUnselected;
	LLUIString				mUnselectedLabel;
	LLColor4				mUnselectedLabelColor;

	LLPointer<LLUIImage>	mImageSelected;
	LLUIString				mSelectedLabel;
	LLColor4				mSelectedLabelColor;

	LLPointer<LLUIImage>	mImageHoverSelected;

	LLPointer<LLUIImage>	mImageHoverUnselected;

	LLPointer<LLUIImage>	mImageDisabled;
	LLUIString				mDisabledLabel;
	LLColor4				mDisabledLabelColor;

	LLPointer<LLUIImage>	mImageDisabledSelected;
	LLUIString				mDisabledSelectedLabel;
	LLColor4				mDisabledSelectedLabelColor;

	LLUUID			mImageUnselectedID;
	LLUUID			mImageSelectedID;
	LLUUID			mImageHoverSelectedID;
	LLUUID			mImageHoverUnselectedID;
	LLUUID			mImageDisabledID;
	LLUUID			mImageDisabledSelectedID;
	std::string		mImageUnselectedName;
	std::string		mImageSelectedName;
	std::string		mImageHoverSelectedName;
	std::string		mImageHoverUnselectedName;
	std::string		mImageDisabledName;
	std::string		mImageDisabledSelectedName;

	LLColor4		mHighlightColor;
	LLColor4		mUnselectedBgColor;
	LLColor4		mSelectedBgColor;

	LLColor4		mImageColor;
	LLColor4		mDisabledImageColor;

	BOOL			mIsToggle;
	BOOL			mToggleState;
	BOOL			mScaleImage;

	BOOL			mDropShadowedText;

	BOOL			mBorderEnabled;

	BOOL			mFlashing;

	LLFontGL::HAlign mHAlign;
	S32				mLeftHPad;
	S32				mRightHPad;

	F32				mHoverGlowStrength;
	F32				mCurGlowStrength;

	BOOL			mNeedsHighlight;
	BOOL			mCommitOnReturn;

	std::string		mHelpURL;

	LLPointer<LLUIImage> mImagep;

	LLFrameTimer	mFlashingTimer;
};

#endif  // LL_LLBUTTON_H
