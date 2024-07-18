/**
 * @file appearance_utility.cpp
 * @author Don Kjer <don@lindenlab.com>, Nyx Linden
 * @brief Utility for processing avatar appearance without a full viewer implementation.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

// linden includes
#include "linden_common.h"
#include "llapr.h"

// project includes
#include "llappappearanceutility.h"

int main(int argc, char** argv)
{
	// Create an application instance.
	ll_init_apr();
	LLAppAppearanceUtility* app = new LLAppAppearanceUtility(argc, argv);

	// Assume success, unless exception is thrown.
	EResult rv = RV_SUCCESS;
	try
	{
		// Process command line and initialize system.
		if (app->init())
		{
			// Run process.
			app->frame();
		}
	}
	catch (LLAppException& e)
	{
		// Deal with errors.
		rv = e.getStatusCode();
	}

	// Clean up application instance.
	app->cleanup();
	delete app;
	ll_cleanup_apr();

	return (int) rv;
}


