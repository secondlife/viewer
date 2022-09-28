/** 
 * @file llpuppetmodule.h
 * @brief Implementation of llpuppetmodule class.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

#pragma once
#ifndef LL_LLPUPPETMODULE_H
#define LL_LLPUPPETMODULE_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include <memory>
#include <string>
#include <vector>

#include "llevents.h"
#include "lleventapi.h"
#include "llsingleton.h"

class LLLeap;


// Bit mask for puppetry parts enabling
enum LLPuppetPartMask
{   // These match the string integer parameters in menu_viewer.xml
    PPM_HEAD          =  1,
    PPM_FACE          =  2,
    PPM_LEFT_HAND     =  4,
    PPM_RIGHT_HAND    =  8,
    PPM_FINGERS       = 16,
    PPM_ALL           = 31
};


class LLPuppetModule : public LLSingleton<LLPuppetModule>,
                       public LLEventAPI
{
    LLSINGLETON(LLPuppetModule);
    LOG_CLASS(LLPuppetModule);

    // Singleton to manage a pointer to the LLLeap module that provides puppetry functions
public:
    typedef std::shared_ptr<LLLeap> puppet_module_ptr_t;

    void setLeapModule(std::weak_ptr<LLLeap> mod, const std::string & module_name);
    puppet_module_ptr_t getLeapModule() const;
    bool havePuppetModule() const; // True if module is loaded
    void disableHeadMotion() const;
    void enableHeadMotion() const;
    void clearLeapModule();
    void sendCommand(const std::string& command, const LLSD& args = LLSD()) const;

    const std::string & getModuleName() const { return mModuleName; };

    void setEnabledPart(S32 part_num, bool enable);     // Enable puppetry on body part - head, face, left / right hands
    S32  getEnabledPart(S32 mask = PPM_ALL) const;

    void setCameraNumber(S32 num);  // C++ caller
    void setCameraNumber_(S32 num); // LEAP caller
    S32 getCameraNumber() const;    // C++ caller
    void getCameraNumber_(const LLSD& request) const; // LEAP caller

    void sendCameraNumber();
    void sendEnabledParts();
    void send_skeleton(const LLSD& sd=LLSD::emptyMap());

    bool getEcho() const { return mPlayServerEcho; };
    void setEcho(bool play_server_echo);

    bool isSending() const   { return mIsSending; };
    void setSending(bool sending);

    bool isReceiving() const { return mIsReceiving; };
    void setReceiving(bool receiving);

    F32  getRange() const { return mRange; }
    void setRange(F32 range);

    typedef std::map<std::string, F64> active_joint_map_t;     // Map of used joints and last time
    void addActiveJoint(const std::string & joint_name);
    bool isActiveJoint(const std::string & joint_name);
    const active_joint_map_t & getActiveJoints() const { return mActiveJoints; }

    void parsePuppetryResponse(LLSD response);
private:

    virtual ~LLPuppetModule()
    {
    };

    void setPuppetryOptions(LLSD options);
    static void SetPuppetryOptionsCoro(const std::string& capurl, LLSD options);

    mutable std::weak_ptr<LLLeap> mLeapModule; // weak pointer to module
    std::string mModuleName;
    LLBoundListener mListener;
    LLTempBoundListener mPlugin;        //For event pump to send leap updates to plug-ins

    active_joint_map_t     mActiveJoints;    // Map of used joints and last time seen

    bool mPlayServerEcho = false; // true to play own avatar from server data stream, not directy from leap module
    bool mIsSending = false;      // true when streaming to simulator
    bool mIsReceiving = true;     // true when getting stream from simulator
    LLSD mSkeletonData;           // Send to the expression module on request.
    F32  mRange = 25.0f;
};

#endif
