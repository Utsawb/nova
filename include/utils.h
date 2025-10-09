#pragma once
#ifndef UTILS_H
#define UTILS_H

#include "include.h"

#include <glm/glm.hpp>
#include <string>
#include <memory>

class Program;
class BaseViewportFBO;
class EventData;

/**
 * @brief Struct to hold context information for the GLFW window. This allows for callback functions to access information within other scopes.
 */
struct WindowContext {
    Camera* camera;
    bool* is_cursorVisible;
    bool* key_toggles;
    bool* is_mainViewportHovered;
    BaseViewportFBO* mainSceneFBO;
    BaseViewportFBO* frameSceneFBO;
};

std::string OpenFileDialog();

Program genPhongProg(const std::string &resource_dir);
Program genInstProg(const std::string &resource_dir);
Program genBasicProg(const std::string &resource_dir);

Program genTextureProg(const std::string &resource_dir);

void sendToPhongShader(const Program &prog, const MatrixStack &P, const MatrixStack &MV, const vec3 &lightPos, const vec3 &lightCol, const BPMaterial &mat);
void sendToTextureShader(const Program& prog, const MatrixStack& P, const MatrixStack& MV);

// GLFW Callbacks //
void error_callback(int error, const char *description);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void cursor_position_callback(GLFWwindow *window, double xmouse, double ymouse);
void char_callback(GLFWwindow *window, unsigned int key);
void resize_callback(GLFWwindow *window, int width, int height);
bool genBiggestWindow(GLFWwindow *&window, const std::string &window_name="GLFW Window");

// ImGui //
void initImGuiStyle(ImGuiStyle &style);
void drawGUIDockspace();

/**
 * @brief Calls drawGUIDockspace. Displays all FBOs and processing options. Populates parameter classes with user input
 * @param camera 
 * @param fps 
 * @param particle_scale 
 * @param is_mainViewportHovered 
 * @param mainSceneFBO 
 * @param frameScenceFBO 
 * @param evtData 
 * @param datafilepath 
 * @param video_name 
 * @param recording 
 * @param datadirectory 
 * @param loadFile 
 */
void drawGUI(const Camera& camera, float fps, float &particle_scale, float &maxZ, bool &is_mainViewportHovered,
    BaseViewportFBO &mainSceneFBO, FrameViewportFBO &frameScenceFBO, std::shared_ptr<EventData> &evtData, std::string &datafilepath, 
    std::string &video_name, bool &recording, std::string& datadirectory, bool &loadFile, bool &dataStreamed, bool &resetStream, bool &pauseStream, float &particleTimeDensity);

float randFloat();
glm::vec3 randXYZ();
void genVBO(GLuint &vbo, size_t num_bytes, size_t draw_type=GL_STATIC_DRAW);

#define printvec3(var) pv3(#var, var)
inline void pv3(const char *varname, const glm::vec3 &vec) {
    printf("%s: %f, %f, %f\n", varname, vec.x, vec.y, vec.z);
}

#define printmat3(var) pm3(#var, var)
inline void pm3(const char *varname, const glm::mat3 &mat) {
    printf("%s:\n", varname);
    printf("%f, %f, %f\n", mat[0][0], mat[0][1], mat[0][2]);
    printf("%f, %f, %f\n", mat[1][0], mat[1][1], mat[1][2]);
    printf("%f, %f, %f\n", mat[2][0], mat[2][1], mat[2][2]);
}


#endif // UTILS_H