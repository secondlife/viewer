/**
 * @file llviewernetwork.cpp
 * @author James Cook, Richard Nelson
 * @brief Networking constants and globals for viewer.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llviewernetwork.h"
#include "llviewercontrol.h"
#include "llsdserialize.h"
#include "llsecapi.h"
#include "lltrans.h"
#include "llweb.h"


/// key used to store the grid, and the name attribute in the grid data
const std::string  GRID_VALUE = "keyname";
/// the value displayed in the grid selector menu, and other human-oriented text
const std::string  GRID_LABEL_VALUE = "label";
/// the value used on the --grid command line argument
const std::string  GRID_ID_VALUE = "grid_login_id";
/// the url for the login cgi script
const std::string  GRID_LOGIN_URI_VALUE = "login_uri";
///
const std::string  GRID_HELPER_URI_VALUE = "helper_uri";
/// the splash page url
const std::string  GRID_LOGIN_PAGE_VALUE = "login_page";
/// internal data on system grids
const std::string  GRID_IS_SYSTEM_GRID_VALUE = "system_grid";
/// whether this is single or double names
const std::string  GRID_LOGIN_IDENTIFIER_TYPES = "login_identifier_types";

// defines slurl formats associated with various grids.
// we need to continue to support existing forms, as slurls
// are shared between viewers that may not understand newer
// forms.
const std::string GRID_SLURL_BASE = "slurl_base";
const std::string GRID_APP_SLURL_BASE = "app_slurl_base";

const std::string DEFAULT_LOGIN_PAGE = "http://viewer-login.agni.lindenlab.com/";

const std::string MAIN_GRID_LOGIN_URI = "https://login.agni.lindenlab.com/cgi-bin/login.cgi";

const std::string MAIN_GRID_SLURL_BASE = "http://maps.secondlife.com/secondlife/";
const std::string SYSTEM_GRID_APP_SLURL_BASE = "secondlife:///app";

const char* SYSTEM_GRID_SLURL_BASE = "secondlife://%s/secondlife/";
const char* DEFAULT_SLURL_BASE = "https://%s/region/";
const char* DEFAULT_APP_SLURL_BASE = "x-grid-location-info://%s/app";

LLGridManager::LLGridManager()
:	mIsInProductionGrid(false)
{
	// by default, we use the 'grids.xml' file in the user settings directory
	// this file is an LLSD file containing multiple grid definitions.
	// This file does not contain definitions for secondlife.com grids,
	// as that would be a security issue when they are overwritten by
	// an attacker.  Don't want someone snagging a password.
	std::string grid_file = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,
														   "grids.xml");
	LL_DEBUGS("GridManager")<<LL_ENDL;

	initialize(grid_file);

}


LLGridManager::LLGridManager(const std::string& grid_file)
{
	// initialize with an explicity grid file for testing.
	LL_DEBUGS("GridManager")<<LL_ENDL;
	initialize(grid_file);
}

//
// LLGridManager - class for managing the list of known grids, and the current
// selection
//


//
// LLGridManager::initialze - initialize the list of known grids based
// on the fixed list of linden grids (fixed for security reasons)
// and the grids.xml file
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
  	addSystemGrid("Second Life Main Grid (Agni)",
				  MAINGRID,
				  MAIN_GRID_LOGIN_URI,
				  "https://secondlife.com/helpers/",
				  DEFAULT_LOGIN_PAGE,
				  "Agni");
	addSystemGrid("Second Life Beta Test Grid (Aditi)",
				  "util.aditi.lindenlab.com",
				  "https://login.aditi.lindenlab.com/cgi-bin/login.cgi",
				  "http://aditi-secondlife.webdev.lindenlab.com/helpers/",
				  DEFAULT_LOGIN_PAGE,
				  "Aditi");

	LLSD other_grids;
	llifstream llsd_xml;
	if (!grid_file.empty())
	{
		LL_INFOS("GridManager")<<"Grid configuration file '"<<grid_file<<"'"<<LL_ENDL;
		llsd_xml.open( grid_file.c_str(), std::ios::in | std::ios::binary );

		// parse through the gridfile, inserting grids into the list unless
		// they overwrite an existing grid.
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

					std::string existingGrid = getGrid(grid);
					if (mGridList.has(key_name) || !existingGrid.empty())
					{
						LL_WARNS("GridManager") << "Cannot override existing grid '" << key_name << "'; ignoring definition from '"<<grid_file<<"'" << LL_ENDL;
					}
					else if ( addGrid(grid) )
					{
						LL_INFOS("GridManager") << "added grid '"<<key_name<<"'"<<LL_ENDL;
					}
					else
					{
						LL_WARNS("GridManager") << "failed to add invalid grid '"<<key_name<<"'"<<LL_ENDL;
					}
				}
				llsd_xml.close();
			}
			else
			{
				LL_WARNS("GridManager")<<"Failed to parse grid configuration '"<<grid_file<<"'"<<LL_ENDL;
			}
		}
		else
		{
			LL_WARNS("GridManager")<<"Failed to open grid configuration '"<<grid_file<<"'"<<LL_ENDL;
		}
	}
	else
	{
		LL_DEBUGS("GridManager")<<"no grid file specified"<<LL_ENDL;
	}

	// load a grid from the command line.
	// if the actual grid name is specified from the command line,
	// set it as the 'selected' grid.
	std::string cmd_line_grid = gSavedSettings.getString("CmdLineGridChoice");
	if(!cmd_line_grid.empty())
	{
		// try to find the grid assuming the command line parameter is
		// the case-insensitive 'label' of the grid.  ie 'Agni'
		mGrid = getGrid(cmd_line_grid);
		if(mGrid.empty())
		{
			LL_WARNS("GridManager")<<"Unknown grid '"<<cmd_line_grid<<"'"<<LL_ENDL;
		}
		else
		{
			LL_INFOS("GridManager")<<"Command line specified '"<<cmd_line_grid<<"': "<<mGrid<<LL_ENDL;
		}
	}
	else
	{
		// if a grid was not passed in via the command line, grab it from the CurrentGrid setting.
		// if there's no current grid, that's ok as it'll be either set by the value passed
		// in via the login uri if that's specified, or will default to maingrid
		std::string last_grid = gSavedSettings.getString("CurrentGrid");
		if ( ! getGrid(last_grid).empty() )
		{
			LL_INFOS("GridManager")<<"Using last grid: "<<last_grid<<LL_ENDL;
			mGrid = last_grid;
		}
		else
		{
			LL_INFOS("GridManager")<<"Last grid '"<<last_grid<<"' not configured"<<LL_ENDL;
		}
	}

	if(mGrid.empty())
	{
		// no grid was specified so default to maingrid
		LL_INFOS("GridManager") << "Default grid to "<<MAINGRID<< LL_ENDL;
		mGrid = MAINGRID;
	}

	LLControlVariablePtr grid_control = gSavedSettings.getControl("CurrentGrid");
	if (grid_control.notNull())
	{
		grid_control->getSignal()->connect(boost::bind(&LLGridManager::updateIsInProductionGrid, this));
	}

	// since above only triggers on changes, trigger the callback manually to initialize state
	updateIsInProductionGrid();

	setGridChoice(mGrid);
}

LLGridManager::~LLGridManager()
{
}

//
// LLGridManager::addGrid - add a grid to the grid list, populating the needed values
// if they're not populated yet.
//

bool LLGridManager::addGrid(LLSD& grid_data)
{
	bool added = false;
	if (grid_data.isMap() && grid_data.has(GRID_VALUE))
	{
		std::string grid = utf8str_tolower(grid_data[GRID_VALUE].asString());

		if ( getGrid(grid_data[GRID_VALUE]).empty() && getGrid(grid).empty() )
		{
			std::string grid_id = grid_data.has(GRID_ID_VALUE) ? grid_data[GRID_ID_VALUE].asString() : "";
			if ( getGrid(grid_id).empty() )
			{
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

				if (!grid_data.has(GRID_LOGIN_IDENTIFIER_TYPES))
				{
					// non system grids and grids that haven't already been configured with values
					// get both types of credentials.
					grid_data[GRID_LOGIN_IDENTIFIER_TYPES] = LLSD::emptyArray();
					grid_data[GRID_LOGIN_IDENTIFIER_TYPES].append(CRED_IDENTIFIER_TYPE_AGENT);
					grid_data[GRID_LOGIN_IDENTIFIER_TYPES].append(CRED_IDENTIFIER_TYPE_ACCOUNT);
				}

				LL_DEBUGS("GridManager") <<grid<<"\n"
										 <<"  id:          "<<grid_data[GRID_ID_VALUE].asString()<<"\n"
										 <<"  label:       "<<grid_data[GRID_LABEL_VALUE].asString()<<"\n"
										 <<"  login page:  "<<grid_data[GRID_LOGIN_PAGE_VALUE].asString()<<"\n"
										 <<"  helper page: "<<grid_data[GRID_HELPER_URI_VALUE].asString()<<"\n";
				/* still in LL_DEBUGS */ 
				for (LLSD::array_const_iterator login_uris = grid_data[GRID_LOGIN_URI_VALUE].beginArray();
					 login_uris != grid_data[GRID_LOGIN_URI_VALUE].endArray();
					 login_uris++)
				{
					LL_CONT << "  login uri:   "<<login_uris->asString()<<"\n";
				}
				LL_CONT << LL_ENDL;
				mGridList[grid] = grid_data;
				added = true;
			}
			else
			{
				LL_WARNS("GridManager")<<"duplicate grid id'"<<grid_id<<"' ignored"<<LL_ENDL;
			}
		}
		else
		{
			LL_WARNS("GridManager")<<"duplicate grid name '"<<grid<<"' ignored"<<LL_ENDL;
		}
	}
	else
	{
		LL_WARNS("GridManager")<<"invalid grid definition ignored"<<LL_ENDL;
	}
	return added;
}

