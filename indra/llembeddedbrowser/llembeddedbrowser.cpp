
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

    const unsigned int checkerSize = 16 + rand() % 64;
    unsigned char color_a[4] =
    {
        mCurrentUrl == "red" ? (unsigned char)(64 + rand() % 128) : (unsigned char)0,
        mCurrentUrl == "green" ? (unsigned char)(64 + rand() % 128) : (unsigned char)0,
        mCurrentUrl == "blue" ? (unsigned char)(64 + rand() % 128) : (unsigned char)0,
        255
    };
    unsigned char color_b[4] =
    {
        mCurrentUrl == "red" ? (unsigned char)(192 + rand() % 64) : (unsigned char)0,
        mCurrentUrl == "green" ? (unsigned char)(192 + rand() % 64) : (unsigned char)0,
        mCurrentUrl == "blue" ? (unsigned char)(192 + rand() % 64) : (unsigned char)0,
        255
    };

    for (unsigned int y = 0; y < mBrowserTabHeight; ++y)
    {
        for (unsigned int x = 0; x < mBrowserTabWidth; ++x)
        {
            unsigned char* pixel = ((x / checkerSize) + (y / checkerSize)) % 2 == 0 ? color_a : color_b;

            size_t offset = (y * mBrowserTabWidth + x) * mBrowserTabDepth;
            for (unsigned int c = 0; c < mBrowserTabDepth; ++c)
            {
                mBrowserTabPixels[offset + c] = pixel[c];
            }
        }
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
    unsigned int frame_rate = 10;

    while (! isQuitting())
    {
        mBrowser->update(mBrowserId);
        ms_sleep(1000 / frame_rate);
    }
}
