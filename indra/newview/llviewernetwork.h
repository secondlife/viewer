/** 
 * @file llviewernetwork.h
 * @author James Cook
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

#ifndef LL_LLVIEWERNETWORK_H
#define LL_LLVIEWERNETWORK_H

// @TODO this really should be private, but is used in llslurl
#define MAINGRID "util.agni.lindenlab.com"

/// Exception thrown when a grid is not valid
class LLInvalidGridName
{
public:
	LLInvalidGridName(std::string grid) : mGrid(grid)
	{
	}
	std::string name() { return mGrid; }
protected:
	std::string mGrid;
};

/**
 * @brief A singleton class to manage the grids available to the viewer.
 *
 * This class maintains several properties for each known grid, and provides
 * interfaces for obtaining each of these properties given a specified
 * grid.  Grids are specified by either of two identifiers, each of which
 * must be unique among all known grids:
 * - grid name : DNS name for the grid
 * - grid id   : a short form (conventionally a single word) 
 *
 * This class maintains the currently selected grid, and provides short
 * form accessors for each of the properties of the selected grid.
 **/
class LLGridManager : public LLSingleton<LLGridManager>
{
  public:
	/* ================================================================
	 * @name Initialization and Configuration
	 * @{
	 */
	/// Instantiate the grid manager, load default grids, selects the default grid
	LLGridManager(const std::string& grid_file);
	LLGridManager();
	~LLGridManager();
	
	/// add grids from an external grids file
	void initialize(const std::string& grid_file);
	
	//@}
	
	/* ================================================================
	 * @name Grid Identifiers 
	 * @{
	 * The id is a short form (typically one word) grid name,
	 * It should be used in URL path elements or parameters
	 *
	 * Each grid also has a "label", intented to be a user friendly
	 * descriptive form (it is used in the login panel grid menu, for example).
	 */
	/// Return the name of a grid, given either its name or its id
	std::string getGrid( const std::string &grid );

	/// Get the id (short form selector) for a given grid
	std::string getGridId(const std::string& grid);

	/// Get the id (short form selector) for the selected grid
	std::string getGridId() { return getGridId(mGrid); }

	/// Get the user-friendly long form descriptor for a given grid
	std::string getGridLabel(const std::string& grid);
	
	/// Get the user-friendly long form descriptor for the selected grid
	std::string getGridLabel() { return getGridLabel(mGrid); }

	/// Retrieve a map of grid-name -> label
	std::map<std::string, std::string> getKnownGrids();

	//@}

	/* ================================================================
	 * @name Login related properties
	 * @{
	 */

	/**
	 * Get the login uris for the specified grid.
	 * The login uri for a grid is the target of the authentication request.
	 * A grid may have multple login uris, so they are returned as a vector.
	 */
	void getLoginURIs(const std::string& grid, std::vector<std::string>& uris);
	
	/// Get the login uris for the selected grid
	void getLoginURIs(std::vector<std::string>& uris);
	
	/// Get the URI for webdev help functions for the specified grid
	std::string getHelperURI(const std::string& grid);

	/// Get the URI for webdev help functions for the selected grid
	std::string getHelperURI() { return getHelperURI(mGrid); }

	/// Get the url of the splash page to be displayed prior to login
	std::string getLoginPage(const std::string& grid_name);

	/// Get the URI for the login splash page for the selected grid
	std::string getLoginPage();

	/// Get the id to be used as a short name in url path components or parameters
	std::string getGridLoginID();

	/// Get an array of the login types supported by the grid
	void getLoginIdentifierTypes(LLSD& idTypes);
	/**< the types are "agent" and "avatar";
	 * one means single-name (someone Resident) accounts and other first/last name accounts
	 * I am not sure which is which
	 */

	//@}

	/* ================================================================
	 * @name URL Construction Properties
	 * @{
	 */

	/// Return the slurl prefix (everything up to but not including the region) for a given grid
	std::string getSLURLBase(const std::string& grid);

	/// Return the slurl prefix (everything up to but not including the region) for the selected grid
	std::string getSLURLBase() { return getSLURLBase(mGrid); }
	
	/// Return the application URL prefix for the given grid
	std::string getAppSLURLBase(const std::string& grid);

	/// Return the application URL prefix for the selected grid
	std::string getAppSLURLBase() { return getAppSLURLBase(mGrid); }	

	//@}

	/* ================================================================
	 * @name Selecting the current grid
	 * @{
	 * At initialization, the current grid is set by the first of:
	 * -# The value supplied by the --grid command line option (setting CmdLineGridChoice);
	 *    Note that a default for this may be set at build time.
	 * -# The grid used most recently (setting CurrentGrid)
	 * -# The main grid (Agni)
	 */

	/// Select a given grid as the current grid.  
	void setGridChoice(const std::string& grid);

	/// Returns the name of the currently selected grid 
	std::string getGrid() const { return mGrid; }

	//@}

	/// Is the given grid one of the hard-coded default grids (Agni or Aditi)
	bool isSystemGrid(const std::string& grid);

	/// Is the selected grid one of the hard-coded default grids (Agni or Aditi)
	bool isSystemGrid() { return isSystemGrid(mGrid); }

	/// Is the selected grid a production grid?
	bool isInProductionGrid();
	/**
	 * yes, that's not a very helpful description.
	 * I don't really know why that is different from isSystemGrid()
	 * In practice, the implementation is that it
	 * @returns true if the login uri for the grid is the uri for MAINGRID
	 */

  private:
	
	/// Add a grid to the list of grids 
	bool addGrid(LLSD& grid_info);
	///< @returns true if successfully added

	void updateIsInProductionGrid();

	// helper function for adding the hard coded grids
	void addSystemGrid(const std::string& label, 
					   const std::string& name, 
					   const std::string& login, 
					   const std::string& helper,
					   const std::string& login_page,
					   const std::string& login_id = "");	
	
	
	std::string mGrid;
	std::string mGridFile;
	LLSD mGridList;
	bool mIsInProductionGrid;
};

const S32 MAC_ADDRESS_BYTES = 6;

#endif
