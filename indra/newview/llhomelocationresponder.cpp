/** 
 * @file llhomelocationresponder.cpp
 * @author Meadhbh Hamrick
 * @brief Processes responses to the HomeLocation CapReq
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

/* File Inclusions */
#include "llviewerprecompiledheaders.h"

#include "llhomelocationresponder.h"
#include "llsdutil.h"
#include "llagent.h"
#include "llviewerregion.h"

void LLHomeLocationResponder::result( const LLSD& content )
{
  LLVector3 agent_pos;
  bool      error = true;
		
  do {
	
    // was the call to /agent/<agent-id>/home-location successful?
    // If not, we keep error set to true
    if( ! content.has("success") )
    {
      break;
    }
		
    if( 0 != strncmp("true", content["success"].asString().c_str(), 4 ) )
    {
      break;
    }
		
    // did the simulator return a "justified" home location?
    // If no, we keep error set to true
    if( ! content.has( "HomeLocation" ) )
    {
      break;
    }
		
    if( ! content["HomeLocation"].has("LocationPos") )
    {
      break;
    }
		
    if( ! content["HomeLocation"]["LocationPos"].has("X") )
    {
      break;
    }

    agent_pos.mV[VX] = content["HomeLocation"]["LocationPos"]["X"].asInteger();
		
    if( ! content["HomeLocation"]["LocationPos"].has("Y") )
    {
      break;
    }

    agent_pos.mV[VY] = content["HomeLocation"]["LocationPos"]["Y"].asInteger();
		
    if( ! content["HomeLocation"]["LocationPos"].has("Z") )
    {
      break;
    }

    agent_pos.mV[VZ] = content["HomeLocation"]["LocationPos"]["Z"].asInteger();
		
    error = false;
  } while( 0 );
	
  if( ! error )
  {
    llinfos << "setting home position" << llendl;
		
    LLViewerRegion *viewer_region = gAgent.getRegion();
    gAgent.setHomePosRegion( viewer_region->getHandle(), agent_pos );
  }
}

void LLHomeLocationResponder::errorWithContent( U32 status, const std::string& reason, const LLSD& content )
{
	llwarns << "LLHomeLocationResponder error [status:" << status << "]: " << content << llendl;
}
