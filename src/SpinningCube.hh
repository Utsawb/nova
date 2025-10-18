#pragma once

#include <SDL3/SDL_gpu.h>
#ifndef SPINNING_CUBE_HH
#define SPINNING_CUBE_HH

#include "pch.hh"

#include "RenderTarget.hh"
#include "UploadBuffer.hh"

#include "shaders/spinning_cube_frag.h"
#include "shaders/spinning_cube_vert.h"

class SpinningCube
{
    private:
        struct Vertex
        {
                float x, y, z;
                float r, g, b, a;
        };

        static constexpr const Vertex cubeVertices[] = {
            // Front face (red)
            {-0.5f, -0.5f, 0.5f, 1.f, 0.f, 0.f, 1.f},
            {0.5f, -0.5f, 0.5f, 1.f, 0.f, 0.f, 1.f},
            {0.5f, 0.5f, 0.5f, 1.f, 0.f, 0.f, 1.f},
            {-0.5f, -0.5f, 0.5f, 1.f, 0.f, 0.f, 1.f},
            {0.5f, 0.5f, 0.5f, 1.f, 0.f, 0.f, 1.f},
            {-0.5f, 0.5f, 0.5f, 1.f, 0.f, 0.f, 1.f},
            // Back face (green)
            {-0.5f, -0.5f, -0.5f, 0.f, 1.f, 0.f, 1.f},
            {-0.5f, 0.5f, -0.5f, 0.f, 1.f, 0.f, 1.f},
            {0.5f, 0.5f, -0.5f, 0.f, 1.f, 0.f, 1.f},
            {-0.5f, -0.5f, -0.5f, 0.f, 1.f, 0.f, 1.f},
            {0.5f, 0.5f, -0.5f, 0.f, 1.f, 0.f, 1.f},
            {0.5f, -0.5f, -0.5f, 0.f, 1.f, 0.f, 1.f},
            // Left face (blue)
            {-0.5f, -0.5f, -0.5f, 0.f, 0.f, 1.f, 1.f},
            {-0.5f, -0.5f, 0.5f, 0.f, 0.f, 1.f, 1.f},
            {-0.5f, 0.5f, 0.5f, 0.f, 0.f, 1.f, 1.f},
            {-0.5f, -0.5f, -0.5f, 0.f, 0.f, 1.f, 1.f},
            {-0.5f, 0.5f, 0.5f, 0.f, 0.f, 1.f, 1.f},
            {-0.5f, 0.5f, -0.5f, 0.f, 0.f, 1.f, 1.f},
            // Right face (yellow)
            {0.5f, -0.5f, -0.5f, 1.f, 1.f, 0.f, 1.f},
            {0.5f, 0.5f, 0.5f, 1.f, 1.f, 0.f, 1.f},
            {0.5f, -0.5f, 0.5f, 1.f, 1.f, 0.f, 1.f},
            {0.5f, -0.5f, -0.5f, 1.f, 1.f, 0.f, 1.f},
            {0.5f, 0.5f, -0.5f, 1.f, 1.f, 0.f, 1.f},
            {0.5f, 0.5f, 0.5f, 1.f, 1.f, 0.f, 1.f},
            // Top face (cyan)
            {-0.5f, 0.5f, 0.5f, 0.f, 1.f, 1.f, 1.f},
            {0.5f, 0.5f, 0.5f, 0.f, 1.f, 1.f, 1.f},
            {0.5f, 0.5f, -0.5f, 0.f, 1.f, 1.f, 1.f},
            {-0.5f, 0.5f, 0.5f, 0.f, 1.f, 1.f, 1.f},
            {0.5f, 0.5f, -0.5f, 0.f, 1.f, 1.f, 1.f},
            {-0.5f, 0.5f, -0.5f, 0.f, 1.f, 1.f, 1.f},
            // Bottom face (magenta)
            {-0.5f, -0.5f, 0.5f, 1.f, 0.f, 1.f, 1.f},
            {-0.5f, -0.5f, -0.5f, 1.f, 0.f, 1.f, 1.f},
            {0.5f, -0.5f, -0.5f, 1.f, 0.f, 1.f, 1.f},
            {-0.5f, -0.5f, 0.5f, 1.f, 0.f, 1.f, 1.f},
            {0.5f, -0.5f, -0.5f, 1.f, 0.f, 1.f, 1.f},
            {0.5f, -0.5f, 0.5f, 1.f, 0.f, 1.f, 1.f}};

        SDL_GPUDevice *gpu_device = nullptr;
        SDL_GPUShader *vs = nullptr;
        SDL_GPUShader *fs = nullptr;
        SDL_GPUBuffer *vertex_buffer = nullptr;
        SDL_GPUVertexBufferDescription vertex_buffer_desc{};
        SDL_GPUVertexAttribute vertex_buffer_attributes[2];
        SDL_GPUColorTargetDescription color_target_desc;
        SDL_GPUGraphicsPipeline *pipeline = nullptr;

