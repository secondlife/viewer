/**
 * @file llavatariconctrl.cpp
 * @brief LLAvatarIconCtrl class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llavatariconctrl.h"

#include <boost/signals2.hpp>

// viewer includes
#include "llagent.h"
#include "llcallingcard.h" // for LLAvatarTracker
#include "llavataractions.h"
#include "llmenugl.h"
#include "lluictrlfactory.h"
#include "llagentdata.h"
#include "llfloaterimsession.h"
#include "llviewertexture.h"
#include "llavatarappearancedefines.h"

// library includes
#include "llavatarnamecache.h"

#define MENU_ITEM_VIEW_PROFILE 0
#define MENU_ITEM_SEND_IM 1

static LLDefaultChildRegistry::Register<LLAvatarIconCtrl> r("avatar_icon");

namespace LLInitParam
{
    void TypeValues<LLAvatarIconCtrlEnums::ESymbolPos>::declareValues()
    {
        declare("BottomLeft",   LLAvatarIconCtrlEnums::BOTTOM_LEFT);
        declare("BottomRight",  LLAvatarIconCtrlEnums::BOTTOM_RIGHT);
        declare("TopLeft",      LLAvatarIconCtrlEnums::TOP_LEFT);
        declare("TopRight",     LLAvatarIconCtrlEnums::TOP_RIGHT);
    }
}


bool LLAvatarIconIDCache::LLAvatarIconIDCacheItem::expired()
{
    const F64 SEC_PER_DAY_PLUS_HOUR = (24.0 + 1.0) * 60.0 * 60.0;
    F64 delta = LLDate::now().secondsSinceEpoch() - cached_time.secondsSinceEpoch();
    if (delta > SEC_PER_DAY_PLUS_HOUR)
        return true;
    return false;
}

void LLAvatarIconIDCache::load  ()
{
    LL_INFOS() << "Loading avatar icon id cache." << LL_ENDL;

    // build filename for each user
    std::string resolved_filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, mFilename);
    llifstream file(resolved_filename.c_str());

    if (!file.is_open())
        return;

    // add each line in the file to the list
    int uuid_len = UUID_STR_LENGTH-1;
    std::string line;
    while (std::getline(file, line))
    {
        LLUUID avatar_id;
        LLUUID icon_id;
        LLDate date;

        if (line.length()<=uuid_len*2)
            continue; // short line, bail out to prevent substr calls throwing exception.

        std::string avatar_id_str = line.substr(0,uuid_len);
        std::string icon_id_str = line.substr(uuid_len,uuid_len);

        std::string date_str = line.substr(uuid_len*2, line.length()-uuid_len*2);

        if(!avatar_id.set(avatar_id_str) || !icon_id.set(icon_id_str) || !date.fromString(date_str))
            continue;

        LLAvatarIconIDCacheItem item = {icon_id,date};
        mCache[avatar_id] = item;
    }

    file.close();

}

void LLAvatarIconIDCache::save  ()
{
    std::string resolved_filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, mFilename);

    // open a file for writing
    llofstream file (resolved_filename.c_str());
    if (!file.is_open())
    {
        LL_WARNS() << "can't open avatar icons cache file\"" << mFilename << "\" for writing" << LL_ENDL;
        return;
    }

    for(std::map<LLUUID,LLAvatarIconIDCacheItem>::iterator it = mCache.begin();it!=mCache.end();++it)
    {
        if(!it->second.expired())
        {
            file << it->first << it->second.icon_id << it->second.cached_time << std::endl;
        }
    }

    file.close();
}

LLUUID* LLAvatarIconIDCache::get        (const LLUUID& avatar_id)
{
    std::map<LLUUID,LLAvatarIconIDCacheItem>::iterator it = mCache.find(avatar_id);
    if(it==mCache.end())
        return 0;
    if(it->second.expired())
        return 0;
    return &it->second.icon_id;
}

void LLAvatarIconIDCache::add       (const LLUUID& avatar_id,const LLUUID& icon_id)
{
    LLAvatarIconIDCacheItem item = {icon_id,LLDate::now()};
    mCache[avatar_id] = item;
}

void LLAvatarIconIDCache::remove    (const LLUUID& avatar_id)
{
    mCache.erase(avatar_id);
}


LLAvatarIconCtrl::Params::Params()
:   avatar_id("avatar_id"),
    draw_tooltip("draw_tooltip", true),
    default_icon_name("default_icon_name"),
    symbol_hpad("symbol_hpad"),
    symbol_vpad("symbol_vpad"),
    symbol_size("symbol_size", 1),
    symbol_pos("symbol_pos", LLAvatarIconCtrlEnums::BOTTOM_RIGHT)
{
    changeDefault(min_width, 32);
    changeDefault(min_height, 32);
}


LLAvatarIconCtrl::LLAvatarIconCtrl(const LLAvatarIconCtrl::Params& p)
:   LLIconCtrl(p),
    LLAvatarPropertiesObserver(),
    mAvatarId(),
    mFullName(),
    mDrawTooltip(p.draw_tooltip),
    mDefaultIconName(p.default_icon_name),
    mAvatarNameCacheConnection(),
    mSymbolHpad(p.symbol_hpad),
    mSymbolVpad(p.symbol_vpad),
    mSymbolSize(p.symbol_size),
    mSymbolPos(p.symbol_pos)
{
    mPriority = LLViewerFetchedTexture::BOOST_ICON;

    // don't request larger image then necessary to save gl memory,
    // but ensure that quality is sufficient
    LLRect rect = p.rect;
    mMaxHeight = llmax((S32)p.min_height, rect.getHeight());
    mMaxWidth = llmax((S32)p.min_width, rect.getWidth());

    if (p.avatar_id.isProvided())
    {
        LLSD value(p.avatar_id);
        setValue(value);
    }
    else
    {
        LLIconCtrl::setValue(mDefaultIconName, LLViewerFetchedTexture::BOOST_UI);
    }
}

LLAvatarIconCtrl::~LLAvatarIconCtrl()
{
    if (mAvatarId.notNull())
    {
        LLAvatarPropertiesProcessor::getInstance()->removeObserver(mAvatarId, this);
        // Name callbacks will be automatically disconnected since LLUICtrl is trackable
    }

    if (mAvatarNameCacheConnection.connected())
    {
        mAvatarNameCacheConnection.disconnect();
    }
}

//virtual
void LLAvatarIconCtrl::setValue(const LLSD& value)
{
    if (value.isUUID())
    {
        LLAvatarPropertiesProcessor* app =
            LLAvatarPropertiesProcessor::getInstance();
        if (mAvatarId.notNull())
        {
            app->removeObserver(mAvatarId, this);
        }

        if (mAvatarId != value.asUUID())
        {
            mAvatarId = value.asUUID();

            // *BUG: This will return stale icons if a user changes their
            // profile picture. However, otherwise we send too many upstream
            // AvatarPropertiesRequest messages.

            // to get fresh avatar icon use
            // LLAvatarIconIDCache::getInstance()->remove(avatar_id);

            // Check if cache already contains image_id for that avatar
            if (!updateFromCache())
            {
                // *TODO: Consider getting avatar icon/badge directly from
                // People API, rather than sending AvatarPropertyRequest
                // messages.  People API already hits the user table.
                LLIconCtrl::setValue(mDefaultIconName, LLViewerFetchedTexture::BOOST_UI);
                app->addObserver(mAvatarId, this);
                app->sendAvatarLegacyPropertiesRequest(mAvatarId);
            }
            else if (gAgentID == mAvatarId)
            {
                // Always track any changes to our own icon id
                app->addObserver(mAvatarId, this);
            }
        }
    }
    else
    {
        LLIconCtrl::setValue(value);
    }

    fetchAvatarName();
}

void LLAvatarIconCtrl::fetchAvatarName()
{
    if (mAvatarId.notNull())
    {
        if (mAvatarNameCacheConnection.connected())
        {
            mAvatarNameCacheConnection.disconnect();
        }
        mAvatarNameCacheConnection = LLAvatarNameCache::get(mAvatarId, boost::bind(&LLAvatarIconCtrl::onAvatarNameCache, this, _1, _2));
    }
}

bool LLAvatarIconCtrl::updateFromCache()
{
    LLUUID* icon_id_ptr = LLAvatarIconIDCache::getInstance()->get(mAvatarId);
    if(!icon_id_ptr)
        return false;

    const LLUUID& icon_id = *icon_id_ptr;

    // Update the avatar
    if (icon_id.notNull())
    {
        LLIconCtrl::setValue(icon_id);
    }
    else
    {
        LLIconCtrl::setValue(mDefaultIconName, LLViewerFetchedTexture::BOOST_UI);
        return false;
    }

    return true;
}

//virtual
void LLAvatarIconCtrl::processProperties(void* data, EAvatarProcessorType type)
{
    // Both APT_PROPERTIES_LEGACY and APT_PROPERTIES have icon data.
    // 'Legacy' is cheaper to request so LLAvatarIconCtrl issues that,
    // but own icon should track any source for the sake of timely updates.
    //
    // If this needs to change, make sure to update onCommitProfileImage
    // to issue right kind of request
    if (APT_PROPERTIES_LEGACY == type)
    {
        LLAvatarLegacyData* avatar_data = static_cast<LLAvatarLegacyData*>(data);
        if (avatar_data)
        {
            if (avatar_data->avatar_id != mAvatarId)
            {
                return;
            }

            LLAvatarIconIDCache::getInstance()->add(mAvatarId,avatar_data->image_id);
            updateFromCache();
        }
    }
    else if (APT_PROPERTIES == type)
    {
        LLAvatarData* avatar_data = static_cast<LLAvatarData*>(data);
        if (avatar_data)
        {
            if (avatar_data->avatar_id != mAvatarId)
            {
                return;
            }

            LLAvatarIconIDCache::getInstance()->add(mAvatarId, avatar_data->image_id);
            updateFromCache();
        }
    }
}

void LLAvatarIconCtrl::onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
{
    mAvatarNameCacheConnection.disconnect();

    if (agent_id == mAvatarId)
    {
        // Most avatar icon controls are next to a UI element that shows
        // a display name, so only show username.
        mFullName = av_name.getUserName();

        if (mDrawTooltip)
        {
            setToolTip(mFullName);
        }
        else
        {
            setToolTip(std::string());
        }
    }
}
