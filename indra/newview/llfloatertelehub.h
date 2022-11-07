/** 
 * @file llfloatertelehub.h
 * @author James Cook
 * @brief LLFloaterTelehub class definition
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#ifndef LL_LLFLOATERTELEHUB_H
#define LL_LLFLOATERTELEHUB_H

#include "llfloater.h"

class LLMessageSystem;
class LLObjectSelection;

const S32 MAX_SPAWNPOINTS_PER_TELEHUB = 16;

class LLFloaterTelehub : public LLFloater
{
public:
    LLFloaterTelehub(const LLSD& key);
    ~LLFloaterTelehub();
    
    /*virtual*/ BOOL postBuild();
    /*virtual*/ void onOpen(const LLSD& key);

    /*virtual*/ void draw();
    
    static BOOL renderBeacons();
    static void addBeacons();

    void refresh();
    void sendTelehubInfoRequest();

    void onClickConnect();
    void onClickDisconnect();
    void onClickAddSpawnPoint();
    void onClickRemoveSpawnPoint();

    static void processTelehubInfo(LLMessageSystem* msg, void**);
    void unpackTelehubInfo(LLMessageSystem* msg);

private:
    LLUUID mTelehubObjectID;    // null if no telehub
    std::string mTelehubObjectName;
    LLVector3 mTelehubPos;  // region local, fallback if viewer can't see the object
    LLQuaternion mTelehubRot;

    S32 mNumSpawn;
    LLVector3 mSpawnPointPos[MAX_SPAWNPOINTS_PER_TELEHUB];
    
    LLSafeHandle<LLObjectSelection> mObjectSelection;

    static LLFloaterTelehub* sInstance;
};

#endif
