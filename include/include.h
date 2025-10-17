#pragma once
#ifndef INCLUDE_H
#define INCLUDE_H

/* TODO: Doubt it's an immediate concern, but if it is we can remove this file. It is better to just
        declare in the file what you need in addition to forward declarations where you can in header files.
*/

// Constants & STL
#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <vector>
#include <random>

// GLEW / GLFW
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Eigen / GLM //
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Eigen/Dense>

// Dear ImGui //
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

// CUSTOM CLASSES //
#include "GLSL.h"
#include "Camera.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Mesh.h"
#include "BPMaterial.h"
#include "EventData.h"
#include "MainScene.h"
#include "frameScene.h"
#include "ContributionFunc.h"
#include "FontRenderer.h"

// UTILS //
#include "utils.h"

#endif