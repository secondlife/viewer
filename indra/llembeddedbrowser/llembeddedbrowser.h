/**
 * @file llembeddedbrowser.h
 * @brief Definition of LLEmbeddedBrowser class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#pragma once

#include <string>

#include "llsingleton.h"
#include "llmutex.h"

class LLEmbeddedBrowser;

class LLEmbeddedBrowserUpdateThread :
    public LLThread
{
    public:
        LLEmbeddedBrowserUpdateThread(LLEmbeddedBrowser* browser, unsigned int id)
            : LLThread("EmbeddedBrowserUpdate"),
              mBrowser(browser),
              mBrowserId(id)
        {}

        void run() override;

    private:
        LLEmbeddedBrowser* mBrowser;
        unsigned int mBrowserId;
};

class LLEmbeddedBrowser : public LLSingleton<LLEmbeddedBrowser>
{
    LLSINGLETON(LLEmbeddedBrowser);

    public:
        ~LLEmbeddedBrowser();

        void init();
        void reset();

        unsigned int create(const std::string& url);
        void destroy(unsigned int id);
        void update(unsigned int id);
        void updateAll();
        const unsigned char* getPixels(unsigned int id);
        void navigate(const std::string& url);

    private:
        LLMutex mPixelMutex;
        LLEmbeddedBrowserUpdateThread* mUpdateThread = nullptr;
        unsigned char* mBrowserTabPixels = nullptr;
        unsigned int mBrowserTabWidth = 512;
        unsigned int mBrowserTabHeight = 512;
        unsigned int mBrowserTabDepth = 3;
        std::string mCurrentUrl;
};
