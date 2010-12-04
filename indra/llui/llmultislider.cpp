/** 
 * @file llmultisldr.cpp
 * @brief LLMultiSlider base class
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "linden_common.h"

#include "llmultislider.h"
#include "llui.h"

#include "llgl.h"
#include "llwindow.h"
#include "llfocusmgr.h"
#include "llkeyboard.h"			// for the MASK constants
#include "llcontrol.h"
#include "lluictrlfactory.h"

#include <sstream>

static LLDefaultChildRegistry::Register<LLMultiSlider> r("multi_slider_bar");

const F32 FLOAT_THRESHOLD = 0.00001f;

S32 LLMultiSlider::mNameCounter = 0;

LLMultiSlider::SliderParams::SliderParams()
:	name("name"),
	value("value", 0.f)
{
}

LLMultiSlider::Params::Params()
:	max_sliders("max_sliders", 1),
	allow_overlap("allow_overlap", false),
	draw_track("draw_track", true),
	use_triangle("use_triangle", false),
	track_color("track_color"),
	thumb_disabled_color("thumb_disabled_color"),
	thumb_outline_color("thumb_outline_color"),
	thumb_center_color("thumb_center_color"),
	thumb_center_selected_color("thumb_center_selected_color"),
	triangle_color("triangle_color"),
	mouse_down_callback("mouse_down_callback"),
	mouse_up_callback("mouse_up_callback"),
	thumb_width("thumb_width"),
	sliders("slider")
{
	name = "multi_slider_bar";
	mouse_opaque(true);
	follows.flags(FOLLOWS_LEFT | FOLLOWS_TOP);
}

LLMultiSlider::LLMultiSlider(const LLMultiSlider::Params& p)
:	LLF32UICtrl(p),
	mMouseOffset( 0 ),
	mDragStartThumbRect( 0, getRect().getHeight(), p.thumb_width, 0 ),
	mMaxNumSliders(p.max_sliders),
	mAllowOverlap(p.allow_overlap),
	mDrawTrack(p.draw_track),
	mUseTriangle(p.use_triangle),
	mTrackColor(p.track_color()),
	mThumbOutlineColor(p.thumb_outline_color()),
	mThumbCenterColor(p.thumb_center_color()),
	mThumbCenterSelectedColor(p.thumb_center_selected_color()),
	mDisabledThumbColor(p.thumb_disabled_color()),
	mTriangleColor(p.triangle_color()),
	mThumbWidth(p.thumb_width),
	mMouseDownSignal(NULL),
	mMouseUpSignal(NULL)
{
	mValue.emptyMap();
	mCurSlider = LLStringUtil::null;
	
	if (p.mouse_down_callback.isProvided())
	{
		setMouseDownCallback(initCommitCallback(p.mouse_down_callback));
	}
	if (p.mouse_up_callback.isProvided())
	{
		setMouseUpCallback(initCommitCallback(p.mouse_up_callback));
	}

	for (LLInitParam::ParamIterator<SliderParams>::const_iterator it = p.sliders.begin();
		it != p.sliders.end();
		++it)
	{
		if (it->name.isProvided())
		{
			addSlider(it->value, it->name);
		}
		else
		{
			addSlider(it->value);
		}
	}
}

LLMultiSlider::~LLMultiSlider()
{
	delete mMouseDownSignal;
	delete mMouseUpSignal;
}


void LLMultiSlider::setSliderValue(const std::string& name, F32 value, BOOL from_event)
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

	S32 left_edge = mThumbWidth/2;
	S32 right_edge = getRect().getWidth() - (mThumbWidth/2);

	S32 x = left_edge + S32( t * (right_edge - left_edge) );
	mThumbRects[name].mLeft = x - (mThumbWidth/2);
	mThumbRects[name].mRight = x + (mThumbWidth/2);
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

F32 LLMultiSlider::getSliderValue(const std::string& name) const
{
	return (F32)mValue[name].asReal();
}

void LLMultiSlider::setCurSlider(const std::string& name)
{
	if(mValue.has(name)) {
		mCurSlider = name;
	}
}

const std::string& LLMultiSlider::addSlider()
{
	return addSlider(mInitialValue);
}

const std::string& LLMultiSlider::addSlider(F32 val)
{
	std::stringstream newName;
	F32 initVal = val;

	if(mValue.size() >= mMaxNumSliders) {
		return LLStringUtil::null;
	}

	// create a new name
	newName << "sldr" << mNameCounter;
	mNameCounter++;

	bool foundOne = findUnusedValue(initVal);
	if(!foundOne) {
		return LLStringUtil::null;
	}

	// add a new thumb rect
	mThumbRects[newName.str()] = LLRect( 0, getRect().getHeight(), mThumbWidth, 0 );

	// add the value and set the current slider to this one
	mValue.insert(newName.str(), initVal);
	mCurSlider = newName.str();

	// move the slider
	setSliderValue(mCurSlider, initVal, TRUE);

	return mCurSlider;
}

void LLMultiSlider::addSlider(F32 val, const std::string& name)
{
	F32 initVal = val;

	if(mValue.size() >= mMaxNumSliders) {
		return;
	}

	bool foundOne = findUnusedValue(initVal);
	if(!foundOne) {
		return;
	}

	// add a new thumb rect
	mThumbRects[name] = LLRect( 0, getRect().getHeight(), mThumbWidth, 0 );

	// add the value and set the current slider to this one
	mValue.insert(name, initVal);
	mCurSlider = name;

	// move the slider
	setSliderValue(mCurSlider, initVal, TRUE);
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


void LLMultiSlider::deleteSlider(const std::string& name)
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
		std::map<std::string, LLRect>::iterator mIt = mThumbRects.end();
		mIt--;
		mCurSlider = mIt->first;
	}
}

void LLMultiSlider::clear()
{
	while(mThumbRects.size() > 0) {
		deleteCurSlider();
	}

	LLF32UICtrl::clear();
}

BOOL LLMultiSlider::handleHover(S32 x, S32 y, MASK mask)
{
	if( gFocusMgr.getMouseCapture() == this )
	{
		S32 left_edge = mThumbWidth/2;
		S32 right_edge = getRect().getWidth() - (mThumbWidth/2);

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

		if (mMouseUpSignal)
			(*mMouseUpSignal)( this, LLSD() );

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
	if (mMouseDownSignal)
		(*mMouseDownSignal)( this, LLSD() );

	if (MASK_CONTROL & mask) // if CTRL is modifying
	{
		setCurSliderValue(mInitialValue);
		onCommit();
	}
	else
	{
		// scroll through thumbs to see if we have a new one selected and select that one
		std::map<std::string, LLRect>::iterator mIt = mThumbRects.begin();
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
			mMouseOffset = (mThumbRects[mCurSlider].mLeft + mThumbWidth/2) - x;
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
	static LLUICachedControl<S32> extra_triangle_height ("UIExtraTriangleHeight", 0);
	static LLUICachedControl<S32> extra_triangle_width ("UIExtraTriangleWidth", 0);
	LLColor4 curThumbColor;

	std::map<std::string, LLRect>::iterator mIt;
	std::map<std::string, LLRect>::iterator curSldrIt;

	// Draw background and thumb.

	// drawing solids requires texturing be disabled
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	LLRect rect(mDragStartThumbRect);

	F32 opacity = getEnabled() ? 1.f : 0.3f;

	// Track
	LLUIImagePtr thumb_imagep = LLUI::getUIImage("Rounded_Square");

	static LLUICachedControl<S32> multi_track_height ("UIMultiTrackHeight", 0);
	S32 height_offset = (getRect().getHeight() - multi_track_height) / 2;
	LLRect track_rect(0, getRect().getHeight() - height_offset, getRect().getWidth(), height_offset );


	if(mDrawTrack)
	{
		track_rect.stretch(-1);
		thumb_imagep->draw(track_rect, mTrackColor.get() % opacity);
	}

	// if we're supposed to use a drawn triangle
	// simple gl call for the triangle
	if(mUseTriangle) {

		for(mIt = mThumbRects.begin(); mIt != mThumbRects.end(); mIt++) {

			gl_triangle_2d(
				mIt->second.mLeft - extra_triangle_width, 
				mIt->second.mTop + extra_triangle_height,
				mIt->second.mRight + extra_triangle_width, 
				mIt->second.mTop + extra_triangle_height,
				mIt->second.mLeft + mIt->second.getWidth() / 2, 
				mIt->second.mBottom - extra_triangle_height,
				mTriangleColor.get(), TRUE);
		}
	}
	else if (!thumb_imagep)
	{
		// draw all the thumbs
		curSldrIt = mThumbRects.end();
		for(mIt = mThumbRects.begin(); mIt != mThumbRects.end(); mIt++) {
			
			// choose the color
			curThumbColor = mThumbCenterColor.get();
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
			gl_rect_2d(curSldrIt->second, mThumbCenterSelectedColor.get(), TRUE);
		}

		// and draw the drag start
		if (gFocusMgr.getMouseCapture() == this)
		{
			gl_rect_2d(mDragStartThumbRect, mThumbCenterColor.get() % opacity, FALSE);
		}
	}
	else if( gFocusMgr.getMouseCapture() == this )
	{
		// draw drag start
		thumb_imagep->drawSolid(mDragStartThumbRect, mThumbCenterColor.get() % 0.3f);

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
			curThumbColor = mThumbCenterColor.get();
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
			thumb_imagep->drawSolid(curSldrIt->second, mThumbCenterSelectedColor.get());
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
			curThumbColor = mThumbCenterColor.get();
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
			thumb_imagep->drawSolid(curSldrIt->second, mThumbCenterSelectedColor.get() % opacity);
		}
	}

	LLF32UICtrl::draw();
}
boost::signals2::connection LLMultiSlider::setMouseDownCallback( const commit_signal_t::slot_type& cb ) 
{ 
	if (!mMouseDownSignal) mMouseDownSignal = new commit_signal_t();
	return mMouseDownSignal->connect(cb); 
}

boost::signals2::connection LLMultiSlider::setMouseUpCallback(	const commit_signal_t::slot_type& cb )   
{ 
	if (!mMouseUpSignal) mMouseUpSignal = new commit_signal_t();
	return mMouseUpSignal->connect(cb); 
}