//
// LLGridManager::addSystemGrid - helper for adding a system grid.
void LLGridManager::addSystemGrid(const std::string& label,
								  const std::string& name,
								  const std::string& login_uri,
								  const std::string& helper,
								  const std::string& login_page,
								  const std::string& login_id)
{
	LLSD grid = LLSD::emptyMap();
	grid[GRID_VALUE] = name;
	grid[GRID_LABEL_VALUE] = label;
	grid[GRID_HELPER_URI_VALUE] = helper;
	grid[GRID_LOGIN_URI_VALUE] = LLSD::emptyArray();
	grid[GRID_LOGIN_URI_VALUE].append(login_uri);
	grid[GRID_LOGIN_PAGE_VALUE] = login_page;
	grid[GRID_IS_SYSTEM_GRID_VALUE] = true;
	grid[GRID_LOGIN_IDENTIFIER_TYPES] = LLSD::emptyArray();
	grid[GRID_LOGIN_IDENTIFIER_TYPES].append(CRED_IDENTIFIER_TYPE_AGENT);

	grid[GRID_APP_SLURL_BASE] = SYSTEM_GRID_APP_SLURL_BASE;
	if (login_id.empty())
	{
		grid[GRID_ID_VALUE] = name;
	}
	else
	{
		grid[GRID_ID_VALUE] = login_id;
	}

	if (name == std::string(MAINGRID))
	{
		grid[GRID_SLURL_BASE] = MAIN_GRID_SLURL_BASE;
	}
	else
	{
		grid[GRID_SLURL_BASE] = llformat(SYSTEM_GRID_SLURL_BASE, grid[GRID_ID_VALUE].asString().c_str());
	}

	addGrid(grid);
}

