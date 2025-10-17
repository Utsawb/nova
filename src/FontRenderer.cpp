/*
    FontRenderer Implementation
    Uses stb_truetype to render TrueType fonts in OpenGL
*/

#include "FontRenderer.h"
#include "Program.h"
#include "MatrixStack.h"
#include "GLSL.h"
#include <fstream>
#include <cstring>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

FontRenderer::FontRenderer(const std::string &ttf_path) 
    : fontPath(ttf_path), textProgram(nullptr), fontBuffer(nullptr), 
      bitmapAtlas(nullptr), atlasWidth(512), atlasHeight(512), 
      fontSize(32.0f), textureID(0), vaoID(0), vboID(0), initialized(false)
{
}

FontRenderer::~FontRenderer() {
    if (fontBuffer) {
        delete[] fontBuffer;
    }
    if (bitmapAtlas) {
        delete[] bitmapAtlas;
    }
    if (textureID) {
        glDeleteTextures(1, &textureID);
    }
    if (vboID) {
        glDeleteBuffers(1, &vboID);
    }
    if (vaoID) {
        glDeleteVertexArrays(1, &vaoID);
    }
}

void FontRenderer::loadFont() {
    // Read the font file
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open font file: " << fontPath << std::endl;
        return;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    fontBuffer = new unsigned char[size];
    if (!file.read((char*)fontBuffer, size)) {
        std::cerr << "Failed to read font file" << std::endl;
        delete[] fontBuffer;
        fontBuffer = nullptr;
        return;
    }
    file.close();
    
    // Initialize the font
    if (!stbtt_InitFont(&fontInfo, fontBuffer, 0)) {
        std::cerr << "Failed to initialize font" << std::endl;
        delete[] fontBuffer;
        fontBuffer = nullptr;
        return;
    }
    
    // Create bitmap atlas for ASCII characters (32-126)
    bitmapAtlas = new unsigned char[atlasWidth * atlasHeight];
    
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);
    
    int x = 2; // Start with some padding
    int y = 2;
    int maxRowHeight = 0;
    
    // Clear the atlas
    memset(bitmapAtlas, 0, atlasWidth * atlasHeight);
    
    // Bake each character into the atlas
    for (int c = 32; c < 127; c++) {
        int advance, lsb, x0, y0, x1, y1;
        stbtt_GetCodepointHMetrics(&fontInfo, c, &advance, &lsb);
        stbtt_GetCodepointBitmapBox(&fontInfo, c, scale, scale, &x0, &y0, &x1, &y1);
        
        int charWidth = x1 - x0;
        int charHeight = y1 - y0;
        
        // Move to next row if we run out of space
        if (x + charWidth + 2 >= atlasWidth) {
            x = 2;
            y += maxRowHeight + 2;
            maxRowHeight = 0;
        }
        
        // Check if we have vertical space
        if (y + charHeight + 2 >= atlasHeight) {
            std::cerr << "Font atlas too small!" << std::endl;
            break;
        }
        
        // Render the character into the atlas
        stbtt_MakeCodepointBitmap(&fontInfo, 
                                  bitmapAtlas + x + y * atlasWidth,
                                  charWidth, charHeight, atlasWidth,
                                  scale, scale, c);
        
        // Store glyph information
        CharGlyph glyph;
        glyph.x0 = (float)x / atlasWidth;
        glyph.y0 = (float)y / atlasHeight;
        glyph.x1 = (float)(x + charWidth) / atlasWidth;
        glyph.y1 = (float)(y + charHeight) / atlasHeight;
        glyph.xoff = (float)x0;
        glyph.yoff = (float)y0;
        glyph.advance = scale * advance;
        glyph.width = (float)charWidth;
        glyph.height = (float)charHeight;
        
        glyphs[(char)c] = glyph;
        
        x += charWidth + 2;
        maxRowHeight = std::max(maxRowHeight, charHeight);
    }
}

void FontRenderer::createTexture() {
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Upload the bitmap atlas
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 
                 0, GL_RED, GL_UNSIGNED_BYTE, bitmapAtlas);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void FontRenderer::setupBuffers() {
    // Create VAO and VBO
    glGenVertexArrays(1, &vaoID);
    glGenBuffers(1, &vboID);
    
    glBindVertexArray(vaoID);
    glBindBuffer(GL_ARRAY_BUFFER, vboID);
    
    // Reserve space for vertices (will be updated per character)
    // Each quad needs 6 vertices * (2 pos + 2 tex) = 24 floats per character
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4 * 256, nullptr, GL_DYNAMIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    // Texture coordinate attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void FontRenderer::init() {
    if (initialized) return;
    
    loadFont();
    if (!fontBuffer) {
        std::cerr << "Font loading failed" << std::endl;
        return;
    }
    
    createTexture();
    setupBuffers();
    
    initialized = true;
    
    GLSL::checkError(GET_FILE_LINE);
}

