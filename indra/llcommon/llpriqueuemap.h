/** 
 * @file llpriqueuemap.h
 * @brief Priority queue implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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
#ifndef LL_LLPRIQUEUEMAP_H
#define LL_LLPRIQUEUEMAP_H

#include <map>

//
// Priority queue, implemented under the hood as a
// map.  Needs to be done this way because none of the
// standard STL containers provide a representation
// where it's easy to reprioritize.
//

template <class DATA>
class LLPQMKey
{
public:
    LLPQMKey(const F32 priority, DATA data) : mPriority(priority), mData(data)
    {
    }

    bool operator<(const LLPQMKey &b) const
    {
        if (mPriority > b.mPriority)
        {
            return TRUE;
        }
        if (mPriority < b.mPriority)
        {
            return FALSE;
        }
        if (mData > b.mData)
        {
            return TRUE;
        }
        return FALSE;
    }

    F32 mPriority;
    DATA mData;
};

template <class DATA_TYPE>
class LLPriQueueMap
{
public:
    typedef typename std::map<LLPQMKey<DATA_TYPE>, DATA_TYPE>::iterator pqm_iter;
    typedef std::pair<LLPQMKey<DATA_TYPE>, DATA_TYPE> pqm_pair;
    typedef void (*set_pri_fn)(DATA_TYPE &data, const F32 priority);
    typedef F32 (*get_pri_fn)(DATA_TYPE &data);


    LLPriQueueMap(set_pri_fn set_pri, get_pri_fn get_pri) : mSetPriority(set_pri), mGetPriority(get_pri)
    {
    }

    void push(const F32 priority, DATA_TYPE data)
    {
#ifdef _DEBUG
        pqm_iter iter = mMap.find(LLPQMKey<DATA_TYPE>(priority, data));
        if (iter != mMap.end())
        {
            LL_ERRS() << "Pushing already existing data onto queue!" << LL_ENDL;
        }
#endif
        mMap.insert(pqm_pair(LLPQMKey<DATA_TYPE>(priority, data), data));
    }

    BOOL pop(DATA_TYPE *datap)
    {
        pqm_iter iter;
        iter = mMap.begin();
        if (iter == mMap.end())
        {
            return FALSE;
        }
        *datap = (*(iter)).second;
        mMap.erase(iter);

        return TRUE;
    }

    void reprioritize(const F32 new_priority, DATA_TYPE data)
    {
        pqm_iter iter;
        F32 cur_priority = mGetPriority(data);
        LLPQMKey<DATA_TYPE> cur_key(cur_priority, data);
        iter = mMap.find(cur_key);
        if (iter == mMap.end())
        {
            LL_WARNS() << "Data not on priority queue!" << LL_ENDL;
            // OK, try iterating through all of the data and seeing if we just screwed up the priority
            // somehow.
            for (iter = mMap.begin(); iter != mMap.end(); iter++)
            {
                if ((*(iter)).second == data)
                {
                    LL_ERRS() << "Data on priority queue but priority not matched!" << LL_ENDL;
                }
            }
            return;
        }

        mMap.erase(iter);
        mSetPriority(data, new_priority);
        push(new_priority, data);
    }

    S32 getLength() const
    {
        return (S32)mMap.size();
    }

    // Hack: public for use by the transfer manager, ugh.
    std::map<LLPQMKey<DATA_TYPE>, DATA_TYPE> mMap;
protected:
    void (*mSetPriority)(DATA_TYPE &data, const F32 priority);
    F32 (*mGetPriority)(DATA_TYPE &data);
};

#endif // LL_LLPRIQUEUEMAP_H
