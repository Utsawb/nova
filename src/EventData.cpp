#include "EventData.h"
#include "utils.h"

#include <algorithm>
#include <cstdio>
#include <dv-processing/core/utils.hpp>
#include <omp.h>
#include <windows.h>

using std::vector, std::cout, std::endl;

// do in order of declaration below
EventData::EventData() : camera_resolution(0.0f), diffScale(0.0f),
    earliestTimestamp(0), latestTimestamp(0), shutterType(TIME_SHUTTER), 
    timeWindow_L(0.0f), timeWindow_R(0.0f), eventWindow_L(0), eventWindow_R(0),
    timeShutterWindow_L(0.0f), timeShutterWindow_R(0.0f), eventShutterWindow_L(0),
    eventShutterWindow_R(0), spaceWindow(0.0f), minXYZ(std::numeric_limits<float>::max()),
    maxXYZ(std::numeric_limits<float>::lowest()), center(0.0f), negColor({1.0f, 0.0f, 0.0f}), 
    posColor({0.0f, 1.0f, 0.0f}), isPositiveOnly(false), unitType(1), evtParticlesSSBO(0), outputDataSSBO(0), countersSSBO(0), computeInitialized(false) {}

EventData::~EventData() {
    if (instVBO) {
        glDeleteBuffers(1, &instVBO);
        instVBO = 0;
    }

    if (instVBO) {
        glDeleteBuffers(1, &instVBO);
        instVBO = 0;
    }
    
    if (evtParticlesSSBO) {
        glDeleteBuffers(1, &evtParticlesSSBO);
        evtParticlesSSBO = 0;
    }
    
    if (outputDataSSBO) {
        glDeleteBuffers(1, &outputDataSSBO);
        outputDataSSBO = 0;
    }
    
    if (countersSSBO) {
        glDeleteBuffers(1, &countersSSBO);
        countersSSBO = 0;
    }
}

void EventData::reset() {
    // TODO: Do we want to free the memory? Because if we go from like 100'000 particles -> 10 we should. Otherwise, better to keep
    evtParticles.clear();
    earliestTimestamp = 0;
    latestTimestamp = 0;
    minXYZ = glm::vec3(std::numeric_limits<float>::max());
    maxXYZ = glm::vec3(std::numeric_limits<float>::lowest());
    center = glm::vec3(0.0f);
    timeWindow_L = -1.0f;
    timeWindow_R = -1.0f;
    spaceWindow = glm::vec4(0.0f);

    if (instVBO) {
        glDeleteBuffers(1, &instVBO);
        instVBO = 0;
    }
}

