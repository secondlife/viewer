
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

#include <cstring>
#include <functional>
#include <iostream>
#include <vector>

#include "linden_common.h"

#include "llembeddedbrowser.h"

#include "llthread.h"

// Fills one full row of `width` pixels (each `depth` bytes) alternating between
// first_color/second_color every `checker_size` pixels. Builds each color block via
// exponential-doubling memcpy (write one pixel, then repeatedly double the filled
// span) instead of a per-pixel loop, so the whole row is laid down with O(log
// checker_size) memcpy calls instead of O(width) individual pixel writes.
static void fillCheckerRow(unsigned char* row, unsigned int width, unsigned int depth, unsigned int checker_size,
                            const unsigned char* first_color, const unsigned char* second_color)
{
    unsigned int x = 0;
    bool use_first = true;
    while (x < width)
    {
        unsigned int block_pixels = llmin(checker_size, width - x);
        const unsigned char* color = use_first ? first_color : second_color;
        unsigned char* block_start = row + (size_t)x * depth;

        memcpy(block_start, color, depth);
        unsigned int filled = 1;
        while (filled < block_pixels)
        {
            unsigned int copy_count = llmin(filled, block_pixels - filled);
            memcpy(block_start + (size_t)filled * depth, block_start, (size_t)copy_count * depth);
            filled += copy_count;
        }

        x += block_pixels;
        use_first = !use_first;
    }
}

LLEmbeddedBrowserTab::LLEmbeddedBrowserTab(LLEmbeddedBrowser* browser, unsigned int id, const std::string& url, unsigned int width, unsigned int height) :
    mWidth(width),
    mHeight(height),
    mCurrentUrl(url),
    mRng(std::random_device{}() ^ (unsigned int)id)
{
    mPixels = new unsigned char[mWidth * mHeight * mDepth];

    mUpdateThread = std::make_unique<LLEmbeddedBrowserUpdateThread>(browser, id);
    mUpdateThread->start();
}

LLEmbeddedBrowserTab::~LLEmbeddedBrowserTab()
{
    if (mUpdateThread)
    {
        mUpdateThread->shutdown();
        mUpdateThread.reset();
    }

    delete[] mPixels;
    mPixels = nullptr;
}

void LLEmbeddedBrowserTab::update()
{
    LLMutexLock lock(&mPixelMutex);

    // Draw a checkerboard pattern with colors based on the current URL: "red"/"green"/"blue"
    // get a randomized single-channel color (as before); any other URL gets a stable,
    // always-visible two-tone pattern derived from a hash of the URL itself. Uses a
    // per-tab RNG rather than the CRT's global rand(), since each tab now updates on
    // its own thread and rand() isn't guaranteed thread-safe across platforms.
    auto rand_in = [this](unsigned int n) { return std::uniform_int_distribution<unsigned int>(0, n - 1)(mRng); };

    const unsigned int checker_size = 16 + rand_in(64);
    unsigned char color_a[4] = { 0, 0, 0, 255 };
    unsigned char color_b[4] = { 0, 0, 0, 255 };

    if (mCurrentUrl == "red" || mCurrentUrl == "green" || mCurrentUrl == "blue")
    {
        unsigned int channel = (mCurrentUrl == "red") ? 0 : (mCurrentUrl == "green") ? 1 : 2;
        color_a[channel] = (unsigned char)(64 + rand_in(128));
        color_b[channel] = (unsigned char)(192 + rand_in(64));
    }
    else
    {
        std::hash<std::string> hasher;
        size_t hash = hasher(mCurrentUrl);
        for (unsigned int c = 0; c < 3; ++c)
        {
            unsigned char base = (unsigned char)((hash >> (c * 8)) & 0xFF);
            color_a[c] = base / 2;
            color_b[c] = (unsigned char)(255 - (base / 2));
        }
    }

    // Blocks repeat every checker_size rows, so only two distinct row patterns ever
    // occur (one starting with color_a, one starting with color_b). Build each once,
    // then lay down every output row with a single whole-row memcpy -- this replaces
    // the O(width*height) per-pixel loop with O(height) large, vectorizable memcpy
    // calls (plus the two O(log checker_size) row builds), which keeps mPixelMutex
    // held for a small fraction of the time on large (e.g. 4096x4096) buffers.
    std::vector<unsigned char> row_starts_a((size_t)mWidth * mDepth);
    std::vector<unsigned char> row_starts_b((size_t)mWidth * mDepth);
    fillCheckerRow(row_starts_a.data(), mWidth, mDepth, checker_size, color_a, color_b);
    fillCheckerRow(row_starts_b.data(), mWidth, mDepth, checker_size, color_b, color_a);

    for (unsigned int y = 0; y < mHeight; ++y)
    {
        bool row_starts_with_a = ((y / checker_size) % 2 == 0);
        const unsigned char* src = row_starts_with_a ? row_starts_a.data() : row_starts_b.data();
        memcpy(mPixels + (size_t)y * mWidth * mDepth, src, (size_t)mWidth * mDepth);
    }
}

const unsigned char* LLEmbeddedBrowserTab::getPixels()
{
    LLMutexLock lock(&mPixelMutex);

    return mPixels;
}

