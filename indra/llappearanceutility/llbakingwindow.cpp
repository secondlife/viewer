/**
 * @file llbakingwindow.cpp
 * @brief Implementation of LLBakingWindow class.
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

// project includes
#include "llappappearanceutility.h"
#include "llbakingshadermgr.h"
#include "llbakingwindow.h"
#include "llgl.h"
#include "llgltexture.h"
#include "llvertexbuffer.h"

LLBakingWindow::LLBakingWindow(S32 width, S32 height)
{
    const S32 WINDOW_ORIGIN_X = 0;
    const S32 WINDOW_ORIGIN_Y = 0;
    const U32 FLAGS = 32; // *TODO: Why did mapserver use this?  mFlags looks unused.
    const bool NO_FULLSCREEN = FALSE;
    const bool NO_CLEAR_BG = FALSE;
    const bool NO_DISABLE_VSYNC = FALSE;
    const bool NO_IGNORE_PIXEL_DEPTH = FALSE;
    const bool USE_GL = TRUE;
    mWindow = LLWindowManager::createWindow(this,
        "appearanceutility", "Appearance Utility",
        WINDOW_ORIGIN_X, WINDOW_ORIGIN_Y,
        width, height,
        FLAGS,
        NO_FULLSCREEN,
        NO_CLEAR_BG,
        NO_DISABLE_VSYNC, //gSavedSettings.getBOOL("DisableVerticalSync"),
        USE_GL, // not headless
        NO_IGNORE_PIXEL_DEPTH); //gIgnorePixelDepth = FALSE

    if (NULL == mWindow)
    {
        throw LLAppException(RV_UNABLE_TO_INIT_GL);
    }

    LLVertexBuffer::initClass(mWindow);

    gGL.init(true);

    if (!LLBakingShaderMgr::sInitialized)
    { //immediately initialize shaders
        LLBakingShaderMgr::sInitialized = TRUE;
        LLBakingShaderMgr::instance()->setShaders();
    }

    LLImageGL::initClass(mWindow, LLGLTexture::MAX_GL_IMAGE_CATEGORY, TRUE, FALSE);

    // mWindow->show();
    // mWindow->bringToFront();
    // mWindow->focusClient();
    // mWindow->gatherInput();
}

LLBakingWindow::~LLBakingWindow()
{
    if (mWindow)
    {
        LLWindowManager::destroyWindow(mWindow);
    }
    mWindow = NULL;
}

void LLBakingWindow::swapBuffers()
{
    mWindow->swapBuffers();
}
