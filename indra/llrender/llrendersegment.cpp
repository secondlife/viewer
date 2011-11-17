/** 
 * @file llrendersegment.cpp
 * @brief 
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

//	Sphere creates a set of display lists that can then be called to create 
//	a lit sphere at different LOD levels.  You only need one instance of sphere 
//	per viewer - then call the appropriate list.  

#include "linden_common.h"
#include "llrendersegment.h"
#include "llerror.h"
#include "llglheaders.h"

LLRenderSegment gSegment;
//=============================================================================
void LLRenderSegment::renderSegment( const LLVector3& start, const LLVector3& end, int color )
{
	LLColor4 colorA =  LLColor4::yellow;

	gGL.color4fv( colorA.mV );

	gGL.begin(LLRender::LINES);
	{
		gGL.vertex3fv( start.mV );

		gGL.vertex3fv( end.mV );
	}

	gGL.end();
	
}
//=============================================================================