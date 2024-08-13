/**
 * @file llscrollbar.cpp
 * @brief Scrollbar UI widget
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

#include "linden_common.h"

#include "llscrollbar.h"

#include "llmath.h"
#include "lltimer.h"
#include "v3color.h"

#include "llbutton.h"
#include "llcriticaldamp.h"
#include "llkeyboard.h"
#include "llui.h"
#include "llfocusmgr.h"
#include "llwindow.h"
#include "llcontrol.h"
#include "llrender.h"
#include "lluictrlfactory.h"

static LLDefaultChildRegistry::Register<LLScrollbar> register_scrollbar("scroll_bar");

LLScrollbar::Params::Params()
:   orientation ("orientation", HORIZONTAL),
    doc_size ("doc_size", 0),
    doc_pos ("doc_pos", 0),
    page_size ("page_size", 0),
    step_size ("step_size", 1),
    thumb_image_vertical("thumb_image_vertical"),
    thumb_image_horizontal("thumb_image_horizontal"),
    track_image_vertical("track_image_vertical"),
    track_image_horizontal("track_image_horizontal"),
    track_color("track_color"),
    thumb_color("thumb_color"),
    thickness("thickness"),
    up_button("up_button"),
    down_button("down_button"),
    left_button("left_button"),
    right_button("right_button"),
    bg_visible("bg_visible", false),
    bg_color("bg_color", LLColor4::black)
{}

LLScrollbar::LLScrollbar(const Params & p)
:       LLUICtrl(p),
        mChangeCallback( p.change_callback() ),
        mOrientation( p.orientation ),
        mDocSize( p.doc_size ),
        mDocPos( p.doc_pos ),
        mPageSize( p.page_size ),
        mStepSize( p.step_size ),
        mDocChanged(false),
        mDragStartX( 0 ),
        mDragStartY( 0 ),
        mHoverGlowStrength(0.15f),
        mCurGlowStrength(0.f),
        mTrackColor( p.track_color() ),
        mThumbColor ( p.thumb_color() ),
        mThumbImageV(p.thumb_image_vertical),
        mThumbImageH(p.thumb_image_horizontal),
        mTrackImageV(p.track_image_vertical),
        mTrackImageH(p.track_image_horizontal),
        mThickness(p.thickness.isProvided() ? p.thickness : LLUI::getInstance()->mSettingGroups["config"]->getS32("UIScrollbarSize")),
        mBGVisible(p.bg_visible),
        mBGColor(p.bg_color)
{
    updateThumbRect();

    // Page up and page down buttons
    LLRect line_up_rect;
    LLRect line_down_rect;

    if( VERTICAL == mOrientation )
    {
        line_up_rect.setLeftTopAndSize( 0, getRect().getHeight(), mThickness, mThickness );
        line_down_rect.setOriginAndSize( 0, 0, mThickness, mThickness );
    }
    else // HORIZONTAL
    {
        line_up_rect.setOriginAndSize( 0, 0, mThickness, mThickness );
        line_down_rect.setOriginAndSize( getRect().getWidth() - mThickness, 0, mThickness, mThickness );
    }

    LLButton::Params up_btn(mOrientation == VERTICAL ? p.up_button : p.left_button);
    up_btn.name(std::string("Line Up"));
    up_btn.rect(line_up_rect);
    up_btn.click_callback.function(boost::bind(&LLScrollbar::onLineUpBtnPressed, this, _2));
    up_btn.mouse_held_callback.function(boost::bind(&LLScrollbar::onLineUpBtnPressed, this, _2));
    up_btn.tab_stop(false);
    up_btn.follows.flags = (mOrientation == VERTICAL ? (FOLLOWS_RIGHT | FOLLOWS_TOP) : (FOLLOWS_LEFT | FOLLOWS_BOTTOM));

    mLineUpBtn = LLUICtrlFactory::create<LLButton>(up_btn);
    addChild(mLineUpBtn);

    LLButton::Params down_btn(mOrientation == VERTICAL ? p.down_button : p.right_button);
    down_btn.name(std::string("Line Down"));
    down_btn.rect(line_down_rect);
    down_btn.follows.flags(FOLLOWS_RIGHT|FOLLOWS_BOTTOM);
    down_btn.click_callback.function(boost::bind(&LLScrollbar::onLineDownBtnPressed, this, _2));
    down_btn.mouse_held_callback.function(boost::bind(&LLScrollbar::onLineDownBtnPressed, this, _2));
    down_btn.tab_stop(false);

    mLineDownBtn = LLUICtrlFactory::create<LLButton>(down_btn);
    addChild(mLineDownBtn);
}


LLScrollbar::~LLScrollbar()
{
    // Children buttons killed by parent class
}

void LLScrollbar::setDocParams( S32 size, S32 pos )
{
    mDocSize = size;
    setDocPos(pos);
    mDocChanged = true;

    updateThumbRect();
}

// returns true if document position really changed
bool LLScrollbar::setDocPos(S32 pos, bool update_thumb)
{
    pos = llclamp(pos, 0, getDocPosMax());
    if (pos != mDocPos)
    {
        mDocPos = pos;
        mDocChanged = true;

        if( mChangeCallback )
        {
            mChangeCallback( mDocPos, this );
        }

        if( update_thumb )
        {
            updateThumbRect();
        }
        return true;
    }
    return false;
}

void LLScrollbar::setDocSize(S32 size)
{
    if (size != mDocSize)
    {
        mDocSize = size;
        setDocPos(mDocPos);
        mDocChanged = true;

        updateThumbRect();
    }
}

void LLScrollbar::setPageSize( S32 page_size )
{
    if (page_size != mPageSize)
    {
        mPageSize = page_size;
        setDocPos(mDocPos);
        mDocChanged = true;

        updateThumbRect();
    }
}

bool LLScrollbar::isAtBeginning() const
{
    return mDocPos == 0;
}

bool LLScrollbar::isAtEnd() const
{
    return mDocPos == getDocPosMax();
}


void LLScrollbar::updateThumbRect()
{
//  llassert( 0 <= mDocSize );
//  llassert( 0 <= mDocPos && mDocPos <= getDocPosMax() );

    const S32 THUMB_MIN_LENGTH = 16;

    S32 window_length = (mOrientation == LLScrollbar::HORIZONTAL) ? getRect().getWidth() : getRect().getHeight();
    S32 thumb_bg_length = llmax(0, window_length - 2 * mThickness);
    S32 visible_lines = llmin( mDocSize, mPageSize );
    S32 thumb_length = mDocSize ? llmin(llmax( visible_lines * thumb_bg_length / mDocSize, THUMB_MIN_LENGTH), thumb_bg_length) : thumb_bg_length;

    S32 variable_lines = mDocSize - visible_lines;

    if( mOrientation == LLScrollbar::VERTICAL )
    {
        S32 thumb_start_max = thumb_bg_length + mThickness;
        S32 thumb_start_min = mThickness + THUMB_MIN_LENGTH;
        S32 thumb_start = variable_lines ? llmin( llmax(thumb_start_max - (mDocPos * (thumb_bg_length - thumb_length)) / variable_lines, thumb_start_min), thumb_start_max ) : thumb_start_max;

        mThumbRect.mLeft =  0;
        mThumbRect.mTop = thumb_start;
        mThumbRect.mRight = mThickness;
        mThumbRect.mBottom = thumb_start - thumb_length;
    }
    else
    {
        // Horizontal
        S32 thumb_start_max = thumb_bg_length + mThickness - thumb_length;
        S32 thumb_start_min = mThickness;
        S32 thumb_start = variable_lines ? llmin(llmax( thumb_start_min + (mDocPos * (thumb_bg_length - thumb_length)) / variable_lines, thumb_start_min), thumb_start_max ) : thumb_start_min;

        mThumbRect.mLeft = thumb_start;
        mThumbRect.mTop = mThickness;
        mThumbRect.mRight = thumb_start + thumb_length;
        mThumbRect.mBottom = 0;
    }
}

bool LLScrollbar::handleMouseDown(S32 x, S32 y, MASK mask)
{
    // Check children first
    bool handled_by_child = LLView::childrenHandleMouseDown(x, y, mask) != NULL;
    if( !handled_by_child )
    {
        if( mThumbRect.pointInRect(x,y) )
        {
            // Start dragging the thumb
            // No handler needed for focus lost since this clas has no state that depends on it.
            gFocusMgr.setMouseCapture( this );
            mDragStartX = x;
            mDragStartY = y;
            mOrigRect.mTop = mThumbRect.mTop;
            mOrigRect.mBottom = mThumbRect.mBottom;
            mOrigRect.mLeft = mThumbRect.mLeft;
            mOrigRect.mRight = mThumbRect.mRight;
            mLastDelta = 0;
        }
        else
        {
            if(
                ( (LLScrollbar::VERTICAL == mOrientation) && (mThumbRect.mTop < y) ) ||
                ( (LLScrollbar::HORIZONTAL == mOrientation) && (x < mThumbRect.mLeft) )
            )
            {
                // Page up
                pageUp(0);
            }
            else
            if(
                ( (LLScrollbar::VERTICAL == mOrientation) && (y < mThumbRect.mBottom) ) ||
                ( (LLScrollbar::HORIZONTAL == mOrientation) && (mThumbRect.mRight < x) )
            )
            {
                // Page down
                pageDown(0);
            }
        }
    }

    return true;
}


bool LLScrollbar::handleHover(S32 x, S32 y, MASK mask)
{
    // Note: we don't bother sending the event to the children (the arrow buttons)
    // because they'll capture the mouse whenever they need hover events.

    bool handled = false;
    if( hasMouseCapture() )
    {
        S32 height = getRect().getHeight();
        S32 width = getRect().getWidth();

        if( VERTICAL == mOrientation )
        {
//          S32 old_pos = mThumbRect.mTop;

            S32 delta_pixels = y - mDragStartY;
            if( mOrigRect.mBottom + delta_pixels < mThickness )
            {
                delta_pixels = mThickness - mOrigRect.mBottom - 1;
            }
            else
            if( mOrigRect.mTop + delta_pixels > height - mThickness )
            {
                delta_pixels = height - mThickness - mOrigRect.mTop + 1;
            }

            mThumbRect.mTop = mOrigRect.mTop + delta_pixels;
            mThumbRect.mBottom = mOrigRect.mBottom + delta_pixels;

            S32 thumb_length = mThumbRect.getHeight();
            S32 thumb_track_length = height - 2 * mThickness;


            if( delta_pixels != mLastDelta || mDocChanged)
            {
                // Note: delta_pixels increases as you go up.  mDocPos increases down (line 0 is at the top of the page).
                S32 usable_track_length = thumb_track_length - thumb_length;
                if( 0 < usable_track_length )
                {
                    S32 variable_lines = getDocPosMax();
                    S32 pos = mThumbRect.mTop;
                    F32 ratio = F32(pos - mThickness - thumb_length) / usable_track_length;

                    S32 new_pos = llclamp( S32(variable_lines - ratio * variable_lines + 0.5f), 0, variable_lines );
                    // Note: we do not call updateThumbRect() here.  Instead we let the thumb and the document go slightly
                    // out of sync (less than a line's worth) to make the thumb feel responsive.
                    changeLine( new_pos - mDocPos, false );
                }
            }

            mLastDelta = delta_pixels;

        }
        else
        {
            // Horizontal
//          S32 old_pos = mThumbRect.mLeft;

            S32 delta_pixels = x - mDragStartX;

            if( mOrigRect.mLeft + delta_pixels < mThickness )
            {
                delta_pixels = mThickness - mOrigRect.mLeft - 1;
            }
            else
            if( mOrigRect.mRight + delta_pixels > width - mThickness )
            {
                delta_pixels = width - mThickness - mOrigRect.mRight + 1;
            }

            mThumbRect.mLeft = mOrigRect.mLeft + delta_pixels;
            mThumbRect.mRight = mOrigRect.mRight + delta_pixels;

            S32 thumb_length = mThumbRect.getWidth();
            S32 thumb_track_length = width - 2 * mThickness;

            if( delta_pixels != mLastDelta || mDocChanged)
            {
                // Note: delta_pixels increases as you go up.  mDocPos increases down (line 0 is at the top of the page).
                S32 usable_track_length = thumb_track_length - thumb_length;
                if( 0 < usable_track_length )
                {
                    S32 variable_lines = getDocPosMax();
                    S32 pos = mThumbRect.mLeft;
                    F32 ratio = F32(pos - mThickness) / usable_track_length;

                    S32 new_pos = llclamp( S32(ratio * variable_lines + 0.5f), 0, variable_lines);

                    // Note: we do not call updateThumbRect() here.  Instead we let the thumb and the document go slightly
                    // out of sync (less than a line's worth) to make the thumb feel responsive.
                    changeLine( new_pos - mDocPos, false );
                }
            }

            mLastDelta = delta_pixels;
        }

        getWindow()->setCursor(UI_CURSOR_ARROW);
        LL_DEBUGS("UserInput") << "hover handled by " << getName() << " (active)" << LL_ENDL;
        handled = true;
    }
    else
    {
        handled = childrenHandleHover( x, y, mask ) != NULL;
    }

    // Opaque
    if( !handled )
    {
        getWindow()->setCursor(UI_CURSOR_ARROW);
        LL_DEBUGS("UserInput") << "hover handled by " << getName() << " (inactive)"  << LL_ENDL;
        handled = true;
    }

    mDocChanged = false;
    return handled;
} // end handleHover


bool LLScrollbar::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
    bool handled = changeLine( clicks * mStepSize, true );
    return handled;
}

bool LLScrollbar::handleScrollHWheel(S32 x, S32 y, S32 clicks)
{
    bool handled = false;
    if (LLScrollbar::HORIZONTAL == mOrientation)
    {
        handled = changeLine(clicks * mStepSize, true);
    }
    return handled;
}

bool LLScrollbar::handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
                                    EDragAndDropType cargo_type, void *cargo_data, EAcceptance *accept, std::string &tooltip_msg)
{
    // enable this to get drag and drop to control scrollbars
    //if (!drop)
    //{
    //  //TODO: refactor this
    //  S32 variable_lines = getDocPosMax();
    //  S32 pos = (VERTICAL == mOrientation) ? y : x;
    //  S32 thumb_length = (VERTICAL == mOrientation) ? mThumbRect.getHeight() : mThumbRect.getWidth();
    //  S32 thumb_track_length = (VERTICAL == mOrientation) ? (getRect().getHeight() - 2 * SCROLLBAR_SIZE) : (getRect().getWidth() - 2 * SCROLLBAR_SIZE);
    //  S32 usable_track_length = thumb_track_length - thumb_length;
    //  F32 ratio = (VERTICAL == mOrientation) ? F32(pos - SCROLLBAR_SIZE - thumb_length) / usable_track_length
    //      : F32(pos - SCROLLBAR_SIZE) / usable_track_length;
    //  S32 new_pos = (VERTICAL == mOrientation) ? llclamp( S32(variable_lines - ratio * variable_lines + 0.5f), 0, variable_lines )
    //      : llclamp( S32(ratio * variable_lines + 0.5f), 0, variable_lines );
    //  changeLine( new_pos - mDocPos, true );
    //}
    //return true;
    return false;
}

bool LLScrollbar::handleMouseUp(S32 x, S32 y, MASK mask)
{
    bool handled = false;
    if( hasMouseCapture() )
    {
        gFocusMgr.setMouseCapture( NULL );
        handled = true;
    }
    else
    {
        // Opaque, so don't just check children
        handled = LLView::handleMouseUp( x, y, mask );
    }

    return handled;
}

bool LLScrollbar::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    // just treat a double click as a second click
    return handleMouseDown(x, y, mask);
}


void LLScrollbar::reshape(S32 width, S32 height, bool called_from_parent)
{
    if (width == getRect().getWidth() && height == getRect().getHeight()) return;
    LLView::reshape( width, height, called_from_parent );

    if (mOrientation == VERTICAL)
    {
        mLineUpBtn->reshape(mLineUpBtn->getRect().getWidth(), llmin(getRect().getHeight() / 2, mThickness));
        mLineDownBtn->reshape(mLineDownBtn->getRect().getWidth(), llmin(getRect().getHeight() / 2, mThickness));
        mLineUpBtn->setOrigin(0, getRect().getHeight() - mLineUpBtn->getRect().getHeight());
        mLineDownBtn->setOrigin(0, 0);
    }
    else
    {
        mLineUpBtn->reshape(llmin(getRect().getWidth() / 2, mThickness), mLineUpBtn->getRect().getHeight());
        mLineDownBtn->reshape(llmin(getRect().getWidth() / 2, mThickness), mLineDownBtn->getRect().getHeight());
        mLineUpBtn->setOrigin(0, 0);
        mLineDownBtn->setOrigin(getRect().getWidth() - mLineDownBtn->getRect().getWidth(), 0);
    }
    updateThumbRect();
}


void LLScrollbar::draw()
{
    if (!getRect().isValid()) return;

    if(mBGVisible)
    {
        gl_rect_2d(getLocalRect(), mBGColor.get(), true);
    }

    S32 local_mouse_x;
    S32 local_mouse_y;
    LLUI::getInstance()->getMousePositionLocal(this, &local_mouse_x, &local_mouse_y);
    bool other_captor = gFocusMgr.getMouseCapture() && gFocusMgr.getMouseCapture() != this;
    bool hovered = getEnabled() && !other_captor && (hasMouseCapture() || mThumbRect.pointInRect(local_mouse_x, local_mouse_y));
    if (hovered)
    {
        mCurGlowStrength = lerp(mCurGlowStrength, mHoverGlowStrength, LLSmoothInterpolation::getInterpolant(0.05f));
    }
    else
    {
        mCurGlowStrength = lerp(mCurGlowStrength, 0.f, LLSmoothInterpolation::getInterpolant(0.05f));
    }

    // Draw background and thumb.
    if (   ( mOrientation == VERTICAL&&(mThumbImageV.isNull() || mThumbImageH.isNull()) )
        || (mOrientation == HORIZONTAL&&(mTrackImageH.isNull() || mTrackImageV.isNull()) ))
    {
        gl_rect_2d(mOrientation == HORIZONTAL ? mThickness : 0,
        mOrientation == VERTICAL ? getRect().getHeight() - 2 * mThickness : getRect().getHeight(),
        mOrientation == HORIZONTAL ? getRect().getWidth() - 2 * mThickness : getRect().getWidth(),
        mOrientation == VERTICAL ? mThickness : 0, mTrackColor.get(), true);

        gl_rect_2d(mThumbRect, mThumbColor.get(), true);

    }
    else
    {
        // Thumb
        LLRect outline_rect = mThumbRect;
        outline_rect.stretch(2);
        // Background

        if(mOrientation == HORIZONTAL)
        {
            mTrackImageH->drawSolid(mThickness                              //S32 x
                                   , 0                                      //S32 y
                                   , getRect().getWidth() - 2 * mThickness  //S32 width
                                   , getRect().getHeight()                  //S32 height
                                   , mTrackColor.get());                    //const LLColor4& color

            if (gFocusMgr.getKeyboardFocus() == this)
            {
                mTrackImageH->draw(outline_rect, gFocusMgr.getFocusColor());
            }

            mThumbImageH->draw(mThumbRect, mThumbColor.get());
            if (mCurGlowStrength > 0.01f)
            {
                gGL.setSceneBlendType(LLRender::BT_ADD_WITH_ALPHA);
                mThumbImageH->drawSolid(mThumbRect, LLColor4(1.f, 1.f, 1.f, mCurGlowStrength));
                gGL.setSceneBlendType(LLRender::BT_ALPHA);
            }

        }
        else if(mOrientation == VERTICAL)
        {
            mTrackImageV->drawSolid(  0                                     //S32 x
                                   , mThickness                             //S32 y
                                   , getRect().getWidth()                   //S32 width
                                   , getRect().getHeight() - 2 * mThickness //S32 height
                                   , mTrackColor.get());                    //const LLColor4& color
            if (gFocusMgr.getKeyboardFocus() == this)
            {
                mTrackImageV->draw(outline_rect, gFocusMgr.getFocusColor());
            }

            mThumbImageV->draw(mThumbRect, mThumbColor.get());
            if (mCurGlowStrength > 0.01f)
            {
                gGL.setSceneBlendType(LLRender::BT_ADD_WITH_ALPHA);
                mThumbImageV->drawSolid(mThumbRect, LLColor4(1.f, 1.f, 1.f, mCurGlowStrength));
                gGL.setSceneBlendType(LLRender::BT_ALPHA);
            }
        }
    }

    // Draw children
    LLView::draw();
} // end draw


bool LLScrollbar::changeLine( S32 delta, bool update_thumb )
{
    return setDocPos(mDocPos + delta, update_thumb);
}

void LLScrollbar::setValue(const LLSD& value)
{
    setDocPos((S32) value.asInteger());
}


bool LLScrollbar::handleKeyHere(KEY key, MASK mask)
{
    if (getDocPosMax() == 0 && !getVisible())
    {
        return false;
    }

    bool handled = false;

    switch( key )
    {
    case KEY_HOME:
        setDocPos( 0 );
        handled = true;
        break;

    case KEY_END:
        setDocPos( getDocPosMax() );
        handled = true;
        break;

    case KEY_DOWN:
        setDocPos( getDocPos() + mStepSize );
        handled = true;
        break;

    case KEY_UP:
        setDocPos( getDocPos() - mStepSize );
        handled = true;
        break;

    case KEY_PAGE_DOWN:
        pageDown(1);
        break;

    case KEY_PAGE_UP:
        pageUp(1);
        break;
    }

    return handled;
}

void LLScrollbar::pageUp(S32 overlap)
{
    if (mDocSize > mPageSize)
    {
        changeLine( -(mPageSize - overlap), true );
    }
}

void LLScrollbar::pageDown(S32 overlap)
{
    if (mDocSize > mPageSize)
    {
        changeLine( mPageSize - overlap, true );
    }
}

void LLScrollbar::onLineUpBtnPressed( const LLSD& data )
{
    changeLine( -mStepSize, true );
}

void LLScrollbar::onLineDownBtnPressed( const LLSD& data )
{
    changeLine( mStepSize, true );
}

void LLScrollbar::setThickness(S32 thickness)
{
    mThickness = thickness < 0 ? LLUI::getInstance()->mSettingGroups["config"]->getS32("UIScrollbarSize") : thickness;
}
