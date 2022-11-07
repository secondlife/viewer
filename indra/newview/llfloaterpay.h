/** 
 * @file llfloaterpay.h
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

#ifndef LLFLOATERPAY_H
#define LLFLOATERPAY_H

#include "llsafehandle.h"

class LLObjectSelection;
class LLUUID;
class LLViewerRegion;

typedef void (*money_callback)(const LLUUID&, LLViewerRegion*,S32,BOOL,S32,const std::string&);

namespace LLFloaterPayUtil
{
    /// Register with LLFloaterReg
    void registerFloater();

    /// Pay into an in-world object, which will trigger scripts and eventually
    /// transfer the L$ to the resident or group that owns the object.
    /// Objects must be selected.  Recipient (primary) object may be a child.
    void payViaObject(money_callback callback,
                      LLSafeHandle<LLObjectSelection> selection);
    
    /// Pay an avatar or group directly, not via an object in the world.
    /// Scripts are not notified, L$ can be direcly transferred.
    void payDirectly(money_callback callback,
                     const LLUUID& target_id,
                     bool is_group);
}

#endif // LLFLOATERPAY_H
