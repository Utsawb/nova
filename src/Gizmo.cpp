#include "Gizmo.h"
#include <iostream>

void Gizmo::BeginFrameDockerCompatable() {
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
}

void Gizmo::EditCamera(const Camera& camera, ImVec2 size) {

    float x_distance = .8f;
    float scale = .7f;
    ImVec2 backWindowPos  = ImGui::GetWindowPos();
    ImVec2 backContentSz  = size;

    ImGuiViewport *viewport = ImGui::GetMainViewport();


    // we want the gizmo to be in the right corner
    ImVec2 gizmoWindowPos = ImVec2(backWindowPos.x + x_distance * backContentSz.x, backWindowPos.y);
    ImVec2 gizmoContentSz = ImVec2(backContentSz.x * (1-scale), backContentSz.y * (1-scale));

    ImGuizmo::SetRect(gizmoWindowPos.x, gizmoWindowPos.y, gizmoContentSz.x, gizmoContentSz.y);

    BeginFrameDockerCompatable();

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 5.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        gizmoContentSz.x / gizmoContentSz.y,  // match window aspect
        0.1f,
        100.0f
    );

    glm::mat4 identity = glm::mat4(1.0f); // static gizmo transform

    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        ImGuizmo::ROTATE,
        ImGuizmo::WORLD,
        glm::value_ptr(identity)
    );
}