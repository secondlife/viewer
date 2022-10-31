/** 
 * @file llscrolllistitem.h
 * @brief Scroll lists are composed of rows (items), each of which 
 * contains columns (cells).
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

#ifndef LLSCROLLLISTITEM_H
#define LLSCROLLLISTITEM_H

#include "llpointer.h"      // LLPointer<>
#include "llsd.h"
#include "v4color.h"
#include "llinitparam.h"
#include "llscrolllistcell.h"
#include "llcoord.h"

#include <vector>

class LLCheckBoxCtrl;
class LLResizeBar;
class LLScrollListCtrl;
class LLScrollColumnHeader;
class LLUIImage;

//---------------------------------------------------------------------------
// LLScrollListItem
//---------------------------------------------------------------------------
class LLScrollListItem
{
    friend class LLScrollListCtrl;
public:
    struct Params : public LLInitParam::Block<Params>
    {
        Optional<bool>      enabled;
        Optional<void*>     userdata;
        Optional<LLSD>      value;
        Optional<LLSD>      alt_value;
        
        Ignored             name; // use for localization tools
        Ignored             type; 
        Ignored             length; 

        Multiple<LLScrollListCell::Params> columns;

        Params()
        :   enabled("enabled", true),
            value("value"),
            alt_value("alt_value"),
            name("name"),
            type("type"),
            length("length"),
            columns("columns")
        {
            addSynonym(columns, "column");
            addSynonym(value, "id");
        }
    };

    virtual ~LLScrollListItem();

    void    setSelected( BOOL b );
    BOOL    getSelected() const             { return mSelected; }

    void    setEnabled( BOOL b )            { mEnabled = b; }
    BOOL    getEnabled() const              { return mEnabled; }

    void    setHighlighted( BOOL b );
    BOOL    getHighlighted() const          { return mHighlighted; }

    void    setSelectedCell( S32 cell );
    S32     getSelectedCell() const         { return mSelectedIndex; }

    void    setHoverCell( S32 cell );
    S32     getHoverCell() const            { return mHoverIndex; }

    void    setUserdata( void* userdata )   { mUserdata = userdata; }
    void*   getUserdata() const             { return mUserdata; }

    virtual LLUUID  getUUID() const         { return mItemValue.asUUID(); }
    LLSD    getValue() const                { return mItemValue; }
    LLSD    getAltValue() const             { return mItemAltValue; }
    
    void    setRect(LLRect rect)            { mRectangle = rect; }
    LLRect  getRect() const                 { return mRectangle; }

    void    addColumn( const LLScrollListCell::Params& p );

    void    setNumColumns(S32 columns);

    void    setColumn( S32 column, LLScrollListCell *cell );
    
    S32     getNumColumns() const;

    LLScrollListCell *getColumn(const S32 i) const;

    std::string getContentsCSV() const;

    virtual void draw(const LLRect& rect,
                      const LLColor4& fg_color,
                      const LLColor4& hover_color, // highlight/hover selection of whole item or cell
                      const LLColor4& select_color, // highlight/hover selection of whole item or cell
                      const LLColor4& highlight_color, // highlights contents of cells (ex: text)
                      S32 column_padding);

protected:
    LLScrollListItem( const Params& );

private:
    BOOL    mSelected;
    BOOL    mHighlighted;
    S32     mHoverIndex;
    S32     mSelectedIndex;
    BOOL    mEnabled;
    void*   mUserdata;
    LLSD    mItemValue;
    LLSD    mItemAltValue;
    std::vector<LLScrollListCell *> mColumns;
    LLRect  mRectangle;
};

#endif
