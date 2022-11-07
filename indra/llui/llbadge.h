/** 
 * @file llbadge.h
 * @brief Header for badges
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

#ifndef LL_LLBADGE_H
#define LL_LLBADGE_H

#include <string>

#include "lluicolor.h"
#include "lluictrl.h"
#include "llstring.h"
#include "lluiimage.h"
#include "llview.h"

//
// Declarations
//

class LLFontGL;
class LLScrollContainer;
class LLUICtrlFactory;

//
// Relative Position Alignment
//

namespace LLRelPos
{
    enum Location
    {
        CENTER  = 0,

        LEFT    = (1 << 0),
        RIGHT   = (1 << 1),

        TOP     = (1 << 2),
        BOTTOM  = (1 << 3),

        BOTTOM_LEFT     = (BOTTOM | LEFT),
        BOTTOM_RIGHT    = (BOTTOM | RIGHT),

        TOP_LEFT        = (TOP | LEFT),
        TOP_RIGHT       = (TOP | RIGHT),
    };

    inline bool IsBottom(Location relPos)   { return (relPos & BOTTOM) == BOTTOM; }
    inline bool IsCenter(Location relPos)   { return (relPos == CENTER); }
    inline bool IsLeft(Location relPos)     { return (relPos & LEFT) == LEFT; }
    inline bool IsRight(Location relPos)    { return (relPos & RIGHT) == RIGHT; }
    inline bool IsTop(Location relPos)      { return (relPos & TOP) == TOP; }
}

// NOTE: This needs to occur before Optional<LLRelPos::Location> declaration for proper compilation.
namespace LLInitParam
{
    template<>
    struct TypeValues<LLRelPos::Location> : public TypeValuesHelper<LLRelPos::Location>
    {
        static void declareValues();
    };
}

//
// Classes
//

class LLBadge
: public LLUICtrl
{
public:
    struct Params 
    : public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        Optional< LLHandle<LLView> >    owner;  // Mandatory in code but not in xml
        
        Optional< LLUIImage* >          border_image;
        Optional< LLUIColor >           border_color;

        Optional< LLUIImage* >          image;
        Optional< LLUIColor >           image_color;
        
        Optional< std::string >         label;
        Optional< LLUIColor >           label_color;

        Optional< S32 >                 label_offset_horiz;
        Optional< S32 >                 label_offset_vert;

        Optional< LLRelPos::Location >  location;
        Optional< S32 >                 location_offset_hcenter;
        Optional< S32 >                 location_offset_vcenter;
        Optional< U32 >                 location_percent_hcenter;
        Optional< U32 >                 location_percent_vcenter;

        Optional< F32 >                 padding_horiz;
        Optional< F32 >                 padding_vert;
        
        Params();
        
        bool equals(const Params&) const;
    };
    
protected:
    friend class LLUICtrlFactory;
    LLBadge(const Params& p);

public:

    ~LLBadge();

    bool                addToView(LLView * view);

    virtual void        draw();

    const std::string   getLabel() const { return wstring_to_utf8str(mLabel); }
    void                setLabel( const LLStringExplicit& label);

    void                setDrawAtParentTop(bool draw_at_top) { mDrawAtParentTop = draw_at_top;}

private:
    LLPointer< LLUIImage >  mBorderImage;
    LLUIColor               mBorderColor;

    const LLFontGL*         mGLFont;
    
    LLPointer< LLUIImage >  mImage;
    LLUIColor               mImageColor;
    
    LLUIString              mLabel;
    LLUIColor               mLabelColor;

    S32                     mLabelOffsetHoriz;
    S32                     mLabelOffsetVert;

    LLRelPos::Location      mLocation;
    S32                     mLocationOffsetHCenter;
    S32                     mLocationOffsetVCenter;
    F32                     mLocationPercentHCenter;
    F32                     mLocationPercentVCenter;
    
    LLHandle< LLView >      mOwner;

    F32                     mPaddingHoriz;
    F32                     mPaddingVert;

    LLScrollContainer*      mParentScroller;
    bool                    mDrawAtParentTop;
};

// Build time optimization, generate once in .cpp file
#ifndef LLBADGE_CPP
extern template class LLBadge* LLView::getChild<class LLBadge>(const std::string& name, BOOL recurse) const;
#endif

#endif  // LL_LLBADGE_H
