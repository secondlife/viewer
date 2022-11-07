/**
* @file llsculptidsize.cpp
* @brief LLSculptIDSize class implementation
*
* $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "llsculptidsize.h"
#include "llvovolume.h"
#include "lldrawable.h"
#include "llvoavatar.h"
//boost
#include "boost/make_shared.hpp"

//...........

extern LLControlGroup gSavedSettings;

//...........

typedef std::pair<LLSculptIDSize::container_BY_SCULPT_ID_view::iterator, LLSculptIDSize::container_BY_SCULPT_ID_view::iterator> pair_iter_iter_BY_SCULPT_ID_t;

//...........

void _nothing_to_do_func(int) { /*nothing todo here because of the size it's a shared member*/ }

void LLSculptIDSize::inc(const LLDrawable *pdrawable, int sz)
{
    llassert(sz >= 0);

    if (!pdrawable) return;
    LLVOVolume* vvol = pdrawable->getVOVolume();
    if (!vvol) return;
    if (!vvol->isAttachment()) return;
    if (!vvol->getAvatar()) return;
    if (vvol->getAvatar()->isSelf()) return;
    LLVolume *vol = vvol->getVolume();
    if (!vol) return;

    const LLUUID &sculptId = vol->getParams().getSculptID();
    if (sculptId.isNull()) return;

    unsigned int total_size = 0;

    pair_iter_iter_BY_SCULPT_ID_t itLU = mSizeInfo.get<tag_BY_SCULPT_ID>().equal_range(sculptId);
    if (itLU.first == itLU.second)
    { //register
        llassert(mSizeInfo.get<tag_BY_DRAWABLE>().end() == mSizeInfo.get<tag_BY_DRAWABLE>().find(pdrawable));
        mSizeInfo.get<tag_BY_DRAWABLE>().insert(Info(pdrawable, sz, boost::make_shared<SizeSum>(sz), sculptId));
        total_size = sz;
    }
    else
    { //update + register
        Info &nfo = const_cast<Info &>(*itLU.first);
        //calc new size
        total_size = nfo.getSizeSum() + sz;
        nfo.mSharedSizeSum->mSizeSum = total_size;
        nfo.mSize = sz;
        //update size for all LLDrwable in range of sculptId
        for (pair_iter_iter_BY_SCULPT_ID_t::first_type it = itLU.first; it != itLU.second; ++it)
        {
            mSizeInfo.get<tag_BY_SIZE>().modify_key(mSizeInfo.project<tag_BY_SIZE>(it), boost::bind(&_nothing_to_do_func, _1));
        }

        //trying insert the LLDrawable
        mSizeInfo.get<tag_BY_DRAWABLE>().insert(Info(pdrawable, sz, nfo.mSharedSizeSum, sculptId));
    }

    static LLCachedControl<U32> render_auto_mute_byte_limit(gSavedSettings, "RenderAutoMuteByteLimit", 0U);

    if (0 != render_auto_mute_byte_limit && total_size > render_auto_mute_byte_limit)
    {
        pair_iter_iter_BY_SCULPT_ID_t it_eqr = mSizeInfo.get<tag_BY_SCULPT_ID>().equal_range(sculptId);
        for (; it_eqr.first != it_eqr.second; ++it_eqr.first)
        {
            const Info &i = *it_eqr.first;
            LLVOVolume *pVVol = i.mDrawable->getVOVolume();
            if (pVVol
                && !pVVol->isDead()
                && pVVol->isAttachment()
                && !pVVol->getAvatar()->isSelf()
                && LLVOVolume::NO_LOD != pVVol->getLOD()
                )
            {
                addToUnloaded(sculptId);
                //immediately
                const_cast<LLDrawable*>(i.mDrawable)->unload();
            }
        }
    }
}

void LLSculptIDSize::dec(const LLDrawable *pdrawable)
{
    container_BY_DRAWABLE_view::iterator it = mSizeInfo.get<tag_BY_DRAWABLE>().find(pdrawable);
    if (mSizeInfo.get<tag_BY_DRAWABLE>().end() == it) return;

    unsigned int size = it->getSizeSum() - it->getSize();

    if (0 == size)
    {
        mSizeInfo.get<tag_BY_SCULPT_ID>().erase(it->getSculptId());
    }
    else
    {
        Info &nfo = const_cast<Info &>(*it);
        nfo.mSize = 0;
        pair_iter_iter_BY_SCULPT_ID_t itLU = mSizeInfo.get<tag_BY_SCULPT_ID>().equal_range(it->getSculptId());
        it->mSharedSizeSum->mSizeSum = size;
        for (pair_iter_iter_BY_SCULPT_ID_t::first_type it = itLU.first; it != itLU.second; ++it)
        {
            mSizeInfo.get<tag_BY_SIZE>().modify_key(mSizeInfo.project<tag_BY_SIZE>(it), boost::bind(&_nothing_to_do_func, _1));
        }
    }
}

void LLSculptIDSize::rem(const LLUUID &sculptId)
{
    mSizeInfo.get<tag_BY_SCULPT_ID>().erase(sculptId);
}

void LLSculptIDSize::resetSizeSum(const LLUUID &sculptId)
{
    const pair_iter_iter_BY_SCULPT_ID_t itLU = mSizeInfo.get<tag_BY_SCULPT_ID>().equal_range(sculptId);

    if (itLU.first != itLU.second) {
        itLU.first->mSharedSizeSum->mSizeSum = 0;
    }

    for (pair_iter_iter_BY_SCULPT_ID_t::first_type it = itLU.first, itE = itLU.second; it != itE; ++it)
    {
        mSizeInfo.get<tag_BY_SIZE>().modify_key(mSizeInfo.project<tag_BY_SIZE>(it), boost::bind(&_nothing_to_do_func, _1));
    }
}
