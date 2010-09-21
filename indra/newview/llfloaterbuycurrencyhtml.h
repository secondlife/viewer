/** 
 * @file llfloaterbuycurrencyhtml.h
 * @brief buy currency implemented in HTML floater - uses embedded media browser control
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

#ifndef LL_LLFLOATERBUYCURRENCYHTML_H
#define LL_LLFLOATERBUYCURRENCYHTML_H

#include "llfloater.h"
#include "llmediactrl.h"

class LLFloaterBuyCurrencyHTML : 
	public LLFloater, 
	public LLViewerMediaObserver
{
	public:
		LLFloaterBuyCurrencyHTML( const LLSD& key );

		/*virtual*/ BOOL postBuild();
		/*virtual*/ void onClose( bool app_quitting );

		// inherited from LLViewerMediaObserver
		/*virtual*/ void handleMediaEvent( LLPluginClassMedia* self, EMediaEvent event );

		// allow our controlling parent to tell us paramters
		void setParams( bool specific_sum_requested, const std::string& message, S32 sum );

		// parse and construct URL and set browser to navigate there.
		void navigateToFinalURL();

	private:
		LLMediaCtrl* mBrowser;
		bool mSpecificSumRequested;
		std::string mMessage;
		S32 mSum;
};

#endif  // LL_LLFLOATERBUYCURRENCYHTML_H
