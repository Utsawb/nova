#include "Gizmo.h"

void Gizmo::EditCamera(const Camera& camera, matrix_t& matrix) {
    ImGuizmo::BeginFrame();
}

void Gizmo::EditCamera(const Camera& camera, float viewportWidth) {

    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    // Match ImGuizmo rect to the Main Viewport ImGui window
    ImVec2 windowPos  = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);

    ImGuizmo::BeginFrame();


    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 5.0f),   // Camera position
        glm::vec3(0.0f, 0.0f, 0.0f),   // Look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)    // Up vector
    );

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),           // Field of view
        1.0f,                           // Aspect ratio (width / height)
        0.1f,                           // Near plane
        100.0f                          // Far plane
    );

    // Example: Draw small static gizmo in top-right corner
    // glm::mat4 view = camera.GetViewMatrix();
    // glm::mat4 proj = camera.GetProjectionMatrix();
    glm::mat4 identity = glm::mat4(1.0f);

    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        ImGuizmo::ROTATE,
        ImGuizmo::WORLD,
        glm::value_ptr(identity)
    );
}