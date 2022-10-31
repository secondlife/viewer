/** 
 * @file llscrollbar.h
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

#ifndef LL_SCROLLBAR_H
#define LL_SCROLLBAR_H

#include "stdtypes.h"
#include "lluictrl.h"
#include "v4color.h"
#include "llbutton.h"

//
// Classes
//
class LLScrollbar
: public LLUICtrl
{
public:

    typedef boost::function<void (S32, LLScrollbar*)> callback_t;
    struct Params 
    :   public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        Mandatory<EOrientation>         orientation;
        Mandatory<S32>                  doc_size;
        Mandatory<S32>                  doc_pos;
        Mandatory<S32>                  page_size;

        Optional<callback_t>            change_callback;
        Optional<S32>                   step_size;
        Optional<S32>                   thickness;

        Optional<LLUIImage*>            thumb_image_vertical,
                                        thumb_image_horizontal,
                                        track_image_horizontal,
                                        track_image_vertical;

        Optional<bool>                  bg_visible;

        Optional<LLUIColor>             track_color,
                                        thumb_color,
                                        bg_color;

        Optional<LLButton::Params>      up_button;
        Optional<LLButton::Params>      down_button;
        Optional<LLButton::Params>      left_button;
        Optional<LLButton::Params>      right_button;

        Params();
    };

protected:
    LLScrollbar (const Params & p);
    friend class LLUICtrlFactory;

public:
    virtual ~LLScrollbar();

    virtual void setValue(const LLSD& value);

    // Overrides from LLView
    virtual BOOL    handleKeyHere(KEY key, MASK mask);
    virtual BOOL    handleMouseDown(S32 x, S32 y, MASK mask);
    virtual BOOL    handleMouseUp(S32 x, S32 y, MASK mask);
    virtual BOOL    handleDoubleClick(S32 x, S32 y, MASK mask);
    virtual BOOL    handleHover(S32 x, S32 y, MASK mask);
    virtual BOOL    handleScrollWheel(S32 x, S32 y, S32 clicks);
    virtual BOOL    handleScrollHWheel(S32 x, S32 y, S32 clicks);
    virtual BOOL    handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop, 
        EDragAndDropType cargo_type, void *cargo_data, EAcceptance *accept, std::string &tooltip_msg);

    virtual void    reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

    virtual void    draw();

    // How long the "document" is.
    void                setDocSize( S32 size );
    S32                 getDocSize() const      { return mDocSize; }

    // How many "lines" the "document" has scrolled.
    // 0 <= DocPos <= DocSize - DocVisibile
    bool                setDocPos( S32 pos, BOOL update_thumb = TRUE );
    S32                 getDocPos() const       { return mDocPos; }

    BOOL                isAtBeginning();
    BOOL                isAtEnd();

    // Setting both at once.
    void                setDocParams( S32 size, S32 pos );

    // How many "lines" of the "document" is can appear on a page.
    void                setPageSize( S32 page_size );
    S32                 getPageSize() const     { return mPageSize; }
    
    // The farthest the document can be scrolled (top of the last page).
    S32                 getDocPosMax() const    { return llmax( 0, mDocSize - mPageSize); }

    void                pageUp(S32 overlap);
    void                pageDown(S32 overlap);

    void                onLineUpBtnPressed(const LLSD& data);
    void                onLineDownBtnPressed(const LLSD& data);

    S32                 getThickness() const { return mThickness; }
    void                setThickness(S32 thickness);

private:
    void                updateThumbRect();
    bool                changeLine(S32 delta, BOOL update_thumb );

    callback_t          mChangeCallback;

    const EOrientation  mOrientation;   
    S32                 mDocSize;       // Size of the document that the scrollbar is modeling.  Units depend on the user.  0 <= mDocSize.
    S32                 mDocPos;        // Position within the doc that the scrollbar is modeling, in "lines" (user size)
    S32                 mPageSize;      // Maximum number of lines that can be seen at one time.
    S32                 mStepSize;
    BOOL                mDocChanged;

    LLRect              mThumbRect;
    S32                 mDragStartX;
    S32                 mDragStartY;
    F32                 mHoverGlowStrength;
    F32                 mCurGlowStrength;

    LLRect              mOrigRect;
    S32                 mLastDelta;

    LLUIColor           mTrackColor;
    LLUIColor           mThumbColor;
    LLUIColor           mBGColor;

    bool                mBGVisible;

    LLUIImagePtr        mThumbImageV;
    LLUIImagePtr        mThumbImageH;
    LLUIImagePtr        mTrackImageV;
    LLUIImagePtr        mTrackImageH;

    S32                 mThickness;
};


#endif  // LL_SCROLLBAR_H