void FontRenderer::setProgram(Program *prog) {
    textProgram = prog;
}

glm::mat4 FontRenderer::computeTextTransform(const glm::vec3 &position, const glm::vec3 &normal) {
    // Create a coordinate system where the text faces the normal direction
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    
    // If normal is too close to up vector, use a different up vector
    if (std::abs(glm::dot(normal, up)) > 0.99f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    
    // Create orthonormal basis
    glm::vec3 right = glm::normalize(glm::cross(up, normal));
    glm::vec3 actualUp = glm::cross(normal, right);
    
    // Build the transform matrix
    glm::mat4 transform(1.0f);
    transform[0] = glm::vec4(right, 0.0f);
    transform[1] = glm::vec4(actualUp, 0.0f);
    transform[2] = glm::vec4(normal, 0.0f);
    transform[3] = glm::vec4(position, 1.0f);
    
    return transform;
}

void FontRenderer::drawText(const std::string &text, const glm::vec3 &position, 
                           const glm::vec3 &normal, float scale, const glm::vec3 &color,
                           MatrixStack &P, MatrixStack &MV) {
    if (!initialized || !textProgram) {
        std::cerr << "FontRenderer not initialized or no shader program set" << std::endl;
        return;
    }
    
    if (text.empty()) return;
    
    // Calculate total text width for centering
    float totalWidth = 0.0f;
    for (char c : text) {
        if (glyphs.find(c) != glyphs.end()) {
            totalWidth += glyphs[c].advance;
        }
    }
    
    // Build vertices for all characters
    std::vector<float> vertices;
    vertices.reserve(text.length() * 6 * 4); // 6 vertices, 4 floats each
    
    float xpos = -totalWidth * 0.5f * scale; // Start from left to center the text
    float ypos = 0.0f;
    
    for (char c : text) {
        if (glyphs.find(c) == glyphs.end()) {
            continue;
        }
        
        CharGlyph &glyph = glyphs[c];
        
        float x = xpos + glyph.xoff * scale;
        float y = ypos + glyph.yoff * scale;
        float w = glyph.width * scale;
        float h = glyph.height * scale;
        
        // First triangle
        vertices.push_back(x);     vertices.push_back(y + h);
        vertices.push_back(glyph.x0); vertices.push_back(glyph.y0);
        
        vertices.push_back(x);     vertices.push_back(y);
        vertices.push_back(glyph.x0); vertices.push_back(glyph.y1);
        
        vertices.push_back(x + w); vertices.push_back(y);
        vertices.push_back(glyph.x1); vertices.push_back(glyph.y1);
        
        // Second triangle
        vertices.push_back(x);     vertices.push_back(y + h);
        vertices.push_back(glyph.x0); vertices.push_back(glyph.y0);
        
        vertices.push_back(x + w); vertices.push_back(y);
        vertices.push_back(glyph.x1); vertices.push_back(glyph.y1);
        
        vertices.push_back(x + w); vertices.push_back(y + h);
        vertices.push_back(glyph.x1); vertices.push_back(glyph.y0);
        
        xpos += glyph.advance * scale;
    }
    
    // Update VBO with vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vboID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // Compute transform and update model-view matrix
    glm::mat4 textTransform = computeTextTransform(position, normal);
    MV.pushMatrix();
    MV.multMatrix(textTransform);
    
    // Bind shader and set uniforms
    textProgram->bind();
    glUniformMatrix4fv(textProgram->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P.topMatrix()));
    glUniformMatrix4fv(textProgram->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV.topMatrix()));
    glUniform3f(textProgram->getUniform("textColor"), color.r, color.g, color.b);
    
    // Enable blending for text rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(textProgram->getUniform("fontTexture"), 0);
    
    // Draw the text
    glBindVertexArray(vaoID);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertices.size() / 4));
    glBindVertexArray(0);
    
    glDisable(GL_BLEND);
    textProgram->unbind();
    
    MV.popMatrix();
    
    GLSL::checkError(GET_FILE_LINE);
}

