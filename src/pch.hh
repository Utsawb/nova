#pragma once
#ifndef PCH_HH
#define PCH_HH

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>
#include <memory>
#include <print>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_gpu.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <backends/imgui_impl_sdlgpu3_shaders.h>

#endif // PCH_HH