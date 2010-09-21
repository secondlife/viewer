/** 
 * @file llfloatersearch.h
 * @author Martin Reddy
 * @brief Search floater - uses an embedded web browser control
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

