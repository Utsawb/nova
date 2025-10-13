#pragma once
#ifndef COMPUTE_PROGRAM_H
#define COMPUTE_PROGRAM_H

#include <map>
#include <string>
#include <GL/glew.h>

/**
 * @brief An OpenGL Compute Shader Program
 */
class ComputeProgram
{
public:
    ComputeProgram();
    virtual ~ComputeProgram();

    void setVerbose(bool v) { verbose = v; }
    bool isVerbose() const { return verbose; }

    void setShaderName(const std::string &c);
    virtual bool init();
    virtual void bind();
    virtual void unbind();
    void dispatch(GLuint numGroupsX, GLuint numGroupsY = 1, GLuint numGroupsZ = 1);

    void addUniform(const std::string &name);
    GLint getUniform(const std::string &name) const;

protected:
    std::string cShaderName;

private:
    GLuint pid;
    std::map<std::string, GLint> uniforms;
    bool verbose;
};

#endif // COMPUTE_PROGRAM_H

