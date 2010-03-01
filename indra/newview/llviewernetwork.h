/** 
 * @file llviewernetwork.h
 * @author James Cook
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

#ifndef LL_LLVIEWERNETWORK_H
#define LL_LLVIEWERNETWORK_H
                                                                                                       
extern const char* DEFAULT_LOGIN_PAGE;
      
#define GRID_VALUE "name"
#define GRID_LABEL_VALUE "label"
#define GRID_ID_VALUE "grid_login_id"
#define GRID_LOGIN_URI_VALUE "login_uri"
#define GRID_HELPER_URI_VALUE "helper_uri"
#define GRID_LOGIN_PAGE_VALUE "login_page"
#define GRID_IS_SYSTEM_GRID_VALUE "system_grid"
#define GRID_IS_FAVORITE_VALUE "favorite"
#define GRID_LOGIN_CREDENTIAL_PAGE_TYPE_VALUE "credential_type"
#define GRID_LOGIN_CREDENTIAL_PAGE_TYPE_AGENT "agent"
#define GRID_LOGIN_CREDENTIAL_PAGE_TYPE_ACCOUNT "account"
#define MAINGRID "util.agni.lindenlab.com"

// defines slurl formats associated with various grids.
// we need to continue to support existing forms, as slurls
// are shared between viewers that may not understand newer
// forms.
#define GRID_SLURL_BASE "slurl_base"
#define GRID_APP_SLURL_BASE "app_slurl_base"

class LLInvalidGridName
{
public:
	LLInvalidGridName(std::string grid) : mGrid(grid)
	{
	}
protected:
	std::string mGrid;
};


/**
 * @brief A class to manage the grids available to the viewer
 * including persistance.  This class also maintains the currently
 * selected grid.
 * 
 **/
class LLGridManager : public LLSingleton<LLGridManager>
{
public:
	
	// when the grid manager is instantiated, the default grids are automatically
	// loaded, and the grids favorites list is loaded from the xml file.
	LLGridManager(const std::string& grid_file);
	LLGridManager();
	~LLGridManager();
	
	void initialize(const std::string& grid_file);
	// grid list management
	
	// add a grid to the list of grids
	void addGrid(LLSD& grid_info);	

	// retrieve a map of grid-name <-> label
	// by default only return the user visible grids
	std::map<std::string, std::string> getKnownGrids(bool favorites_only=FALSE);
	
	LLSD getGridInfo(const std::string& grid)
	{
		if(mGridList.has(grid))
		{
			return mGridList[grid];
		}
		else
		{
			return LLSD();
		}
	}
	
	// current grid management

	// select a given grid as the current grid.  If the grid
	// is not a known grid, then it's assumed to be a dns name for the
	// grid, and the various URIs will be automatically generated.
	void setGridChoice(const std::string& grid);
	
	
	std::string getGridLabel() { return mGridList[mGrid][GRID_LABEL_VALUE]; } 	
	std::string getGrid() const { return mGrid; }
	void getLoginURIs(std::vector<std::string>& uris);
	std::string getHelperURI() {return mGridList[mGrid][GRID_HELPER_URI_VALUE];}
	std::string getLoginPage() {return mGridList[mGrid][GRID_LOGIN_PAGE_VALUE];}
	std::string getGridLoginID() { return mGridList[mGrid][GRID_ID_VALUE]; }	
	std::string getLoginPage(const std::string& grid) { return mGridList[grid][GRID_LOGIN_PAGE_VALUE]; }
	
	// build a slurl for the given region within the selected grid
	std::string getSLURLBase(const std::string& grid);
	std::string getSLURLBase() { return getSLURLBase(mGrid); }
	
	std::string getAppSLURLBase(const std::string& grid);
	std::string getAppSLURLBase() { return getAppSLURLBase(mGrid); }	
	
	LLSD getGridInfo() { return mGridList[mGrid]; }
	
	std::string getGridByLabel( const std::string &grid_label);
	
	bool isSystemGrid(const std::string& grid) 
	{ 
		return mGridList.has(grid) &&
		      mGridList[grid].has(GRID_IS_SYSTEM_GRID_VALUE) && 
	           mGridList[grid][GRID_IS_SYSTEM_GRID_VALUE].asBoolean(); 
	}
	bool isSystemGrid() { return isSystemGrid(mGrid); }
	// Mark this grid as a favorite that should be persisited on 'save'
	// this is currently used to persist a grid after a successful login
	void setFavorite() { mGridList[mGrid][GRID_IS_FAVORITE_VALUE] = TRUE; }
	
	bool isInProductionGrid();
	void saveFavorites();
	void clearFavorites();

protected:

	// helper function for adding the predefined grids
	void addSystemGrid(const std::string& label, 
					   const std::string& name, 
					   const std::string& login, 
					   const std::string& helper,
					   const std::string& login_page,
					   const std::string& login_id = "");	
	
	
	std::string mGrid;
	std::string mGridFile;
	LLSD mGridList;
};

const S32 MAC_ADDRESS_BYTES = 6;

#endif
