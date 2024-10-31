#pragma once
#include "llswapchain.h"

class LLSwapchainXR : public LLSwapchain
{
    XrSwapchain mSwapchain;
    std::vector<XrSwapchainImageOpenGLKHR> mImages;

public:
    LLSwapchainXR(U32 format, U32 width, U32 height, XrViewConfigurationView viewInfo);
    ~LLSwapchainXR();

    void create(U32 count) override;
    void bind() override;
    void flush() override;

    XrSwapchain getSwapchain() { return mSwapchain; }
};