/**
 * @file llslurl.h
 * @brief Handles "SLURL fragments" like Ahern/123/45 for
 * startup processing, login screen, prefs, etc.
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2006-2010, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
	static const char* MAPS_SECONDLIFE_COM;
	static const char* SLURL_X_GRID_LOCATION_INFO_SCHEME;
	static LLSLURL START_LOCATION;
	static const char* SIM_LOCATION_HOME;
	static const char* SIM_LOCATION_LAST;
	static const char* SLURL_APP_PATH;
	static const char* SLURL_REGION_PATH;	
	
	enum SLURL_TYPE { 
		INVALID, 
		LOCATION,
		HOME_LOCATION,
		LAST_LOCATION,
		APP,
		HELP 
	};
		
	
	LLSLURL(): mType(LAST_LOCATION)  { }
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