        std::unordered_map<std::string, RenderTarget> &render_targets;

    public:
        SpinningCube(SDL_GPUDevice *gpu_device, UploadBuffer *upload_buffer, SDL_GPUCopyPass *copy_pass,
                     std::unordered_map<std::string, RenderTarget> &render_targets)
            : gpu_device(gpu_device), render_targets(render_targets)
        {
            SDL_GPUShaderCreateInfo vs_create_info = {.code_size = sizeof spinning_cube_vert,
                                                      .code = (const unsigned char *)spinning_cube_vert,
                                                      .entrypoint = "main",
                                                      .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                                      .stage = SDL_GPU_SHADERSTAGE_VERTEX,
                                                      .num_samplers = 0,
                                                      .num_storage_textures = 0,
                                                      .num_storage_buffers = 0,
                                                      .num_uniform_buffers = 1};
            vs = SDL_CreateGPUShader(gpu_device, &vs_create_info);

            SDL_GPUShaderCreateInfo fs_create_info = {.code_size = sizeof spinning_cube_frag,
                                                      .code = (const unsigned char *)spinning_cube_frag,
                                                      .entrypoint = "main",
                                                      .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                                      .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
                                                      .num_samplers = 0,
                                                      .num_storage_textures = 0,
                                                      .num_storage_buffers = 0,
                                                      .num_uniform_buffers = 0};
            fs = SDL_CreateGPUShader(gpu_device, &fs_create_info);

            SDL_GPUBufferCreateInfo vertex_buffer_create_info = {.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                                                 .size = sizeof(cubeVertices)};
            vertex_buffer = SDL_CreateGPUBuffer(gpu_device, &vertex_buffer_create_info);
            upload_buffer->upload_to_gpu(copy_pass, vertex_buffer, cubeVertices, sizeof(cubeVertices));

            vertex_buffer_desc = {
                .slot = 0,
                .pitch = sizeof(Vertex),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            };

            vertex_buffer_attributes[0] = {
                .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0};

            vertex_buffer_attributes[1] = {.location = 1,
                                           .buffer_slot = 0,
                                           .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                                           .offset = sizeof(float) * 3};

            color_target_desc = {
                .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM,
            };

            SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {
                .vertex_shader = vs,
                .fragment_shader = fs,
                .vertex_input_state = {.vertex_buffer_descriptions = &vertex_buffer_desc,
                                       .num_vertex_buffers = 1,
                                       .vertex_attributes = vertex_buffer_attributes,
                                       .num_vertex_attributes = 2},
                .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                .rasterizer_state = {.cull_mode = SDL_GPU_CULLMODE_BACK},
                .depth_stencil_state = {.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
                                        .enable_depth_test = true,
                                        .enable_depth_write = true},
                .target_info = {.color_target_descriptions = &color_target_desc,
                                .num_color_targets = 1,
                                .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
                                .has_depth_stencil_target = true}};
            pipeline = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipelineInfo);

            SDL_GPUTextureCreateInfo color_create_info = {
                .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM,
                .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
                .width = 1920,
                .height = 1080,
                .layer_count_or_depth = 1,
                .num_levels = 1,
                .sample_count = SDL_GPU_SAMPLECOUNT_1,
            };
            render_targets["SpinningCubeColor"] = {SDL_CreateGPUTexture(gpu_device, &color_create_info), 1920, 1080};

            SDL_GPUTextureCreateInfo depth_create_info = {
                .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
                .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
                .width = 1920,
                .height = 1080,
                .layer_count_or_depth = 1,
                .num_levels = 1,
                .sample_count = SDL_GPU_SAMPLECOUNT_1,
            };
            render_targets["SpinningCubeDepth"] = {SDL_CreateGPUTexture(gpu_device, &depth_create_info), 1920, 1080};
        }

        ~SpinningCube()
        {
            SDL_ReleaseGPUTexture(gpu_device, render_targets["SpinningCubeDepth"].texture);
            SDL_ReleaseGPUTexture(gpu_device, render_targets["SpinningCubeColor"].texture);
            SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline);
            SDL_ReleaseGPUShader(gpu_device, vs);
            SDL_ReleaseGPUShader(gpu_device, fs);
            SDL_ReleaseGPUBuffer(gpu_device, vertex_buffer);
        }

        void update()
        {
        }

        void render(SDL_GPUCommandBuffer *command_buffer, SDL_GPUTexture *color_texture, SDL_GPUTexture *depth_texture)
        {
        }
};

#endif