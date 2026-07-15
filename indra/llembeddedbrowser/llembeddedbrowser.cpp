
/**
 * @file llembeddedbrowser.cpp
 * @brief Implementation of LLEmbeddedBrowser class
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

#include <iostream>

#include "linden_common.h"

#include "llembeddedbrowser.h"


#include "llthread.h"

LLEmbeddedBrowser::LLEmbeddedBrowser()
{
    std::cout << "LLEmbeddedBrowser created" << std::endl;
}

LLEmbeddedBrowser::~LLEmbeddedBrowser()
{
    std::cout << "LLEmbeddedBrowser destroyed" << std::endl;
}

void LLEmbeddedBrowser::init()
{
    std::cout << "Initializing LLEmbeddedBrowser" << std::endl;

    mCurrentUrl = "red";
}

void LLEmbeddedBrowser::reset()
{

}

unsigned int LLEmbeddedBrowser::create(const std::string& url)
{
    mBrowserTabPixels = new unsigned char[mBrowserTabWidth * mBrowserTabHeight * mBrowserTabDepth];

    mCurrentUrl = url;

    mUpdateThread = new LLEmbeddedBrowserUpdateThread(this, 0 /* id */);
    mUpdateThread->start();

    return 0;
}

void LLEmbeddedBrowser::destroy(unsigned int id)
{
    if (mUpdateThread)
    {
        std::cout << "@@@ shutting down LLEmbeddedBrowserUpdateThread" << std::endl;
        mUpdateThread->shutdown();   // public: signals quit + wakes the thread
        delete mUpdateThread;
        mUpdateThread = nullptr;
    }
}

void LLEmbeddedBrowser::update(unsigned int id)
{
    LLMutexLock lock(&mPixelMutex);

    for (unsigned int i = 0; i < mBrowserTabWidth * mBrowserTabHeight * mBrowserTabDepth; i += mBrowserTabDepth)
    {
        mBrowserTabPixels[i + 0] = mCurrentUrl == "red" ? rand() % 256 : 0;
        mBrowserTabPixels[i + 1] = mCurrentUrl == "green" ? rand() % 256 : 0;
        mBrowserTabPixels[i + 2] = mCurrentUrl == "blue" ? rand() % 256 : 0;
    }
}

void LLEmbeddedBrowser::updateAll()
{

}

const unsigned char* LLEmbeddedBrowser::getPixels(unsigned int id)
{
    LLMutexLock lock(&mPixelMutex);

    return mBrowserTabPixels;
}

void LLEmbeddedBrowser::navigate(const std::string& url)
{
    mCurrentUrl = url;
}

void LLEmbeddedBrowserUpdateThread::run()
{
    unsigned int frame_rate = 5;

    while (! isQuitting())
    {
        mBrowser->update(mBrowserId);
        ms_sleep(1000 / frame_rate);
    }
}
