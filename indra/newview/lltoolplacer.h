/** 
 * @file lltoolplacer.h
 * @brief Tool for placing new objects into the world
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_TOOLPLACER_H
#define LL_TOOLPLACER_H

#include "llpanel.h"
#include "lltool.h"

class LLButton;
class LLViewerRegion;

////////////////////////////////////////////////////
// LLToolPlacer

class LLToolPlacer
 :  public LLTool
{
public:
    LLToolPlacer();

    virtual BOOL    placeObject(S32 x, S32 y, MASK mask);
    virtual BOOL    handleHover(S32 x, S32 y, MASK mask);
    virtual void    handleSelect(); // do stuff when your tool is selected
    virtual void    handleDeselect();   // clean up when your tool is deselected

    static void setObjectType( LLPCode type )       { sObjectType = type; }
    static LLPCode getObjectType()                  { return sObjectType; }

protected:
    static LLPCode  sObjectType;

private:
    BOOL addObject( LLPCode pcode, S32 x, S32 y, U8 use_physics );
    BOOL raycastForNewObjPos( S32 x, S32 y, LLViewerObject** hit_obj, S32* hit_face, 
                              BOOL* b_hit_land, LLVector3* ray_start_region, LLVector3* ray_end_region, LLViewerRegion** region );
    BOOL addDuplicate(S32 x, S32 y);
};

#endif