void EventData::initInstancing(Program &progInst) {
    // Generate / initialize a VBO here. GL_STATIC_DRAW may be better, should test
    genVBO(instVBO, evtParticles.size() * sizeof(glm::vec4), GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, instVBO);
    GLint aInstPos = progInst.getAttribute("aInstPos");

    // Update the vertex attribute pointer every 1 * sizeof(glm::vec4) bytes
    glEnableVertexAttribArray(aInstPos);
    glVertexAttribPointer(aInstPos, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);
    glVertexAttribDivisor(aInstPos, 1); // Update once per instance (not per vertex)
    
    // Pass in the existing data
    glBufferSubData(GL_ARRAY_BUFFER, 0, evtParticles.size() * sizeof(glm::vec4), evtParticles.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void EventData::initParticlesFromFile(const std::string &filename) {
    dv::io::MonoCameraRecording reader(filename);
    camera_resolution = glm::vec2(reader.getEventResolution().value().width, reader.getEventResolution().value().height);

    // If someone calls init again, we should always reset
    reset();

    // https://dv-processing.inivation.com/rel_1_7/reading_data.html#read-events-from-a-file
    uint counter = 0; // Necessary for modFreq;
    while (reader.isRunning()) {
        if (const auto events = reader.getNextEventBatch(); events.has_value()) {
            for (auto &evt : events.value()) {
                if (counter++ % modFreq != 0) { continue; } // TODO instead skip batch if possible

                long long evtTimestamp = evt.timestamp();
                if (evtParticles.empty()) {
                    earliestTimestamp = evtTimestamp;
                }
                
                // Assumedly the last event batch and event has the latest timestamp, but not sure - so use max(...)
                latestTimestamp = std::max(latestTimestamp, evtTimestamp);

                // We can sort of "normalize" the timestamp to start at 0 this way.
                float relativeTimestamp = static_cast<float>(evtTimestamp - earliestTimestamp);
                glm::vec4 evt_xytp = glm::vec4(
                    static_cast<float>(evt.x()),
                    static_cast<float>(evt.y()),
                    relativeTimestamp,
                    static_cast<float>(evt.polarity()) // (float)true == 1.0f, (float)false == 0.0f
                );

                evtParticles.push_back(evt_xytp);

                // glm::min/max does componentwise; .x = min(.x, candidate_x), .y = min(.y, candidate_y), ... 
                minXYZ = glm::min(minXYZ, glm::vec3(evt_xytp));
                maxXYZ = glm::max(maxXYZ, glm::vec3(evt_xytp));
            }
        }
    }

    // TODO: This is arbitrary, we can should define as a constant somewhere
    // Apply scale
    this->diffScale = 5000.0f / static_cast<float>(latestTimestamp - earliestTimestamp);
    for (auto &evt : evtParticles) {
        evt.z *= diffScale;
    }

    // Normalize the timestamp of the min/max XYZ for bounding box
    this->minXYZ.z *= diffScale;
    this->maxXYZ.z *= diffScale;
    this->center = 0.5f * (minXYZ + maxXYZ);
    
    this->spaceWindow = glm::vec4(minXYZ.y, maxXYZ.x, maxXYZ.y, minXYZ.x);

    printf("Loaded %zu particles from %s\n", evtParticles.size(), filename.c_str());
}

void EventData::initParticlesEmpty() {
    // If someone calls init again, we should always reset
    reset();

    evtParticles.push_back(glm::vec4(0.0f,0.0f,1.0f,0.0f));

    earliestTimestamp=1.0f;
    latestTimestamp=1.0f;

    // TODO: This is arbitrary, we can should define as a constant somewhere
    // Apply scale
    this->diffScale = 5.0f;
    for (auto &evt : evtParticles) {
        evt.z *= diffScale;
    }

    timeWindow_L = 0.0f;
    timeWindow_R = 1.0f;

    // Normalize the timestamp of the min/max XYZ for bounding box
    this->minXYZ = glm::vec3(0.0f,0.0f,0.0f);
    this->maxXYZ = glm::vec3(0.0f,0.0f,0.0f);
    this->center = glm::vec3(0.0f,0.0f,0.0f);
    
    this->spaceWindow = glm::vec4(minXYZ.y, maxXYZ.x, maxXYZ.y, minXYZ.x);

    printf("Loaded 0 particles\n");
}

// TODO: Move precalculable things to an init
void EventData::drawBoundingBoxWireframe(MatrixStack &MV, MatrixStack &P, Program &progBasic) {
    const glm::vec3 &scaled_minXYZ = minXYZ; 
    const glm::vec3 &scaled_maxXYZ = maxXYZ;
    
    glLineWidth(2.0f);

    glm::vec3 corners[16] = {
        { scaled_minXYZ.x, scaled_minXYZ.y, scaled_minXYZ.z },
        { scaled_maxXYZ.x, scaled_minXYZ.y, scaled_minXYZ.z },
        { scaled_maxXYZ.x, scaled_maxXYZ.y, scaled_minXYZ.z },
        { scaled_minXYZ.x, scaled_maxXYZ.y, scaled_minXYZ.z },
        { scaled_minXYZ.x, scaled_minXYZ.y, scaled_maxXYZ.z },
        { scaled_maxXYZ.x, scaled_minXYZ.y, scaled_maxXYZ.z },
        { scaled_maxXYZ.x, scaled_maxXYZ.y, scaled_maxXYZ.z },
        { scaled_minXYZ.x, scaled_maxXYZ.y, scaled_maxXYZ.z },
        { spaceWindow.w, spaceWindow.x, timeWindow_L },
        { spaceWindow.y, spaceWindow.x, timeWindow_L },
        { spaceWindow.y, spaceWindow.z, timeWindow_L },
        { spaceWindow.w, spaceWindow.z, timeWindow_L },
        { spaceWindow.w, spaceWindow.x, timeWindow_R },
        { spaceWindow.y, spaceWindow.x, timeWindow_R },
        { spaceWindow.y, spaceWindow.z, timeWindow_R },
        { spaceWindow.w, spaceWindow.z, timeWindow_R }
    };


    int edges[24][2] = {
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7},

        {8,9},   {9,10},  {10,11}, {11,8},
        {12,13}, {13,14}, {14,15}, {15,12},
        {8,12},  {9,13},  {10,14}, {11,15}
    };

    // Instead of immediate mode use VBOs
    static GLuint lineVBO, lineVAO;
    static bool initialized = false;
    
    if (!initialized) {
        glGenBuffers(1, &lineVBO);
        glGenVertexArrays(1, &lineVAO);
        initialized = true;
    }
    
    // Buffer for x0, y0, z0, x1, y1, ... of each line segment
    static std::vector<float> line_posbuf(24 * 2 * 3); // TODO static?
    line_posbuf.clear();
    for (int i = 0; i < 24; i++) {
        // Starting XYZ
        line_posbuf.push_back(corners[edges[i][0]].x);
        line_posbuf.push_back(corners[edges[i][0]].y);
        line_posbuf.push_back(corners[edges[i][0]].z);
        
        // Ending XYZ
        line_posbuf.push_back(corners[edges[i][1]].x);
        line_posbuf.push_back(corners[edges[i][1]].y);
        line_posbuf.push_back(corners[edges[i][1]].z);
    }
    
    // Bind VAO and update VBO
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, line_posbuf.size() * sizeof(float), line_posbuf.data(), GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    progBasic.bind();
    MV.pushMatrix();

    // TODO: Can change, for now white
    glm::vec3 color_line(1.0f, 1.0f, 1.0f);
    BPMaterial mat_line;
    mat_line.ka = color_line;
    mat_line.kd = color_line;
    mat_line.ks = color_line;
    mat_line.s = 10.0f;
    
    sendToPhongShader(progBasic, P, MV, glm::vec3(0.0f), color_line, mat_line);
    glDrawArrays(GL_LINES, 0, (GLsizei)line_posbuf.size() / 3);
    
    MV.popMatrix();
    progBasic.unbind();
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glLineWidth(1.0f);
}

/* THIS IS WITHOUT INSTANCING; DEPRECATED */
void EventData::draw(MatrixStack &MV, MatrixStack &P, Program &prog,
    float particleScale, const glm::vec3 &lightPos,
    const BPMaterial &lightMat, const Mesh &meshSphere) { 

    prog.bind();
    MV.pushMatrix();
        for (size_t i = 0; i < evtParticles.size(); i++) {
            if (i % modFreq == 0) {
                MV.pushMatrix();
                    MV.translate(evtParticles[i]);
                    MV.scale(particleScale);

                    glm::vec3 color = glm::vec3(0.0f, 1.0f, 0.0f);
                    // apply tint if t not in [timeWindow_L, timeWindow_R]
                    if (i < eventWindow_L || i > eventWindow_R) {
                        color = glm::vec3(0.5f, 0.5f, 0.5f);
                    }

                    sendToPhongShader(prog, P, MV, lightPos, color, lightMat);
                    meshSphere.draw(prog);
                MV.popMatrix();
            }
        }
        
        drawBoundingBoxWireframe(MV, P, prog);
    MV.popMatrix();
    prog.unbind();
}

void EventData::drawInstanced(MatrixStack &MV, MatrixStack &P, Program &progInst, Program &progBasic,
    float particleScale) {
    
    if (evtParticles.empty() || modFreq == 0) {
        return;
    }

    size_t instCt = std::max(1ULL, evtParticles.size());

    // glBindVertexArray(meshSphere.getVAOID());

    glBindBuffer(GL_ARRAY_BUFFER, instVBO);
    GLint aInstPos = progInst.getAttribute("aInstPos");
    if (aInstPos >= 0) {
        glEnableVertexAttribArray(aInstPos);
        glVertexAttribPointer(aInstPos, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);
        glVertexAttribDivisor(aInstPos, 1);
    }
    else {
        printf("aInstPos not found in shader\n");
        return;
    }

    // Send uniforms to GPU/shader
    progInst.bind();
    glUniformMatrix4fv(progInst.getUniform("P"), 1, GL_FALSE, glm::value_ptr(P.topMatrix()));
    glUniformMatrix4fv(progInst.getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV.topMatrix()));
    glUniformMatrix4fv(progInst.getUniform("MV_it"), 1, GL_FALSE, 
                      glm::value_ptr(glm::inverse(glm::transpose(MV.topMatrix()))));
    glUniform1f(progInst.getUniform("particleScale"), particleScale);
    glUniform3fv(progInst.getUniform("negColor"), 1, glm::value_ptr(negColor));
    glUniform3fv(progInst.getUniform("posColor"), 1, glm::value_ptr(posColor));

    // meshSphere.draw(prog, true, 0, instCt);
    glPointSize((GLfloat)particleScale);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArraysInstanced(GL_POINTS, 0, 1, (GLsizei)instCt);

    glDisableVertexAttribArray(aInstPos);
    glVertexAttribDivisor(aInstPos, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    progInst.unbind();
    GLSL::checkError();

    // Draw bounding box / wireframe
    drawBoundingBoxWireframe(MV, P, progBasic);

    GLSL::checkError();
}

// I <3 Zelun
static inline bool within_inc(uint val, uint left, uint right) {
    return left <= val && val <= right;
}

void EventData::setResourceDir(const std::string &resource_dir) {
    resourceDir = resource_dir;
}

void EventData::initComputeShader()
{
    if (computeInitialized)
    {
        return;
    }

    if (resourceDir.empty())
    {
        printf("Warning: resource directory not set, using default path\n");
        resourceDir = "resources/";
    }

    computeProg.setShaderName(resourceDir + "digital_shutter.comp");
    if (!computeProg.init())
    {
        printf("Failed to initialize compute shader\n");
        return;
    }

    // Add uniforms
    computeProg.bind();
    computeProg.addUniform("eventBound_L");
    computeProg.addUniform("eventBound_R");
    computeProg.addUniform("spaceWindow");
    computeProg.addUniform("isPositiveOnly");
    computeProg.addUniform("useMorlet");
    computeProg.addUniform("morletFreq");
    computeProg.addUniform("morletCenterT");
    computeProg.addUniform("morletH");
    computeProg.addUniform("baseContribution");
    computeProg.unbind();

    computeInitialized = true;
}

void EventData::initComputeBuffers()
{
    if (evtParticles.empty())
    {
        return;
    }

    // Create/update event particles SSBO
    if (evtParticlesSSBO == 0)
    {
        glGenBuffers(1, &evtParticlesSSBO);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, evtParticlesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, evtParticles.size() * sizeof(glm::vec4), 
                 evtParticles.data(), GL_DYNAMIC_READ);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Create output data SSBO (max size = input size)
    if (outputDataSSBO == 0)
    {
        glGenBuffers(1, &outputDataSSBO);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputDataSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, evtParticles.size() * sizeof(glm::vec3), 
                 nullptr, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Create counters SSBO (outputCount, rollingXBits, rollingYBits)
    if (countersSSBO == 0)
    {
        glGenBuffers(1, &countersSSBO);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, countersSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * sizeof(GLuint), nullptr, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    GLSL::checkError(GET_FILE_LINE);
}

void EventData::drawFrame(Program &prog, glm::vec2 viewport_resolution, bool morlet, float freq, bool pca)
{
    float timeBound_L, timeBound_R;
    int eventBound_L, eventBound_R;

    // Set up point size
    float aspectWidth = viewport_resolution.x / static_cast<float>(camera_resolution.x); // FIXME change name camera_res
    float aspectHeight = viewport_resolution.y / static_cast<float>(camera_resolution.y);
    glPointSize(1.0f * glm::max(aspectHeight, aspectWidth));

    // Generate buffers
    static GLuint VBO, VAO;
    static bool initialized = false;
    if (!initialized)
    {
        glGenBuffers(1, &VBO);
        glGenVertexArrays(1, &VAO);
        initialized = true;
    }

    // Set up bounds
    timeBound_L = timeWindow_L + timeShutterWindow_L;
    timeBound_R = timeWindow_L + timeShutterWindow_R;
    eventBound_L = eventWindow_L + eventShutterWindow_L;
    eventBound_R = eventWindow_L + eventShutterWindow_R;

    // Initialize compute shader on first use
    if (!computeInitialized)
    {
        initComputeShader();
        initComputeBuffers();
    }

    float rollingX(0), rollingY(0);
    float f = freq / 1000000 / diffScale;

    GLuint outputCount = 0;
    
    // Use GPU compute shader for event processing
    if (computeInitialized && !evtParticles.empty() && eventBound_L <= eventBound_R)
    {
        // Bind SSBOs to their binding points
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, evtParticlesSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, outputDataSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, countersSSBO);
        
        // Reset counters
        GLuint resetData[3] = {0, 0, 0};
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, countersSSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 3 * sizeof(GLuint), resetData);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Bind compute shader and set uniforms
        computeProg.bind();
        
        float center_t = timeBound_L + (timeBound_R - timeBound_L) * 0.5f;
        
        glUniform1i(computeProg.getUniform("eventBound_L"), eventBound_L);
        glUniform1i(computeProg.getUniform("eventBound_R"), eventBound_R);
        glUniform4fv(computeProg.getUniform("spaceWindow"), 1, glm::value_ptr(spaceWindow));
        glUniform1i(computeProg.getUniform("isPositiveOnly"), isPositiveOnly ? 1 : 0);
        glUniform1i(computeProg.getUniform("useMorlet"), morlet ? 1 : 0);
        glUniform1f(computeProg.getUniform("morletFreq"), f);
        glUniform1f(computeProg.getUniform("morletCenterT"), center_t);
        glUniform1f(computeProg.getUniform("morletH"), MorletFunc::h);
        glUniform1f(computeProg.getUniform("baseContribution"), BaseFunc::contribution);

        // Dispatch compute shader
        int numEvents = eventBound_R - eventBound_L + 1;
        int numWorkGroups = (numEvents + 255) / 256; // 256 threads per work group
        
        GLSL::checkError(GET_FILE_LINE);
        
        if (numWorkGroups > 0)
        {
            computeProg.dispatch(numWorkGroups, 1, 1);
        }
        
        GLSL::checkError(GET_FILE_LINE);

        // Memory barrier to ensure compute shader writes are visible
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        computeProg.unbind();

        // Read back only the counters (small data - not a bottleneck)
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, countersSSBO);
        GLuint counters[3];
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 3 * sizeof(GLuint), counters);
        outputCount = counters[0];
        rollingX = *reinterpret_cast<float*>(&counters[1]);
        rollingY = *reinterpret_cast<float*>(&counters[2]);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    // Render directly from GPU buffer (no CPU readback!)
    if (outputCount > 0)
    {
        glBindVertexArray(VAO);
        
        // Bind the SSBO as a vertex buffer - no data copy needed!
        glBindBuffer(GL_ARRAY_BUFFER, outputDataSSBO);

        prog.bind();

        int pos = prog.getAttribute("pos");
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
        glVertexAttribDivisor(pos, 1);

        glm::mat4 projection = glm::ortho(minXYZ.x, maxXYZ.x, minXYZ.y, maxXYZ.y);
        glUniformMatrix4fv(prog.getUniform("projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glDrawArraysInstanced(GL_POINTS, 0, 1, static_cast<GLsizei>(outputCount));

        prog.unbind();

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (pca && outputCount > 0)
    {
        // PCA requires CPU-side data - read back only when needed
        std::vector<glm::vec3> pcaData(outputCount);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputDataSSBO);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, outputCount * sizeof(glm::vec3), pcaData.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        
        // Calculate mean
        float inverseNumElems = 1.0f / static_cast<float>(outputCount);
        float mean_x = rollingX * inverseNumElems;
        float mean_y = rollingY * inverseNumElems;

        // Calculate covariance
        float cov_x_x(0.0f), cov_x_y(0.0f), cov_y_y(0.0f);
#pragma omp parallel
        {
#pragma omp for reduction(+ : cov_x_y) reduction(+ : cov_x_x) reduction(+ : cov_y_y)
            for (int i = 0; i < (int)outputCount; i++)
            {
                float x = pcaData[i].x;
                float y = pcaData[i].y;

                cov_x_y += (x - mean_x) * (y - mean_y);
                cov_x_x += (x - mean_x) * (x - mean_x);
                cov_y_y += (y - mean_y) * (y - mean_y);
            }
        }

        inverseNumElems = 1.0f / (static_cast<float>(outputCount) - 1.0f);
        cov_x_y *= inverseNumElems;
        cov_x_x *= inverseNumElems;
        cov_y_y *= inverseNumElems;

        // Eigenvalue calculation
        float a = 1;
        float b = -(cov_x_x + cov_y_y);
        float c = cov_x_x * cov_y_y - cov_x_y * cov_x_y;

        float eigen1 = (-b + std::sqrt(b * b - 4 * a * c)) / (2 * a);
        float eigen2 = (-b - std::sqrt(b * b - 4 * a * c)) / (2 * a);

        // Eigenvectors
        std::vector<glm::vec3> eigenvectors;
        eigenvectors.push_back(std::sqrt(eigen1) * glm::normalize(glm::vec3(eigen1 - cov_y_y, cov_x_y, 1)));
        eigenvectors.push_back(std::sqrt(eigen2) * glm::normalize(glm::vec3(eigen2 - cov_y_y, cov_x_y, 1)));

        // Transform to camera space
        glm::mat4 projection = glm::ortho(minXYZ.x, maxXYZ.x, minXYZ.y, maxXYZ.y);
        glm::vec4 mean_cameraspace(mean_x, mean_y, 1.0f, 1.0f);
        mean_cameraspace = projection * mean_cameraspace;
        eigenvectors.at(0) = projection * glm::vec4(eigenvectors.at(0).x, eigenvectors.at(0).y, 0.0f, 0.0f);
        eigenvectors.at(1) = projection * glm::vec4(eigenvectors.at(1).x, eigenvectors.at(1).y, 0.0f, 0.0f);

        // Add mean position to eigenvectors
        eigenvectors.at(0) += glm::vec3(mean_cameraspace);
        eigenvectors.at(1) += glm::vec3(mean_cameraspace);

        // Draw eigenvector lines
        glDisable(GL_BLEND); // Line should be opaque
        glLineWidth(5.0f);   // Could make setable
        glBegin(GL_LINES);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(mean_cameraspace.x, mean_cameraspace.y, mean_cameraspace.z);
        glVertex3f(eigenvectors[0].x, eigenvectors[0].y, 1.0f);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(mean_cameraspace.x, mean_cameraspace.y, mean_cameraspace.z);
        glVertex3f(eigenvectors[1].x, eigenvectors[1].y, 1.0f);
        glEnd();
    }

    GLSL::checkError(GET_FILE_LINE);
}


void EventData::normalizeTime() {
    float factor = diffScale * TIME_CONVERSION;
    minXYZ.z *= factor;
    maxXYZ.z *= factor;
    timeWindow_L *= factor;
    timeWindow_R *= factor;
    timeShutterWindow_L *= factor;
    timeShutterWindow_R *= factor;
}

void EventData::oddizeTime() {
    float factor = diffScale * TIME_CONVERSION;
    minXYZ.z /= factor;
    maxXYZ.z /= factor;
    timeWindow_L /= factor;
    timeWindow_R /= factor;
    timeShutterWindow_L /= factor;
    timeShutterWindow_R /= factor;
}

float EventData::getTimestamp(uint eventIndex, float oddFactor) const {
    return evtParticles[eventIndex].z / oddFactor;
}

inline bool lessVec4_t(const glm::vec4& a, const glm::vec4& b) {
    return a.z < b.z;
}

// If timestamp does not exist return first event included in window
uint EventData::getFirstEvent(float timestamp, float normFactor) const {
    assert(this->evtParticles.size() != 0);
    glm::vec4 timestampVec4(0.0f, 0.0f, timestamp * normFactor, 0.0f); 

    auto lb = std::lower_bound(evtParticles.begin(), evtParticles.end(), timestampVec4, lessVec4_t);
    if (lb == evtParticles.end()) {
        return static_cast<uint>(evtParticles.size() - 1);
    }
    return std::distance(evtParticles.begin(), lb);
} 

// If timestamp does not exist return last event included in window
uint EventData::getLastEvent(float timestamp, float normFactor) const {
    assert(this->evtParticles.size() != 0);
    glm::vec4 timestampVec4(0.0f, 0.0f, timestamp * normFactor, 0.0f); 

    auto ub = std::upper_bound(evtParticles.begin(), evtParticles.end(), timestampVec4, lessVec4_t);
    if (ub == evtParticles.begin()) {
        return 0;
    }
    return std::distance(evtParticles.begin(), --ub);
}