// return a list of grid name -> grid label mappings for UI purposes
std::map<std::string, std::string> LLGridManager::getKnownGrids()
{
	std::map<std::string, std::string> result;
	for(LLSD::map_iterator grid_iter = mGridList.beginMap();
		grid_iter != mGridList.endMap();
		grid_iter++)
	{
		result[grid_iter->first] = grid_iter->second[GRID_LABEL_VALUE].asString();
	}

	return result;
}

void LLGridManager::setGridChoice(const std::string& grid)
{
	// Set the grid choice based on a string.
	LL_DEBUGS("GridManager")<<"requested "<<grid<<LL_ENDL;
 	std::string grid_name = getGrid(grid); // resolved either the name or the id to the name

	if(!grid_name.empty())
	{
		LL_INFOS("GridManager")<<"setting "<<grid_name<<LL_ENDL;
		mGrid = grid_name;
		gSavedSettings.setString("CurrentGrid", grid_name);
		
		updateIsInProductionGrid();
	}
	else
	{
		// the grid was not in the list of grids.
		LL_WARNS("GridManager")<<"unknown grid "<<grid<<LL_ENDL;
	}
}

std::string LLGridManager::getGrid( const std::string &grid )
{
	std::string grid_name;

	if (mGridList.has(grid))
	{
		// the grid was the long name, so we're good, return it
		grid_name = grid;
	}
	else
	{
		// search the grid list for a grid with a matching id
		for(LLSD::map_iterator grid_iter = mGridList.beginMap();
			grid_name.empty() && grid_iter != mGridList.endMap();
			grid_iter++)
		{
			if (grid_iter->second.has(GRID_ID_VALUE))
			{
				if (0 == (LLStringUtil::compareInsensitive(grid,
														   grid_iter->second[GRID_ID_VALUE].asString())))
				{
					// found a matching label, return this name
					grid_name = grid_iter->first;
				}
			}
		}
	}
	return grid_name;
}

