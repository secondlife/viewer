/**
 * @file llfloaterembeddedbrowsertest.cpp
 * @author Callum Prentice
 * @brief Test floater for developing the embedded web browser in the viewer
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llviewertexture.h"
#include "llembeddedbrowser.h"

#include "llfloaterembeddedbrowsertest.h"

LLFloaterEmbeddedBrowserTest::LLFloaterEmbeddedBrowserTest(const LLSD& key)
    : LLFloater("floater_embedded_browser_test")
{
    mMemoryBufferTexWidth = 0;
    mMemoryBufferTexHeight = 0;
    mMemoryBufferSwatchLeft = 0;
    mMemoryBufferSwatchTop = 0;
    mMemoryBufferSwatchWidth = 0;
    mMemoryBufferSwatchHeight = 0;
    mMemoryBufferSwatchDepth = 4;
    mMemoryBufferRaw = nullptr;
    mMemoryBufferSwatch = nullptr;
}

LLFloaterEmbeddedBrowserTest::~LLFloaterEmbeddedBrowserTest()
{
}

bool LLFloaterEmbeddedBrowserTest::postBuild()
{
    LLEmbeddedBrowser::getInstance()->init();

    // Buttons that add or remove a new tab
    getChild<LLUICtrl>("add_tab_btn")->setCommitCallback(boost::bind(&LLFloaterEmbeddedBrowserTest::onAddTabBtn, this));
    getChild<LLUICtrl>("rem_tab_btn")->setCommitCallback(boost::bind(&LLFloaterEmbeddedBrowserTest::onRemTabBtn, this));

    // Buttons that browse to a specific URL (red, green, blue for now) in current browser tab
    getChild<LLUICtrl>("browse_red_btn")->setCommitCallback(boost::bind(&LLFloaterEmbeddedBrowserTest::onBrowseBtn, this, "red"));
    getChild<LLUICtrl>("browse_green_btn")->setCommitCallback(boost::bind(&LLFloaterEmbeddedBrowserTest::onBrowseBtn, this, "green"));
    getChild<LLUICtrl>("browse_blue_btn")->setCommitCallback(boost::bind(&LLFloaterEmbeddedBrowserTest::onBrowseBtn, this, "blue"));

    // Close the floater
    getChild<LLUICtrl>("close_btn")->setCommitCallback(boost::bind(&LLFloaterEmbeddedBrowserTest::onCloseBtn, this));

    // Do this here versus the constructor so that the floater size has
    // been established and we can eventually get correct size for the swatch
    createUI();

    LLEmbeddedBrowser::getInstance()->create("red", mMemoryBufferSwatchWidth, mMemoryBufferSwatchHeight);

    return true;
}

void LLFloaterEmbeddedBrowserTest::onAddTabBtn()
{
    std::cout << "LLFloaterEmbeddedBrowserTest::onAddTabBtn()" << std::endl;
}

void LLFloaterEmbeddedBrowserTest::onRemTabBtn()
{
    std::cout << "LLFloaterEmbeddedBrowserTest::onRemTabBtn()" << std::endl;
}

void LLFloaterEmbeddedBrowserTest::onCloseBtn()
{
    LLEmbeddedBrowser::getInstance()->destroy(0);

    closeFloater();
}

void LLFloaterEmbeddedBrowserTest::onBrowseBtn(const std::string url)
{
    std::cout << "LLFloaterEmbeddedBrowserTest::onBrowseBtn() - url: " << url << std::endl;

    LLEmbeddedBrowser::getInstance()->navigate(url);
}

void LLFloaterEmbeddedBrowserTest::draw()
{
    LLFloater::draw();

    if (mMemoryBufferSwatch != nullptr)
    {
        const unsigned char* src = LLEmbeddedBrowser::getInstance()->getPixels(0);
        if (src != nullptr)
        {
            unsigned char* dst = mMemoryBufferRaw->getData();

            const unsigned int src_stride = mMemoryBufferSwatchWidth * mMemoryBufferSwatchDepth;
            const unsigned int dst_stride = mMemoryBufferTexWidth * mMemoryBufferSwatchDepth;

            for (unsigned int y = 0; y < mMemoryBufferSwatchHeight; ++y)
            {
                memcpy(dst + y * dst_stride, src + y * src_stride, src_stride);
            }

            mMemoryBufferSwatch->setSubImage(mMemoryBufferRaw, 0, 0, mMemoryBufferTexWidth, mMemoryBufferTexHeight);

            const F32 u_max = (F32)mMemoryBufferSwatchWidth / (F32)mMemoryBufferTexWidth;
            const F32 v_max = (F32)mMemoryBufferSwatchHeight / (F32)mMemoryBufferTexHeight;

            gGL.getTexUnit(0)->bind(mMemoryBufferSwatch);
            gGL.color4f(1.f, 1.f, 1.f, 1.f);
            gGL.begin(LLRender::TRIANGLES);
            {
                S32 left = mMemoryBufferSwatchLeft;
                S32 top = mMemoryBufferSwatchTop;
                S32 right = left + mMemoryBufferSwatchWidth;
                S32 bottom = top - mMemoryBufferSwatchHeight;

                gGL.texCoord2f(0.f, 0.f);
                gGL.vertex2i(left, top);
                gGL.texCoord2f(0.f, v_max);
                gGL.vertex2i(left, bottom);
                gGL.texCoord2f(u_max, v_max);
                gGL.vertex2i(right, bottom);

                gGL.texCoord2f(0.f, 0.f);
                gGL.vertex2i(left, top);
                gGL.texCoord2f(u_max, v_max);
                gGL.vertex2i(right, bottom);
                gGL.texCoord2f(u_max, 0.f);
                gGL.vertex2i(right, top);
            }
            gGL.end();
        }
    }
}

static unsigned int nextPowerOfTwo(unsigned int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

void LLFloaterEmbeddedBrowserTest::createUI()
{
    const unsigned int margin = 16; // margin around the swatch on all sides
    const unsigned int legacy_height = 18; // the height of the legacy header area (title bar, etc.)
    const unsigned int button_height = 30; // the height consumed by buttons and their at the bottom of the floater

    LLRect floater_rect = getRect();
    mMemoryBufferSwatchLeft = margin;
    mMemoryBufferSwatchTop = floater_rect.getHeight() - (legacy_height + margin);

    mMemoryBufferSwatchWidth = floater_rect.getWidth() - (2 * margin);
    mMemoryBufferSwatchHeight = floater_rect.getHeight() - (2 * margin + legacy_height + button_height);

    mMemoryBufferTexWidth = nextPowerOfTwo(mMemoryBufferSwatchWidth);
    mMemoryBufferTexHeight = nextPowerOfTwo(mMemoryBufferSwatchHeight);

    mMemoryBufferRaw = new LLImageRaw(mMemoryBufferTexWidth, mMemoryBufferTexHeight, mMemoryBufferSwatchDepth);
    mMemoryBufferRaw->clear(0, 0, 0, 255);

    mMemoryBufferSwatch = LLViewerTextureManager::getLocalTexture((LLImageRaw*)mMemoryBufferRaw, false);
    gGL.getTexUnit(0)->bind(mMemoryBufferSwatch);
    mMemoryBufferSwatch->setAddressMode(LLTexUnit::TAM_CLAMP);
}
