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
    mMemoryBufferRaw = nullptr;
    mMemoryBufferSwatch = nullptr;
}

LLFloaterEmbeddedBrowserTest::~LLFloaterEmbeddedBrowserTest()
{
}

bool LLFloaterEmbeddedBrowserTest::postBuild()
{
    LLEmbeddedBrowser::getInstance()->init();

    LLEmbeddedBrowser::getInstance()->create("red");

    // Buttons that add or remove a new tab
    getChild<LLUICtrl>("add_tab_btn")->setCommitCallback(boost::bind(&LLFloaterEmbeddedBrowserTest::onAddTabBtn, this));
    getChild<LLUICtrl>("rem_tab_btn")->setCommitCallback(boost::bind(&LLFloaterEmbeddedBrowserTest::onRemTabBtn, this));

    // Buttons that browse to a specific URL (red, green, blue for now) in current browser tab
    getChild<LLUICtrl>("browse_red_btn")->setCommitCallback(boost::bind(&LLFloaterEmbeddedBrowserTest::onBrowseBtn, this, "red"));
    getChild<LLUICtrl>("browse_green_btn")->setCommitCallback(boost::bind(&LLFloaterEmbeddedBrowserTest::onBrowseBtn, this, "green"));
    getChild<LLUICtrl>("browse_blue_btn")->setCommitCallback(boost::bind(&LLFloaterEmbeddedBrowserTest::onBrowseBtn, this, "blue"));

    // Close the floater
    getChild<LLUICtrl>("close_btn")->setCommitCallback(boost::bind(&LLFloaterEmbeddedBrowserTest::onCloseBtn, this));

    // Do this here vss th constructor so that the floater size has
    // been established and we can get correct size for the swatch
    createUI();

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
        unsigned char* bits = mMemoryBufferRaw->getData();
        if (bits != nullptr)
        {
            memcpy(bits, LLEmbeddedBrowser::getInstance()->getPixels(0),
                   mMemoryBufferSwatchWidth * mMemoryBufferSwatchHeight * mMemoryBufferSwatchDepth);

            // TODO: make 512 a variable once non-power-of-2 textures are supported in gl_draw_scaled_image_with_border
            mMemoryBufferSwatch->setSubImage(mMemoryBufferRaw, 0, 0, 512, 512);

            gl_draw_scaled_image_with_border(
                mMemoryBufferSwatchLeft, mMemoryBufferSwatchTop - mMemoryBufferSwatchHeight,
                mMemoryBufferSwatchWidth, mMemoryBufferSwatchHeight,
                mMemoryBufferSwatch,
                LLColor4::white
            );
        }
    }
}

void LLFloaterEmbeddedBrowserTest::createUI()
{
    const unsigned int margin = 16;
    const unsigned int legacy_height = 18;

    LLRect floater_rect = getRect();
    mMemoryBufferSwatchLeft = margin;
    mMemoryBufferSwatchTop = floater_rect.getHeight() - (legacy_height + margin);
    mMemoryBufferSwatchWidth = 512; // TODO: gl_draw_scaled_image_with_border with non-power of 2 textures
    mMemoryBufferSwatchHeight = 512;
    mMemoryBufferSwatchDepth = 3;

    mMemoryBufferRaw = new LLImageRaw(mMemoryBufferSwatchWidth, mMemoryBufferSwatchHeight, mMemoryBufferSwatchDepth);

    mMemoryBufferSwatch = LLViewerTextureManager::getLocalTexture((LLImageRaw*)mMemoryBufferRaw, false);
    gGL.getTexUnit(0)->bind(mMemoryBufferSwatch);
    mMemoryBufferSwatch->setAddressMode(LLTexUnit::TAM_CLAMP);
}
