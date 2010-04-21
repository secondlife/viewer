/** 
 * @file llviewernetwork.cpp
 * @author James Cook, Richard Nelson
 * @brief Networking constants and globals for viewer.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
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

#include "llviewerprecompiledheaders.h"

#include "llviewernetwork.h"
#include "llviewercontrol.h"
#include "llsdserialize.h"
#include "llweb.h"

                                                            
const char* DEFAULT_LOGIN_PAGE = "http://secondlife.com/app/login/";

const char* SYSTEM_GRID_SLURL_BASE = "secondlife://%s/secondlife/";
const char* MAIN_GRID_SLURL_BASE = "http://maps.secondlife.com/secondlife/";
const char* SYSTEM_GRID_APP_SLURL_BASE = "secondlife:///app";

const char* DEFAULT_SLURL_BASE = "https://%s/region/";
const char* DEFAULT_APP_SLURL_BASE = "x-grid-location-info://%s/app";

LLGridManager::LLGridManager()
{
	// by default, we use the 'grids.xml' file in the user settings directory
	// this file is an LLSD file containing multiple grid definitions.
	// This file does not contain definitions for secondlife.com grids,
	// as that would be a security issue when they are overwritten by
	// an attacker.  Don't want someone snagging a password.
	std::string grid_file = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,
														   "grids.xml");
	initialize(grid_file);
	
}


LLGridManager::LLGridManager(const std::string& grid_file)
{
	// initialize with an explicity grid file for testing.
	initialize(grid_file);
}

//
// LLGridManager - class for managing the list of known grids, and the current
// selection
//


//
// LLGridManager::initialze - initialize the list of known grids based
// on the fixed list of linden grids (fixed for security reasons)
// the grids.xml file
// and the command line.
void LLGridManager::initialize(const std::string& grid_file)
{
	// default grid list.
	// Don't move to a modifiable file for security reasons,
	mGrid.clear() ;
	// set to undefined
	mGridList = LLSD();
	mGridFile = grid_file;
	// as we don't want an attacker to override our grid list
	// to point the default grid to an invalid grid
	addSystemGrid("None", "", "", "", DEFAULT_LOGIN_PAGE);
	


  	addSystemGrid("Secondlife.com (Agni)",                                                                                             
				  MAINGRID,                                               
				  "https://login.agni.lindenlab.com/cgi-bin/login.cgi",                    
				  "https://secondlife.com/helpers/",     
				  DEFAULT_LOGIN_PAGE);
	addSystemGrid("Aditi",                                                                                             
				  "util.aditi.lindenlab.com",                                              
				  "https://login.aditi.lindenlab.com/cgi-bin/login.cgi",                   
				  "http://aditi-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE);
	addSystemGrid("Aruna",                                                                                            
				  "util.aruna.lindenlab.com",                                              
				  "https://login.aruna.lindenlab.com/cgi-bin/login.cgi",                   
				  "http://aruna-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE);
	addSystemGrid("Durga",                                                                                            
				  "util.durga.lindenlab.com",                                              
				  "https://login.durga.lindenlab.com/cgi-bin/login.cgi",                   
				  "http://durga-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE);
	addSystemGrid("Ganga",                                                                                            
				  "util.ganga.lindenlab.com",                                              
				  "https://login.ganga.lindenlab.com/cgi-bin/login.cgi",                   
				  "http://ganga-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE);
	addSystemGrid("Mitra",                                                                                            
				  "util.mitra.lindenlab.com",                                              
				  "https://login.mitra.lindenlab.com/cgi-bin/login.cgi",                   
				  "http://mitra-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE);
	addSystemGrid("Mohini",                                                                                           
				  "util.mohini.lindenlab.com",                                             
				  "https://login.mohini.lindenlab.com/cgi-bin/login.cgi",                  
				  "http://mohini-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE);
	addSystemGrid("Nandi",                                                                                            
				  "util.nandi.lindenlab.com",                                              
				  "https://login.nandi.lindenlab.com/cgi-bin/login.cgi",                   
				  "http://nandi-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE);
	addSystemGrid("Radha",                                                                                            
				  "util.radha.lindenlab.com",                                              
				  "https://login.radha.lindenlab.com/cgi-bin/login.cgi",                   
				  "http://radha-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE);
	addSystemGrid("Ravi",                                                                                             
				  "util.ravi.lindenlab.com",                                               
				  "https://login.ravi.lindenlab.com/cgi-bin/login.cgi",                    
				  "http://ravi-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE);
	addSystemGrid("Siva",                                                                                             
				  "util.siva.lindenlab.com",                                               
				  "https://login.siva.lindenlab.com/cgi-bin/login.cgi",                    
				  "http://siva-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE);
	addSystemGrid("Shakti",                                                                                           
				  "util.shakti.lindenlab.com",                                             
				  "https://login.shakti.lindenlab.com/cgi-bin/login.cgi",                  
				  "http://shakti-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE);
	addSystemGrid("Soma",                                                                                             
				  "util.soma.lindenlab.com",                                               
				  "https://login.soma.lindenlab.com/cgi-bin/login.cgi",                    
				  "http://soma-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE);
	
	addSystemGrid("Uma",                                                                                              
				  "util.uma.lindenlab.com",                                                
				  "https://login.uma.lindenlab.com/cgi-bin/login.cgi",                     
				  "http://uma-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE);
	addSystemGrid("Vaak",                                                                                             
				  "util.vaak.lindenlab.com",                                               
				  "https://login.vaak.lindenlab.com/cgi-bin/login.cgi",                    
				  "http://vaak-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE);
	addSystemGrid("Yami",                                                                                             
				  "util.yami.lindenlab.com",                                               
				  "https://login.yami.lindenlab.com/cgi-bin/login.cgi",                    
				  "http://yami-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE);
	addSystemGrid("Local (Linden)",                                                                                    
				  "localhost",                                                             
				  "https://login.dmz.lindenlab.com/cgi-bin/login.cgi",                     
				  "",
				  DEFAULT_LOGIN_PAGE); 

	
	LLSD other_grids;
	llifstream llsd_xml;
	if (!grid_file.empty())
	{
		llsd_xml.open( grid_file.c_str(), std::ios::in | std::ios::binary );

		// parse through the gridfile, inserting grids into the list unless
		// they overwrite a linden grid.
		if( llsd_xml.is_open()) 
		{
			LLSDSerialize::fromXMLDocument( other_grids, llsd_xml );
			if(other_grids.isMap())
			{
				for(LLSD::map_iterator grid_itr = other_grids.beginMap(); 
					grid_itr != other_grids.endMap();
					++grid_itr)
				{
					LLSD::String key_name = grid_itr->first;
					LLSD grid = grid_itr->second;
					// TODO:  Make sure gridfile specified label is not 
					// a system grid label
					LL_INFOS("GridManager") << "reading: " << key_name << LL_ENDL;
					if (mGridList.has(key_name) &&
						mGridList[key_name].has(GRID_IS_SYSTEM_GRID_VALUE))
					{
						LL_INFOS("GridManager") << "Cannot override grid " << key_name << " as it's a system grid" << LL_ENDL;
						// If the system grid does exist in the grids file, and it's marked as a favorite, set it as a favorite.
						if(grid_itr->second.has(GRID_IS_FAVORITE_VALUE) && grid_itr->second[GRID_IS_FAVORITE_VALUE].asBoolean() )
						{
							mGridList[key_name][GRID_IS_FAVORITE_VALUE] = TRUE;
						}
					}
					else
					{
						try
						{
							addGrid(grid);
							LL_INFOS("GridManager") << "Added grid: " << key_name << LL_ENDL;
						}
						catch (...)
						{
						}
					}
				}
				llsd_xml.close();
			}	
		}     
	}
	
	// load a grid from the command line.
	// if the actual grid name is specified from the command line,
	// set it as the 'selected' grid.
	mGrid = gSavedSettings.getString("CmdLineGridChoice");
	LL_INFOS("GridManager") << "Grid Name: " << mGrid << LL_ENDL;		
	
	// If a command line login URI was passed in, so we should add the command
	// line grid to the list of grids

	LLSD cmd_line_login_uri = gSavedSettings.getLLSD("CmdLineLoginURI");
	if (cmd_line_login_uri.isString())
	{
		LL_INFOS("GridManager") << "adding cmd line login uri" << LL_ENDL;
		// grab the other related URI values
		std::string cmd_line_helper_uri = gSavedSettings.getString("CmdLineHelperURI");
		std::string cmd_line_login_page = gSavedSettings.getString("LoginPage");
		
		// we've a cmd line login, so add a grid for the command line,
		// overwriting any existing grids
		LLSD grid = LLSD::emptyMap();
		grid[GRID_LOGIN_URI_VALUE] = LLSD::emptyArray();
		grid[GRID_LOGIN_URI_VALUE].append(cmd_line_login_uri);
		LL_INFOS("GridManager") << "cmd line login uri: " << cmd_line_login_uri.asString() << LL_ENDL;
		LLURI uri(cmd_line_login_uri.asString());
		if (mGrid.empty())
		{
			// if a grid name was not passed in via the command line,
			// then set the grid name based on the hostname of the 
			// login uri
			mGrid = uri.hostName();
		}

		grid[GRID_VALUE] = mGrid;

		if (mGridList.has(mGrid) && mGridList[mGrid].has(GRID_LABEL_VALUE))
		{
			grid[GRID_LABEL_VALUE] = mGridList[mGrid][GRID_LABEL_VALUE];
		}
		else
		{
			grid[GRID_LABEL_VALUE] = mGrid;			
		}
		if(!cmd_line_helper_uri.empty())
		{
			grid[GRID_HELPER_URI_VALUE] = cmd_line_helper_uri;
		}

		if(!cmd_line_login_page.empty())
		{
			grid[GRID_LOGIN_PAGE_VALUE] = cmd_line_login_page;
		}
		// if the login page, helper URI value, and so on are not specified,
		// add grid will generate them.

		// Also, we will override a system grid if values are passed in via the command
		// line, for testing.  These values will not be remembered though.
		if (mGridList.has(mGrid) && mGridList[mGrid].has(GRID_IS_SYSTEM_GRID_VALUE))
		{
			grid[GRID_IS_SYSTEM_GRID_VALUE] = TRUE;
		}
		addGrid(grid);
	}
	
	// if a grid was not passed in via the command line, grab it from the CurrentGrid setting.
	if (mGrid.empty())
	{

		mGrid = gSavedSettings.getString("CurrentGrid");
	}

	if (mGrid.empty() || !mGridList.has(mGrid))
	{
		// the grid name was empty, or the grid isn't actually in the list, then set it to the
		// appropriate default.
		LL_INFOS("GridManager") << "Resetting grid as grid name " << mGrid << " is not in the list" << LL_ENDL;
		mGrid = MAINGRID;
	}
	LL_INFOS("GridManager") << "Selected grid is " << mGrid << LL_ENDL;		
	gSavedSettings.setString("CurrentGrid", mGrid);

}

LLGridManager::~LLGridManager()
{
	saveFavorites();
}

//
// LLGridManager::addGrid - add a grid to the grid list, populating the needed values
// if they're not populated yet.
//

void LLGridManager::addGrid(LLSD& grid_data)
{
	if (grid_data.isMap() && grid_data.has(GRID_VALUE))
	{
		std::string grid = utf8str_tolower(grid_data[GRID_VALUE]);

		// grid should be in the form of a dns address
		if (!grid.empty() &&
			grid.find_first_not_of("abcdefghijklmnopqrstuvwxyz1234567890-_. ") != std::string::npos)
		{
			printf("grid name: %s", grid.c_str());
			throw LLInvalidGridName(grid);
		}
		
		// populate the other values if they don't exist
		if (!grid_data.has(GRID_LABEL_VALUE)) 
		{
			grid_data[GRID_LABEL_VALUE] = grid;
		}
		if (!grid_data.has(GRID_ID_VALUE))
		{
			grid_data[GRID_ID_VALUE] = grid;
		}
		
		// if the grid data doesn't include any of the URIs, then 
		// generate them from the grid, which should be a dns address
		if (!grid_data.has(GRID_LOGIN_URI_VALUE)) 
		{
			grid_data[GRID_LOGIN_URI_VALUE] = LLSD::emptyArray();
			grid_data[GRID_LOGIN_URI_VALUE].append(std::string("https://") + 
													grid + "/cgi-bin/login.cgi");
		}
		// Populate to the default values
		if (!grid_data.has(GRID_LOGIN_PAGE_VALUE)) 
		{
			grid_data[GRID_LOGIN_PAGE_VALUE] = std::string("http://") + grid + "/app/login/";
		}		
		if (!grid_data.has(GRID_HELPER_URI_VALUE)) 
		{
			grid_data[GRID_HELPER_URI_VALUE] = std::string("https://") + grid + "/helpers/";
		}		
		LL_INFOS("GridManager") << "ADDING: " << grid << LL_ENDL;
		mGridList[grid] = grid_data;		
	}
}

//
// LLGridManager::addSystemGrid - helper for adding a system grid.
void LLGridManager::addSystemGrid(const std::string& label, 
								  const std::string& name, 
								  const std::string& login, 
								  const std::string& helper,
								  const std::string& login_page,
								  const std::string& login_id)
{
	LLSD grid = LLSD::emptyMap();
	grid[GRID_VALUE] = name;
	grid[GRID_LABEL_VALUE] = label;
	grid[GRID_HELPER_URI_VALUE] = helper;
	grid[GRID_LOGIN_URI_VALUE] = LLSD::emptyArray();
	grid[GRID_LOGIN_URI_VALUE].append(login);
	grid[GRID_LOGIN_PAGE_VALUE] = login_page;
	grid[GRID_IS_SYSTEM_GRID_VALUE] = TRUE;
	grid[GRID_LOGIN_CREDENTIAL_PAGE_TYPE_VALUE] = GRID_LOGIN_CREDENTIAL_PAGE_TYPE_AGENT;
	
	grid[GRID_APP_SLURL_BASE] = SYSTEM_GRID_APP_SLURL_BASE;
	if (login_id.empty())
	{
		grid[GRID_ID_VALUE] = name;
	}
	else
	{
		grid[GRID_ID_VALUE] = login_id;
	}
	
	// only add the system grids beyond agni to the visible list
	// if we're building a debug version.
	if (name == std::string(MAINGRID))
	{
		grid[GRID_SLURL_BASE] = MAIN_GRID_SLURL_BASE;		
		grid[GRID_IS_FAVORITE_VALUE] = TRUE;		
	}
	else
	{
		grid[GRID_SLURL_BASE] = llformat(SYSTEM_GRID_SLURL_BASE, label.c_str());
	}
	addGrid(grid);
}

// return a list of grid name -> grid label mappings for UI purposes
std::map<std::string, std::string> LLGridManager::getKnownGrids(bool favorite_only)
{
	std::map<std::string, std::string> result;
	for(LLSD::map_iterator grid_iter = mGridList.beginMap();
		grid_iter != mGridList.endMap();
		grid_iter++) 
	{
		if(!favorite_only || grid_iter->second.has(GRID_IS_FAVORITE_VALUE))
		{
			result[grid_iter->first] = grid_iter->second[GRID_LABEL_VALUE].asString();
		}
	}

	return result;
}

void LLGridManager::setGridChoice(const std::string& grid)
{
	// Set the grid choice based on a string.
	// The string can be:
	// - a grid label from the gGridInfo table 
	// - a hostname
	// - an ip address

	// loop through.  We could do just a hash lookup but we also want to match
	// on label
	for(LLSD::map_iterator grid_iter = mGridList.beginMap();
		grid_iter != mGridList.endMap();
		grid_iter++) 
	{
		if((grid == grid_iter->first) || 
		   (grid == grid_iter->second[GRID_LABEL_VALUE].asString()))
		{
			mGrid = grid_iter->second[GRID_VALUE].asString();
			gSavedSettings.setString("CurrentGrid", grid_iter->second[GRID_VALUE]);			
			return; 

		}
	}
	LLSD grid_data = LLSD::emptyMap();
	grid_data[GRID_VALUE] = grid;
	addGrid(grid_data);
	mGrid = grid;
	gSavedSettings.setString("CurrentGrid", grid);
}

std::string LLGridManager::getGridByLabel( const std::string &grid_label)
{
	for(LLSD::map_iterator grid_iter = mGridList.beginMap();
		grid_iter != mGridList.endMap();
		grid_iter++) 
	{
		if (grid_iter->second.has(GRID_LABEL_VALUE) && (grid_iter->second[GRID_LABEL_VALUE].asString() == grid_label))
		{
			return grid_iter->first;
		}
	}
	return std::string();
}

void LLGridManager::getLoginURIs(std::vector<std::string>& uris)
{
	uris.clear();
	for (LLSD::array_iterator llsd_uri = mGridList[mGrid][GRID_LOGIN_URI_VALUE].beginArray();
		 llsd_uri != mGridList[mGrid][GRID_LOGIN_URI_VALUE].endArray();
		 llsd_uri++)
	{
		uris.push_back(llsd_uri->asString());
	}
}

bool LLGridManager::isInProductionGrid()
{
	// *NOTE:Mani This used to compare GRID_INFO_AGNI to gGridChoice,
	// but it seems that loginURI trumps that.
	std::vector<std::string> uris;
	getLoginURIs(uris);
	if (uris.size() < 1)
	{
		return 1;
	}
	LLStringUtil::toLower(uris[0]);
	if((uris[0].find("agni") != std::string::npos))
	{
		return true;
	}

	return false;
}

void LLGridManager::saveFavorites()
{
	// filter out just those marked as favorites
	LLSD output_grid_list = LLSD::emptyMap();
	for(LLSD::map_iterator grid_iter = mGridList.beginMap();
		grid_iter != mGridList.endMap();
		grid_iter++)
	{
		if(grid_iter->second.has(GRID_IS_FAVORITE_VALUE))
		{
			output_grid_list[grid_iter->first] = grid_iter->second;
		}
	}       
	llofstream llsd_xml;
	llsd_xml.open( mGridFile.c_str(), std::ios::out | std::ios::binary);	
	LLSDSerialize::toPrettyXML(output_grid_list, llsd_xml);
	llsd_xml.close();
}


// build a slurl for the given region within the selected grid
std::string LLGridManager::getSLURLBase(const std::string& grid)
{
	std::string grid_base;
	if(mGridList.has(grid) && mGridList[grid].has(GRID_SLURL_BASE))
	{
		return mGridList[grid][GRID_SLURL_BASE].asString();
	}
	else
	{
		return  llformat(DEFAULT_SLURL_BASE, grid.c_str());
	}
}

// build a slurl for the given region within the selected grid
std::string LLGridManager::getAppSLURLBase(const std::string& grid)
{
	std::string grid_base;
	if(mGridList.has(grid) && mGridList[grid].has(GRID_APP_SLURL_BASE))
	{
	  return mGridList[grid][GRID_APP_SLURL_BASE].asString();
	}
	else
	{
	  return  llformat(DEFAULT_APP_SLURL_BASE, grid.c_str());
	}
}
