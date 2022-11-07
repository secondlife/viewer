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
#include "llkeyboard.h"         // for the MASK constants
#include "llcontrol.h"
#include "lluictrlfactory.h"
#include "lluiimage.h"

#include <sstream>

static LLDefaultChildRegistry::Register<LLMultiSlider> r("multi_slider_bar");

const F32 FLOAT_THRESHOLD = 0.00001f;

S32 LLMultiSlider::mNameCounter = 0;

LLMultiSlider::SliderParams::SliderParams()
:   name("name"),
    value("value", 0.f)
{
}

LLMultiSlider::Params::Params()
:   max_sliders("max_sliders", 1),
    allow_overlap("allow_overlap", false),
    loop_overlap("loop_overlap", false),
    orientation("orientation"),
    overlap_threshold("overlap_threshold", 0),
    draw_track("draw_track", true),
    use_triangle("use_triangle", false),
    track_color("track_color"),
    thumb_disabled_color("thumb_disabled_color"),
    thumb_highlight_color("thumb_highlight_color"),
    thumb_outline_color("thumb_outline_color"),
    thumb_center_color("thumb_center_color"),
    thumb_center_selected_color("thumb_center_selected_color"),
    thumb_image("thumb_image"),
    triangle_color("triangle_color"),
    mouse_down_callback("mouse_down_callback"),
    mouse_up_callback("mouse_up_callback"),
    thumb_width("thumb_width"),
    sliders("slider")
{}

