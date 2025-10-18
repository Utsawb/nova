#include "pch.hh"

#include "AppContext.hh"
#include "GUI.hh"
#include "ParameterStore.hh"


// This function runs once at startup.
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    *appstate = new AppContext();
    AppContext &app_context = **(AppContext **)appstate;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create SDL window
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    app_context.window = SDL_CreateWindow("Nova", (int)(1280 * main_scale), (int)(720 * main_scale), window_flags);
    if (app_context.window == NULL)
    {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create GPU Device
    app_context.gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan");
    if (app_context.gpu_device == NULL)
    {
        SDL_Log("Couldn't create GPU device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Claim window for GPU Device
    if (!SDL_ClaimWindowForGPUDevice(app_context.gpu_device, app_context.window))
    {
        SDL_Log("Couldn't claim window for GPU device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetGPUSwapchainParameters(app_context.gpu_device, app_context.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                  SDL_GPU_PRESENTMODE_VSYNC);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup scaling
    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;
    io.ConfigDpiScaleFonts = true;
    io.ConfigDpiScaleViewports = true;

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLGPU(app_context.window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {
        .Device = app_context.gpu_device,
        .ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(app_context.gpu_device, app_context.window),
        .MSAASamples = SDL_GPU_SAMPLECOUNT_1,
        .SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
        .PresentMode = SDL_GPU_PRESENTMODE_VSYNC};
    ImGui_ImplSDLGPU3_Init(&init_info);

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppContext &app_context = *(AppContext *)appstate;

    ImGui_ImplSDL3_ProcessEvent(event);

    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppContext &app_context = *(AppContext *)appstate;

    // Skip rendering if window is minimized
    if (SDL_GetWindowFlags(app_context.window) & SDL_WINDOW_MINIMIZED)
    {
        SDL_Delay(10);
        return SDL_APP_CONTINUE;
    }

    // Start the Dear ImGui frame
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Create a simple demo window
    ImGui::ShowDemoWindow();

    // Rendering
    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

    // End frame (required for proper ImGui frame lifecycle)
    ImGui::EndFrame();

    // Update and Render additional Platform Windows
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(app_context.gpu_device);

    SDL_GPUTexture *swapchain_texture;
    SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, app_context.window, &swapchain_texture, nullptr, nullptr);

    if (swapchain_texture != nullptr && !is_minimized)
    {
        // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
        ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

        // Setup and start a render pass
        SDL_GPUColorTargetInfo target_info = {};
        target_info.texture = swapchain_texture;
        target_info.clear_color = SDL_FColor{0.45f, 0.55f, 0.60f, 1.00f};
        target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        target_info.store_op = SDL_GPU_STOREOP_STORE;
        target_info.mip_level = 0;
        target_info.layer_or_depth_plane = 0;
        target_info.cycle = false;
        SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);

        // Render ImGui
        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

        SDL_EndGPURenderPass(render_pass);
    }

    // Submit the command buffer
    SDL_SubmitGPUCommandBuffer(command_buffer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    AppContext &app_context = *(AppContext *)appstate;

    // Wait for GPU to finish
    if (app_context.gpu_device)
    {
        SDL_WaitForGPUIdle(app_context.gpu_device);
    }

    // Cleanup ImGui
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    // Cleanup SDL3 GPU
    if (app_context.gpu_device && app_context.window)
    {
        SDL_ReleaseWindowFromGPUDevice(app_context.gpu_device, app_context.window);
    }
    if (app_context.gpu_device)
    {
        SDL_DestroyGPUDevice(app_context.gpu_device);
    }

    // Cleanup SDL
    if (app_context.window)
    {
        SDL_DestroyWindow(app_context.window);
    }
    SDL_Quit();

    delete (AppContext *)appstate;
}