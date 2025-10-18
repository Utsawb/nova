#pragma once

#ifndef GUI_HH
#define GUI_HH

#include "pch.hh"

#include "ParameterStore.hh"
#include "RenderTarget.hh"

#include "fonts/CascadiaCode.ttf.h"

class GUI
{
    private:
        ParameterStore *parameter_store;
        SDL_Window *window = nullptr;
        SDL_GPUDevice *gpu_device = nullptr;
        std::unordered_map<std::string, RenderTarget> *render_targets;
        ImDrawData * draw_data = nullptr;

    public:
        GUI(ParameterStore *parameter_store, SDL_Window *window, SDL_GPUDevice *gpu_device)
            : parameter_store(parameter_store), window(window), gpu_device(gpu_device)
        {
            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO &io = ImGui::GetIO();
            (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            void *font_memory = malloc(sizeof CascadiaCode_ttf);
            std::memcpy(font_memory, CascadiaCode_ttf, sizeof CascadiaCode_ttf);
            io.Fonts->AddFontFromMemoryTTF(font_memory, sizeof CascadiaCode_ttf, 16.0f);

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();

            // Setup scaling
            float scaling_factor = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
            ImGuiStyle &style = ImGui::GetStyle();
            style.ScaleAllSizes(scaling_factor);
            style.FontScaleDpi = scaling_factor;
            io.ConfigDpiScaleFonts = true;
            io.ConfigDpiScaleViewports = true;

            // Setup Platform/Renderer backends
            ImGui_ImplSDL3_InitForSDLGPU(window);
            ImGui_ImplSDLGPU3_InitInfo init_info = {.Device = gpu_device,
                                                    .ColorTargetFormat =
                                                        SDL_GetGPUSwapchainTextureFormat(gpu_device, window),
                                                    .MSAASamples = SDL_GPU_SAMPLECOUNT_1,
                                                    .SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                                    .PresentMode = SDL_GPU_PRESENTMODE_VSYNC};
            ImGui_ImplSDLGPU3_Init(&init_info);
        }

        ~GUI()
        {
            // Cleanup ImGui
            ImGui_ImplSDL3_Shutdown();
            ImGui_ImplSDLGPU3_Shutdown();
            ImGui::DestroyContext();
        }

        void event_handler(SDL_Event *event)
        {
            ImGui_ImplSDL3_ProcessEvent(event);
        }

        // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
        void prepare_to_render(SDL_GPUCommandBuffer *command_buffer)
        {
            // Start the Dear ImGui frame
            ImGui_ImplSDLGPU3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            // Create a simple demo window
            ImGui::ShowDemoWindow();

            // Rendering
            ImGui::Render();
            draw_data = ImGui::GetDrawData();

            // End frame (required for proper ImGui frame lifecycle)
            ImGui::EndFrame();

            // Update and Render additional Platform Windows
            ImGuiIO &io = ImGui::GetIO();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);
        }

        void render(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass)
        {
            ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
        }
};

#endif // GUI_HH