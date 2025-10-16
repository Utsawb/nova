#include "Gizmo.h"
#include <iostream>

void Gizmo::EditCamera(Camera& camera, ImVec2 size) {

    float x_distance = .8f;
    float scale = .7f;
    ImVec2 backWindowPos  = ImGui::GetWindowPos();
    ImVec2 backContentSz  = size;

    ImGuiViewport *viewport = ImGui::GetMainViewport();


    // we want the gizmo to be in the right corner
    ImVec2 gizmoWindowPos = ImVec2(backWindowPos.x + x_distance * backContentSz.x, backWindowPos.y);
    ImVec2 gizmoContentSz = ImVec2(backContentSz.x * (1-scale), backContentSz.y * (1-scale));
    ImGuizmo::SetRect(gizmoWindowPos.x, gizmoWindowPos.y, gizmoContentSz.x, gizmoContentSz.y);

    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());

    glm::mat4 view = camera.calcLookAt();

    glm::mat4 projection = camera.calcProjectionMatrix();

    glm::mat4 identity = glm::mat4(1.0f); // static gizmo transform

    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        ImGuizmo::ROTATE,
        ImGuizmo::WORLD,
        glm::value_ptr(identity)
    );

    ImGui::SetItemAllowOverlap();

    if (ImGuizmo::IsUsing()) {
        // Extract rotation from identity matrix
        glm::vec3 forward = glm::normalize(glm::vec3(identity[2])); // Z column
        camera.gizmoUpdate(forward);
    }
}
