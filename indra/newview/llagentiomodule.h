/** 
 * @file llagentiomodule.h
 * @brief Implementation of llagentiomodule class.
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
#ifndef LL_LLAGENTMODULE_H
#define LL_LLAGENTMODULE_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include <memory>
#include <string>
#include <vector>

#include "llevents.h"
#include "lleventapi.h"
#include "lazyeventapi.h"
#include "llsingleton.h"

class LLLeap;
using namespace LL;

class LLAgentIOModule : public LLSingleton<LLAgentIOModule>,
	public LLEventAPI
{
    LLSINGLETON(LLAgentIOModule);
    LOG_CLASS(LLAgentIOModule);

    // Singleton to manage a pointer to the LLLeap module that provides agentio functions
public:
    LLAgentIOModule(const LL::LazyEventAPIParams& params);
    typedef std::shared_ptr<LLLeap> module_ptr_t;

    void setLeapModule(std::weak_ptr<LLLeap> mod, const std::string & module_name);
    module_ptr_t getLeapModule() const;
    bool haveAgentIOModule() const; // True if module is loaded
    void clearLeapModule();
    void sendCommand(const std::string& command, const LLSD& args = LLSD()) const;

    const std::string & getModuleName() const { return mModuleName; };
    void sendLookAt();
    void sendAgentOrientation();
    void sendCameraOrientation();
    void setCamera(const LLSD& data);


private:

    virtual ~LLAgentIOModule()
    {
    };

	/*
    void setAgentIOOptions(LLSD options);
    static void SetAgentIOOptionsCoro(const std::string& capurl, LLSD options);
	*/
    mutable std::weak_ptr<LLLeap> mLeapModule; // weak pointer to module
    std::string mModuleName;
    LLBoundListener mListener;
    LLTempBoundListener mPlugin;        //For event pump to send leap updates to plug-ins

    bool mIsSending = false;      // true when streaming to simulator
    bool mIsReceiving = true;     // true when getting stream from simulator
};

class LLAgentIORegistrar : LazyEventAPI <LLAgentIOModule>
{
public:
	LLAgentIORegistrar();
};
#endif