std::string LLGridManager::getGridLabel(const std::string& grid)
{
	std::string grid_label;
	std::string grid_name = getGrid(grid);
	if (!grid.empty())
	{
		grid_label = mGridList[grid_name][GRID_LABEL_VALUE].asString();
	}
	else
	{
		LL_WARNS("GridManager")<<"invalid grid '"<<grid<<"'"<<LL_ENDL;
	}
	LL_DEBUGS("GridManager")<<"returning "<<grid_label<<LL_ENDL;
	return grid_label;
}

std::string LLGridManager::getGridId(const std::string& grid)
{
	std::string grid_id;
	std::string grid_name = getGrid(grid);
	if (!grid.empty())
	{
		grid_id = mGridList[grid_name][GRID_ID_VALUE].asString();
	}
	else
	{
		LL_WARNS("GridManager")<<"invalid grid '"<<grid<<"'"<<LL_ENDL;
	}
	LL_DEBUGS("GridManager")<<"returning "<<grid_id<<LL_ENDL;
	return grid_id;
}

void LLGridManager::getLoginURIs(const std::string& grid, std::vector<std::string>& uris)
{
	uris.clear();
	std::string grid_name = getGrid(grid);
	if (!grid_name.empty())
	{
		for (LLSD::array_iterator llsd_uri = mGridList[grid_name][GRID_LOGIN_URI_VALUE].beginArray();
			 llsd_uri != mGridList[grid_name][GRID_LOGIN_URI_VALUE].endArray();
			 llsd_uri++)
		{
			uris.push_back(llsd_uri->asString());
		}
	}
	else
	{
		LL_WARNS("GridManager")<<"invalid grid '"<<grid<<"'"<<LL_ENDL;
	}
}

void LLGridManager::getLoginURIs(std::vector<std::string>& uris)
{
	getLoginURIs(mGrid, uris);
}