LLMultiSlider::LLMultiSlider(const LLMultiSlider::Params& p)
:   LLF32UICtrl(p),
    mMouseOffset( 0 ),
    mMaxNumSliders(p.max_sliders),
    mAllowOverlap(p.allow_overlap),
    mLoopOverlap(p.loop_overlap),
    mDrawTrack(p.draw_track),
    mUseTriangle(p.use_triangle),
    mTrackColor(p.track_color()),
    mThumbOutlineColor(p.thumb_outline_color()),
    mThumbCenterColor(p.thumb_center_color()),
    mThumbCenterSelectedColor(p.thumb_center_selected_color()),
    mDisabledThumbColor(p.thumb_disabled_color()),
    mTriangleColor(p.triangle_color()),
    mThumbWidth(p.thumb_width),
    mOrientation((p.orientation() == "vertical") ? VERTICAL : HORIZONTAL),
    mMouseDownSignal(NULL),
    mMouseUpSignal(NULL)
{
    mValue.emptyMap();
    mCurSlider = LLStringUtil::null;

    if (mOrientation == HORIZONTAL)
    {
        mDragStartThumbRect = LLRect(0, getRect().getHeight(), p.thumb_width, 0);
    }
    else
    {
        mDragStartThumbRect = LLRect(0, p.thumb_width, getRect().getWidth(), 0);
    }

    if (p.mouse_down_callback.isProvided())
    {
        setMouseDownCallback(initCommitCallback(p.mouse_down_callback));
    }
    if (p.mouse_up_callback.isProvided())
    {
        setMouseUpCallback(initCommitCallback(p.mouse_up_callback));
    }

    if (p.overlap_threshold.isProvided() && p.overlap_threshold > mIncrement)
    {
        mOverlapThreshold = p.overlap_threshold - mIncrement;
    }
    else
    {
        mOverlapThreshold = 0;
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

    mRoundedSquareImgp = LLUI::getUIImage("Rounded_Square");
    if (p.thumb_image.isProvided())
    {
        mThumbImagep = LLUI::getUIImage(p.thumb_image());
    }
    mThumbHighlightColor = p.thumb_highlight_color.isProvided() ? p.thumb_highlight_color() : static_cast<LLUIColor>(gFocusMgr.getFocusColor());
}

LLMultiSlider::~LLMultiSlider()
{
    delete mMouseDownSignal;
    delete mMouseUpSignal;
}

F32 LLMultiSlider::getNearestIncrement(F32 value) const
{
    value = llclamp(value, mMinValue, mMaxValue);

    // Round to nearest increment (bias towards rounding down)
    value -= mMinValue;
    value += mIncrement / 2.0001f;
    value -= fmod(value, mIncrement);
    return mMinValue + value;
}

void LLMultiSlider::setSliderValue(const std::string& name, F32 value, BOOL from_event)
{
    // exit if not there
    if(!mValue.has(name)) {
        return;
    }

    F32 newValue = getNearestIncrement(value);

    // now, make sure no overlap
    // if we want that
    if(!mAllowOverlap) {
        bool hit = false;

        // look at the current spot
        // and see if anything is there
        LLSD::map_iterator mIt = mValue.beginMap();

        // increment is our distance between points, use to eliminate round error
        F32 threshold = mOverlapThreshold + (mIncrement / 4);
        // If loop overlap is enabled, check if we overlap with points 'after' max value (project to lower)
        F32 loop_up_check = (mLoopOverlap && (value + threshold) > mMaxValue) ? (value + threshold - mMaxValue + mMinValue) : mMinValue - 1.0f;
        // If loop overlap is enabled, check if we overlap with points 'before' min value (project to upper)
        F32 loop_down_check = (mLoopOverlap && (value - threshold) < mMinValue) ? (value - threshold - mMinValue + mMaxValue) : mMaxValue + 1.0f;

        for(;mIt != mValue.endMap(); mIt++)
        {
            F32 locationVal = (F32)mIt->second.asReal();
            // Check nearby values
            F32 testVal = locationVal - newValue;
            if (testVal > -threshold
                && testVal < threshold
                && mIt->first != name)
            {
                hit = true;
                break;
            }
            if (mLoopOverlap)
            {
                // Check edge overlap values
                if (locationVal < loop_up_check)
                {
                    hit = true;
                    break;
                }
                if (locationVal > loop_down_check)
                {
                    hit = true;
                    break;
                }
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
    if (mOrientation == HORIZONTAL)
    {
        S32 left_edge = mThumbWidth/2;
        S32 right_edge = getRect().getWidth() - (mThumbWidth/2);

        S32 x = left_edge + S32( t * (right_edge - left_edge) );

        mThumbRects[name].mLeft = x - (mThumbWidth / 2);
        mThumbRects[name].mRight = x + (mThumbWidth / 2);
    }
    else
    {
        S32 bottom_edge = mThumbWidth/2;
        S32 top_edge = getRect().getHeight() - (mThumbWidth/2);

        S32 x = bottom_edge + S32( t * (top_edge - bottom_edge) );

        mThumbRects[name].mTop = x + (mThumbWidth / 2);
        mThumbRects[name].mBottom = x - (mThumbWidth / 2);
    }
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
    if (mValue.has(name))
    {
        return (F32)mValue[name].asReal();
    }
    return 0;
}

void LLMultiSlider::setCurSlider(const std::string& name)
{
    if(mValue.has(name)) {
        mCurSlider = name;
    }
}

F32 LLMultiSlider::getSliderValueFromPos(S32 xpos, S32 ypos) const
{
    F32 t = 0;
    if (mOrientation == HORIZONTAL)
    {
        S32 left_edge = mThumbWidth / 2;
        S32 right_edge = getRect().getWidth() - (mThumbWidth / 2);

        xpos += mMouseOffset;
        xpos = llclamp(xpos, left_edge, right_edge);

        t = F32(xpos - left_edge) / (right_edge - left_edge);
    }
    else
    {
        S32 bottom_edge = mThumbWidth / 2;
        S32 top_edge = getRect().getHeight() - (mThumbWidth / 2);

        ypos += mMouseOffset;
        ypos = llclamp(ypos, bottom_edge, top_edge);

        t = F32(ypos - bottom_edge) / (top_edge - bottom_edge);
    }

    return((t * (mMaxValue - mMinValue)) + mMinValue);
}


LLRect LLMultiSlider::getSliderThumbRect(const std::string& name) const
{
    auto it = mThumbRects.find(name);
    if (it != mThumbRects.end())
        return (*it).second;
    return LLRect();
}

void LLMultiSlider::setSliderThumbImage(const std::string &name)
{
    if (!name.empty())
    {
        mThumbImagep = LLUI::getUIImage(name);
    }
    else
        clearSliderThumbImage();
}

void LLMultiSlider::clearSliderThumbImage()
{
    mThumbImagep = NULL;
}

void LLMultiSlider::resetCurSlider()
{
    mCurSlider = LLStringUtil::null;
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
    if (mOrientation == HORIZONTAL)
    {
        mThumbRects[newName.str()] = LLRect(0, getRect().getHeight(), mThumbWidth, 0);
    }
    else
    {
        mThumbRects[newName.str()] = LLRect(0, mThumbWidth, getRect().getWidth(), 0);
    }

    // add the value and set the current slider to this one
    mValue.insert(newName.str(), initVal);
    mCurSlider = newName.str();

    // move the slider
    setSliderValue(mCurSlider, initVal, TRUE);

    return mCurSlider;
}

bool LLMultiSlider::addSlider(F32 val, const std::string& name)
{
    F32 initVal = val;

    if(mValue.size() >= mMaxNumSliders) {
        return false;
    }

    bool foundOne = findUnusedValue(initVal);
    if(!foundOne) {
        return false;
    }

    // add a new thumb rect
    if (mOrientation == HORIZONTAL)
    {
        mThumbRects[name] = LLRect(0, getRect().getHeight(), mThumbWidth, 0);
    }
    else
    {
        mThumbRects[name] = LLRect(0, mThumbWidth, getRect().getWidth(), 0);
    }

    // add the value and set the current slider to this one
    mValue.insert(name, initVal);
    mCurSlider = name;

    // move the slider
    setSliderValue(mCurSlider, initVal, TRUE);

    return true;
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
        F32 threshold = mAllowOverlap ? FLOAT_THRESHOLD : mOverlapThreshold + (mIncrement / 4);
        LLSD::map_iterator mIt = mValue.beginMap();
        for(;mIt != mValue.endMap(); mIt++) {
            
            F32 testVal = (F32)mIt->second.asReal() - initVal;
            if(testVal > -threshold && testVal < threshold)
            {
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
            LL_WARNS() << "Whoa! Too many multi slider elements to add one to" << LL_ENDL;
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
    while(mThumbRects.size() > 0 && mValue.size() > 0) {
        deleteCurSlider();
    }

    if (mThumbRects.size() > 0 || mValue.size() > 0)
    {
        LL_WARNS() << "Failed to fully clear Multi slider" << LL_ENDL;
    }

    LLF32UICtrl::clear();
}

BOOL LLMultiSlider::handleHover(S32 x, S32 y, MASK mask)
{
    if( gFocusMgr.getMouseCapture() == this )
    {
        setCurSliderValue(getSliderValueFromPos(x, y));
        onCommit();

        getWindow()->setCursor(UI_CURSOR_ARROW);
        LL_DEBUGS("UserInput") << "hover handled by " << getName() << " (active)" << LL_ENDL;       
    }
    else
    {
        if (getEnabled())
        {
            mHoverSlider.clear();
            std::map<std::string, LLRect>::iterator  mIt = mThumbRects.begin();
            for (; mIt != mThumbRects.end(); mIt++)
            {
                if (mIt->second.pointInRect(x, y))
                {
                    mHoverSlider = mIt->first;
                    break;
                }
            }
        }
        else
        {
            mHoverSlider.clear();
        }

        getWindow()->setCursor(UI_CURSOR_ARROW);
        LL_DEBUGS("UserInput") << "hover handled by " << getName() << " (inactive)" << LL_ENDL;     
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

        if (!mCurSlider.empty())
        {
            // Find the offset of the actual mouse location from the center of the thumb.
            if (mThumbRects[mCurSlider].pointInRect(x,y))
            {
                if (mOrientation == HORIZONTAL)
                {
                    mMouseOffset = (mThumbRects[mCurSlider].mLeft + mThumbWidth / 2) - x;
                }
                else
                {
                    mMouseOffset = (mThumbRects[mCurSlider].mBottom + mThumbWidth / 2) - y;
                }
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
    }
    make_ui_sound("UISndClick");

    return TRUE;
}

BOOL    LLMultiSlider::handleKeyHere(KEY key, MASK mask)
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

/*virtual*/
void LLMultiSlider::onMouseLeave(S32 x, S32 y, MASK mask)
{
    mHoverSlider.clear();
    LLF32UICtrl::onMouseLeave(x, y, mask);
}

void LLMultiSlider::draw()
{
    static LLUICachedControl<S32> extra_triangle_height ("UIExtraTriangleHeight", 0);
    static LLUICachedControl<S32> extra_triangle_width ("UIExtraTriangleWidth", 0);
    LLColor4 curThumbColor;

    std::map<std::string, LLRect>::iterator mIt;
    std::map<std::string, LLRect>::iterator curSldrIt;
    std::map<std::string, LLRect>::iterator hoverSldrIt;

    // Draw background and thumb.

    // drawing solids requires texturing be disabled
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

    LLRect rect(mDragStartThumbRect);

    F32 opacity = getEnabled() ? 1.f : 0.3f;

    // Track
    static LLUICachedControl<S32> multi_track_height_width ("UIMultiTrackHeight", 0);
    S32 height_offset = 0;
    S32 width_offset = 0;
    if (mOrientation == HORIZONTAL)
    {
        height_offset = (getRect().getHeight() - multi_track_height_width) / 2;
    }
    else
    {
        width_offset = (getRect().getWidth() - multi_track_height_width) / 2;
    }
    LLRect track_rect(width_offset, getRect().getHeight() - height_offset, getRect().getWidth() - width_offset, height_offset);


    if(mDrawTrack)
    {
        track_rect.stretch(-1);
        mRoundedSquareImgp->draw(track_rect, mTrackColor.get() % opacity);
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
                mTriangleColor.get() % opacity, TRUE);
        }
    }
    else if (!mRoundedSquareImgp && !mThumbImagep)
    {
        // draw all the thumbs
        curSldrIt = mThumbRects.end();
        hoverSldrIt = mThumbRects.end();
        for(mIt = mThumbRects.begin(); mIt != mThumbRects.end(); mIt++) {
            
            // choose the color
            curThumbColor = mThumbCenterColor.get();
            if(mIt->first == mCurSlider) {
                
                curSldrIt = mIt;
                continue;
            }
            if (mIt->first == mHoverSlider && getEnabled() && gFocusMgr.getMouseCapture() != this)
            {
                // draw last, after current one
                hoverSldrIt = mIt;
                continue;
            }

            // the draw command
            gl_rect_2d(mIt->second, curThumbColor, TRUE);
        }

        // now draw the current and hover sliders
        if(curSldrIt != mThumbRects.end())
        {
            gl_rect_2d(curSldrIt->second, mThumbCenterSelectedColor.get(), TRUE);
        }

        // and draw the drag start
        if (gFocusMgr.getMouseCapture() == this)
        {
            gl_rect_2d(mDragStartThumbRect, mThumbCenterColor.get() % opacity, FALSE);
        }
        else if (hoverSldrIt != mThumbRects.end())
        {
            gl_rect_2d(hoverSldrIt->second, mThumbCenterSelectedColor.get(), TRUE);
        }
    }
    else
    {
        LLMouseHandler* capture = gFocusMgr.getMouseCapture();
        if (capture == this)
        {
            // draw drag start (ghost)
            if (mThumbImagep)
            {
                mThumbImagep->draw(mDragStartThumbRect, mThumbCenterColor.get() % 0.3f);
            }
            else
            {
                mRoundedSquareImgp->drawSolid(mDragStartThumbRect, mThumbCenterColor.get() % 0.3f);
            }
        }

        // draw the highlight
        if (hasFocus())
        {
            if (!mCurSlider.empty())
            {
                if (mThumbImagep)
                {
                    mThumbImagep->drawBorder(mThumbRects[mCurSlider], mThumbHighlightColor, gFocusMgr.getFocusFlashWidth());
                }
                else
                {
                    mRoundedSquareImgp->drawBorder(mThumbRects[mCurSlider], gFocusMgr.getFocusColor(), gFocusMgr.getFocusFlashWidth());
                }
            }
        }
        if (!mHoverSlider.empty())
        {
            if (mThumbImagep)
            {
                mThumbImagep->drawBorder(mThumbRects[mHoverSlider], mThumbHighlightColor, gFocusMgr.getFocusFlashWidth());
            }
            else
            {
                mRoundedSquareImgp->drawBorder(mThumbRects[mHoverSlider], gFocusMgr.getFocusColor(), gFocusMgr.getFocusFlashWidth());
            }
        }

        // draw the thumbs
        curSldrIt = mThumbRects.end();
        hoverSldrIt = mThumbRects.end();
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
            if (mIt->first == mHoverSlider && getEnabled() && gFocusMgr.getMouseCapture() != this) 
            {
                // don't draw now, draw last, after current one
                hoverSldrIt = mIt;
                continue;
            }
            
            // the draw command
            if (mThumbImagep)
            {
                if (getEnabled())
                {
                    mThumbImagep->draw(mIt->second);
                }
                else
                {
                    mThumbImagep->draw(mIt->second, LLColor4::grey % 0.8f);
                }
            }
            else if (capture == this)
            {
                mRoundedSquareImgp->drawSolid(mIt->second, curThumbColor);
            }
            else
            {
                mRoundedSquareImgp->drawSolid(mIt->second, curThumbColor % opacity);
            }
        }
        
        // draw cur and hover slider last
        if(curSldrIt != mThumbRects.end()) 
        {
            if (mThumbImagep)
            {
                if (getEnabled())
                {
                    mThumbImagep->draw(curSldrIt->second);
                }
                else
                {
                    mThumbImagep->draw(curSldrIt->second, LLColor4::grey % 0.8f);
                }
            }
            else if (capture == this)
            {
                mRoundedSquareImgp->drawSolid(curSldrIt->second, mThumbCenterSelectedColor.get());
            }
            else
            {
                mRoundedSquareImgp->drawSolid(curSldrIt->second, mThumbCenterSelectedColor.get() % opacity);
            }
        }
        if(hoverSldrIt != mThumbRects.end()) 
        {
            if (mThumbImagep)
            {
                mThumbImagep->draw(hoverSldrIt->second);
            }
            else
            {
                mRoundedSquareImgp->drawSolid(hoverSldrIt->second, mThumbCenterSelectedColor.get());
            }
        }
    }

    LLF32UICtrl::draw();
}
boost::signals2::connection LLMultiSlider::setMouseDownCallback( const commit_signal_t::slot_type& cb ) 
{ 
    if (!mMouseDownSignal) mMouseDownSignal = new commit_signal_t();
    return mMouseDownSignal->connect(cb); 
}

boost::signals2::connection LLMultiSlider::setMouseUpCallback(  const commit_signal_t::slot_type& cb )   
{ 
    if (!mMouseUpSignal) mMouseUpSignal = new commit_signal_t();
    return mMouseUpSignal->connect(cb); 
}