bool LLEmbeddedBrowserTab::copyPixels(std::vector<unsigned char>& out_pixels, unsigned int& out_width, unsigned int& out_height)
{
    LLMutexLock lock(&mPixelMutex);

    out_width = mWidth;
    out_height = mHeight;
    out_pixels.assign(mPixels, mPixels + (size_t)mWidth * mHeight * mDepth);
    return true;
}

void LLEmbeddedBrowserTab::navigate(const std::string& url)
{
    LLMutexLock lock(&mPixelMutex);
    mCurrentUrl = url;
}

void LLEmbeddedBrowserTab::resize(unsigned int width, unsigned int height)
{
    LLMutexLock lock(&mPixelMutex);

    if (width == mWidth && height == mHeight)
    {
        return;
    }

    delete[] mPixels;
    mWidth = width;
    mHeight = height;
    mPixels = new unsigned char[mWidth * mHeight * mDepth]();
}

unsigned int LLEmbeddedBrowserTab::getWidth() const
{
    LLMutexLock lock(&mPixelMutex);
    return mWidth;
}

unsigned int LLEmbeddedBrowserTab::getHeight() const
{
    LLMutexLock lock(&mPixelMutex);
    return mHeight;
}

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
}

void LLEmbeddedBrowser::reset()
{
    LLMutexLock lock(&mTabsMutex);
    mTabs.clear();
}

std::shared_ptr<LLEmbeddedBrowserTab> LLEmbeddedBrowser::findTab(unsigned int id)
{
    LLMutexLock lock(&mTabsMutex);
    auto it = mTabs.find(id);
    return (it != mTabs.end()) ? it->second : nullptr;
}

unsigned int LLEmbeddedBrowser::create(const std::string& url, unsigned int width, unsigned int height)
{
    width = llmin(width, mMaxWidth);
    height = llmin(height, mMaxHeight);

    LLMutexLock lock(&mTabsMutex);
    unsigned int id = mNextTabId++;
    mTabs[id] = std::make_shared<LLEmbeddedBrowserTab>(this, id, url, width, height);
    return id;
}

void LLEmbeddedBrowser::setMaxDimensions(unsigned int max_width, unsigned int max_height)
{
    mMaxWidth = max_width;
    mMaxHeight = max_height;
}

void LLEmbeddedBrowser::destroy(unsigned int id)
{
    LLMutexLock lock(&mTabsMutex);
    mTabs.erase(id);
}

void LLEmbeddedBrowser::resize(unsigned int id, unsigned int width, unsigned int height)
{
    width = llmin(width, mMaxWidth);
    height = llmin(height, mMaxHeight);

    if (auto tab = findTab(id))
    {
        tab->resize(width, height);
    }
}

void LLEmbeddedBrowser::update(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        tab->update();
    }
}

void LLEmbeddedBrowser::updateAll()
{
    std::vector<std::shared_ptr<LLEmbeddedBrowserTab>> tabs;
    {
        LLMutexLock lock(&mTabsMutex);
        tabs.reserve(mTabs.size());
        for (auto& entry : mTabs)
        {
            tabs.push_back(entry.second);
        }
    }
    for (auto& tab : tabs)
    {
        tab->update();
    }
}

const unsigned char* LLEmbeddedBrowser::getPixels(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        return tab->getPixels();
    }
    return nullptr;
}

bool LLEmbeddedBrowser::copyPixels(unsigned int id, std::vector<unsigned char>& out_pixels, unsigned int& out_width, unsigned int& out_height)
{
    if (auto tab = findTab(id))
    {
        return tab->copyPixels(out_pixels, out_width, out_height);
    }
    return false;
}

unsigned int LLEmbeddedBrowser::getWidth(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        return tab->getWidth();
    }
    return 0;
}

unsigned int LLEmbeddedBrowser::getHeight(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        return tab->getHeight();
    }
    return 0;
}

void LLEmbeddedBrowser::navigate(unsigned int id, const std::string& url)
{
    if (auto tab = findTab(id))
    {
        tab->navigate(url);
    }
}

void LLEmbeddedBrowserUpdateThread::run()
{
    // Scale the update rate down for large tabs so the per-frame full-buffer fill/lock
    // cost stays roughly bounded regardless of tab size: small tabs (e.g. <= 512x512)
    // run at max_fps, and the rate falls off as pixel count grows, floored at min_fps.
    const unsigned int max_fps = 60;
    const unsigned int min_fps = 10;
    const unsigned long long budget_pixels_per_sec = 512ull * 512ull * max_fps;

    while (! isQuitting())
    {
        mBrowser->update(mBrowserId);

        unsigned long long pixels = (unsigned long long)mBrowser->getWidth(mBrowserId) * (unsigned long long)mBrowser->getHeight(mBrowserId);

        unsigned int fps = max_fps;
        if (pixels > 0)
        {
            unsigned long long computed_fps = llclamp(budget_pixels_per_sec / pixels, (unsigned long long)min_fps, (unsigned long long)max_fps);
            fps = (unsigned int)computed_fps;
        }

        ms_sleep(1000 / fps);
    }
}