std::string LLGridManager::getHelperURI(const std::string& grid)
{
	std::string helper_uri;
	std::string grid_name = getGrid(grid);
	if (!grid_name.empty())
	{
		helper_uri = mGridList[grid_name][GRID_HELPER_URI_VALUE].asString();
	}
	else
	{
		LL_WARNS("GridManager")<<"invalid grid '"<<grid<<"'"<<LL_ENDL;
	}

	LL_DEBUGS("GridManager")<<"returning "<<helper_uri<<LL_ENDL;
	return helper_uri;
}

std::string LLGridManager::getLoginPage(const std::string& grid)
{
	std::string grid_login_page;
	std::string grid_name = getGrid(grid);
	if (!grid_name.empty())
	{
		grid_login_page = mGridList[grid_name][GRID_LOGIN_PAGE_VALUE].asString();
	}
	else
	{
		LL_WARNS("GridManager")<<"invalid grid '"<<grid<<"'"<<LL_ENDL;
	}
	return grid_login_page;
}

std::string LLGridManager::getLoginPage()
{
	std::string login_page = mGridList[mGrid][GRID_LOGIN_PAGE_VALUE].asString();
	LL_DEBUGS("GridManager")<<"returning "<<login_page<<LL_ENDL;
	return login_page;
}

void LLGridManager::getLoginIdentifierTypes(LLSD& idTypes)
{
	idTypes = mGridList[mGrid][GRID_LOGIN_IDENTIFIER_TYPES];
}

std::string LLGridManager::getGridLoginID()
{
	return mGridList[mGrid][GRID_ID_VALUE];
}

void LLGridManager::updateIsInProductionGrid()
{
	mIsInProductionGrid = false;

	// *NOTE:Mani This used to compare GRID_INFO_AGNI to gGridChoice,
	// but it seems that loginURI trumps that.
	std::vector<std::string> uris;
	getLoginURIs(uris);
	if (uris.empty())
	{
		mIsInProductionGrid = true;
	}
	else
	{
		for ( std::vector<std::string>::iterator uri_it = uris.begin();
			  ! mIsInProductionGrid && uri_it != uris.end();
			  uri_it++
			 )
		{
			if( MAIN_GRID_LOGIN_URI == *uri_it )
			{
				mIsInProductionGrid = true;
			}
		}
	}
}

bool LLGridManager::isInProductionGrid()
{
	return mIsInProductionGrid;
}

bool LLGridManager::isSystemGrid(const std::string& grid)
{
	std::string grid_name = getGrid(grid);

	return (   !grid_name.empty()
			&& mGridList.has(grid)
			&& mGridList[grid].has(GRID_IS_SYSTEM_GRID_VALUE)
			&& mGridList[grid][GRID_IS_SYSTEM_GRID_VALUE].asBoolean()
			);
}

// build a slurl for the given region within the selected grid
std::string LLGridManager::getSLURLBase(const std::string& grid)
{
	std::string grid_base = "";
	std::string grid_name = getGrid(grid);
	if( ! grid_name.empty() && mGridList.has(grid_name) )
	{
		if (mGridList[grid_name].has(GRID_SLURL_BASE))
		{
			grid_base = mGridList[grid_name][GRID_SLURL_BASE].asString();
		}
		else
		{
			grid_base = llformat(DEFAULT_SLURL_BASE, grid_name.c_str());
		}
	}
	LL_DEBUGS("GridManager")<<"returning '"<<grid_base<<"'"<<LL_ENDL;
	return grid_base;
}

// build a slurl for the given region within the selected grid
std::string LLGridManager::getAppSLURLBase(const std::string& grid)
{
	std::string grid_base = "";
	std::string grid_name = getGrid(grid);
	if(!grid_name.empty() && mGridList.has(grid))
	{
		if (mGridList[grid].has(GRID_APP_SLURL_BASE))
		{
			grid_base = mGridList[grid][GRID_APP_SLURL_BASE].asString();
		}
		else
		{
			grid_base = llformat(DEFAULT_APP_SLURL_BASE, grid_name.c_str());
		}
	}
	LL_DEBUGS("GridManager")<<"returning '"<<grid_base<<"'"<<LL_ENDL;
	return grid_base;
}
