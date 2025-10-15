#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>  // <-- contains lookAt() and perspective()
#include <glm/gtc/type_ptr.hpp>          // <-- contains value_ptr()
#include "Camera.h"
#include "imgui.h"
#include "ImGuizmo.h"

using matrix_t = glm::mat4;

class Gizmo {
    public:
    static void EditCamera(const Camera& camera, matrix_t& matrix);
    static void EditCamera(const Camera& camera, float viewport);
};