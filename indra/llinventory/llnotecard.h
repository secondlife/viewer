/** 
 * @file llnotecard.h
 * @brief LLNotecard class declaration
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_NOTECARD_H
#define LL_NOTECARD_H

#include "llpointer.h"
#include "llinventory.h"

class LLNotecard
{
public:
    /**
     * @brief anonymous enumeration to set max size.
     */
    enum
    {
        MAX_SIZE = 65536
    };
    
    LLNotecard(S32 max_text = LLNotecard::MAX_SIZE);
    virtual ~LLNotecard();

    bool importStream(std::istream& str);
    bool exportStream(std::ostream& str);

    const std::vector<LLPointer<LLInventoryItem> >& getItems() const { return mItems; }
    const std::string& getText() const { return mText; }
    std::string& getText() { return mText; }

    void setItems(const std::vector<LLPointer<LLInventoryItem> >& items);
    void setText(const std::string& text);
    S32 getVersion() { return mVersion; }
    S32 getEmbeddedVersion() { return mEmbeddedVersion; }
    
private:
    bool importEmbeddedItemsStream(std::istream& str);
    bool exportEmbeddedItemsStream(std::ostream& str);
    std::vector<LLPointer<LLInventoryItem> > mItems;
    std::string mText;
    S32 mMaxText;
    S32 mVersion;
    S32 mEmbeddedVersion;
};

#endif /* LL_NOTECARD_H */
