/** 
 * @file llfloatersearch.h
 * @author Martin Reddy
 * @brief Search floater - uses an embedded web browser control
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLFLOATERSEARCH_H
#define LL_LLFLOATERSEARCH_H

#include "llfloater.h"
#include "llviewermediaobserver.h"

#include <string>

class LLMediaCtrl;

///
/// The search floater allows users to perform all search operations.
/// All search functionality is now implemented via web services and
/// so this floater simply embeds a web view and displays the search
/// web page. The browser control is explicitly marked as "trusted"
/// so that the user can click on teleport links in search results.
///
class LLFloaterSearch : 
	public LLFloater, 
	public LLViewerMediaObserver
{
public:
	LLFloaterSearch(const LLSD& key);

	/// show the search floater with a new search
	/// see search() for details on the key parameter.
	/*virtual*/ void onOpen(const LLSD& key);

	/*virtual*/ void onClose(bool app_quitting);

	/// perform a search with the specific search term.
	/// The key should be a map that can contain the following keys:
	///  - "id": specifies the text phrase to search for
	///  - "category": one of "all" (default), "people", "places",
	///    "events", "groups", "wiki", "destinations", "classifieds"
	void search(const LLSD &key);

	/// changing godmode can affect the search results that are
	/// returned by the search website - use this method to tell the
	/// search floater that the user has changed god level.
	void godLevelChanged(U8 godlevel);

private:
	/*virtual*/ BOOL postBuild();

	// inherited from LLViewerMediaObserver
	/*virtual*/ void handleMediaEvent(LLPluginClassMedia *self, EMediaEvent event);

	LLMediaCtrl *mBrowser;
	LLSD        mCategoryPaths;
	U8          mSearchGodLevel;
};

#endif  // LL_LLFLOATERSEARCH_H

