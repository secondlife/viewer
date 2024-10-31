#pragma once

#ifdef LL_WINDOWS
#define XR_USE_PLATFORM_WIN32
#include <windows.h>
#include <Unknwn.h>
#endif

#define XR_USE_GRAPHICS_API_OPENGL

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include "llcommon.h"
#include "llmath.h"
#include "v3math.h"
#include "llquaternion.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "llrender.h"

class LLSwapchain
{
protected:
    U32 mHeight = 0;
    U32 mWidth  = 0;
    U32 mFormat = 0;

    std::vector<U32> mFBO;
    std::vector<U32> mColorAttachment;

    // For regular OpenGL this should always be 0.
    // For OpenXR this will increment every time we acquire a new image.
    // On OpenXR this will wrap around to 0 when it reaches the swapchain count.
    U32 mCurrentImageIndex = 0;

public:
    LLSwapchain(U32 format, U32 width, U32 height);
    ~LLSwapchain();

    virtual void create(U32 count);
    virtual void addColorAttachment(U32 index);
    virtual void bind();
    virtual void flush();

    void blitToBuffer(U32 buffer, U32 width, U32 height, bool swap = false);

    U32 getWidth() { return mWidth; }
    U32 getHeight() { return mHeight; }
    U32 getCount() { return (U32)mFBO.size(); }
};
