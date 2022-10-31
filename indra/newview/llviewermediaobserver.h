/**
 * @file llviewermediaobserver.h
 * @brief Methods to override to catch events from LLViewerMedia class
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

#ifndef LLVIEWERMEDIAOBSERVER_H
#define LLVIEWERMEDIAOBSERVER_H

#include "llpluginclassmediaowner.h"

class LLViewerMediaEventEmitter;

class LLViewerMediaObserver : public LLPluginClassMediaOwner
{
public:
    virtual ~LLViewerMediaObserver();
    
private:
    // Emitters will manage this list in addObserver/remObserver.
    friend class LLViewerMediaEventEmitter;
    std::list<LLViewerMediaEventEmitter *> mEmitters;
};


#if 0
    // Classes that inherit from LLViewerMediaObserver should add this to their class declaration:
    
    // inherited from LLViewerMediaObserver
    /*virtual*/ void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);
    
    /* and will probably need to add this to their cpp file:

    #include "llpluginclassmedia.h"

    */
    
    // The list of events is in llpluginclassmediaowner.h
    
    
#endif


#endif // LLVIEWERMEDIAOBSERVER_H

