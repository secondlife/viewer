#include "llswapchain.h"

LLSwapchain::LLSwapchain(U32 format, U32 width, U32 height): mWidth(width), mHeight(height), mFormat(format)
{
}

LLSwapchain::~LLSwapchain()
{
    glDeleteFramebuffers((GLsizei)mFBO.size(), mFBO.data());
}

void LLSwapchain::create(U32 count)
{
	mFBO.resize(count, 0);
    mColorAttachment.resize(count, 0);
	glGenFramebuffers(count, mFBO.data());
}

void LLSwapchain::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO[mCurrentImageIndex]);
}

void LLSwapchain::flush()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}