#include "pch.hh"

#include "ParameterStore.hh"

#pragma once
#ifndef APP_CONTEXT_HH
#define APP_CONTEXT_HH

struct AppContext
{
    SDL_Window *window = nullptr;
    SDL_GPUDevice *gpu_device = nullptr;
    ParameterStore *parameter_store = nullptr;
};

#endif