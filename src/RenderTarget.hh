#pragma once

#ifndef RENDER_TARGET_HH
#define RENDER_TARGET_HH

#include "pch.hh"

struct RenderTarget
{
    SDL_GPUTexture *texture;
    unsigned width;
    unsigned height;
};

#endif