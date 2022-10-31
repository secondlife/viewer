/** 
 * @file lllivefile.h
 * @brief Automatically reloads a file whenever it changes or is removed.
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

#ifndef LL_LLLIVEFILE_H
#define LL_LLLIVEFILE_H

extern const F32 DEFAULT_CONFIG_FILE_REFRESH;


class LL_COMMON_API LLLiveFile
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
