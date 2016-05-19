/** 
 * @file llviewerjoint.h
 * @brief Implementation of LLViewerJoint class
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

#ifndef LL_LLVIEWERJOINT_H
#define LL_LLVIEWERJOINT_H

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "llavatarjoint.h"
#include "lljointpickname.h"

class LLFace;
class LLViewerJointMesh;

//-----------------------------------------------------------------------------
// class LLViewerJoint
//-----------------------------------------------------------------------------
class LLViewerJoint :
	public virtual LLAvatarJoint
{
public:
	LLViewerJoint();
	LLViewerJoint(S32 joint_num);
	// *TODO: Only used for LLVOAvatarSelf::mScreenp.  *DOES NOT INITIALIZE mResetAfterRestoreOldXform*
	LLViewerJoint(const std::string &name, LLJoint *parent = NULL);
	virtual ~LLViewerJoint();

	// Render character hierarchy.
	// Traverses the entire joint hierarchy, setting up
	// transforms and calling the drawShape().
	// Derived classes may add text/graphic output.
	virtual U32 render( F32 pixelArea, BOOL first_pass = TRUE, BOOL is_dummy = FALSE );	// Returns triangle count

	// Draws the shape attached to a joint.
	// Called by render().
	virtual U32 drawShape( F32 pixelArea, BOOL first_pass = TRUE, BOOL is_dummy = FALSE );
	virtual void drawNormals() {}
};

#endif // LL_LLVIEWERJOINT_H


