#pragma once
#ifndef EVENT_DATA_H
#define EVENT_DATA_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "MatrixStack.h"
#include "Program.h"
#include "BPMaterial.h"
#include "Mesh.h"
#include "ComputeProgram.h"
#include <dv-processing/io/mono_camera_recording.hpp>

/*
    We can treat the data processed from dv-processing as particles in 3D space
    where the x and y coordinates are the position in the image plane and the z
    coordinate is the timestamp.

    Because timestamp is an int64_t, we should acknowledge possible truncation when
    converting to float.
*/

/**
 * @brief Primary wrapper class to store point cloud event data read in from a .aedat4 file.
 */
class EventData {
    public:
        EventData();
        ~EventData();

        /**
        * @brief Resets the EventData object to its initial state. Clears the vector of particles and resets all other member variables.
        */
        void reset();
        
        /**
         * @brief Initializes instancing (https://learnopengl.com/Advanced-OpenGL/Instancing) for the event particles using the provided shader program.
         * @param progInst
         */
        void initInstancing(Program &instancingProg);
        
        /**
         * @brief Initializes the particles from a file. The file should be in the format of aedat4.
         * @param filename 
         */
        void initParticlesFromFile(const std::string &filename);

        /**
         * @brief Initializes the EventData object in an empty state; upon initialization, no particles are loaded.
         */
        void initParticlesEmpty();

        /**
         * @brief Draw the bounding box wireframe for the event data.
         * @param MV 
         * @param P 
         * @param progBasic 
         */
        void drawBoundingBoxWireframe(MatrixStack &MV, MatrixStack &P, Program &progBasic);
        
        /**
         * @brief Draw event data particles in serial with each particle as a sphere mesh.
         * @param MV 
         * @param P 
         * @param prog 
         * @param particleScale 
         * @param lightPos 
         * @param lightColor 
         * @param lightMat 
         * @param meshSphere 
         */
        void draw(MatrixStack &MV, MatrixStack &P, Program &prog,
            float particleScale, const glm::vec3 &lightPos,
            const BPMaterial &lightMat, const Mesh &meshSphere);
            
        /**
         * @brief Draw event data particles with instancing using gl primitive points
         * @param MV 
         * @param P 
         * @param progInst 
         * @param progBasic 
         * @param particleScale 
         */
        void drawInstanced(MatrixStack &MV, MatrixStack &P, Program &progInst, Program &progBasic,
            float particleScale);
        
        /**
         * @brief Computes the weight of valid events (within shutter) and passes them into the vertex to render DCE
         * @param prog bound to access the associated shaders and uniforms
         * @param viewport_resolution used to compute needed point size
         * @param morlet specifies the contribution function to be used
         * @param freq used to calculate morlet shutter contribution if needed
         * @param pca specifies whether pca is computed and displayed
         */
        void drawFrame(Program &prog, glm::vec2 viewport_resolution, 
            bool morlet, float freq, bool pca);

        
        /**
         * @brief Set the resource directory path for compute shader initialization
         * @param resource_dir Path to resources directory
         */
        void setResourceDir(const std::string &resource_dir);

        /**
         * @brief Initialize GPU compute shader for event processing
         */
        void initComputeShader();

        /**
         * @brief Initialize GPU buffers for compute shader
         */
        void initComputeBuffers();

        /**
         * @brief Used by utils/drawGUI to allow for changing back into time from specified unit of time
         */
        void normalizeTime();
        /**
         * @brief Used by utils/drawGUI to allow for changing from normalized time into a specified unit of time
         */
        void oddizeTime();

        /**
         * @brief Returns the timestamp associated with the event index
         * @param eventIndex 
         * @param oddFactor controls whether the timestamp is normalized or in a specific unit 
         * @return float 
         */
        float getTimestamp(uint eventIndex, float oddFactor = 1.0f) const;
        /**
         * @brief Get the first event index after or equal to the timestamp
         * @param timestamp 
         * @param normFactor controls whether the timestamp must be normalized
         * @return uint 
         */
        uint getFirstEvent(float timestamp, float normFactor = 1.0f) const;
        /**
         * @brief Get the first event index before or equal to the timestamp
         * @param timestamp 
         * @param normFactor controls whether the timestamp must be normalized
         * @return uint 
         */
        uint getLastEvent(float timestamp, float normFactor = 1.0f) const;

        const float getDiffScale() const { return diffScale; }
        const glm::vec3 &getCenter() const { return center; }
        const glm::vec3 getMin_XYZ() const { return minXYZ; }
        const glm::vec3 getMax_XYZ() const { return maxXYZ; }
        const float &getMaxTimestamp() const { return maxXYZ.z; }
        const float &getMinTimestamp() const { return minXYZ.z; }
        const uint getMaxEvent() const { return static_cast<const uint>(evtParticles.size()); }
        
        float &getTimeWindow_L() { return timeWindow_L; }
        float &getTimeWindow_R() { return timeWindow_R; }
        uint &getEventWindow_L() { return eventWindow_L; }
        uint &getEventWindow_R() { return eventWindow_R; }
        glm::vec4 &getSpaceWindow() { return spaceWindow; }
        float &getTimeShutterWindow_L() { return timeShutterWindow_L; }
        float &getTimeShutterWindow_R() { return timeShutterWindow_R; }
        uint &getEventShutterWindow_L() { return eventShutterWindow_L; }
        uint &getEventShutterWindow_R() { return eventShutterWindow_R; }
        int &getShutterType() { return shutterType; }
        bool &getIsPositiveOnly() { return isPositiveOnly; }
        int &getUnitType() { return unitType; }

        glm::vec3 &getNegColor() { return negColor; }
        glm::vec3 &getPosColor() { return posColor; }

        static inline int TIME_CONVERSION; 
        static const int TIME_SHUTTER = 0; // values must match ImGui::Combo order in utils.cpp
        static const int EVENT_SHUTTER = 1;
        static inline uint modFreq = 1; // only draw the modFreq'th particle of the ones we read in
    private:
        glm::vec2 camera_resolution;
        float diffScale;

        // TODO: Might be better to just store a std::bitset for polarity, and something dynamic like a color
        // indicator for a (although we would need a vec3 for a full RGB)
        std::vector<glm::vec4> evtParticles; // x, y, t, polarity (false=0.0, true=1.0)

        long long earliestTimestamp;
        long long latestTimestamp;

        float timeWindow_L;
        float timeWindow_R;
        uint eventWindow_L;
        uint eventWindow_R;

        // Maybe move to FrameViewportFBO or own helper struct
        int shutterType;
        float timeShutterWindow_L;
        float timeShutterWindow_R;
        uint eventShutterWindow_L;
        uint eventShutterWindow_R;

        glm::vec4 spaceWindow; // x = top, y = right, z = bottom, w = left 

        // We can define a bounding box and thus center to rotate around
        glm::vec3 minXYZ;
        glm::vec3 maxXYZ;
        glm::vec3 center;

        // Instancing
        GLuint instVBO;

        glm::vec3 negColor;
        glm::vec3 posColor;

        bool isPositiveOnly;
        int unitType;

        // GPU Compute resources
        ComputeProgram computeProg;
        GLuint evtParticlesSSBO;
        GLuint outputDataSSBO;
        GLuint countersSSBO;
        bool computeInitialized;
        std::string resourceDir;
};

#endif // EVENT_DATA_H