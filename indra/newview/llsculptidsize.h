/**
* @file llsculptidsize.h
* @brief LLSculptIDSize class definition
*
* $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLSCULPTIDSIZE_H
#define LL_LLSCULPTIDSIZE_H

#include "lluuid.h"

//std
#include <set>
//boost
#include "boost/multi_index_container.hpp"
#include "boost/multi_index/ordered_index.hpp"
#include "boost/multi_index/mem_fun.hpp"

class LLDrawable;


class LLSculptIDSize
{
public:
    struct SizeSum
    {
        SizeSum(int size) 
            : mSizeSum(size)
        {}
        unsigned int mSizeSum;
    };

    struct Info
    {
        typedef boost::shared_ptr<SizeSum> PtrSizeSum;

        Info(const LLDrawable *drawable, int size, PtrSizeSum sizeInfo, LLUUID sculptId)
            : mDrawable(drawable)
            , mSize(size)
            , mSharedSizeSum(sizeInfo)
            , mSculptId(sculptId)
        {}

        const LLDrawable *mDrawable;
        unsigned int mSize;
        PtrSizeSum  mSharedSizeSum;
        LLUUID mSculptId;

        inline const LLDrawable*    getPtrLLDrawable() const { return mDrawable; }
        inline unsigned int         getSize() const { return mSize; }
        inline unsigned int         getSizeSum() const { return mSharedSizeSum->mSizeSum; }
        inline LLUUID               getSculptId() const { return mSculptId; }
        PtrSizeSum                  getSizeInfo() { return mSharedSizeSum; }
    };

public:
    //tags
    struct tag_BY_DRAWABLE {};
    struct tag_BY_SCULPT_ID {};
    struct tag_BY_SIZE {};

    //container
    typedef boost::multi_index_container <
        Info,
        boost::multi_index::indexed_by <
        boost::multi_index::ordered_unique< boost::multi_index::tag<tag_BY_DRAWABLE>
        , boost::multi_index::const_mem_fun<Info, const LLDrawable*, &Info::getPtrLLDrawable>
        >
        , boost::multi_index::ordered_non_unique<boost::multi_index::tag<tag_BY_SCULPT_ID>
        , boost::multi_index::const_mem_fun<Info, LLUUID, &Info::getSculptId>
        >
        , boost::multi_index::ordered_non_unique < boost::multi_index::tag<tag_BY_SIZE>
        , boost::multi_index::const_mem_fun < Info, unsigned int, &Info::getSizeSum >
        >
        >
    > container;

    //views
    typedef container::index<tag_BY_DRAWABLE>::type         container_BY_DRAWABLE_view;
    typedef container::index<tag_BY_SCULPT_ID>::type        container_BY_SCULPT_ID_view;
    typedef container::index<tag_BY_SIZE>::type             container_BY_SIZE_view;

private:
    LLSculptIDSize()
    {}

public:
    static LLSculptIDSize & instance() 
    {
        static LLSculptIDSize inst;
        return inst;
    }

public:
    void inc(const LLDrawable *pdrawable, int sz);
    void dec(const LLDrawable *pdrawable);
    void rem(const LLUUID &sculptId);

    inline void addToUnloaded(const LLUUID &sculptId) { mMarkAsUnloaded.insert(sculptId); }
    inline void remFromUnloaded(const LLUUID &sculptId) { mMarkAsUnloaded.erase(sculptId); }
    inline bool isUnloaded(const LLUUID &sculptId) const { return mMarkAsUnloaded.end() != mMarkAsUnloaded.find(sculptId); }
    inline void clearUnloaded() { mMarkAsUnloaded.clear(); }
    
    void resetSizeSum(const LLUUID &sculptId);

    inline const container & getSizeInfo() const { return mSizeInfo; }

private:
    container mSizeInfo;
    typedef std::set<LLUUID> std_LLUUID;
    std_LLUUID mMarkAsUnloaded;
};

#endif
