#include "llswapchain.h"

LLSwapchain::LLSwapchain(U32 format, U32 width, U32 height): mWidth(width), mHeight(height), mFormat(format)
{
}

LLSwapchain::~LLSwapchain()
{
    glDeleteTextures((GLsizei)mColorAttachment.size(), mColorAttachment.data());
    glDeleteFramebuffers((GLsizei)mFBO.size(), mFBO.data());
}

void LLSwapchain::create(U32 count)
{
	mFBO.resize(count, 0);
    mColorAttachment.resize(count, 0);
	glGenFramebuffers(count, mFBO.data());
}

void LLSwapchain::addColorAttachment(U32 index)
{
    U32 tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, mFormat, mWidth, mHeight, 0, GL_SRGB_ALPHA, GL_UNSIGNED_BYTE, nullptr);
    mColorAttachment[index] = tex;
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO[index]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorAttachment[index], 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LLSwapchain::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO[mCurrentImageIndex]);
}

void LLSwapchain::flush()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LLSwapchain::blitToBuffer(U32 buffer, U32 width, U32 height, bool swap)
{
    U32 src = mFBO[mCurrentImageIndex];
    U32 dst = buffer;
    U32 srcWidth = mWidth;
    U32 srcHeight = mHeight;

    if (swap)
    {
        U32 buffer = dst;
        dst        = src;
        src        = buffer;

        srcWidth = width;
        srcHeight = height;
        width     = mWidth;
        height    = mHeight;
    }

    glBlitNamedFramebuffer(src, dst, 0, 0, mWidth, mWidth, 0, 0, mWidth, mWidth, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}