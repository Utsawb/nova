#include "MainScene.h"
#include <iostream>
#include <GL/glew.h>

BaseViewportFBO::BaseViewportFBO() : fbo(0), colorTexture(0), depthRBO(0), width(0), height(0), dirtyBit(false) {}

BaseViewportFBO::~BaseViewportFBO()
{
    if (fbo != 0)
    {
        glDeleteFramebuffers(1, &fbo);
    }
    if (colorTexture != 0)
    {
        glDeleteTextures(1, &colorTexture);
    }
    if (depthRBO != 0)
    {
        glDeleteRenderbuffers(1, &depthRBO);
    }
}

bool BaseViewportFBO::initialize(int w, int h, bool frame)
{
    dirtyBit = true;

    width = w;
    height = h;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);

    if (frame)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr); // Note: GL_R32F possible if color does not change
    }
    else
    { // TODO ask if better to just pick higher resolution
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, colorTexture, 0);

    glGenRenderbuffers(1, &depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, depthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << __LINE__ << ":" << __FILE__ << ": Framebuffer initialization failed" << std::endl;
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void BaseViewportFBO::resize(int w, int h, bool frame)
{ // FIXME use overriding/hiding
    initialize(w, h, frame);
}

void BaseViewportFBO::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void BaseViewportFBO::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint BaseViewportFBO::getColorTexture() const
{
    return colorTexture;
}
