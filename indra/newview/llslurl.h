/**
 * @file llslurl.h
 * @brief Handles "SLURL fragments" like Ahern/123/45 for
 * startup processing, login screen, prefs, etc.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
#ifndef LLSLURL_H
#define LLSLURL_H

#include "llstring.h"


// represents a location in a grid

class LLSLURL
{
public:
	static const char* SLURL_HTTPS_SCHEME;
	static const char* SLURL_HTTP_SCHEME;
	static const char* SLURL_SL_SCHEME;
	static const char* SLURL_SECONDLIFE_SCHEME;
	static const char* SLURL_SECONDLIFE_PATH;
	static const char* SLURL_COM;
	static const char* WWW_SLURL_COM;
	static const char* SECONDLIFE_COM;
	static const char* MAPS_SECONDLIFE_COM;
	static const char* SLURL_X_GRID_LOCATION_INFO_SCHEME;
	static LLSLURL START_LOCATION;
	static const char* SIM_LOCATION_HOME;
	static const char* SIM_LOCATION_LAST;
	static const char* SLURL_APP_PATH;
	static const char* SLURL_REGION_PATH;	
	
	// if you modify this enumeration, update typeName as well
	enum SLURL_TYPE { 
		INVALID, 
		LOCATION,
		HOME_LOCATION,
		LAST_LOCATION,
		APP,
		HELP,
		NUM_SLURL_TYPES // must be last
	};
		
	
	LLSLURL(): mType(INVALID)  { }
	LLSLURL(const std::string& slurl);
	LLSLURL(const std::string& grid, const std::string& region);
	LLSLURL(const std::string& region, const LLVector3& position);
	LLSLURL(const std::string& grid, const std::string& region, const LLVector3& position);
	LLSLURL(const std::string& grid, const std::string& region, const LLVector3d& global_position);
	LLSLURL(const std::string& region, const LLVector3d& global_position);
	LLSLURL(const std::string& command, const LLUUID&id, const std::string& verb);
	
	SLURL_TYPE getType() const { return mType; }
	
	std::string getSLURLString() const;
	std::string getLoginString() const;
	std::string getLocationString() const; 
	std::string getGrid() const { return mGrid; }
	std::string getRegion() const { return mRegion; }
	LLVector3   getPosition() const { return mPosition; }
	std::string getAppCmd() const { return mAppCmd; }
	std::string getAppQuery() const { return mAppQuery; }
	LLSD        getAppQueryMap() const { return mAppQueryMap; }
	LLSD        getAppPath() const { return mAppPath; }
	
	bool        isValid() const { return mType != INVALID; }
	bool        isSpatial() const { return (mType == LAST_LOCATION) || (mType == HOME_LOCATION) || (mType == LOCATION); }
	
	bool operator==(const LLSLURL& rhs);
	bool operator!=(const LLSLURL&rhs);

    std::string asString() const ;

protected:
	static const std::string typeName[NUM_SLURL_TYPES];
	/// Get a human-readable version of the type for logging
	static std::string getTypeString(SLURL_TYPE type);	
	
	SLURL_TYPE mType;
	
	// used for Apps and Help
	std::string mAppCmd;
	LLSD        mAppPath;
	LLSD        mAppQueryMap;
	std::string mAppQuery;
	
	std::string mGrid;  // reference to grid manager grid
	std::string mRegion;
	LLVector3  mPosition;
};

#endif // LLSLURL_H
