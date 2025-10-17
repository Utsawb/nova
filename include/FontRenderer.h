/*
    FontRenderer - A class for rendering TrueType text in 3D using stb_truetype
    
    Usage:
        FontRenderer renderer("resources/CascadiaCode.ttf");
        renderer.init();
        renderer.drawText("Hello World", position, normal, scale, color, P, MV);
*/

#pragma once
#ifndef FONTRENDERER_H
#define FONTRENDERER_H

#include "include.h"
#include "stb_truetype.h"
#include <map>

class Program;
class MatrixStack;

/**
 * @brief Character glyph information
 */
struct CharGlyph {
    float x0, y0, x1, y1;  // Texture coordinates
    float xoff, yoff;       // Offset from baseline
    float advance;          // Horizontal advance
    float width, height;    // Glyph dimensions
};

/**
 * @brief Font renderer using stb_truetype for text rendering in 3D space
 */
class FontRenderer {
public:
    FontRenderer(const std::string &ttf_path);
    ~FontRenderer();
    
    /**
     * @brief Initialize OpenGL resources (call after OpenGL context is created)
     */
    void init();
    
    /**
     * @brief Draw text in 3D space
     * @param text The text string to render
     * @param position The center position of the text
     * @param normal The normal vector (text faces this direction)
     * @param scale Scale factor for the text size
     * @param color Text color (RGB)
     * @param P Projection matrix stack
     * @param MV Model-view matrix stack
     */
    void drawText(const std::string &text, const glm::vec3 &position, 
                  const glm::vec3 &normal, float scale, const glm::vec3 &color,
                  MatrixStack &P, MatrixStack &MV);
    
    /**
     * @brief Set the shader program for text rendering
     * @param prog The shader program to use
     */
    void setProgram(Program *prog);
    
private:
    void loadFont();
    void createTexture();
    void setupBuffers();
    glm::mat4 computeTextTransform(const glm::vec3 &position, const glm::vec3 &normal);
    
    std::string fontPath;
    Program *textProgram;
    
    // Font data
    unsigned char *fontBuffer;
    stbtt_fontinfo fontInfo;
    unsigned char *bitmapAtlas;
    int atlasWidth;
    int atlasHeight;
    float fontSize;
    
    // Character glyph information
    std::map<char, CharGlyph> glyphs;
    
    // OpenGL resources
    GLuint textureID;
    GLuint vaoID;
    GLuint vboID;
    bool initialized;
};

#endif // FONTRENDERER_H
