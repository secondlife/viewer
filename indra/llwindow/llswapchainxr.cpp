#include "llswapchainxr.h"
#include "llxrmanager.h"
#include "llrender.h"
#include "llgl.h"

LLSwapchainXR::LLSwapchainXR(U32 format, U32 width, U32 height, XrViewConfigurationView viewInfo) : LLSwapchain(format, width, height)
{
	auto manager = LLXRManager::getInstance();

	XrSwapchainCreateInfo swapchainCreateInfo = { XR_TYPE_SWAPCHAIN_CREATE_INFO, nullptr };
    swapchainCreateInfo.createFlags           = 0;
    swapchainCreateInfo.usageFlags            = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.format                = format;
    swapchainCreateInfo.sampleCount           = viewInfo.recommendedSwapchainSampleCount;
    swapchainCreateInfo.width                 = viewInfo.recommendedImageRectWidth;
    swapchainCreateInfo.height                = viewInfo.recommendedImageRectHeight;
    swapchainCreateInfo.faceCount             = 1;
    swapchainCreateInfo.arraySize             = 1;
    swapchainCreateInfo.mipCount              = 1;

    if (XR_FAILED(xrCreateSwapchain(manager->getXRSession(), &swapchainCreateInfo, &mSwapchain)))
    {
        LL_ERRS("XRManager") << "Failed to create Color Swapchain." << LL_ENDL;
        return;
    }

    U32 colorSwapchainImageCount = 0;
    if (XR_FAILED(xrEnumerateSwapchainImages(mSwapchain, 0, &colorSwapchainImageCount, nullptr)))
    {
        LL_ERRS("XRManager") << "Failed to enumerate Color Swapchain Images." << LL_ENDL;
        return;
    }

    create(colorSwapchainImageCount);
}

LLSwapchainXR::~LLSwapchainXR()
{
    xrDestroySwapchain(mSwapchain);
}

void LLSwapchainXR::create(U32 count)
{
    mImages.resize(count, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR, nullptr });

    if(XR_FAILED(xrEnumerateSwapchainImages(mSwapchain, count, &count, (XrSwapchainImageBaseHeader*)mImages.data())))
	{
		LL_ERRS("XRManager") << "Failed to enumerate Color Swapchain Images." << LL_ENDL;
		return;
	}

    LLSwapchain::create(count);

    for (U8 i = 0; i < count; i++)
    {
        mColorAttachment[i] = mImages[i].image;
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorAttachment[i], 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void LLSwapchainXR::bind()
{
    XrSwapchainImageAcquireInfo acquireInfo = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, nullptr };

    if (XR_FAILED(xrAcquireSwapchainImage(mSwapchain, &acquireInfo, &mCurrentImageIndex)))
    {
        LL_ERRS("XRManager") << "Failed to acquire Swapchain Image." << LL_ENDL;
        return;
    }

    XrSwapchainImageWaitInfo waitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, nullptr };
    waitInfo.timeout                  = XR_INFINITE_DURATION;
    if (XR_FAILED(xrWaitSwapchainImage(mSwapchain, &waitInfo)))
    {
        LL_ERRS("XRManager") << "Failed to wait for Swapchain Image." << LL_ENDL;
        return;
    }

    LLSwapchain::bind();

    stop_glerror();
}

void LLSwapchainXR::flush()
{
    LLSwapchain::flush();

    XrSwapchainImageReleaseInfo releaseInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, nullptr };
    if (XR_FAILED(xrReleaseSwapchainImage(mSwapchain, &releaseInfo)))
    {
        LL_ERRS("XRManager") << "Failed to release Swapchain Image." << LL_ENDL;
        return;
    }
}