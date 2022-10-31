/**
 * @file lloutfitobserver.h
 * @brief Outfit observer facade.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_OUTFITOBSERVER_H
#define LL_OUTFITOBSERVER_H

#include "llsingleton.h"
#include "llmd5.h"

/**
 * Outfit observer facade that provides simple possibility to subscribe on
 * BOF(base outfit) replaced, BOF changed, COF(current outfit) changed events.
 */
class LLOutfitObserver: public LLInventoryObserver, public LLSingleton<LLOutfitObserver>
{
    LLSINGLETON(LLOutfitObserver);
    virtual ~LLOutfitObserver();

public:

    virtual void changed(U32 mask);

    void notifyOutfitLockChanged() { mOutfitLockChanged();  }

    typedef boost::signals2::signal<void (void)> signal_t;

    void addBOFReplacedCallback(const signal_t::slot_type& cb) { mBOFReplaced.connect(cb); }

    void addBOFChangedCallback(const signal_t::slot_type& cb) { mBOFChanged.connect(cb); }

    void addCOFChangedCallback(const signal_t::slot_type& cb) { mCOFChanged.connect(cb); }

    void addCOFSavedCallback(const signal_t::slot_type& cb) { mCOFSaved.connect(cb); }

    void addOutfitLockChangedCallback(const signal_t::slot_type& cb) { mOutfitLockChanged.connect(cb); }

protected:

    /** Get a version of an inventory category specified by its UUID */
    static S32 getCategoryVersion(const LLUUID& cat_id);

    static const std::string& getCategoryName(const LLUUID& cat_id);

    bool checkCOF();

    void checkBaseOutfit();

    //last version number of a COF category
    S32 mCOFLastVersion;

    LLUUID mBaseOutfitId;

    S32 mBaseOutfitLastVersion;
    std::string mLastBaseOutfitName;

    bool mLastOutfitDirtiness;

    LLMD5 mItemNameHash;

private:
    signal_t mBOFReplaced;
    signal_t mBOFChanged;
    signal_t mCOFChanged;
    signal_t mCOFSaved;

    /**
     * Signal for changing state of outfit lock.
     */
    signal_t mOutfitLockChanged;
};

#endif /* LL_OUTFITOBSERVER_H */
