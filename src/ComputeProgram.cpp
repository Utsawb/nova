#include "ComputeProgram.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "GLSL.h"

ComputeProgram::ComputeProgram() : pid(0), verbose(true) {}

ComputeProgram::~ComputeProgram() {
    if (pid) {
        glDeleteProgram(pid);
    }
}

void ComputeProgram::setShaderName(const std::string &c) { cShaderName = c; }

bool ComputeProgram::init() {
    // Read compute shader source
    std::ifstream cFile(cShaderName);
    if (!cFile.is_open()) {
        std::cerr << "Failed to open compute shader file: " << cShaderName
                  << std::endl;
        return false;
    }

    std::stringstream cStream;
    cStream << cFile.rdbuf();
    std::string cSource = cStream.str();
    const char *cSourcePtr = cSource.c_str();

    // Create and compile compute shader
    GLuint cShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cShader, 1, &cSourcePtr, nullptr);
    glCompileShader(cShader);

    // Check for compile errors
    GLint success;
    glGetShaderiv(cShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(cShader, 512, nullptr, infoLog);
        std::cerr << "Compute shader compilation failed:\n"
                  << infoLog << std::endl;
        glDeleteShader(cShader);
        return false;
    }

    if (verbose) {
        std::cout << "Compute shader compiled successfully: " << cShaderName
                  << std::endl;
    }

    // Create program and attach shader
    pid = glCreateProgram();
    if (pid == 0) {
        std::cerr << "Error creating compute shader program" << std::endl;
        return false;
    }
    glAttachShader(pid, cShader);
    glLinkProgram(pid);

    // Check for linking errors
    glGetProgramiv(pid, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(pid, 512, nullptr, infoLog);
        std::cerr << "Compute program linking failed:\n"
                  << infoLog << std::endl;
        glDeleteShader(cShader);
        return false;
    }

    // Clean up shader (no longer needed after linking)
    glDeleteShader(cShader);

    if (verbose) {
        std::cout << "Compute program linked successfully" << std::endl;
    }

    return true;
}

void ComputeProgram::bind() { glUseProgram(pid); }

void ComputeProgram::unbind() { glUseProgram(0); }

void ComputeProgram::dispatch(GLuint numGroupsX, GLuint numGroupsY,
                              GLuint numGroupsZ) {
    glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
    if (numGroupsX == 0 || numGroupsY == 0 || numGroupsZ == 0) {
        cout << "Warning: dispatching compute shader with zero work groups" << endl;
    }
    // Memory barrier to ensure compute shader writes are visible
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void ComputeProgram::addUniform(const std::string &name) {
    if (pid == 0) {
        cout << "Error: program not initialized - cannot add uniform" << endl;
        return;
    }
    GLint loc = glGetUniformLocation(pid, name.c_str());
    if (loc < 0 && verbose) {
        std::cerr << "Warning: uniform '" << name
                  << "' not found in compute shader" << std::endl;
    }
    uniforms[name] = loc;
}

GLint ComputeProgram::getUniform(const std::string &name) const {
    auto it = uniforms.find(name);
    if (it != uniforms.end()) {
        return it->second;
    }
    return -1;
}
