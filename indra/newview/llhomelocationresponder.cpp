/** 
 * @file llhomelocationresponder.cpp
 * @author Meadhbh Hamrick
 * @brief Processes responses to the HomeLocation CapReq
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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

void LLHomeLocationResponder::error( U32 status, const std::string& reason )
{
  llinfos << "received error(" << reason  << ")" << llendl;
}
