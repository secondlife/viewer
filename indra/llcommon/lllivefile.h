/** 
 * @file lllivefile.h
 * @brief Automatically reloads a file whenever it changes or is removed.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#ifndef LL_LLLIVEFILE_H
#define LL_LLLIVEFILE_H

extern const F32 DEFAULT_CONFIG_FILE_REFRESH;


class LLLiveFile
{
public:
	LLLiveFile(const std::string& filename, const F32 refresh_period = 5.f);
	virtual ~LLLiveFile();

	/**
	 * @brief Check to see if this live file should reload.
	 *
	 * Call this before using anything that was read & cached
	 * from the file.
	 *
	 * This method calls the <code>loadFile()</code> method if
	 * any of:
	 *   file has a new modify time since the last check
	 *   file used to exist and now does not
	 *   file used to not exist but now does
	 * @return Returns true if the file was reloaded.
	 */
	bool checkAndReload();
	

	std::string filename() const;

	/**
	 * @brief Add this live file to an automated recheck.
	 *
	 * Normally, just calling checkAndReload() is enough. In some
	 * cases though, you may need to let the live file periodically
	 * check itself.
	 */
	void addToEventTimer();

	void setRefreshPeriod(F32 seconds);

protected:
	/**
	 * @breif Implement this to load your file if it changed.
	 *
	 * This method is called automatically by <code>checkAndReload()</code>,
	 * so though you must implement this in derived classes, you do
	 * not need to call it manually.
	 * @return Returns true if the file was successfully loaded.
	 */
	virtual bool loadFile() = 0;

	/**
	 * @brief Implement this method if you want to get a change callback.
	 *
	 * This virtual function will be called automatically at the end
	 * of <code>checkAndReload()</code> if a new configuration was
	 * loaded. This does not track differences between the current and
	 * newly loaded file, so any successful load event will trigger a
	 * <code>changed()</code> callback. Default is to do nothing.
	 */
	virtual void changed() {}

private:
	class Impl;
	Impl& impl;
};

#endif //LL_LLLIVEFILE_H
