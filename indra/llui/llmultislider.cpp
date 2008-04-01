/** 
 * @file llmultisldr.cpp
 * @brief LLMultiSlider base class
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2007, Linden Research, Inc.
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

#include "linden_common.h"

#include "llmultislider.h"
#include "llui.h"

#include "llgl.h"
#include "llwindow.h"
#include "llfocusmgr.h"
#include "llkeyboard.h"			// for the MASK constants
#include "llcontrol.h"
#include "llimagegl.h"

#include <sstream>

static LLRegisterWidget<LLMultiSlider> r("multi_slider_bar");

const S32 MULTI_THUMB_WIDTH = 8;
const S32 MULTI_TRACK_HEIGHT = 6;
const F32 FLOAT_THRESHOLD = 0.00001f;
const S32 EXTRA_TRIANGLE_WIDTH = 2;
const S32 EXTRA_TRIANGLE_HEIGHT = -2;

S32 LLMultiSlider::mNameCounter = 0;

LLMultiSlider::LLMultiSlider( 
	const LLString& name,
	const LLRect& rect,
	void (*on_commit_callback)(LLUICtrl* ctrl, void* userdata),
	void* callback_userdata,
	F32 initial_value,
	F32 min_value,
	F32 max_value,
	F32 increment,
	S32 max_sliders,
	BOOL allow_overlap,
	BOOL draw_track,
	BOOL use_triangle,
	const LLString& control_name)
	:
	LLUICtrl( name, rect, TRUE,	on_commit_callback, callback_userdata, 
		FOLLOWS_LEFT | FOLLOWS_TOP),

	mInitialValue( initial_value ),
	mMinValue( min_value ),
	mMaxValue( max_value ),
	mIncrement( increment ),
	mMaxNumSliders(max_sliders),
	mAllowOverlap(allow_overlap),
	mDrawTrack(draw_track),
	mUseTriangle(use_triangle),
	mMouseOffset( 0 ),
	mDragStartThumbRect( 0, getRect().getHeight(), MULTI_THUMB_WIDTH, 0 ),
	mTrackColor(		LLUI::sColorsGroup->getColor( "MultiSliderTrackColor" ) ),
	mThumbOutlineColor(	LLUI::sColorsGroup->getColor( "MultiSliderThumbOutlineColor" ) ),
	mThumbCenterColor(	LLUI::sColorsGroup->getColor( "MultiSliderThumbCenterColor" ) ),
	mThumbCenterSelectedColor(	LLUI::sColorsGroup->getColor( "MultiSliderThumbCenterSelectedColor" ) ),
	mDisabledThumbColor(LLUI::sColorsGroup->getColor( "MultiSliderDisabledThumbColor" ) ),
	mTriangleColor(LLUI::sColorsGroup->getColor( "MultiSliderTriangleColor" ) ),
	mMouseDownCallback( NULL ),
	mMouseUpCallback( NULL )
{
	mValue.emptyMap();
	mCurSlider = LLString::null;

	// properly handle setting the starting thumb rect
	// do it this way to handle both the operating-on-settings
	// and standalone ways of using this
	setControlName(control_name, NULL);
	setValue(getValue());
}

void LLMultiSlider::setSliderValue(const LLString& name, F32 value, BOOL from_event)
{
	// exit if not there
	if(!mValue.has(name)) {
		return;
	}

	value = llclamp( value, mMinValue, mMaxValue );

	// Round to nearest increment (bias towards rounding down)
	value -= mMinValue;
	value += mIncrement/2.0001f;
	value -= fmod(value, mIncrement);
	F32 newValue = mMinValue + value;

	// now, make sure no overlap
	// if we want that
	if(!mAllowOverlap) {
		bool hit = false;

		// look at the current spot
		// and see if anything is there
		LLSD::map_iterator mIt = mValue.beginMap();
		for(;mIt != mValue.endMap(); mIt++) {
			
			F32 testVal = (F32)mIt->second.asReal() - newValue;
			if(testVal > -FLOAT_THRESHOLD && testVal < FLOAT_THRESHOLD &&
				mIt->first != name) {
				hit = true;
				break;
			}
		}

		// if none found, stop
		if(hit) {
			return;
		}
	}
	

	// now set it in the map
	mValue[name] = newValue;

	// set the control if it's the current slider and not from an event
	if (!from_event && name == mCurSlider)
	{
		setControlValue(mValue);
	}
	
	F32 t = (newValue - mMinValue) / (mMaxValue - mMinValue);

	S32 left_edge = MULTI_THUMB_WIDTH/2;
	S32 right_edge = getRect().getWidth() - (MULTI_THUMB_WIDTH/2);

	S32 x = left_edge + S32( t * (right_edge - left_edge) );
	mThumbRects[name].mLeft = x - (MULTI_THUMB_WIDTH/2);
	mThumbRects[name].mRight = x + (MULTI_THUMB_WIDTH/2);
}

void LLMultiSlider::setValue(const LLSD& value)
{
	// only do if it's a map
	if(value.isMap()) {
		
		// add each value... the first in the map becomes the current
		LLSD::map_const_iterator mIt = value.beginMap();
		mCurSlider = mIt->first;

		for(; mIt != value.endMap(); mIt++) {
			setSliderValue(mIt->first, (F32)mIt->second.asReal(), TRUE);
		}
	}
}

F32 LLMultiSlider::getSliderValue(const LLString& name) const
{
	return (F32)mValue[name].asReal();
}

void LLMultiSlider::setCurSlider(const LLString& name)
{
	if(mValue.has(name)) {
		mCurSlider = name;
	}
}

const LLString& LLMultiSlider::addSlider()
{
	return addSlider(mInitialValue);
}

const LLString& LLMultiSlider::addSlider(F32 val)
{
	std::stringstream newName;
	F32 initVal = val;

	if(mValue.size() >= mMaxNumSliders) {
		return LLString::null;
	}

	// create a new name
	newName << "sldr" << mNameCounter;
	mNameCounter++;

	bool foundOne = findUnusedValue(initVal);
	if(!foundOne) {
		return LLString::null;
	}

	// add a new thumb rect
	mThumbRects[newName.str()] = LLRect( 0, getRect().getHeight(), MULTI_THUMB_WIDTH, 0 );

	// add the value and set the current slider to this one
	mValue.insert(newName.str(), initVal);
	mCurSlider = newName.str();

	// move the slider
	setSliderValue(mCurSlider, initVal, TRUE);

	return mCurSlider;
}

bool LLMultiSlider::findUnusedValue(F32& initVal)
{
	bool firstTry = true;

	// find the first open slot starting with
	// the initial value
	while(true) {
		
		bool hit = false;

		// look at the current spot
		// and see if anything is there
		LLSD::map_iterator mIt = mValue.beginMap();
		for(;mIt != mValue.endMap(); mIt++) {
			
			F32 testVal = (F32)mIt->second.asReal() - initVal;
			if(testVal > -FLOAT_THRESHOLD && testVal < FLOAT_THRESHOLD) {
				hit = true;
				break;
			}
		}

		// if we found one
		if(!hit) {
			break;
		}

		// increment and wrap if need be
		initVal += mIncrement;
		if(initVal > mMaxValue) {
			initVal = mMinValue;
		}

		// stop if it's filled
		if(initVal == mInitialValue && !firstTry) {
			llwarns << "Whoa! Too many multi slider elements to add one to" << llendl;
			return false;
		}

		firstTry = false;
		continue;
	}

	return true;
}


void LLMultiSlider::deleteSlider(const LLString& name)
{
	// can't delete last slider
	if(mValue.size() <= 0) {
		return;
	}

	// get rid of value from mValue and its thumb rect
	mValue.erase(name);
	mThumbRects.erase(name);

	// set to the last created
	if(mValue.size() > 0) {
		std::map<LLString, LLRect>::iterator mIt = mThumbRects.end();
		mIt--;
		mCurSlider = mIt->first;
	}
}

void LLMultiSlider::clear()
{
	while(mThumbRects.size() > 0) {
		deleteCurSlider();
	}

	LLUICtrl::clear();
}

BOOL LLMultiSlider::handleHover(S32 x, S32 y, MASK mask)
{
	if( gFocusMgr.getMouseCapture() == this )
	{
		S32 left_edge = MULTI_THUMB_WIDTH/2;
		S32 right_edge = getRect().getWidth() - (MULTI_THUMB_WIDTH/2);

		x += mMouseOffset;
		x = llclamp( x, left_edge, right_edge );

		F32 t = F32(x - left_edge) / (right_edge - left_edge);
		setCurSliderValue(t * (mMaxValue - mMinValue) + mMinValue );
		onCommit();

		getWindow()->setCursor(UI_CURSOR_ARROW);
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (active)" << llendl;		
	}
	else
	{
		getWindow()->setCursor(UI_CURSOR_ARROW);
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (inactive)" << llendl;		
	}
	return TRUE;
}

BOOL LLMultiSlider::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;

	if( gFocusMgr.getMouseCapture() == this )
	{
		gFocusMgr.setMouseCapture( NULL );

		if( mMouseUpCallback )
		{
			mMouseUpCallback( this, mCallbackUserData );
		}
		handled = TRUE;
		make_ui_sound("UISndClickRelease");
	}
	else
	{
		handled = TRUE;
	}

	return handled;
}

BOOL LLMultiSlider::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// only do sticky-focus on non-chrome widgets
	if (!getIsChrome())
	{
		setFocus(TRUE);
	}
	if( mMouseDownCallback )
	{
		mMouseDownCallback( this, mCallbackUserData );
	}

	if (MASK_CONTROL & mask) // if CTRL is modifying
	{
		setCurSliderValue(mInitialValue);
		onCommit();
	}
	else
	{
		// scroll through thumbs to see if we have a new one selected and select that one
		std::map<LLString, LLRect>::iterator mIt = mThumbRects.begin();
		for(; mIt != mThumbRects.end(); mIt++) {
			
			// check if inside.  If so, set current slider and continue
			if(mIt->second.pointInRect(x,y)) {
				mCurSlider = mIt->first;
				break;
			}
		}

		// Find the offset of the actual mouse location from the center of the thumb.
		if (mThumbRects[mCurSlider].pointInRect(x,y))
		{
			mMouseOffset = (mThumbRects[mCurSlider].mLeft + MULTI_THUMB_WIDTH/2) - x;
		}
		else
		{
			mMouseOffset = 0;
		}

		// Start dragging the thumb
		// No handler needed for focus lost since this class has no state that depends on it.
		gFocusMgr.setMouseCapture( this );
		mDragStartThumbRect = mThumbRects[mCurSlider];				
	}
	make_ui_sound("UISndClick");

	return TRUE;
}

BOOL	LLMultiSlider::handleKeyHere(KEY key, MASK mask)
{
	BOOL handled = FALSE;
	switch(key)
	{
	case KEY_UP:
	case KEY_DOWN:
		// eat up and down keys to be consistent
		handled = TRUE;
		break;
	case KEY_LEFT:
		setCurSliderValue(getCurSliderValue() - getIncrement());
		onCommit();
		handled = TRUE;
		break;
	case KEY_RIGHT:
		setCurSliderValue(getCurSliderValue() + getIncrement());
		onCommit();
		handled = TRUE;
		break;
	default:
		break;
	}
	return handled;
}

void LLMultiSlider::draw()
{
	LLColor4 curThumbColor;

	std::map<LLString, LLRect>::iterator mIt;
	std::map<LLString, LLRect>::iterator curSldrIt;

	// Draw background and thumb.

	// drawing solids requires texturing be disabled
	LLGLSNoTexture no_texture;

	LLRect rect(mDragStartThumbRect);

	F32 opacity = getEnabled() ? 1.f : 0.3f;

	// Track
	LLUIImagePtr thumb_imagep = LLUI::sImageProvider->getUIImage("rounded_square.tga");

	S32 height_offset = (getRect().getHeight() - MULTI_TRACK_HEIGHT) / 2;
	LLRect track_rect(0, getRect().getHeight() - height_offset, getRect().getWidth(), height_offset );


	if(mDrawTrack)
	{
		track_rect.stretch(-1);
		thumb_imagep->draw(track_rect, mTrackColor % opacity);
	}

	// if we're supposed to use a drawn triangle
	// simple gl call for the triangle
	if(mUseTriangle) {

		for(mIt = mThumbRects.begin(); mIt != mThumbRects.end(); mIt++) {

			gl_triangle_2d(
				mIt->second.mLeft - EXTRA_TRIANGLE_WIDTH, 
				mIt->second.mTop + EXTRA_TRIANGLE_HEIGHT,
				mIt->second.mRight + EXTRA_TRIANGLE_WIDTH, 
				mIt->second.mTop + EXTRA_TRIANGLE_HEIGHT,
				mIt->second.mLeft + mIt->second.getWidth() / 2, 
				mIt->second.mBottom - EXTRA_TRIANGLE_HEIGHT,
				mTriangleColor, TRUE);
		}
	}
	else if (!thumb_imagep)
	{
		// draw all the thumbs
		curSldrIt = mThumbRects.end();
		for(mIt = mThumbRects.begin(); mIt != mThumbRects.end(); mIt++) {
			
			// choose the color
			curThumbColor = mThumbCenterColor;
			if(mIt->first == mCurSlider) {
				
				curSldrIt = mIt;
				continue;
				//curThumbColor = mThumbCenterSelectedColor;
			}

			// the draw command
			gl_rect_2d(mIt->second, curThumbColor, TRUE);
		}

		// now draw the current slider
		if(curSldrIt != mThumbRects.end()) {
			gl_rect_2d(curSldrIt->second, mThumbCenterSelectedColor, TRUE);
		}

		// and draw the drag start
		if (gFocusMgr.getMouseCapture() == this)
		{
			gl_rect_2d(mDragStartThumbRect, mThumbCenterColor % opacity, FALSE);
		}
	}
	else if( gFocusMgr.getMouseCapture() == this )
	{
		// draw drag start
		thumb_imagep->drawSolid(mDragStartThumbRect, mThumbCenterColor % 0.3f);

		// draw the highlight
		if (hasFocus())
		{
			thumb_imagep->drawBorder(mThumbRects[mCurSlider], gFocusMgr.getFocusColor(), gFocusMgr.getFocusFlashWidth());
		}

		// draw the thumbs
		curSldrIt = mThumbRects.end();
		for(mIt = mThumbRects.begin(); mIt != mThumbRects.end(); mIt++) 
		{
			// choose the color
			curThumbColor = mThumbCenterColor;
			if(mIt->first == mCurSlider) 
			{
				// don't draw now, draw last
				curSldrIt = mIt;
				continue;				
			}
			
			// the draw command
			thumb_imagep->drawSolid(mIt->second, curThumbColor);
		}
		
		// draw cur slider last
		if(curSldrIt != mThumbRects.end()) 
		{
			thumb_imagep->drawSolid(curSldrIt->second, mThumbCenterSelectedColor);
		}
		
	}
	else
	{ 
		// draw highlight
		if (hasFocus())
		{
			thumb_imagep->drawBorder(mThumbRects[mCurSlider], gFocusMgr.getFocusColor(), gFocusMgr.getFocusFlashWidth());
		}

		// draw thumbs
		curSldrIt = mThumbRects.end();
		for(mIt = mThumbRects.begin(); mIt != mThumbRects.end(); mIt++) 
		{
			
			// choose the color
			curThumbColor = mThumbCenterColor;
			if(mIt->first == mCurSlider) 
			{
				curSldrIt = mIt;
				continue;
				//curThumbColor = mThumbCenterSelectedColor;
			}				
			
			thumb_imagep->drawSolid(mIt->second, curThumbColor % opacity);
		}

		if(curSldrIt != mThumbRects.end()) 
		{
			thumb_imagep->drawSolid(curSldrIt->second, mThumbCenterSelectedColor % opacity);
		}
	}

	LLUICtrl::draw();
}

// virtual
LLXMLNodePtr LLMultiSlider::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	node->createChild("initial_val", TRUE)->setFloatValue(getInitialValue());
	node->createChild("min_val", TRUE)->setFloatValue(getMinValue());
	node->createChild("max_val", TRUE)->setFloatValue(getMaxValue());
	node->createChild("increment", TRUE)->setFloatValue(getIncrement());

	return node;
}


//static
LLView* LLMultiSlider::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("multi_slider_bar");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	F32 initial_value = 0.f;
	node->getAttributeF32("initial_val", initial_value);

	F32 min_value = 0.f;
	node->getAttributeF32("min_val", min_value);

	F32 max_value = 1.f; 
	node->getAttributeF32("max_val", max_value);

	F32 increment = 0.1f;
	node->getAttributeF32("increment", increment);

	S32 max_sliders = 1;
	node->getAttributeS32("max_sliders", max_sliders);

	BOOL allow_overlap = FALSE;
	node->getAttributeBOOL("allow_overlap", allow_overlap);

	BOOL draw_track = TRUE;
	node->getAttributeBOOL("draw_track", draw_track);

	BOOL use_triangle = FALSE;
	node->getAttributeBOOL("use_triangle", use_triangle);

	LLMultiSlider* multiSlider = new LLMultiSlider(name,
							rect,
							NULL,
							NULL,
							initial_value,
							min_value,
							max_value,
							increment,
							max_sliders,
							allow_overlap,
							draw_track,
							use_triangle);

	multiSlider->initFromXML(node, parent);

	return multiSlider;
}
