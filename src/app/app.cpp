#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include "app.h"

App::App()
    : window_("teeny-nes", 1024, 768),
      renderer_(window_.get()),
      imgui_(window_.get(), renderer_.get()),
      show_pattern_table_(true),
      pattern_table_(renderer_.get()),
      show_stats_(true) {}

void App::run() {
  ImGuiIO &io = ImGui::GetIO();

  bool done = false;
  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT)
        done = true;
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(window_.get()))
        done = true;
    }

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // ImGui::ShowDemoWindow();

    render_menu();

    if (show_pattern_table_) {
      pattern_table_.render();
    }
    if (show_stats_) {
      stats_.render();
    }

    ImGui::Render();
    SDL_RenderSetScale(
        renderer_.get(),
        io.DisplayFramebufferScale.x,
        io.DisplayFramebufferScale.y
    );
    const ImVec4 &bg_color = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    SDL_SetRenderDrawColor(
        renderer_.get(),
        (uint8_t)(bg_color.x * 255.0),
        (uint8_t)(bg_color.y * 255.0),
        (uint8_t)(bg_color.z * 255.0),
        0
    );
    SDL_RenderClear(renderer_.get());
    ImGui_ImplSDLRenderer2_RenderDrawData(
        ImGui::GetDrawData(), renderer_.get()
    );
    SDL_RenderPresent(renderer_.get());
  }
}

void App::render_menu() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Debug")) {
      ImGui::MenuItem("Show Pattern Table", nullptr, &show_pattern_table_);
      ImGui::MenuItem("Show Stats", nullptr, &show_stats_);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}
